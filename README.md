# pco_matlab_wrapper

This repo contains a library to control and read images from pco cameras and perform maximum intensity projection (MIP) on the fly.
This library can be called from MATLAB.

It also contains a standalone command line tool to save MIPs as tiff files.

You need to have Visual Studio installed to build both of them.

## MATLAB wrapper
Run the file `buildMatlabWrapper.m` in MATLAB to build the library.
You have to set the path of the PCO SDK at the top of the file.

See `testMatlabWrapper.m` on how to use the library.

## Command line tool

To build the standalone command line tool I use meson.

- Install python3 (put it on the `PATH` to run the next command easily)
- Install meson `pip install meson`
- Adjust path to the PCO SDK in `meson.build`
- In code directory run
  ```
  meson setup builddir
  cd builddir
  meson compile
  ```
- Add `$PCO_SDK_DIR/bin64` to your `PATH` or copy `SC2_Cam.dll` and related camera driver dlls to the current folder.
- Run `pco_transfer.exe -h` to see available options

### Issues
- **Currently tiff files bigger than 4GiB can be written but will lead to a corrupt file**
- It's possible to crash the application in a few ways. E.g. transferring 0 images.