#include <iostream>
#include <cstring>
#include <cstdio>
#include "tiff_writer.hpp"

int main(int argc, char** argv) {
    bool success = true;
    char * filename = tmpnam(nullptr);
    try {
        std::string filename_str(filename);
        std::cout << "Creating test tiff in " << filename_str << std::endl;
        TiffWriter tw(filename_str + ".tif");
        uint16_t* buffer = new uint16_t[1008*1008]();
        memset(buffer, 111, 1008*1008*2);
        for (int i = 0; i < 10; ++i) {
            tw.write_frame(1008, 1008, buffer);
        }

        //TODO Create ~2500 frames to test 4GiB limit

        delete[] buffer;
    } catch (...) {
        success = false;
    }
    if (remove(filename) != 0) {
        std::cerr << "Could not delete temp file" << std::endl;
    }

    return success? 0:1;
}
