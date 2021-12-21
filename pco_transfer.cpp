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
		cam.open(0);
		if (selected == mode::mip) {
			TinyTIFFWriterFile* tif;
			//TODO Split image if > 4GiB
			std::unique_ptr<uint16_t[]> MIP_buffer;
			unsigned int images_to_transfer = images_per_mip * num_mips;
			if (num_mips == std::numeric_limits<unsigned int>::max()) {
				images_to_transfer = std::numeric_limits<unsigned int>::max();
			}
			unsigned int transferred_images = 0;
			unsigned int transferred_mips = 0;
			cam.transfer_internal(segment, skip_images, images_per_mip * num_mips, [&tif, outpath, &MIP_buffer, images_per_mip, &transferred_images, &transferred_mips](unsigned int transfer_image_index, const PCOBuffer& buffer) {
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
		else if (selected == mode::full_transfer) {
			TinyTIFFWriterFile* tif;
			//TODO Split image if > 4GiB
			unsigned int transferred_images = 0;
			cam.transfer_internal(segment, skip_images, num_images, [&tif, outpath, &transferred_images](unsigned int transfer_image_index, const PCOBuffer& buffer) {
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
		cam.close();
		return 0;
	}
	catch (const std::exception& ex) {
		std::cerr << ex.what() << std::endl;
	}
}