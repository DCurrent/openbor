#
# OpenBOR - http://www.LavaLit.com
# -----------------------------------------------------------------------
# Licensed under the BSD license, see LICENSE in OpenBOR root for details.
#
# Copyright (c) 2004 - 2011 OpenBOR Team
#

#!/bin/bash
# Script updates the hardcoded dynamic libraries paths
# to paths that are relative to the executable.

if [ -z "${DWNDEV+xxx}" ]; then
  . environ.sh 10
fi

if [ "${DWNDEV}" == "/opt/mac" ]; then
  exit
fi

############ Update Library References ############

${PREFIX}install_name_tool \
-change ${DWNDEV}/lib/libSDL-1.2.0.dylib \
@executable_path/../Libraries/libSDL-1.2.0.dylib \
releases/DARWIN/OpenBOR.app/Contents/Libraries/libSDL_gfx.13.dylib

${PREFIX}install_name_tool \
-change ${DWNDEV}/lib/libvorbis.0.dylib \
@executable_path/../Libraries/libvorbis.0.dylib \
releases/DARWIN/OpenBOR.app/Contents/Libraries/libvorbisfile.3.dylib

${PREFIX}install_name_tool \
-change ${DWNDEV}/lib/libogg.0.dylib \
@executable_path/../Libraries/libogg.0.dylib \
releases/DARWIN/OpenBOR.app/Contents/Libraries/libvorbisfile.3.dylib

${PREFIX}install_name_tool \
-change ${DWNDEV}/lib/libogg.0.dylib \
@executable_path/../Libraries/libogg.0.dylib \
releases/DARWIN/OpenBOR.app/Contents/Libraries/libvorbis.0.dylib

${PREFIX}install_name_tool \
-change ${DWNDEV}/lib/libz.1.dylib \
@executable_path/../Libraries/libz.1.2.5.dylib \
releases/DARWIN/OpenBOR.app/Contents/Libraries/libpng14.14.dylib

######### Update Executable Library Paths ##########

${PREFIX}install_name_tool \
-change ${DWNDEV}/lib/libSDL-1.2.0.dylib \
@executable_path/../Libraries/libSDL-1.2.0.dylib \
releases/DARWIN/OpenBOR.app/Contents/MacOS/OpenBOR

${PREFIX}install_name_tool \
-change ${DWNDEV}/lib/libSDL_gfx.13.dylib \
@executable_path/../Libraries/libSDL_gfx.13.dylib \
releases/DARWIN/OpenBOR.app/Contents/MacOS/OpenBOR

${PREFIX}install_name_tool \
-change ${DWNDEV}/lib/libvorbisfile.3.dylib \
@executable_path/../Libraries/libvorbisfile.3.dylib \
releases/DARWIN/OpenBOR.app/Contents/MacOS/OpenBOR

${PREFIX}install_name_tool \
-change ${DWNDEV}/lib/libvorbis.0.dylib \
@executable_path/../Libraries/libvorbis.0.dylib \
releases/DARWIN/OpenBOR.app/Contents/MacOS/OpenBOR

${PREFIX}install_name_tool \
-change ${DWNDEV}/lib/libogg.0.dylib \
@executable_path/../Libraries/libogg.0.dylib \
releases/DARWIN/OpenBOR.app/Contents/MacOS/OpenBOR

${PREFIX}install_name_tool \
-change ${DWNDEV}/lib/libpng14.14.dylib \
@executable_path/../Libraries/libpng14.14.dylib \
releases/DARWIN/OpenBOR.app/Contents/MacOS/OpenBOR

${PREFIX}install_name_tool \
-change ${DWNDEV}/lib/libz.1.dylib \
@executable_path/../Libraries/libz.1.2.5.dylib \
releases/DARWIN/OpenBOR.app/Contents/MacOS/OpenBOR
