# Copyright (c) 2010 Bryan Cain
# Released under the terms of the GNU General Public License, version 3.

#!/bin/bash

function check_files {
if test ! -f ../../releases/LINUX/OpenBOR/OpenBOR; then 
  echo "OpenBOR executable (releases/LINUX/OpenBOR/OpenBOR) not found."
  exit
elif test ! -f ../../resources/OpenBOR_Icon_128x128.h; then
  echo "OpenBOR icon (resources/OpenBOR_Icon_128x128.h) not found."
  exit
fi
}

function export_version {
export VERSION_NAME="OpenBOR"
export VERSION_MAJOR=3
export VERSION_MINOR=0
CURDIR=`pwd`
cd ../..
export VERSION_BUILD=`svn info | grep "Last Changed Rev" | sed s/Last\ Changed\ Rev:\ //g`
cd "$CURDIR"
}

function make_common_files {
rm -rf openbor.desktop
echo "[Desktop Entry]
Version=$VERSION_MAJOR.$VERSION_MINOR.$VERSION_BUILD
Name=$VERSION_NAME
GenericName=$VERSION_NAME
Exec=openbor
Comment=Ultimate 2D Game Engine
Terminal=false
Type=Application
Categories=Game;
Icon=openbor" >> openbor.desktop
cp ../../resources/OpenBOR_Icon_128x128.h ./openbor.xpm
cp ../../releases/LINUX/OpenBOR/OpenBOR ./openbor
}

check_files
export_version
make_common_files

