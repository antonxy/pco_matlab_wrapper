//Info about Matlab wrapper generation:
//https://blogs.mathworks.com/developer/2019/07/11/cpp-interface/
//https://ch.mathworks.com/help/matlab/ref/clibgen.generatelibrarydefinition.html
//https://ch.mathworks.com/help/matlab/matlab_external/publish-modified-help-text.html

#ifndef PCO_WRAPPER_H
#define PCO_WRAPPER_H

// Compilation from MATLAB fails with "utility:137: expected an identifier"... without this
#define _HAS_CONDITIONAL_EXPLICIT 0

//Otherwise min macro defined by windows.h messes up std::min
#define NOMINMAX

#include <stdexcept>
#include <windows.h>
#include <string>
#include <functional>

/** Opens the windows console window for MATLAB so that stdout and stderr can be displayed */
void openConsole();

void testStdout();

class PCOError : public std::exception {
public:
    PCOError(int error_code);

    virtual const char* what() const noexcept {
        return error_message;
    }
private:
    char error_message[100];
};

struct PCOBuffer;

class PCOCamera {
public:
    PCOCamera()
		: cam(nullptr)
	{ }

    /** Connects to the camera */
    void open(WORD wCamNum);
    
    void reset_camera_settings();

    /**
    * Set framerate and exposure time 
    * @param frameRateMode
    *        0 - Auto mode, camera decides
    *        1 - Frame rate has priority
    *        2 - Exposure time has priority
    *        3 - Strict, error if the two don't match
    */
    void set_framerate_exposure(WORD frameRateMode, DWORD frameRate_mHz, DWORD expTime_ns);
    
    void set_roi(WORD roiX0, WORD roiY0, WORD roiX1, WORD roiY1);

    //TODO let this take number of images instead of pages
    void set_segment_sizes(DWORD pagesPerSegment[4]);

    void set_active_segment(WORD segment);

    void clear_active_segment();

    /** Validates the configuration of the camera and sets the camera ready for recording */
    void arm_camera();

    void print_transferparameters();

    void start_recording();

    void stop_recording();

    //TODO
    //void wait_for_recording_done();

    /** Transfers images from the segment
    * @param Segment - Camera memory segment to transfer from (Index starts at 1)
    * @param skip_images - Number of images to skip before first image.
    * @param max_images - Number of images to transfer at most (fewer will be transferred if there are fewer in the segment)
    */
    void transfer_to_tiff(WORD segment, unsigned int skip_images, unsigned int max_images, std::string outpath);

    /** Transfers images from the segment and performs MIP on the fly
    * @param Segment - Camera memory segment to transfer from (Index starts at 1)
    * @param skip_images - Number of images to skip before first image.
    * @param images_per_mip - Number of images to join in one mip
    * @param num_mips - Number of mips to perform.
    *        Number of images transferred will be images_per_mip * num_mips.
    */
    void transfer_mip_to_tiff(WORD Segment, unsigned int skip_images, unsigned int images_per_mip, unsigned int num_mips, std::string outpath);

    /** Transfers images from the segment and performs operation given as callback
	* @param Segment - Camera memory segment to transfer from (Index starts at 1)
	* @param skip_images - Number of images to skip before first image.
	* @param max_images - Number of images to transfer at most (fewer will be transferred if there are fewer in the segment).
             Set to maximum int value to transfer all images.
	*/
    void transfer_internal(WORD Segment, unsigned int skip_images, unsigned int max_images, std::function<void(unsigned int, const PCOBuffer &)> image_callback);

    void close();

private:
    HANDLE cam;
};

#endif //PCO_WRAPPER_H
