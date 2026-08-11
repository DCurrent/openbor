#!/usr/bin/env bash

# Linux AMD64, ARM64
rm -rf build.lin.amd64 build.lin.arm64
cmake -DBUILD_LINUX=ON -DTARGET_ARCH="AMD64" -S . -B build.lin.amd64 && cmake --build build.lin.amd64 --config Release -- -j `nproc` || exit 1
cmake -DBUILD_LINUX=ON -DTARGET_ARCH="ARM64" -S . -B build.lin.arm64 && cmake --build build.lin.arm64 --config Release -- -j `nproc` || exit 1

# Windows AMD64
rm -rf build.win.amd64
cmake -DBUILD_WIN=ON -DTARGET_ARCH="AMD64" -S . -B build.win.amd64 && cmake --build build.win.amd64 --config Release -- -j `nproc` || exit 1
