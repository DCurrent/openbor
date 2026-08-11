# OpenBOR Compilation Guide

OpenBOR requires a 64-bit target architecture. Desktop builds use CMake 3.22 or newer with the root `CMakeLists.txt` and platform modules under `cmake/`. Android uses the Gradle/NDK project under `engine/android/`.

The accepted desktop `TARGET_ARCH` values are `AMD64`, `ARM64`, and `UNIVERSAL`, with availability depending on the target platform. Android targets the 64-bit `arm64-v8a` ABI. The legacy engine Makefile and shell build system are no longer supported.

Git must be available on `PATH`. Build workflows invoke `engine/version.sh`, which reads the current Git revision to generate the engine version information.

## Desktop Build Configuration

The root CMake configuration selects one desktop platform target:

- `BUILD_WIN=ON` - Windows
- `BUILD_LINUX=ON` - Linux
- `BUILD_DARWIN=ON` - macOS

If no platform option is supplied, CMake attempts to detect the native host platform automatically. Explicit platform and architecture options are recommended for reproducible builds.

The primary architecture targets are:

- Windows: `AMD64`
- Linux: `AMD64` or `ARM64`
- macOS: `ARM64`
- macOS universal builds: `UNIVERSAL` when the required dual-architecture dependencies are installed
- Android: `arm64-v8a` through Gradle/NDK

Windows ARM64 is recognized by the configuration layer, but MinGW ARM64 cross-compilation is not currently supported.

# Direct Desktop Builds with Ninja

Docker is not required to build OpenBOR. For normal desktop development, the simplest build path is a native compiler environment with CMake and Ninja.

The general pattern is:

```sh
rm -rf build
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release -D<PLATFORM>=ON -DTARGET_ARCH=<ARCH>
cmake --build build --parallel
```

For a debug build, replace `Release` with `Debug`.

Ninja is a single-configuration generator, so `CMAKE_BUILD_TYPE` is selected when configuring. A separate `--config Release` argument is not required when building.

## Windows AMD64 - MSYS2 UCRT64

The recommended direct Windows environment is MSYS2 UCRT64.

Install MSYS2, then open the **MSYS2 UCRT64** terminal. Install the compiler, CMake, Ninja, Git, and OpenBOR dependencies:

```sh
pacman -S --needed \
    git \
    mingw-w64-ucrt-x86_64-gcc \
    mingw-w64-ucrt-x86_64-cmake \
    mingw-w64-ucrt-x86_64-ninja \
    mingw-w64-ucrt-x86_64-SDL2 \
    mingw-w64-ucrt-x86_64-zlib \
    mingw-w64-ucrt-x86_64-libvorbis \
    mingw-w64-ucrt-x86_64-libogg \
    mingw-w64-ucrt-x86_64-libpng \
    mingw-w64-ucrt-x86_64-libvpx
```

From the root of the OpenBOR repository:

```sh
rm -rf build
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release -DBUILD_WIN=ON -DTARGET_ARCH=AMD64
cmake --build build --parallel
```

The packaged executable is written to:

```text
engine/releases/WINDOWS/OpenBOR-x64.exe
```

Run the commands from the UCRT64 terminal rather than the plain MSYS shell so CMake uses the UCRT64 compiler and libraries.

## Linux AMD64

On Debian or Ubuntu, install the native compiler, CMake, Ninja, Git, and required libraries:

```sh
sudo apt-get update
sudo apt-get install -y \
    build-essential \
    git \
    cmake \
    ninja-build \
    libsdl2-dev \
    libvorbis-dev \
    libpng-dev \
    libvpx-dev
```

Configure and build from the repository root:

```sh
rm -rf build
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release -DBUILD_LINUX=ON -DTARGET_ARCH=AMD64
cmake --build build --parallel
```

The packaged executable is written to:

```text
engine/releases/LINUX/OpenBOR
```

## Linux ARM64

On a native ARM64 Linux system, install the same native dependencies and configure with `TARGET_ARCH=ARM64`:

```sh
rm -rf build
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release -DBUILD_LINUX=ON -DTARGET_ARCH=ARM64
cmake --build build --parallel
```

The packaged executable is written to:

```text
engine/releases/LINUX/OpenBOR-arm64
```

For Linux ARM64 cross-compilation from an AMD64 host, the Docker/dev-container environment is generally easier because it already provides the AArch64 compiler and ARM64 development libraries.

## macOS ARM64

Install CMake, Ninja, and the required Homebrew dependencies:

```sh
brew install cmake ninja SDL2 libvorbis libogg libvpx
```

Configure and build from the repository root:

```sh
rm -rf build
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release -DBUILD_DARWIN=ON -DTARGET_ARCH=ARM64
cmake --build build --parallel
```

The application bundle is written to:

```text
engine/releases/DARWIN/OpenBOR.app
```

Universal macOS builds require both ARM64 and x86-64 Homebrew dependency trees and are an advanced configuration. See `cmake/macos-finalize.cmake` for the additional universal-build prefix requirements.

# Android ARM64 Builds

Android does not use the root CMake build. It uses the Gradle project and Android NDK configuration under `engine/android/` and currently targets only the 64-bit `arm64-v8a` ABI.

The Android project currently requires:

- JDK 17
- Android SDK Platform 35
- Android Build Tools 36.0.0
- Android NDK 21.4.7075529

Install the Android command-line tools using the normal Android SDK setup for your operating system, then install the exact project packages with `sdkmanager`:

```sh
sdkmanager "platforms;android-35" "build-tools;36.0.0" "ndk;21.4.7075529"
```

Make sure `ANDROID_SDK_ROOT` or `ANDROID_HOME` points to your Android SDK installation and Java 17 is active.

From the repository root, generate the version information and build a debug APK with the checked-in Gradle wrapper:

```sh
cd engine
bash version.sh
cd android
chmod +x gradlew
./gradlew --no-daemon clean assembleDebug
```

The debug APK is written to:

```text
engine/android/app/build/outputs/apk/debug/OpenBOR.apk
```

Release APKs require signing configuration. See `engine/android/README` for keystore and standalone-game packaging details.

GitHub Actions performs the same ARM64 debug build on every push and pull request, including installation of the pinned SDK, build-tools, and NDK packages.

# Docker and Dev Container Builds

The repository also provides a Docker/dev-container environment for desktop cross-platform compilation. This is useful when building several supported desktop targets from one host or when a native toolchain is inconvenient.

The environment is defined by:

```text
.devcontainer/Dockerfile
.devcontainer/devcontainer.json
```

The current container includes toolchains and dependencies for:

- Linux AMD64
- Linux ARM64
- Windows AMD64 cross-compilation

Android is built separately with its Gradle/NDK toolchain and is not part of `build-all.sh`.

## VS Code Dev Container

Open the repository in Visual Studio Code with the Dev Containers extension installed and reopen the workspace in the provided container. From the integrated terminal, the aggregate build script can be run directly:

```sh
./build-all.sh
```

## Manual Docker Setup

Build the image from the repository root:

```sh
docker build -t openbor .devcontainer
```

Start a temporary container with the repository mounted at `/workspace`:

```sh
docker run -it --rm -v "$(pwd):/workspace" openbor
```

Inside the container, build all configured aggregate desktop targets:

```sh
./build-all.sh
```

The aggregate script currently builds:

- Linux AMD64
- Linux ARM64
- Windows AMD64

A one-line Docker build invocation is also available:

```sh
docker run -it --rm -v "$(pwd):/workspace" openbor ./build-all.sh
```

# Build Output

The main output locations are:

```text
engine/releases/WINDOWS/OpenBOR-x64.exe
engine/releases/LINUX/OpenBOR
engine/releases/LINUX/OpenBOR-arm64
engine/releases/DARWIN/OpenBOR.app
engine/android/app/build/outputs/apk/debug/OpenBOR.apk
```

Desktop platform modules copy completed builds into `engine/releases/`. Android keeps its APK under the Gradle build output directory.

The build workflows also refresh `engine/version.h`, `engine/version.txt`, and platform release metadata through `engine/version.sh`.
