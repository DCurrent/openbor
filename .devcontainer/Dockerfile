FROM --platform=linux/x86_64 mcr.microsoft.com/devcontainers/base:debian

RUN export DEBIAN_FRONTEND=noninteractive && \
    dpkg --add-architecture i386 && \
    dpkg --add-architecture arm64 && \
    apt-get update && \
    apt-get -y install --no-install-recommends \
    cmake build-essential zstd \
    gcc-mingw-w64 \
    gcc-12-i686-linux-gnu \
    gcc-12-aarch64-linux-gnu \
    libsdl2-dev libsdl2-dev:i386 libsdl2-dev:arm64 \
    libvorbis-dev libvorbis-dev:i386 libvorbis-dev:arm64 \
    libpng-dev libpng-dev:i386 libpng-dev:arm64 \
    libvpx-dev libvpx-dev:i386 libvpx-dev:arm64 \
    libbluetooth-dev

# Windows MinGW X86_64 and I686 Libraries
RUN curl -L https://mirror.msys2.org/mingw/mingw64/mingw-w64-x86_64-SDL2-2.30.2-1-any.pkg.tar.zst | tar --use-compress-program=unzstd -xv -C /opt && \
    curl -L https://mirror.msys2.org/mingw/mingw64/mingw-w64-x86_64-zlib-1.3.1-1-any.pkg.tar.zst | tar --use-compress-program=unzstd -xv -C /opt && \
    curl -L https://mirror.msys2.org/mingw/mingw64/mingw-w64-x86_64-libvorbis-1.3.7-2-any.pkg.tar.zst | tar --use-compress-program=unzstd -xv -C /opt && \
    curl -L https://mirror.msys2.org/mingw/mingw64/mingw-w64-x86_64-libogg-1.3.5-1-any.pkg.tar.zst | tar --use-compress-program=unzstd -xv -C /opt && \
    curl -L https://mirror.msys2.org/mingw/mingw64/mingw-w64-x86_64-libpng-1.6.43-1-any.pkg.tar.zst | tar --use-compress-program=unzstd -xv -C /opt && \
    curl -L https://mirror.msys2.org/mingw/mingw64/mingw-w64-x86_64-libvpx-1.14.0-1-any.pkg.tar.zst | tar --use-compress-program=unzstd -xv -C /opt && \
    curl -L https://mirror.msys2.org/mingw/mingw32/mingw-w64-i686-SDL2-2.30.2-1-any.pkg.tar.zst | tar --use-compress-program=unzstd -xv -C /opt && \
    curl -L https://mirror.msys2.org/mingw/mingw32/mingw-w64-i686-zlib-1.3.1-1-any.pkg.tar.zst | tar --use-compress-program=unzstd -xv -C /opt && \
    curl -L https://mirror.msys2.org/mingw/mingw32/mingw-w64-i686-libvorbis-1.3.7-2-any.pkg.tar.zst | tar --use-compress-program=unzstd -xv -C /opt && \
    curl -L https://mirror.msys2.org/mingw/mingw32/mingw-w64-i686-libogg-1.3.5-1-any.pkg.tar.zst | tar --use-compress-program=unzstd -xv -C /opt && \
    curl -L https://mirror.msys2.org/mingw/mingw32/mingw-w64-i686-libpng-1.6.43-1-any.pkg.tar.zst | tar --use-compress-program=unzstd -xv -C /opt && \
    curl -L https://mirror.msys2.org/mingw/mingw32/mingw-w64-i686-libvpx-1.14.0-1-any.pkg.tar.zst | tar --use-compress-program=unzstd -xv -C /opt

# DevkitPRO
COPY installDevkitPro.sh .
RUN ./installDevkitPro.sh
