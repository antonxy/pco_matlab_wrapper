# pco_matlab_wrapper

This repo contains a library to control and read images from pco cameras and perform maximum intensity projection (MIP) on the fly.
This library can be called from MATLAB.

It also contains a standalone command line tool to save MIPs as tiff files.

You need to have Visual Studio installed to build both of them.

## Building
- Install python3 (put it on the `PATH` to run the next command easily)
- Install meson `pip install meson`
- Adjust path to the PCO SDK in `meson.build`
- In code directory run
  ```
  meson setup builddir -Dbuildtype=release  # MATLAB can only link to release build
  cd builddir
  meson compile
  ```
- Add `$PCO_SDK_DIR/bin64` to your `PATH` or copy `SC2_Cam.dll` and related camera driver dlls to the current folder.
- Run `pco_transfer.exe -h` to see available options

## MATLAB wrapper
To additionally build the MATLAB wrapper run the file `buildMatlabWrapper.m`.
You have to set the path of the PCO SDK at the top of the file also.

You can run this file from the command-line using:
```
cd matlab
matlab -batch buildMatlabWrapper
```

See `testMatlabWrapper.m` on how to use the library.

This is also useful for testing the MATLAB library as unloading a library is not possible in MATLAB.
This way you get a new instance of MATLAB each time.
```
matlab -batch testMatlabWrapper
```

The MATLAB wrapper can be used to script multiple recording operations.

## Command line tool
- Add `$PCO_SDK_DIR/bin64` to your `PATH` or copy `SC2_Cam.dll` and related camera driver dlls to the current folder.
- Run `pco_transfer.exe -h` to see available options

## Run tests
- Connect PC to camera
- Run `meson test`

On my PC at least `meson test` must be run from the *Visual Studio Native Tools Command Prompt*,
otherwise it doesn't find `ninja` even though `meson build` can find it.

## Important notes
The library does not check if a file already exists. Make sure you are not overwriting important files.
If a tiff file gets too large it will be split and a number appended to the filename, e.g. `file.tiff -> file_1.tiff -> file_2.tiff`.
So you also have to make sure no file will be overwritten if a number gets appended to the filename.
