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
lib_targets=("native" "others")

for ((i = 0; i < ${#lib_paths[@]}; i = $i + 1)); do
  lib_files=(`otool -L releases/Darwin/OpenBOR.app/Contents/MacOS/OpenBOR | grep -o "${lib_paths[i]}/.*" | sed 's/[[:space:]].*//g'`)
  
  for ((j = 0; j < ${#lib_files[@]}; j = $j + 1)); do
    cp ${lib_paths[i]}/lib/$(basename ${lib_files[j]}) releases/DARWIN/OpenBOR.app/Contents/Frameworks/${lib_targets[i]}/
    chmod 666 releases/DARWIN/OpenBOR.app/Contents/Frameworks/${lib_targets[i]}/*

    install_name_tool -change \
    ${lib_files[j]} \
      "@executable_path/../Frameworks/${lib_targets[i]}/$(basename ${lib_files[j]})" \
      releases/DARWIN/OpenBOR.app/Contents/MacOS/OpenBOR

    dep_lib_files=(`otool -L releases/Darwin/OpenBOR.app/Contents/Frameworks/${lib_targets[i]}/$(basename ${lib_files[j]}) | grep -o "${lib_paths[i]}/.*" | sed 's/[[:space:]].*//g'`)
    for ((k = 0; k < ${#dep_lib_files[@]}; k = $k + 1)); do
      install_name_tool -change \
        ${dep_lib_files[k]} \
        "@executable_path/../Frameworks/${lib_targets[i]}/$(basename ${dep_lib_files[k]})" \
        releases/Darwin/OpenBOR.app/Contents/Frameworks/${lib_targets[i]}/$(basename ${lib_files[j]}) 2>/dev/null
        codesign --force -s - releases/Darwin/OpenBOR.app/Contents/Frameworks/${lib_targets[i]}/$(basename ${lib_files[j]}) 2>/dev/null
    done
  done

  install_name_tool -add_rpath \
    "@rpath/../Frameworks/${lib_targets[i]}" \
    releases/DARWIN/OpenBOR.app/Contents/MacOS/OpenBOR
done

unset IFS