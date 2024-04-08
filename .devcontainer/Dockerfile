FROM --platform=linux/x86_64 mcr.microsoft.com/devcontainers/base:ubuntu

RUN export DEBIAN_FRONTEND=noninteractive && \
    apt-get update && \
    apt-get -y install --no-install-recommends \
	cmake build-essential yasm \
	libsdl2-dev libsdl2-gfx-dev \
	libvorbis-dev libpng-dev libvpx-dev
