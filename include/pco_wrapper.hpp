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
    void open();

	/** Connects to the camera on a specific interface */
	void open(WORD interface_type);

    void reset_camera_settings();

	void reboot();

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

	void set_recorder_mode_sequence();

    /** Validates the configuration of the camera and sets the camera ready for recording */
    void arm_camera();

    /**
    * Set segment sizes in number of images.
    * Since the internal segment size depends on the size of a single image,
    * ROI has to be set **before** setting the segment size.
    * arm_camera has to be called **before** also to update the image resolution.
    */
    void set_segment_sizes(DWORD segment1, DWORD segment2, DWORD segment3, DWORD segment4);

	/** Get camera segment sizes in **RAM pages** not images */
	void get_segment_sizes_pages(DWORD segmentSizes[4]);

    void set_active_segment(WORD segment);

	WORD get_active_segment();

    void clear_active_segment();

    void print_transferparameters();

    /**
    * From PCO SDK docs:
    * If any camera parameter was changed: before setting the Recording State to [run], the function PCO_ArmCamera must be called.
    * This is to ensure that all settings were correctly and are accepted by the camera.
    * If a successful Recording State [run] command is sent and recording is started, the images from a previous record to the active segment are lost.
    */
    void start_recording();

    void stop_recording();

    bool is_recording();

	int get_num_images_in_segment(WORD segment);

	int get_max_num_images_in_segment(WORD segment);

    /** Waits for recording to be done. Returns true if recording stopped, false if timeout occurred. */
    bool wait_for_recording_done(int timeout_ms = 0);

    /** Transfers images from the segment
    * @param segment - Camera memory segment to transfer from (Index starts at 1)
    * @param skip_images - Number of images to skip before first image.
    * @param max_images - Number of images to transfer at most (fewer will be transferred if there are fewer in the segment)
    * @param outpath - Filename of the resulting file.
    *        If the tiff file is too large it will be split and a number appended to the name.
    *        E.g. file.tiff -> file_1.tiff -> file_2.tiff
    *        Make sure you consider this when naming files to avoid overwriting files.
    * @return Number of images actually transferred
    */
    unsigned int transfer_to_tiff(unsigned int skip_images, unsigned int max_images, std::string outpath);

    /** Transfers images from the segment and performs MIP on the fly
    * @param segment - Camera memory segment to transfer from (Index starts at 1)
    * @param skip_images - Number of images to skip before first image.
    * @param images_per_mip - Number of images to join in one mip
    * @param num_mips - Number of mips to transfer at most (fewer will be transferred if there are fewer in the segment).
    *        Number of images transferred will be images_per_mip * num_mips.
    * @param outpath - Filename of the resulting file.
    *        If the tiff file is too large it will be split and a number appended to the name.
    *        E.g. file.tiff -> file_1.tiff -> file_2.tiff
    *        Make sure you consider this when naming files to avoid overwriting files.
    * @return Number of mips actually transferred
    */
    unsigned int transfer_mip_to_tiff(unsigned int skip_images, unsigned int images_per_mip, unsigned int num_mips, std::string outpath);

    void close();

private:
    HANDLE cam;

	/** Transfers images from the segment and performs operation given as callback
	* @param segment - Camera memory segment to transfer from (Index starts at 1)
	* @param skip_images - Number of images to skip before first image.
	* @param max_images - Number of images to transfer at most (fewer will be transferred if there are fewer in the segment).
			 Set to maximum int value to transfer all images.
	*/
	void transfer_internal(unsigned int skip_images, unsigned int max_images, std::function<void(unsigned int, const PCOBuffer&)> image_callback);
};

#endif //PCO_WRAPPER_H
