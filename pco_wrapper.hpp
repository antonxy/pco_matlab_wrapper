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

#include "pco_err.h"
#include "sc2_SDKStructures.h"
#include "SC2_SDKAddendum.h"
#include "SC2_CamExport.h"
#include "SC2_Defs.h"


int doSomething(int theint) {
    return 0;
}

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

class PCOError : public std::exception {
public:
    PCOError(int error_code) {}
    virtual const char* what() const noexcept {
        return "PCO Error";
    }
};

class PCOCamera {
public:
    PCOCamera(WORD wCamNum) {

    }

    /** Connects to the camera */
    void open() {
        int iRet = PCO_OpenCamera(&cam, 0);
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
        //Stop camera recording
        if (RecordingState)
        {
            iRet = PCO_SetRecordingState(cam, 0);
            if (iRet != PCO_NOERROR)
            {
                throw PCOError(iRet);
            }
        }

        //set camera to default state
        iRet = PCO_ResetSettingsToDefault(cam);
        if (iRet != PCO_NOERROR)
        {
            throw PCOError(iRet);
        }
    }

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

    void setup_recording_segement() {
    }

    int transfer_mip(WORD Segment) {
        int iRet;
        int success = 0;

        DWORD ValidImageCnt, MaxImageCnt;
        iRet = PCO_GetNumberOfImagesInSegment(cam, Segment, &ValidImageCnt, &MaxImageCnt);

        //Get image size and settings from camera
        DWORD bufsize, StatusDll, StatusDrv, set;
        WORD XResAct, YResAct, XBin, YBin;
        WORD RoiX0, RoiY0, RoiX1, RoiY1;
        iRet = PCO_GetSegmentImageSettings(cam, Segment, &XResAct, &YResAct,
            &XBin, &YBin, &RoiX0, &RoiY0, &RoiX1, &RoiY1);


        //Allocate 2 buffers
        HANDLE BufEvent[2] = { NULL, NULL };
        short BufNum[2] = { -1, -1 };
        WORD* BufAdr[2] = { NULL, NULL };

        bufsize = XResAct * YResAct * sizeof(WORD);
        for (int b = 0; b < 2; b++)
        {
            iRet = PCO_AllocateBuffer(cam, &BufNum[b], bufsize, &BufAdr[b], &BufEvent[b]);
            if (iRet != PCO_NOERROR) {
                throw std::exception("PCO_AllocateBuffer failed");
            }
        }

        //TODO use c++ instead of malloc
        WORD* MIP_Image = (WORD*)malloc(bufsize); //Should actually be an image buffer, not just int
        if (MIP_Image == NULL)
        {
            throw std::exception("malloc failed, out of memory");
        }
        memset(MIP_Image, 0, bufsize);

        //Read from camera ram
        iRet = PCO_SetImageParameters(cam, XResAct, YResAct, IMAGEPARAMETERS_READ_FROM_SEGMENTS, NULL, 0);

        //printf("Grab recorded images from camera actual valid %d\n", ValidImageCnt);

        //Use two buffers. Start two transfers in the beginning.
        //Wait for the first one to finish, process the data, then start it again for the next image and switch the buffers.
        //This way always at least one buffer is transferring.

        //Start two image transfers
        for (int currentImageIdx = 0; currentImageIdx < ValidImageCnt && currentImageIdx < 2; ++currentImageIdx) {
            iRet = PCO_AddBufferEx(cam, currentImageIdx, currentImageIdx, BufNum[currentImageIdx], XResAct, YResAct, 16);
            if (iRet != PCO_NOERROR) {
                throw std::exception("PCO_AddBufferEx failed");
                //success = -1;
                //goto error;
            }
        }

        int currentBufferIdx = 0;
        for (int currentImageIdx = 0; currentImageIdx < ValidImageCnt; ++currentImageIdx) {
            //wait for image transfer
            DWORD waitstat = WaitForSingleObject(BufEvent[currentBufferIdx], INFINITE);
            if (waitstat == WAIT_OBJECT_0)
            {
                ResetEvent(BufEvent[currentBufferIdx]);
                iRet = PCO_GetBufferStatus(cam, BufNum[currentBufferIdx], &StatusDll, &StatusDrv);

                //!!! IMPORTANT StatusDrv must always be checked for errors 
                if (StatusDrv != PCO_NOERROR)
                {
                    //printf("buf%02d error status 0x%08x m %02d ", currentBufferIdx, StatusDrv, currentImageIdx);
                    throw std::exception("buffer got error status");
                    //success = -1;
                    //goto error;
                }
            }
            else
            {
                //printf("Wait for buffer failed\n");
                throw std::exception("Wait for buffer failed");
                //success = -1;
                //goto error;
            }

            //fold image into MIP
            for (WORD pix = 0; pix < XResAct * YResAct; ++pix) {
                //TODO Check: Visual Studio thinks something is wrong here
                MIP_Image[pix] = max(MIP_Image[pix], BufAdr[currentBufferIdx][pix]);
            }

            //start next image transfer
            if (currentImageIdx + 2 < ValidImageCnt) {
                iRet = PCO_AddBufferEx(cam, currentImageIdx + 2, currentImageIdx + 2, BufNum[currentBufferIdx], XResAct, YResAct, 16);
                if (iRet != PCO_NOERROR) {
                    throw std::exception("PCO_AddBufferEx for next transfer failed");
                    //success = -1;
                    //goto error;
                }
            }

            currentBufferIdx = (currentBufferIdx + 1) % 2; // Switch buffer
        }

        //TODO Do something with the image
        printf("First 100 pixels of the MIP:\n");
        for (int i = 0; i < XResAct * YResAct && i < 100; ++i) {
            printf("%d ", i);
        }
        printf("\n");

    error:
        //!!! IMPORTANT PCO_CancelImages must always be called, after PCO_AddBuffer...() loops
        iRet = PCO_CancelImages(cam);
        for (int b = 0; b < 2; b++) {
            iRet = PCO_FreeBuffer(cam, BufNum[b]);
        }
        free(MIP_Image);
        return success;
    }

    void close() {
        int iRet = PCO_CloseCamera(&cam);
        if (iRet != PCO_NOERROR)
        {
            throw PCOError(iRet);
        }
    }

private:
    HANDLE cam;
};

#endif //PCO_WRAPPER_H