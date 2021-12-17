# pco_matlab_wrapper

**This is quite unfinished, documentation may refer to a (hopefully) future state**

This repo contains a library to control and read images from pco cameras and perform MIP on the fly.
This library can be called from MATLAB.

It also contains a standalone command line tool to save MIPs as tiff files.

You need to have Visual Studio installed to build both of them.

## MATLAB wrapper
Run the file `buildMatlabWrapper.m` in MATLAB to build the library.
You have to set the path of the PCO SDK at the top of the file.

See `testMatlabWrapper.m` on how to use the library.

## Command line tool

To build the standalone command line tool I use meson. You can probably also set it up in Visual Studio if thats your thing.

- Install python3 (put it on the `PATH` to run the next command easily)
- Install meson `pip install meson`
- Adjust path to the PCO SDK in `meson.build`
- In code directory run
```
meson setup builddir
cd builddir
meson compile
```
- Add `$PCO_SDK_DIR/bin64` to your `PATH`
- Run `mip_tool.exe`