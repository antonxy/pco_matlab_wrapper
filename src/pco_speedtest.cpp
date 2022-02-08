#include <iostream>
#include <chrono>
#include "clipp.hpp"
#include "pco_wrapper.hpp"

using namespace clipp;

int main(int argc, char** argv) {
    //
    // Command line options
    //

    bool help = false;

    //Common
    unsigned int num_transfers = 0;
    unsigned int num_images = 0;

    auto common_options = (
        required("-n", "--num_transfers") & integer("num transfers", num_transfers) % "Number of record and transfer operations",
        required("-i", "--num_images") & integer("num images", num_images) % "Number of images per record/transfer"
    );

    auto cli = (
        option("-h", "--help").set(help) % "Show documentation." |
        (
         common_options
        )
    );

    auto fmt = doc_formatting{}.doc_column(30);
    const char* exe_name = "pco_speedtest";
    parsing_result parse_result = parse(argc, argv, cli);
    if (!parse_result) {
        std::cerr << "Invalid arguments. See arguments below or use " << exe_name << " -h for more info\n";
        std::cerr << usage_lines(cli, exe_name, fmt) << '\n';
        return 1;
    }

    if (help) {
        std::cout << make_man_page(cli, exe_name, fmt) << '\n';
        return 0;
    }

    //
    // Execution
    //
    try {
        PCOCamera cam;
        cam.open();
        cam.reset_camera_settings();
        cam.set_recorder_mode_sequence();
        cam.arm_camera();
        cam.set_segment_sizes(num_images, 0, 0, 0);
        cam.set_framerate_exposure(1, 4000000, 1000000);
        cam.arm_camera();
        cam.set_active_segment(1);
        for (int i = 0; i < num_transfers; ++i) {
            auto begin = std::chrono::high_resolution_clock::now();
            cam.start_recording();
            cam.wait_for_recording_done();
            cam.transfer_internal(0, num_images, [](unsigned int, const PCOBuffer&){});
            cam.clear_active_segment();
            auto end = std::chrono::high_resolution_clock::now();
            std::cout << std::chrono::duration_cast<std::chrono::milliseconds>(end-begin).count() << "ms" << std::endl;
        }
        cam.close();
        return 0;
    }
    catch (const std::exception& ex) {
        std::cerr << ex.what() << std::endl;
    }
}
