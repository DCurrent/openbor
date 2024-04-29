# General Overview
The build configuration management tool used for OpenBOR is cmake with a minimum requirement of v3.22.  Cmake can be installed via a standard package managing tool such as APT (Debian/Ubuntu) or can be downloaded/compiled for your development environment of choice via https://cmake.org or which ever packaging management tool you use.

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

# Docker
In this repository we also have support for Docker which allows us to build all supported targets.

    .devcontainer/Dockerfile

## Prerequists
Docker can be installed via a standard package managing tool such as APT (Debian/Ubuntu) or downloaded directly from https://www.docker.com/products/docker-desktop

First time users should install Docker Desktop as it provides a visual interface to manage images, containers and still provides the command line tools.

## Setting up Environment (VSCode)
Open the root folder via VSCode explorer and the IDE will automatically detect and ask permissions to install all necessary plugins.  Once completed you maybe prompted to re-open the project within a container: Click Yes to restart VSCode within the container and simply use the Built-In Terminal to invoke your cmake build commands.

At this point you are ready to build all supported platforms from within the VSCode Terminal.  A script is provided to generate all build targets for distribution.

Terminal Tab:


        vscode ➜ /workspaces/openbor (compiling) $ ./build-all.sh

## Setting up Environment (Manually)
Using the Docker command line tool we will create a base image and then start a container for building a . single or multiple cross-platform targets.

### Creating Base Image
Creating a base image only needs to be done once unless updates were performed to the Dockerfile then a new base image will need to be regenerated.  Assuming you are in the root directory of the repository type the following and wait for the process to complete.

    cd .devcontainer
    docker build -t openbor .

### Instanciating a Container
Return back to the the root of the repository and now you can invoke your development environment which will automatically mount the root directory of the repository and we can now build the targets.

        docker run -it --rm -v $(pwd):/workspace openbor

        root@7e7774eba72b:/workspace$ ls -l

            total 44
            -rw-r--r--  1 root root 6296 Apr 29 11:00 CMakeLists.txt
            -rw-r--r--  1 root root   88 Apr 20 15:13 CODEOWNERS
            -rw-r--r--  1 root root 3211 Apr 20 15:13 CODE_OF_CONDUCT.md
            -rw-r--r--  1 root root   30 Apr 29 11:02 COMPILING.md
            -rw-r--r--  1 root root 2408 Apr 20 15:13 CONTRIBUTING.md
            -rw-r--r--  1 root root 1537 Apr 20 15:13 LICENSE
            -rw-r--r--  1 root root  197 Apr 20 15:13 PULL_REQUEST_TEMPLATE.md
            -rw-r--r--  1 root root 8059 Apr 20 15:13 README.md
            -rwxr-xr-x  1 root root 1053 Apr 23 21:15 build-all.sh
            drwxr-xr-x  8 root root  256 Apr 29 11:00 cmake
            drwxr-xr-x 27 root root  864 Apr 25 01:03 engine
            drwxr-xr-x  3 root root   96 Apr 20 15:13 media
            drwxr-xr-x 14 root root  448 Apr 20 15:13 tools
        

### Building from within a Container
Using the build scripts provided in the repository we can now ensure that all supported targets can be built successfully and are ready for distribution.  Once completed simply type exit to end your container session.

        root@7e7774eba72b:/workspace$ ./build-all.sh
        
        root@7e7774eba72b:/workspace$ ls -l engine/releases/
        
            total 20
            -rw-r--r-- 1 root root   98 Apr 20 15:13 COMPILING.txt
            drwxr-xr-x 3 root root   96 Apr 21 01:28 DARWIN
            -rw-r--r-- 1 root root 1537 Apr 20 15:13 LICENSE.txt
            drwxr-xr-x 9 root root  288 Apr 21 01:24 LINUX
            -rw-r--r-- 1 root root  314 Apr 20 15:13 README.txt
            drwxr-xr-x 7 root root  224 Apr 21 01:24 WII
            drwxr-xr-x 8 root root  256 Apr 21 01:24 WINDOWS
            -rw-r--r-- 1 root root 5871 Apr 20 15:13 translation.txt

        root@7e7774eba72b:/workspace$ exit

### Single line command for building all targets

    docker run -it --rm -v $(pwd):/workspace openbor ./build-all.sh
