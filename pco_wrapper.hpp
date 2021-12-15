//Info about Matlab wrapper generation:
//https://blogs.mathworks.com/developer/2019/07/11/cpp-interface/
//https://ch.mathworks.com/help/matlab/ref/clibgen.generatelibrarydefinition.html
//https://ch.mathworks.com/help/matlab/matlab_external/publish-modified-help-text.html

#ifndef PCO_WRAPPER_H
#define PCO_WRAPPER_H

// Comilation fails with "utility:137: expected an identifier"... without this
#define _HAS_CONDITIONAL_EXPLICIT 0

#include <exception>
#include <windows.h>
#include <cstdio>
#include <iostream>
#include <sstream>
#include <iomanip>
#include <memory>

#include "pco_err.h"
#include "sc2_SDKStructures.h"
#include "SC2_SDKAddendum.h"
#include "SC2_CamExport.h"
#include "SC2_Defs.h"

/** Opens the windows console window for MATLAB so that stdout and stderr can be displayed */
void openConsole() {
    AllocConsole();
    freopen("CONOUT$", "w", stdout);
    freopen("CONOUT$", "w", stderr);
}

void testStdout() {
    std::cout << "Hello from stdout" << std::endl;
    std::cerr << "Hello from stderr" << std::endl;
}

class ImageStack {
public:
    int num_images;
    int cols;
    int rows;

    /** I think in matlab 2019 there is no way to make the matrix have the right size automatically.
        So you have to take the size from the Image properties and pass it in here */
    const uint16_t* getData(int num_images, int cols, int rows) {
        if (num_images != this->num_images || cols != this->cols || rows != this->rows) {
            throw std::exception("Dimensions passed in do not match image dimensions");
        }
        return data.get();
    }

    uint16_t* getDataMut() {
        return data.get();
    }

    ImageStack(int num_images, int cols, int rows)
        : num_images(num_images), cols(cols), rows(rows)
    {
        data = std::unique_ptr<uint16_t[]>(new uint16_t[num_images * cols * rows]);
    }
private:
    std::unique_ptr<uint16_t[]> data;
};

ImageStack getImage() {
    ImageStack im(3, 100, 100);

    uint16_t* data = im.getDataMut();
    int i = 0;
    for (int ch = 0; ch < im.num_images; ++ch) {
        for (int c = 0; c < im.cols; ++c) {
            for (int r = 0; r < im.rows; ++r) {
                data[i] = ch;
                ++i;
            }
        }
    }

    return im;
}

class PCOError : public std::exception {
public:
    PCOError(int error_code) {
        char szErrorString[100];
        PCO_GetErrorTextSDK(error_code, szErrorString, 100);

        std::stringstream ss;
        //ss << "PCO Error 0x";
        //ss << std::setfill('0') << std::setw(sizeof(int) * 2)
        //    << std::hex << error_code;
        //ss << " - ";
        ss << szErrorString;

        message = ss.str();
    }

    virtual const char* what() const noexcept {
        return message.c_str();
    }
private:
    std::string message;
};

class PCOCamera {
public:
    PCOCamera() { }

    /** Connects to the camera */
    void open(WORD wCamNum) {
        int iRet = PCO_OpenCamera(&cam, wCamNum);
        if (iRet != PCO_NOERROR)
        {
            throw PCOError(iRet);
        }

        // Check if camera has internal memory
        PCO_Description strDescription;
        strDescription.wSize = sizeof(PCO_Description);
        iRet = PCO_GetCameraDescription(cam, &strDescription);
        if (strDescription.dwGeneralCapsDESC1 & GENERALCAPS1_NO_RECORDER)
        {
            throw std::exception("Camera found, but it has no internal memory\n");
        }

        WORD RecordingState;
        iRet = PCO_GetRecordingState(cam, &RecordingState);
        //0 = stopped, 1 = running
        //Stop recording if camera is recording
        if (RecordingState)
        {
            iRet = PCO_SetRecordingState(cam, 0);
            if (iRet != PCO_NOERROR)
            {
                throw PCOError(iRet);
            }
        }
    }
    
    void reset_camera_settings() {
        //set camera to default state
        int iRet = PCO_ResetSettingsToDefault(cam);
        if (iRet != PCO_NOERROR)
        {
            throw PCOError(iRet);
        }
    }

    /**
    * Set framerate and exposure time 
    * @param frameRateMode
    *        0 - Auto mode, camera decides
    *        1 - Frame rate has priority
    *        2 - Exposure time has priority
    *        3 - Strict, error if the two don't match
    */
    void set_framerate_exposure(WORD frameRateMode, DWORD frameRate_mHz, DWORD expTime_ns) {
        WORD frameRateStatus;
        int iRet = PCO_SetFrameRate(cam, &frameRateStatus, frameRateMode, &frameRate_mHz, &expTime_ns);
        if (iRet != PCO_NOERROR)
        {
            throw PCOError(iRet);
        }
    }
    
    void set_roi(WORD roiX0, WORD roiY0, WORD roiX1, WORD roiY1) {
        int iRet = PCO_SetROI(cam, roiX0, roiY0, roiX1, roiY1);
        if (iRet != PCO_NOERROR)
        {
            throw PCOError(iRet);
        }
    }

    /** Validates the configuration of the camera and sets the camera ready for recording */
    void arm_camera() {
        int iRet;
        //Arm camera - this makes sure any previous configuration changes are applied
        iRet = PCO_ArmCamera(cam);
        if (iRet != PCO_NOERROR)
        {
            throw PCOError(iRet);
        }

        //Check camera warning or error
        DWORD CameraWarning, CameraError, CameraStatus;
        iRet = PCO_GetCameraHealthStatus(cam, &CameraWarning, &CameraError, &CameraStatus);
        if (CameraWarning != 0) {
            printf("Camera warning set: 0x%08x\n", CameraWarning);
        }
        if (CameraError != 0)
        {
            printf("Camera error set: 0x%08x\n", CameraError);
            throw std::exception("Camera error set\n");
        }
    }

    void print_transferparameters()
    {
        PCO_CameraType strCamType;
        DWORD iRet;
        strCamType.wSize = sizeof(PCO_CameraType);
        iRet = PCO_GetCameraType(cam, &strCamType);
        if (iRet != PCO_NOERROR)
        {
            printf("PCO_GetCameraType failed with errorcode 0x%x\n", iRet);
            return;
        }

        if (strCamType.wInterfaceType == INTERFACE_CAMERALINK)
        {
            PCO_SC2_CL_TRANSFER_PARAM cl_par;

            iRet = PCO_GetTransferParameter(cam, (void*)&cl_par, sizeof(PCO_SC2_CL_TRANSFER_PARAM));
            printf("Camlink Settings:\nBaudrate:    %u\nClockfreq:   %u\n", cl_par.baudrate, cl_par.ClockFrequency);
            printf("Dataformat:  %u 0x%x\nTransmit:    %u\n", cl_par.DataFormat, cl_par.DataFormat, cl_par.Transmit);
        }
    }

    void set_segment_sizes(DWORD pagesPerSegment[4]) {
        //TODO Check PCO_SetStorageMode - maybe allows transfer while recording
        int iRet = PCO_SetCameraRamSegmentSize(cam, pagesPerSegment);
        if (iRet != PCO_NOERROR)
        {
            throw PCOError(iRet);
        }

    }

    void set_active_segment(WORD segment) {
        int iRet = PCO_SetActiveRamSegment(cam, segment);
        if (iRet != PCO_NOERROR)
        {
            throw PCOError(iRet);
        }
    }

    void clear_active_segment() {
        int iRet = PCO_ClearRamSegment(cam);
        if (iRet != PCO_NOERROR)
        {
            throw PCOError(iRet);
        }
    }

    void start_recording() {
        int iRet = PCO_SetRecordingState(cam, 1);
        if (iRet != PCO_NOERROR)
        {
            throw PCOError(iRet);
        }
    }

    void stop_recording() {
        int iRet = PCO_SetRecordingState(cam, 0);
        if (iRet != PCO_NOERROR)
        {
            throw PCOError(iRet);
        }
    }

    /** Transfers all images from the segment and performs MIP on the fly
	* @param Segment - Camera memory segment to transfer from (Index starts at 1)
	* @param start_image_index - First image to transfer (Index starts at 0)
	* @param images_per_mip - Number of images to join in one mip
	* @param num_mips - Number of mips to perform.
	*        Number of images transfered will be images_per_mip * num_mips
	*/
    ImageStack transfer_mip(WORD Segment, int start_image_index, int images_per_mip, int num_mips) {
        int iRet;

        DWORD ValidImageCnt, MaxImageCnt;
        iRet = PCO_GetNumberOfImagesInSegment(cam, Segment, &ValidImageCnt, &MaxImageCnt);
        if (iRet != PCO_NOERROR)
        {
            throw PCOError(iRet);
        }

        //Get image size and settings from camera
        WORD XResAct, YResAct, XBin, YBin;
        WORD RoiX0, RoiY0, RoiX1, RoiY1;
        iRet = PCO_GetSegmentImageSettings(cam, Segment, &XResAct, &YResAct,
            &XBin, &YBin, &RoiX0, &RoiY0, &RoiX1, &RoiY1);
        if (iRet != PCO_NOERROR)
        {
            throw PCOError(iRet);
        }

        //Number of buffers that will be used for transfering images in parallel
        //It seems there is no advantage to using more than 2
        const int NUMBUF = 2;

        //Allocate buffers
        HANDLE BufEvent[NUMBUF];
        short BufNum[NUMBUF];
        WORD* BufAdr[NUMBUF];

        for (int i = 0; i < NUMBUF; ++i)
        {
            BufEvent[i] = NULL;
            BufNum[i] = -1;
            BufAdr[i] = NULL;
        }

        int numPix = XResAct * YResAct;
        DWORD bufsize = numPix * sizeof(WORD);
        for (int b = 0; b < NUMBUF; b++)
        {
            iRet = PCO_AllocateBuffer(cam, &BufNum[b], bufsize, &BufAdr[b], &BufEvent[b]);
            if (iRet != PCO_NOERROR)
            {
                throw PCOError(iRet);
            }
        }

        ImageStack MIP_Image(num_mips, XResAct, YResAct);
        uint16_t* MIP_ImageData = MIP_Image.getDataMut();
		memset(MIP_ImageData, 0, sizeof(uint16_t) * MIP_Image.num_images * MIP_Image.rows * MIP_Image.cols);

        //Read from camera ram
        iRet = PCO_SetImageParameters(cam, XResAct, YResAct, IMAGEPARAMETERS_READ_FROM_SEGMENTS, NULL, 0);
        if (iRet != PCO_NOERROR)
        {
            throw PCOError(iRet);
        }

        printf("Grab recorded images from camera actual valid %d\n", ValidImageCnt);

        //Use two buffers. Start two transfers in the beginning.
        //Wait for the first one to finish, process the data, then start it again for the next image and switch the buffers.
        //This way always at least one buffer is transferring.

        LARGE_INTEGER frequency;
        LARGE_INTEGER start;
        LARGE_INTEGER end;
        QueryPerformanceFrequency(&frequency);
        QueryPerformanceCounter(&start);

		int num_images_to_transfer = min(ValidImageCnt-start_image_index, num_mips * images_per_mip);

        //Start two image transfers
        //!! Camera indexes start at 1
        for (int currentImageIdx = 1; currentImageIdx <= num_images_to_transfer && currentImageIdx <= NUMBUF; ++currentImageIdx) {
            printf("Start transfer %d @ buf %d\n", currentImageIdx, currentImageIdx - 1);
			int camera_image_index = currentImageIdx + start_image_index;
            iRet = PCO_AddBufferEx(cam, camera_image_index, camera_image_index, BufNum[currentImageIdx - 1], XResAct, YResAct, 16);
            if (iRet != PCO_NOERROR) {
                printf("Failed to start transfer, img idx %d, buf idx %d\n", currentImageIdx, currentImageIdx - 1);
                throw std::exception("Failed to start image transfer");
            }
        }

        int currentBufferIdx = 0;
        for (int currentImageIdx = 1; currentImageIdx <= num_images_to_transfer; ++currentImageIdx) {
			int camera_image_index = currentImageIdx + start_image_index;
            //wait for image transfer
            printf("wait for transfer %d @ buf %d\n", currentImageIdx, currentBufferIdx);
            DWORD waitstat = WaitForSingleObject(BufEvent[currentBufferIdx], INFINITE);
            if (waitstat == WAIT_OBJECT_0)
            {
                ResetEvent(BufEvent[currentBufferIdx]);
                DWORD StatusDll = 0;
                DWORD StatusDrv = 0;
                iRet = PCO_GetBufferStatus(cam, BufNum[currentBufferIdx], &StatusDll, &StatusDrv);

                //!!! IMPORTANT StatusDrv must always be checked for errors 
                if (StatusDrv != PCO_NOERROR)
                {
                    printf("buf%02d error status 0x%08x m %02d\n", currentBufferIdx, StatusDrv, currentImageIdx);
                    throw std::exception("Buffer error status");
                }
            }
            else
            {
                printf("Wait for buffer failed\n");
                throw std::exception("Wait for buffer failed");
            }

            printf("received transfer %d\n", currentImageIdx);

            //fold image into MIP
			int current_mip = (currentImageIdx - 1) / images_per_mip;
			printf("MIP image %d into MIP %d\n", currentImageIdx, current_mip);
            for (int pix = 0; pix < numPix; ++pix) {
                WORD val = BufAdr[currentBufferIdx][pix];
                if (val > MIP_ImageData[current_mip * MIP_Image.rows * MIP_Image.cols + pix])
                {
                    MIP_ImageData[current_mip * MIP_Image.rows * MIP_Image.cols + pix] = val;
                }
            }

            printf("processed image %d\n", currentImageIdx);

            //start next image transfer
            if (camera_image_index + NUMBUF <= ValidImageCnt) {
				printf("start transfer %d @ buf %d\n", currentImageIdx + NUMBUF, currentBufferIdx);
                iRet = PCO_AddBufferEx(cam, camera_image_index + NUMBUF, camera_image_index + NUMBUF, BufNum[currentBufferIdx], XResAct, YResAct, 16);
                if (iRet != PCO_NOERROR) {
                    printf("Failed to start following transfer, img idx %d, buf idx %d\n", currentImageIdx, currentBufferIdx);
                    throw std::exception("Failed to start next image transfer");
                }
            }

            currentBufferIdx = (currentBufferIdx + 1) % NUMBUF; // Switch buffer
        }

        QueryPerformanceCounter(&end);
        double interval = (double)(end.QuadPart - start.QuadPart) / frequency.QuadPart;
        printf("Transfered %d images in %f seconds\n", num_images_to_transfer, interval);
        double mb_per_image = double(numPix) * 3 / 2 / 1e6;
        printf("Image size: %d x %d - %f MB\n", XResAct, YResAct, mb_per_image);
        double mb_per_sec = mb_per_image * num_images_to_transfer / interval;
        printf("Transfer speed: %f MB/s\n", mb_per_sec);

        //TODO properly deallocate in case of exception

        //!!! IMPORTANT PCO_CancelImages must always be called, after PCO_AddBuffer...() loops
        iRet = PCO_CancelImages(cam);
        for (int b = 0; b < 2; b++) {
            iRet = PCO_FreeBuffer(cam, BufNum[b]);
        }

        return MIP_Image;
    }

    void close() {
        int iRet = PCO_CloseCamera(cam);
        if (iRet != PCO_NOERROR)
        {
            throw PCOError(iRet);
        }
    }

private:
    HANDLE cam;
};

#endif //PCO_WRAPPER_H