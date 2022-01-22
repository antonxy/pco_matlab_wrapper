#include <string>
#include <memory>
#include <cstdint>

class TiffWriterPimpl;

class TiffWriter {
public:
    TiffWriter(std::string filename);
    ~TiffWriter();
    void write_frame(unsigned int width, unsigned int height, uint16_t* data);
private:
    std::unique_ptr<TiffWriterPimpl> p_impl;
};
