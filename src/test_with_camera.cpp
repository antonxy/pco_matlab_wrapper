#include <iostream>
#include "pco_wrapper.hpp"

int main(int argc, char** argv) {
	bool success = true;
	std::cerr << "Test with camera" << std::endl;
	try {
		PCOCamera cam;
		cam.open();
		cam.reset_camera_settings();
		cam.set_framerate_exposure(1, 1000000, 1000000); // 1kHz, 1ms
		cam.set_recorder_mode_sequence();
		cam.arm_camera();

		// Segment size calculation is somehow incorrect
		// So have to set slightly too many images
		cam.set_segment_sizes(500, 20, 0, 0);
		cam.set_active_segment(1);
		cam.arm_camera();

		//Record something in segment 1
		std::cerr << "Record 1" << std::endl;
		cam.start_recording();
		if (!cam.wait_for_recording_done(5000)) {
			cam.stop_recording();
			std::cerr << "Camera did not stop recording in time" << std::endl;
			success = false;
		}

		//Record something in segment 2
		cam.set_active_segment(2);
		cam.arm_camera();
		std::cerr << "Record 2" << std::endl;
		cam.start_recording();
		if (!cam.wait_for_recording_done(1000)) {
			cam.stop_recording();
			std::cerr << "Camera did not stop recording in time" << std::endl;
			success = false;
		}

		cam.set_active_segment(1);
		std::cerr << "Transfer MIP 1" << std::endl;
		if (cam.transfer_mip_to_tiff(0, 100, 5, "test_mip.tiff") != 5) {
			std::cerr << "Did not transfer correct amount of mips" << std::endl;
			success = false;
		}

		cam.set_active_segment(2);
		std::cerr << "Transfer all Images 2" << std::endl;
		if (cam.transfer_to_tiff(0, 20, "test_all.tiff") != 20) {
			std::cerr << "Did not transfer correct amount of images" << std::endl;
			success = false;
		}

		cam.set_active_segment(3);
		if (cam.transfer_to_tiff(0, 20, "test_none.tiff") != 0) {
			std::cerr << "Did not transfer correct amount of images" << std::endl;
			success = false;
		}

		cam.reset_camera_settings();

		cam.close();
	}
	catch (const std::exception & ex) {
		std::cerr << "Caught exception:" << std::endl;
		std::cerr << ex.what() << std::endl;
		success = false;
	}
	return success ? 0 : 1;
}