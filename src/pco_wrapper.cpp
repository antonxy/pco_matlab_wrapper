#include "pco_wrapper.hpp"

#include <cstdio>
#include <iostream>
#include <memory>
#include <array>

#include "tinytiffwriter.h"
#include "pco_err.h"
#include "sc2_SDKStructures.h"
#include "SC2_SDKAddendum.h"
#include "SC2_CamExport.h"
#include "SC2_Defs.h"

//Uncomment second line to disable debug printing
//#define DEBUGPRINT
#define DEBUGPRINT / ## /

// Use unique_ptr as go style defer
using defer = std::shared_ptr<void>;

void openConsole() {
    AllocConsole();
    freopen("CONOUT$", "w", stdout);
    freopen("CONOUT$", "w", stderr);
}

void testStdout() {
    std::cout << "Hello from stdout" << std::endl;
    std::cerr << "Hello from stderr" << std::endl;
}

PCOError::PCOError(int error_code) {
    PCO_GetErrorTextSDK(error_code, error_message, 100);
}

// Use this to wrap calls to PCO_... functions
// If the function returns an error and exception with the error message will be thrown
void PCOCheck(int error_code) {
    if (error_code != PCO_NOERROR) {
        throw PCOError(error_code);
    }
}


//PCO library buffer into which the library can transfer images
struct PCOBuffer {
    HANDLE cam;
    HANDLE event;
    short num;
    uint16_t* addr;
    WORD xres;
    WORD yres;
    bool allocated = false;

    PCOBuffer(HANDLE cam, WORD xres, WORD yres);

    // I don't want to accidentally create buffers so delete copy constructor
    PCOBuffer(const PCOBuffer&) = delete;
    PCOBuffer& operator= (const PCOBuffer&) = delete;

    // Need to move for construction
    PCOBuffer(PCOBuffer&& other);

    ~PCOBuffer();

    void start_transfer(int camera_image_index);

    void wait_for_buffer();
};

PCOBuffer::PCOBuffer(HANDLE cam, WORD xres, WORD yres)
    : cam(cam), event(NULL), num(-1), addr(NULL), xres(xres), yres(yres)
{
    DWORD bufsize = xres * yres * sizeof(uint16_t);
    DEBUGPRINT printf("Allocate buffer\n");
    PCO_AllocateBuffer(cam, &num, bufsize, &addr, &event);
    allocated = true;
}

// Need to move for construction
PCOBuffer::PCOBuffer(PCOBuffer&& other) {
    cam = other.cam;
    event = other.event;
    num = other.num;
    addr = other.addr;
    xres = other.xres;
    yres = other.yres;
    allocated = other.allocated;
    other.allocated = false; // This makes sure a buffer is not freed twice when moved
}

PCOBuffer::~PCOBuffer() {
    if (allocated) {
        DEBUGPRINT printf("Free buffer\n");
        PCO_FreeBuffer(cam, num);
    }
}

void PCOBuffer::start_transfer(int camera_image_index) {
    PCOCheck(PCO_AddBufferEx(cam, camera_image_index, camera_image_index, num, xres, yres, 16));
}

void PCOBuffer::wait_for_buffer() {
    DWORD waitstat = WaitForSingleObject(event, INFINITE);
    if (waitstat == WAIT_OBJECT_0)
    {
        ResetEvent(event);
        DWORD StatusDll = 0;
        DWORD StatusDrv = 0;
        PCOCheck(PCO_GetBufferStatus(cam, num, &StatusDll, &StatusDrv));

        //!!! IMPORTANT StatusDrv must always be checked for errors 
        if (StatusDrv != PCO_NOERROR)
        {
            printf("buf error status 0x%08x\n", StatusDrv);
            throw std::runtime_error("Buffer error status");
        }
    }
    else
    {
        throw std::runtime_error("Wait for buffer failed");
    }
}

/** Connects to the camera */
void PCOCamera::open(WORD wCamNum) {
    PCOCheck(PCO_OpenCamera(&cam, wCamNum));

    // Check if camera has internal memory
    PCO_Description strDescription;
    strDescription.wSize = sizeof(PCO_Description);
    PCOCheck(PCO_GetCameraDescription(cam, &strDescription));
    if (strDescription.dwGeneralCapsDESC1 & GENERALCAPS1_NO_RECORDER)
    {
        throw std::runtime_error("Camera found, but it has no internal memory\n");
    }

    WORD RecordingState;
    PCOCheck(PCO_GetRecordingState(cam, &RecordingState));
    //0 = stopped, 1 = running
    //Stop recording if camera is recording
    if (RecordingState)
    {
        PCOCheck(PCO_SetRecordingState(cam, 0));
    }
}

void PCOCamera::reset_camera_settings() {
    //set camera to default state
    PCOCheck(PCO_ResetSettingsToDefault(cam));
}

/**
* Set framerate and exposure time 
* @param frameRateMode
*        0 - Auto mode, camera decides
*        1 - Frame rate has priority
*        2 - Exposure time has priority
*        3 - Strict, error if the two don't match
*/
void PCOCamera::set_framerate_exposure(WORD frameRateMode, DWORD frameRate_mHz, DWORD expTime_ns) {
    WORD frameRateStatus;
    PCOCheck(PCO_SetFrameRate(cam, &frameRateStatus, frameRateMode, &frameRate_mHz, &expTime_ns));
}

void PCOCamera::set_roi(WORD roiX0, WORD roiY0, WORD roiX1, WORD roiY1) {
    PCOCheck(PCO_SetROI(cam, roiX0, roiY0, roiX1, roiY1));
}

void PCOCamera::set_segment_sizes(DWORD pagesPerSegment[4]) {
    //TODO need function to get page size
    //TODO Check PCO_SetStorageMode - maybe allows transfer while recording
    PCOCheck(PCO_SetCameraRamSegmentSize(cam, pagesPerSegment));
}

void PCOCamera::set_active_segment(WORD segment) {
    PCOCheck(PCO_SetActiveRamSegment(cam, segment));
}

void PCOCamera::clear_active_segment() {
    PCOCheck(PCO_ClearRamSegment(cam));
}

/** Validates the configuration of the camera and sets the camera ready for recording */
void PCOCamera::arm_camera() {
    //Arm camera - this makes sure any previous configuration changes are applied
    PCOCheck(PCO_ArmCamera(cam));

    //Check camera warning or error
    DWORD CameraWarning, CameraError, CameraStatus;
    PCOCheck(PCO_GetCameraHealthStatus(cam, &CameraWarning, &CameraError, &CameraStatus));
    if (CameraWarning != 0) {
        printf("Camera warning set: 0x%08x\n", CameraWarning);
    }
    if (CameraError != 0)
    {
        printf("Camera error set: 0x%08x\n", CameraError);
        throw std::runtime_error("Camera error set\n");
    }
}

void PCOCamera::print_transferparameters()
{
    PCO_CameraType strCamType;
    strCamType.wSize = sizeof(PCO_CameraType);
    PCOCheck(PCO_GetCameraType(cam, &strCamType));

    if (strCamType.wInterfaceType == INTERFACE_CAMERALINK)
    {
        PCO_SC2_CL_TRANSFER_PARAM cl_par;

        PCOCheck(PCO_GetTransferParameter(cam, (void*)&cl_par, sizeof(PCO_SC2_CL_TRANSFER_PARAM)));

        printf("Camlink Settings:\nBaudrate:    %u\nClockfreq:   %u\n", cl_par.baudrate, cl_par.ClockFrequency);
        printf("Dataformat:  %u 0x%x\nTransmit:    %u\n", cl_par.DataFormat, cl_par.DataFormat, cl_par.Transmit);
    }
}

void PCOCamera::start_recording() {
    PCOCheck(PCO_SetRecordingState(cam, 1));
}

void PCOCamera::stop_recording() {
    PCOCheck(PCO_SetRecordingState(cam, 0));
}

void PCOCamera::transfer_to_tiff(WORD segment, unsigned int skip_images, unsigned int max_images, std::string outpath) {
    TinyTIFFWriterFile* tif;
    //TODO Split image if > 4GiB
    unsigned int transferred_images = 0;
    transfer_internal(segment, skip_images, max_images, [&tif, outpath, &transferred_images](unsigned int transfer_image_index, const PCOBuffer& buffer) {
        if (transfer_image_index == 0) {
            // Allocate image after we receive first one because then we know the image size
            tif = TinyTIFFWriter_open(outpath.c_str(), 16, TinyTIFFWriter_UInt, 0, buffer.xres, buffer.yres, TinyTIFFWriter_Greyscale);
            if (!tif) {
                throw std::runtime_error("Could not create tiff file");
            }
        }

        TinyTIFFWriter_writeImage(tif, buffer.addr);
        transferred_images += 1;
        if (transferred_images % 10 == 0) std::cout << "\r" << transferred_images;
    });
    TinyTIFFWriter_close(tif);
    std::cout << "\rTransferred " << transferred_images << " images" << std::endl;
}

void PCOCamera::transfer_mip_to_tiff(WORD segment, unsigned int skip_images, unsigned int images_per_mip, unsigned int num_mips, std::string outpath) {
    TinyTIFFWriterFile* tif;
    //TODO Split image if > 4GiB
    std::unique_ptr<uint16_t[]> MIP_buffer;
    unsigned int images_to_transfer = images_per_mip * num_mips;
    if (num_mips == std::numeric_limits<unsigned int>::max()) {
        images_to_transfer = std::numeric_limits<unsigned int>::max();
    }
    unsigned int transferred_images = 0;
    unsigned int transferred_mips = 0;
    transfer_internal(segment, skip_images, images_per_mip * num_mips, [&tif, outpath, &MIP_buffer, images_per_mip, &transferred_images, &transferred_mips](unsigned int transfer_image_index, const PCOBuffer& buffer) {
        int numPix = buffer.xres * buffer.yres;
        if (transfer_image_index == 0) {
            // Allocate image after we receive first one because then we know the image size
            tif = TinyTIFFWriter_open(outpath.c_str(), 16, TinyTIFFWriter_UInt, 0, buffer.xres, buffer.yres, TinyTIFFWriter_Greyscale);
            if (!tif) {
                throw std::runtime_error("Could not create tiff file");
            }

            MIP_buffer = std::unique_ptr<uint16_t[]>(new uint16_t[numPix]);
            memset(MIP_buffer.get(), 0, numPix * sizeof(uint16_t));
        }

        // fold image into MIP
        for (int pix = 0; pix < numPix; ++pix) {
            uint16_t val = buffer.addr[pix];
            uint16_t* mip_val = MIP_buffer.get() + pix;
            if (val > * mip_val)
            {
                *mip_val = val;
            }
        }

        if (transfer_image_index % images_per_mip == images_per_mip - 1) {
            //Save image
            TinyTIFFWriter_writeImage(tif, buffer.addr);
            transferred_mips += 1;

            //Reset MIP
            memset(MIP_buffer.get(), 0, numPix * sizeof(uint16_t));
        }
        transferred_images += 1;
        if (transferred_images % 10 == 0) std::cout << "\r" << transferred_images;
    });
    TinyTIFFWriter_close(tif);

    std::cout << "\rTransferred " << transferred_images << " images into " << transferred_mips << " MIPs" << std::endl;
    unsigned int lost_images = transferred_images % transferred_mips;
    if (lost_images != 0) {
        std::cout << "Lost " << lost_images << " images which did not fill a MIP" << std::endl;
    }
}

/** Transfers images from the segment and performs operation given as callback
* @param Segment - Camera memory segment to transfer from (Index starts at 1)
* @param skip_images - Number of images to skip before first image.
* @param max_images - Number of images to transfer at most (fewer will be transfered if there are fewer in the segment).
            Set to maximum int value to transfer all images.
*/
void PCOCamera::transfer_internal(WORD Segment, unsigned int skip_images, unsigned int max_images, std::function<void(unsigned int, const PCOBuffer &)> image_callback) {
    DWORD ValidImageCnt, MaxImageCnt;
    PCOCheck(PCO_GetNumberOfImagesInSegment(cam, Segment, &ValidImageCnt, &MaxImageCnt));

    if (skip_images >= ValidImageCnt) {
        return;
    }

    //Get image size and settings from camera
    WORD XResAct, YResAct, XBin, YBin;
    WORD RoiX0, RoiY0, RoiX1, RoiY1;
    PCOCheck(PCO_GetSegmentImageSettings(cam, Segment, &XResAct, &YResAct,
        &XBin, &YBin, &RoiX0, &RoiY0, &RoiX1, &RoiY1));

    //TODO check if this works correctly, i.e. allocates two buffers
    //Number of buffers that will be used for transfering images in parallel
    //It seems there is no advantage to using more than 2
    constexpr int NUMBUF = 2;
    std::array<PCOBuffer, NUMBUF> pco_buffers = { PCOBuffer(cam, XResAct, YResAct), PCOBuffer(cam, XResAct, YResAct) };

    //Read from camera ram
    PCOCheck(PCO_SetImageParameters(cam, XResAct, YResAct, IMAGEPARAMETERS_READ_FROM_SEGMENTS, NULL, 0));

    DEBUGPRINT printf("Grab recorded images from camera actual valid %d\n", ValidImageCnt);

    //Use two buffers. Start two transfers in the beginning.
    //Wait for the first one to finish, process the data, then start it again for the next image and switch the buffers.
    //This way always at least one buffer is transferring.

    LARGE_INTEGER frequency;
    LARGE_INTEGER start;
    LARGE_INTEGER end;
    QueryPerformanceFrequency(&frequency);
    QueryPerformanceCounter(&start);
    
    unsigned int num_images_to_transfer = std::min(((unsigned int)ValidImageCnt) - skip_images, max_images);

    // Cancel all image transfers when exiting from this function so that nothing is
    // transfered into freed buffers
    defer _1(nullptr, [this](...) {
        PCO_CancelImages(cam);
    });

    // Start two image transfers
    for (int transfer_image_index = 0; transfer_image_index < num_images_to_transfer && transfer_image_index < NUMBUF; ++transfer_image_index) {
        DEBUGPRINT printf("Start transfer %d @ buf %d\n", transfer_image_index, transfer_image_index);
        int camera_image_index = transfer_image_index + skip_images + 1;
        pco_buffers[transfer_image_index].start_transfer(camera_image_index);
    }

    // Wait for tranfers in order, when a transfer is finished use the buffer to start the next one
    int currentBufferIdx = 0;
    for (int transfer_image_index = 0; transfer_image_index < num_images_to_transfer; ++transfer_image_index) {
        int camera_image_index = transfer_image_index + skip_images + 1;
        
        // wait for image transfer
        DEBUGPRINT printf("wait for transfer %d @ buf %d\n", transfer_image_index, currentBufferIdx);
        pco_buffers[currentBufferIdx].wait_for_buffer();

        image_callback(transfer_image_index, pco_buffers[currentBufferIdx]);
        DEBUGPRINT printf("processed image %d\n", transfer_image_index);

        // start next image transfer
        if (transfer_image_index + NUMBUF < num_images_to_transfer) {
            DEBUGPRINT printf("start transfer %d @ buf %d\n", transfer_image_index + NUMBUF, currentBufferIdx);
            pco_buffers[currentBufferIdx].start_transfer(camera_image_index + NUMBUF);
        }

        currentBufferIdx = (currentBufferIdx + 1) % NUMBUF; // Switch buffer
    }

    QueryPerformanceCounter(&end);
    double interval = (double)(end.QuadPart - start.QuadPart) / frequency.QuadPart;
    DEBUGPRINT printf("Transfered %d images in %f seconds\n", num_images_to_transfer, interval);
    double mb_per_image = double(XResAct) * double(YResAct) * 3 / 2 / 1e6;
    DEBUGPRINT printf("Image size: %d x %d - %f MB\n", XResAct, YResAct, mb_per_image);
    double mb_per_sec = mb_per_image * num_images_to_transfer / interval;
    DEBUGPRINT printf("Transfer speed: %f MB/s\n", mb_per_sec);
}

void PCOCamera::close() {
    PCOCheck(PCO_CloseCamera(cam));
}