FROM --platform=linux/x86_64 mcr.microsoft.com/devcontainers/base:debian

RUN export DEBIAN_FRONTEND=noninteractive && \
    dpkg --add-architecture i386 && \
    dpkg --add-architecture arm64 && \
    apt-get update && \
    apt-get -y install --no-install-recommends \
    cmake build-essential yasm \
    gcc-mingw-w64 \
    gcc-12-i686-linux-gnu \
    gcc-12-aarch64-linux-gnu \
    libsdl2-dev libsdl2-dev:i386 libsdl2-dev:arm64 \
    libvorbis-dev libvorbis-dev:i386 libvorbis-dev:arm64 \
    libpng-dev libpng-dev:i386 libpng-dev:arm64 \
    libvpx-dev libvpx-dev:i386 libvpx-dev:arm64
