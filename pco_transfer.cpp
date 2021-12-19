#include <iostream>
#include "clipp.hpp"
#include "pco_wrapper.hpp"
#include "tinytiffwriter.h"

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

	//TODO validate arguments, e.g. don't allow strings, negative numbers

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
			TinyTIFFWriterFile* tif;
			//TODO Split image if > 4GiB
			std::unique_ptr<uint16_t[]> MIP_buffer;
			cam.transfer_internal(segment, skip_images + 1, images_per_mip * num_mips, [&tif, outpath, &MIP_buffer, images_per_mip](unsigned int transfer_image_index, const PCOBuffer& buffer) {
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

					//Reset MIP
					memset(MIP_buffer.get(), 0, numPix * sizeof(uint16_t));
				}
			});
			TinyTIFFWriter_close(tif);
		}
		else if (selected == mode::full_transfer) {
			TinyTIFFWriterFile* tif;
			//TODO Split image if > 4GiB
			cam.transfer_internal(segment, skip_images + 1, num_images, [&tif, outpath](unsigned int transfer_image_index, const PCOBuffer& buffer) {
				if (transfer_image_index == 0) {
					// Allocate image after we receive first one because then we know the image size
					tif = TinyTIFFWriter_open(outpath.c_str(), 16, TinyTIFFWriter_UInt, 0, buffer.xres, buffer.yres, TinyTIFFWriter_Greyscale);
					if (!tif) {
						throw std::runtime_error("Could not create tiff file");
					}
				}

				TinyTIFFWriter_writeImage(tif, buffer.addr);
			});
			TinyTIFFWriter_close(tif);
		}
		cam.close();
		return 0;
	}
	catch (const std::exception& ex) {
		std::cerr << ex.what() << std::endl;
	}
}