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
IFS='
'

#copy libs, used to be in build.sh
lib_copy=(`find ${DWNDEV}/lib -name "libSDL-*.dylib"`)
lib_copy+=(`find ${DWNDEV}/lib -name "libSDL_gfx.*.dylib"`)
lib_copy+=(`find ${DWNDEV}/lib -name "libogg.*.dylib"`)
lib_copy+=(`find ${DWNDEV}/lib -name "libvorbisfile.*.dylib"`)
lib_copy+=(`find ${DWNDEV}/lib -name "libvorbis.*.dylib"`)
lib_copy+=(`find ${DWNDEV}/lib -name "libz.[0-9].dylib"`)
lib_copy+=(`find ${DWNDEV}/lib -name "libpng[0-9][0-9].dylib"`)

for ((i = 0; i < ${#lib_copy[*]}; i = $i + 1)); do
  cp ${lib_copy[i]} ./releases/DARWIN/OpenBOR.app/Contents/Libraries
done

# Order and pairing is critical!!!
lib_ref_patch=(`find ${DWNDEV}/lib -name "libSDL-*.dylib"`)
lib_ref_patch+=(`find ${DWNDEV}/lib -name "libSDL_gfx.*.dylib"`)
lib_ref_patch+=(`find ${DWNDEV}/lib -name "libogg.*.dylib"`)
lib_ref_patch+=(`find ${DWNDEV}/lib -name "libvorbisfile.*.dylib"`)
lib_ref_patch+=(`find ${DWNDEV}/lib -name "libogg.*.dylib"`)
lib_ref_patch+=(`find ${DWNDEV}/lib -name "libvorbis.*.dylib"`)
lib_ref_patch+=(`find ${DWNDEV}/lib -name "libvorbis.*.dylib"`)
lib_ref_patch+=(`find ${DWNDEV}/lib -name "libvorbisfile.*.dylib"`)
lib_ref_patch+=(`find ${DWNDEV}/lib -name "libz.[0-9].dylib"`)
lib_ref_patch+=(`find ${DWNDEV}/lib -name "libpng[0-9][0-9].dylib"`)

for ((i = 0; i < ${#lib_ref_patch[*]}; i = $i + 2)); do

  ${PREFIX}install_name_tool \
  -change ${lib_ref_patch[i]} \
  @executable_path/../Libraries/$(basename ${lib_ref_patch[i]}) \
  releases/DARWIN/OpenBOR.app/Contents/Libraries/$(basename ${lib_ref_patch[i+1]})

done

######### Update Executable Library Paths ##########

for ((i = 0; i < ${#lib_ref_patch[*]}; i = $i + 1)); do

  ${PREFIX}install_name_tool \
  -change ${lib_ref_patch[i]} \
  @executable_path/../Libraries/$(basename ${lib_ref_patch[i]}) \
  releases/DARWIN/OpenBOR.app/Contents/MacOS/OpenBOR

done

