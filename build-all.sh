#!/usr/bin/env bash

# Linux X86, AMD64, ARM64
rm -rf build.lin.x86 build.lin.amd64 build.lin.arm64
cmake -DBUILD_LINUX=ON -DTARGET_ARCH="x86" -S . -B build.lin.x86 && cmake --build build.lin.x86 --config Release -- -j `nproc` || exit 1
cmake -DBUILD_LINUX=ON -DTARGET_ARCH="AMD64" -S . -B build.lin.amd64 && cmake --build build.lin.amd64 --config Release -- -j `nproc` || exit 1
cmake -DBUILD_LINUX=ON -DTARGET_ARCH="ARM64" -S . -B build.lin.arm64 && cmake --build build.lin.arm64 --config Release -- -j `nproc` || exit 1

# Windows
rm -rf build.win.x86 build.win.amd64
cmake -DBUILD_WIN=ON -DTARGET_ARCH="x86" -S . -B build.win.x86 && cmake --build build.win.x86 --config Release -- -j `nproc` || exit 1
cmake -DBUILD_WIN=ON -DTARGET_ARCH="AMD64" -S . -B build.win.amd64 && cmake --build build.win.amd64 --config Release -- -j `nproc` || exit 1

# Nintendo Wii
rm -rf build.wii; mkdir build.wii && cd build.wii && \
$DEVKITPRO/portlibs/wii/bin/powerpc-eabi-cmake -DCMAKE_BUILD_TYPE=Release -DBUILD_WII=ON .. || exit 1
make -j `nproc` || exit 1; cd ..
