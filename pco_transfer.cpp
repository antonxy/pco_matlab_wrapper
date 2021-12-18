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
	unsigned int num_mips = 0;
	unsigned int images_per_mip = 0;

	//Full transfer
	unsigned int num_images = 0;

	//Common
	unsigned int skip_images = 0;
	std::string outpath = "";
	int segment = 1;

	//TODO allow transfer all without setting num_mips/num_images
	//First have to implement saving to files

	auto mip_command = (
		command("mip").set(selected, mode::mip) % "MIP transfer",
		required("-m", "--num_mips") & value("num mips", num_mips) % "Number of MIPs to transfer",
		required("-i", "--images_per_mip") & value("images per mip", images_per_mip) % "Number of images in each MIP. num_mips * images_per_mip will be transferred."
	);

	auto full_transfer_command = (
		command("full").set(selected, mode::full_transfer) % "Full Transfer",
		required("-n", "--num_images") & value("num images", num_images) % "Number of images to transfer"
	);

	auto common_options = (
		option("-s", "--skip_images") & value("skip images", skip_images) % "Number of images to skip before first MIP.",
		option("--segment") & value("segment", segment) % "Camera RAM segment. Index starts at 1.",
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
		cam.open(0);
		if (selected == mode::mip) {
			cam.transfer_mip(segment, skip_images + 1, images_per_mip, num_mips);
		}
		else if (selected == mode::full_transfer) {
			cam.transfer(segment, skip_images + 1, num_images);
		}
		cam.close();
		return 0;
	}
	catch (const std::exception& ex) {
		std::cerr << ex.what() << std::endl;
	}
}