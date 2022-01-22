#include "tiff_writer.hpp"
#include <stdexcept>

#include "tinytiffwriter.h"

class TiffWriterPimpl {
public:
    TinyTIFFWriterFile* tif = nullptr;
    std::string filename;
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

void TiffWriter::write_frame(unsigned int width, unsigned int height, unsigned short bits_per_pixel, char* data) {
    if (bits_per_pixel != 16) {
        throw std::runtime_error("Writing tiff is only implemented to 16bit");
    }

    if (p_impl->tif == nullptr) {
        p_impl->tif = TinyTIFFWriter_open(p_impl->filename.c_str(), 16, TinyTIFFWriter_UInt, 0, width, height, TinyTIFFWriter_Greyscale);
        if (p_impl->tif == nullptr) {
            throw std::runtime_error("Could not open tiff file for writing");
        }
    }

    //TODO avoid writing over 4GiB boundary

    if (!TinyTIFFWriter_writeImage(p_impl->tif, data)) {
        throw std::runtime_error("Writing frame failed");
    }
}
