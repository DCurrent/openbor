#!/usr/bin/env bash

curl -L https://apt.devkitpro.org/install-devkitpro-pacman > install-devkitpro-pacman || exit 1
chmod +x install-devkitpro-pacman || exit 1

./install-devkitpro-pacman << END_OF_INPUT
y
END_OF_INPUT

rm install-devkitpro-pacman || exit 1
ln -s /proc/self/mounts /etc/mtab

/opt/devkitpro/pacman/bin/pacman -S wii-dev ppc-libpng ppc-zlib ppc-libvorbis ppc-libvorbisidec ppc-libogg << END_OF_INPUT

y
END_OF_INPUT

# Clone and install Wii-U Pro Controller Support
git clone https://github.com/SumolX/libwupc.git && \
cp -a libwupc/include/wupc /opt/devkitpro/portlibs/ppc/include/ && \
cp -a libwupc/lib/* /opt/devkitpro/portlibs/ppc/lib/

# Clone and install portlibs compatible ppc VPX development files
git clone https://github.com/SumolX/portlibs-ppc-libvpx.git
cp -a portlibs-ppc-libvpx/include/* /opt/devkitpro/portlibs/ppc/include/ && \
cp -a portlibs-ppc-libvpx/lib/* /opt/devkitpro/portlibs/ppc/lib/
