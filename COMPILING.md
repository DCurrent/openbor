# General Overview
The build configuration management tool used for OpenBOR is cmake with a minimum requirement of v3.22.  Cmake can be installed via a standard package managing tool such as APT (Debian/Ubuntu) or can be downloaded/compiled for your development environment of choice via https://cmake.org

# File Hierarchy
Within the root of the OpenBOR repository you will find CMakeLists.txt which contains the base configuration which covers all the pre-processor features supported by OpenBOR.  A subdirectory labled "cmake" contains target modules which are used to configure your OpenBOR for you platform of choice.  Currently supported is the following list of targets:

    Linux   (linux.cmake)
    Windows (windows.cmake, windows-finalize.cmake)
    Darwin  (macos.cmake, macos-finalize.cmake)
    Wii     (wii.cmake)

# Building Target
Cmake provides various options for building a targets in general, however we are only going to focus on a subset of options.  Typically running the cmake command without parameters is enough for configuration system to identify which host you are running cmake on and building for that specific target.  A few example are provided below to showcase how to configure and build OpenBOR target.

## Configuring and Building Target
Clear previous build directory, Set new configuration for distribution and build target:

    rm -rf build && cmake -S . --config Release -- -j `nprog`
    engine/releases/[TARGET]/OpenBOR

Clear previous build directory, Set new configuration for debuging the build target:

    rm -rf build && cmake -S . --config Debug -- -j `nprog`
    engine/releases/[TARGET]/OpenBOR

## Alternative Build Commands
Clear previous build directory, Set new configuration for distribution and build target:

    rm -rf build
    mkdir build
    cmake .. -DCMAKE_BUILD_TYPE=Release
    make -j `nproc`
    
    engine/releases/[TARGET]/OpenBOR

# Docker Cross-Compilation
In this repository we also have support for Docker which allows us to cross-compile the currently list of supported targets.

    .devcontainer/Dockerfile

