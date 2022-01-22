#include "tiff_writer.hpp"
#include <stdexcept>

#include "tinytiffwriter.h"

class TiffWriterPimpl {
public:
    TinyTIFFWriterFile* tif = nullptr;
    std::string filename;
    unsigned int width;
    unsigned int height;
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

    //TODO avoid writing over 4GiB boundary

    if (TinyTIFFWriter_writeImage(p_impl->tif, data) != TINYTIFF_TRUE) {
        throw std::runtime_error("Writing frame failed");
    }
}
