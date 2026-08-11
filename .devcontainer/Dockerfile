FROM --platform=linux/x86_64 mcr.microsoft.com/devcontainers/base:debian

ENV WORKSPACE=/workspace

RUN export DEBIAN_FRONTEND=noninteractive && \
    dpkg --add-architecture arm64 && \
    apt-get update && \
    apt-get -y install --no-install-recommends \
    cmake build-essential zstd \
    gcc-mingw-w64-x86-64 \
    gcc-12-aarch64-linux-gnu \
    libsdl2-dev libsdl2-dev:arm64 \
    libvorbis-dev libvorbis-dev:arm64 \
    libpng-dev libpng-dev:arm64 \
    libvpx-dev libvpx-dev:arm64 \
    libbluetooth-dev

# Windows MinGW X86_64 Libraries
RUN curl -L https://mirror.msys2.org/mingw/mingw64/mingw-w64-x86_64-SDL2-2.30.2-1-any.pkg.tar.zst | tar --use-compress-program=unzstd -xv -C /opt && \
    curl -L https://mirror.msys2.org/mingw/mingw64/mingw-w64-x86_64-zlib-1.3.1-1-any.pkg.tar.zst | tar --use-compress-program=unzstd -xv -C /opt && \
    curl -L https://mirror.msys2.org/mingw/mingw64/mingw-w64-x86_64-libvorbis-1.3.7-2-any.pkg.tar.zst | tar --use-compress-program=unzstd -xv -C /opt && \
    curl -L https://mirror.msys2.org/mingw/mingw64/mingw-w64-x86_64-libogg-1.3.5-1-any.pkg.tar.zst | tar --use-compress-program=unzstd -xv -C /opt && \
    curl -L https://mirror.msys2.org/mingw/mingw64/mingw-w64-x86_64-libpng-1.6.43-1-any.pkg.tar.zst | tar --use-compress-program=unzstd -xv -C /opt && \
    curl -L https://mirror.msys2.org/mingw/mingw64/mingw-w64-x86_64-libvpx-1.14.0-1-any.pkg.tar.zst | tar --use-compress-program=unzstd -xv -C /opt

# Base Directories
RUN mkdir -p "${WORKSPACE}" && chmod -R 777 "$WORKSPACE"

# Set Volume and Workdir
VOLUME $WORKSPACE
WORKDIR $WORKSPACE
