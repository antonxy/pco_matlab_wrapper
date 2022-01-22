#include <string>
#include <memory>

class TiffWriterPimpl;

class TiffWriter {
public:
    TiffWriter(std::string filename);
    ~TiffWriter();
    void write_frame(unsigned int width, unsigned int height, unsigned short bits_per_pixel, char* data);
private:
    std::unique_ptr<TiffWriterPimpl> p_impl;
};
