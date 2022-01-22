#include <iostream>
#include <cstring>
#include "tiff_writer.hpp"

int main(int argc, char** argv) {
    TiffWriter tw("foo.tiff");
    char* buffer = new char[1008*1008*2]();
    memset(buffer, 111, 1008*1008*2);
    for (int i = 0; i < 10; ++i) {
        tw.write_frame(1008, 1008, 16, buffer);
    }
    delete[] buffer;
}
