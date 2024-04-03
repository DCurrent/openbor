#
# OpenBOR - https://www.chronocrash.com
# -----------------------------------------------------------------------
# Licensed under the BSD license, see LICENSE in OpenBOR root for details.
#
# Copyright (c) OpenBOR Team
#

#!/bin/bash
# Script updates the hardcoded dynamic libraries paths
# to paths that are relative to the executable.

############ Update Library References ############
IFS=$'\n'

lib_paths=("/opt/homebrew" "/usr/local/homebrew")
lib_targets=("arm" "x86")

for ((k = 0; k < ${#lib_paths[@]}; k = $k + 1)); do
  lib_files=(`otool -L ./releases/Darwin/OpenBOR.app/Contents/MacOS/OpenBOR | grep -o "${lib_paths[k]}/.*" | sed 's/[[:space:]].*//g'`)
  
  for ((i = 0; i < ${#lib_files[@]}; i = $i + 1)); do
    cp ${lib_paths[k]}/lib/$(basename ${lib_files[i]}) ./releases/DARWIN/OpenBOR.app/Contents/Libraries/${lib_targets[k]}/
  done

  for ((i = 0; i < ${#lib_files[@]}; i = $i + 2)); do
    install_name_tool -change \
    ${lib_files[i]} \
    @executable_path/../Libraries/${lib_targets[k]}/$(basename ${lib_files[i]}) \
    releases/DARWIN/OpenBOR.app/Contents/MacOS/OpenBOR
  done
done

unset IFS