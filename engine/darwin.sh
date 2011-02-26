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

############ Update Library References ############

install_name_tool \
-change /opt/local/lib/libSDL-1.2.0.dylib \
@executable_path/../Libraries/libSDL-1.2.0.dylib \
releases/DARWIN/OpenBOR.app/Contents/Libraries/libSDL_gfx.13.dylib

install_name_tool \
-change /opt/local/lib/libvorbis.0.dylib \
@executable_path/../Libraries/libvorbis.0.dylib \
releases/DARWIN/OpenBOR.app/Contents/Libraries/libvorbisfile.3.dylib

install_name_tool \
-change /opt/local/lib/libogg.0.dylib \
@executable_path/../Libraries/libogg.0.dylib \
releases/DARWIN/OpenBOR.app/Contents/Libraries/libvorbisfile.3.dylib

install_name_tool \
-change /opt/local/lib/libogg.0.dylib \
@executable_path/../Libraries/libogg.0.dylib \
releases/DARWIN/OpenBOR.app/Contents/Libraries/libvorbis.0.dylib

######### Update Executable Library Paths ##########

install_name_tool \
-change /opt/local/lib/libSDL-1.2.0.dylib \
@executable_path/../Libraries/libSDL-1.2.0.dylib \
releases/DARWIN/OpenBOR.app/Contents/MacOS/OpenBOR

install_name_tool \
-change /opt/local/lib/libSDL_gfx.13.dylib \
@executable_path/../Libraries/libSDL_gfx.13.dylib \
releases/DARWIN/OpenBOR.app/Contents/MacOS/OpenBOR

install_name_tool \
-change /opt/local/lib/libvorbisfile.3.dylib \
@executable_path/../Libraries/libvorbisfile.3.dylib \
releases/DARWIN/OpenBOR.app/Contents/MacOS/OpenBOR

install_name_tool \
-change /opt/local/lib/libvorbis.0.dylib \
@executable_path/../Libraries/libvorbis.0.dylib \
releases/DARWIN/OpenBOR.app/Contents/MacOS/OpenBOR

install_name_tool \
-change /opt/local/lib/libogg.0.dylib \
@executable_path/../Libraries/libogg.0.dylib \
releases/DARWIN/OpenBOR.app/Contents/MacOS/OpenBOR

