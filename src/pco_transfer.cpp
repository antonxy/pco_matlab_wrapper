#include <iostream>
#include "clipp.hpp"
#include "pco_wrapper.hpp"

using namespace clipp;

int main(int argc, char** argv) {
	//
	// Command line options
	//

	bool help = false;

	enum class mode { none, mip, full_transfer };
	mode selected = mode::none;

	//MIP mode
	unsigned int num_mips = std::numeric_limits<unsigned int>::max();
	unsigned int images_per_mip = 0;

	//Full transfer
	unsigned int num_images = std::numeric_limits<unsigned int>::max();

	//Common
	unsigned int skip_images = 0;
	std::string outpath = "";
	int segment = 1;

	auto mip_command = (
		command("mip").set(selected, mode::mip) % "MIP transfer",
		required("-i", "--images_per_mip") & integer("images per mip", images_per_mip) % "Number of images in each MIP. num_mips * images_per_mip will be transferred.",
		option("-m", "--num_mips")& integer("num mips", num_mips) % "Number of MIPs to transfer"
	);

	auto full_transfer_command = (
		command("full").set(selected, mode::full_transfer) % "Full Transfer",
		option("-n", "--num_images") & integer("num images", num_images) % "Number of images to transfer"
	);

	auto common_options = (
		option("-s", "--skip_images") & integer("skip images", skip_images) % "Number of images to skip before first MIP.",
		option("--segment") & integer("segment", segment) % "Camera RAM segment. Index starts at 1.",
		value("output path", outpath)
	);

	auto cli = (
		option("-h", "--help").set(help) % "Show documentation." |
		(
			(mip_command | full_transfer_command),
			common_options
		)
	);

	auto fmt = doc_formatting{}.doc_column(30);
	const char* exe_name = "pco_transfer";
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
		cam.set_active_segment(segment);
		if (selected == mode::mip) {
			cam.transfer_mip_to_tiff(skip_images, images_per_mip, num_mips, outpath);
		}
		else if (selected == mode::full_transfer) {
			cam.transfer_to_tiff(skip_images, num_images, outpath);
		}
		cam.close();
		return 0;
	}
	catch (const std::exception& ex) {
		std::cerr << ex.what() << std::endl;
	}
}
