#include "tiff_writer.hpp"
#include <stdexcept>

#include "tinytiffwriter.h"

class TiffWriterPimpl {
public:
    TinyTIFFWriterFile* tif = nullptr;
    std::string filename;
    unsigned int width;
    unsigned int height;
    unsigned int frames_written = 0;
    unsigned int file_number = 0;
};

TiffWriter::TiffWriter(std::string filename)
: p_impl(new TiffWriterPimpl())
{
    p_impl->filename = filename;
}

TiffWriter::~TiffWriter() {
    if (p_impl->tif != nullptr) {
        TinyTIFFWriter_close(p_impl->tif);
    }
}

std::string number_filename(std::string filename, unsigned int number) {
    std::size_t found = filename.find_last_of(".");
    if (found == std::string::npos) {
        return filename + "_" + std::to_string(number);
    } else {
        return filename.substr(0,found) + "_" + std::to_string(number) + "." + filename.substr(found+1);
    }
}

void TiffWriter::write_frame(unsigned int width, unsigned int height, uint16_t* data) {
    if (p_impl->tif == nullptr) {
        p_impl->tif = TinyTIFFWriter_open(p_impl->filename.c_str(), 16, TinyTIFFWriter_UInt, 0, width, height, TinyTIFFWriter_Greyscale);
        p_impl->width = width;
        p_impl->height = height;
        if (p_impl->tif == nullptr) {
            throw std::runtime_error("Could not open tiff file for writing");
        }
    }

    if (p_impl->width != width || p_impl->height != height) {
        throw std::runtime_error("Image size has to be the same for all frames in a tiff");
    }

    // Avoid writing over 4GiB boundary.
    // This should be a very conservative approximation, but doesn't matter if we make a few more files than neccessary.
    if (((unsigned long long)p_impl->frames_written) * (p_impl->width * p_impl->height * 2 + 4096) > (unsigned long long)1024*1024*1024*3) {
        TinyTIFFWriter_close(p_impl->tif);
        p_impl->file_number++;
        p_impl->frames_written = 0;

        p_impl->tif = TinyTIFFWriter_open(number_filename(p_impl->filename, p_impl->file_number).c_str(), 16, TinyTIFFWriter_UInt, 0, width, height, TinyTIFFWriter_Greyscale);
        if (p_impl->tif == nullptr) {
            throw std::runtime_error("Could not open tiff file for writing");
        }
    }

    if (TinyTIFFWriter_writeImage(p_impl->tif, data) != TINYTIFF_TRUE) {
        throw std::runtime_error("Writing frame failed");
    }
    p_impl->frames_written++;
}
