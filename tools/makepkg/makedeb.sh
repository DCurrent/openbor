# Copyright (c) 2010 Bryan Cain
# Released under the terms of the GNU General Public License, version 3.

#!/bin/bash

function make_control {
rm -rf control
echo "Package: openbor
Version: $VERSION_MAJOR.$VERSION_MINOR.$VERSION_BUILD
Architecture: i386
Maintainer: The OpenBOR Team
Installed-Size: 2505898
Depends: libsdl1.2debian, libsdl-mixer1.2, libsdl-image1.2, libsdl-gfx1.2-4, libpng12-0, zlib1g, libc6
Section: games
Priority: optional
Homepage: http://lavalit.com/
Description: OpenBOR - Ultimate 2D Game Engine
 
 OpenBOR is a highly advanced continuation of the Beats Of Rage beat 'em up 
 engine. Although it was originally designed for beat 'em ups, it is flexible 
 enough to support many other 2D genres as well.  Visit the official site, 
 www.LavaLit.com, for all news, events, and releases of the engine and game 
 modules." >> control
}

function prepare_files {
mkdir -p /tmp/debpackage/DEBIAN/
mkdir -p /tmp/debpackage/usr/games/
mkdir -p /tmp/debpackage/usr/share/applications/
mkdir -p /tmp/debpackage/usr/share/pixmaps/

mv control /tmp/debpackage/DEBIAN/
mv openbor /tmp/debpackage/usr/games/
mv openbor.desktop /tmp/debpackage/usr/share/applications/
mv openbor.xpm /tmp/debpackage/usr/share/pixmaps/
}

. common.sh
make_control
prepare_files
dpkg-deb --build /tmp/debpackage/ openbor_$VERSION_MAJOR.$VERSION_MINOR.$VERSION_BUILD.deb
rm -rf /tmp/debpackage

