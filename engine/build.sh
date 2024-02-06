#!/bin/bash
#
# OpenBOR - http://www.ChronoCrash.com
# -----------------------------------------------------------------------
# All rights reserved, see LICENSE in OpenBOR root for details.
#
# Copyright (c) OpenBOR Team
#

# Building script for all platforms
# build.sh by SX (SumolX@gmail.com)

# Used for resetting path prior to each platform.
export OLD_PATH=$PATH

# Use parellel make to speed up compilation
export MAKEFLAGS=-j4

# Display Version
function version {
  . ./version.sh
  make version
  mkdir -p releases
  cp README ./releases/README.txt
  cp LICENSE ./releases/LICENSE.txt
  cp COMPILING ./releases/COMPILING.txt
  cp translation.txt ./releases/translation.txt
}

# CleanUp Releases
function clean {
  make clean-releases
}

# Distribute Releases
function distribute {
  echo ------------------------------------------------------
  echo "          Validating Platforms Built w/Bash"
  echo ------------------------------------------------------
  echo  

if test -e "releases/WINDOWS/OpenBOR/OpenBOR.exe"; then
    cd ../tools/borpak/source/
    . build.sh win
    cp borpak.exe ../../../engine/releases/WINDOWS/OpenBOR/
    cp ../scripts/packer.bat ../../../engine/releases/WINDOWS/OpenBOR/
    cp ../scripts/paxplode.bat ../../../engine/releases/WINDOWS/OpenBOR/
    cd ../../../engine
  else
    echo "Windows Platform Failed To Build!"
    exit 1
  fi
  if ! test -e "releases/WII/OpenBOR/boot.dol"; then
    echo "Wii Platform Failed To Build!"
    exit 1
  fi
  
  if test -e "releases/LINUX/OpenBOR/OpenBOR"; then
    cd ../tools/borpak/source/
    . build.sh lin
    cp borpak ../../../engine/releases/LINUX/OpenBOR/
    cp ../scripts/packer ../../../engine/releases/LINUX/OpenBOR/
    cp ../scripts/paxplode ../../../engine/releases/LINUX/OpenBOR/
    cd ../../../engine
  else
    if [ $(echo $HOST_PLATFORM | grep -o "Linux") ]; then
      echo "Linux Platform Failed To Build!"
      exit 1
    fi
  fi
  
  echo "All Platforms Created Successfully"
  if ! test "$BUILDBATCH"; then
    TRIMMED_URL=$(svn info | grep "URL:" | sed s/URL:\ svn\+ssh//g)
    if test -n $TRIMMED_URL;  then
      TRIMMED_URL="svn"$TRIMMED_URL
    fi
    svn log $TRIMMED_URL --verbose > ./releases/VERSION_INFO.txt
    7za a -t7z -mx9 -r -x!.svn "./releases/OpenBOR $VERSION.7z" ./releases/*
  fi
  echo
}

# Linux Environment && Compile (common to all architectures)
function linux {
  export PATH=$OLD_PATH
  export GCC_TARGET=$1
  export TARGET_ARCH=$2
  . ./environ.sh 4
  if test $LNXDEV; then
    if [[ ! $BUILD_DEBUG ]] ; then
      make clean BUILD_LINUX=1
    elif [[ "$TARGET_ARCH" == "amd64" ]] ; then
      export NO_RAM_DEBUGGER=1
    fi
    make BUILD_LINUX=1
    if test -f "./OpenBOR"; then
      if test ! -e "./releases/$3"; then
        mkdir ./releases/$3
        mkdir ./releases/$3/OpenBOR
        mkdir ./releases/$3/OpenBOR/Logs
        mkdir ./releases/$3/OpenBOR/Paks
        mkdir ./releases/$3/OpenBOR/Saves
        mkdir ./releases/$3/OpenBOR/ScreenShots
      fi
      mv OpenBOR ./releases/$3/OpenBOR
      echo "moved binary to ./releases/$3/ !"
    fi
    if [[ ! $BUILD_DEBUG ]] ; then
      make clean BUILD_LINUX=1
    fi
  fi
  [ $LNXDEV ]
}

# Compile for Linux under various architectures
function linux_x86 {
  if [ $(uname -s | grep -o "Linux") ]; then
    linux i.86-.*linux.* x86 LINUX || { # try standard 32-bit GCC
    [ $(gcc -dumpmachine | grep -o x86_64-.*linux.*) ] && [ $(gcc -print-multi-lib | grep -o '@m32') ] && # check for x86_64 GCC with 32-bit multilib
    linux x86_64-.*linux.* x86 LINUX; }  # try 64-bit compiler with multilib
  fi
}

function linux_amd64 {
  if [ $(uname -s | grep -o "Linux") ]; then
    linux x86_64-.*linux.* amd64 LINUX_AMD64 || { # try standard 64-bit GCC
    [ $(gcc -dumpmachine | grep -o i.86-.*linux.*) ] && [ $(gcc -print-multi-lib | grep -o '@m64') ] && # check for x86 GCC with 64-bit multilib
    linux i.86-.*linux.* amd64 LINUX_AMD64; } # try 32-bit compiler with multilib
  fi
}

function linux_something {
  if [ ! $1 -o $1 = x86 ]; then
    linux_x86
  elif [ $1 = amd64 ]; then
    linux_amd64
  else
    echo "Error: unknown Linux architecture '$1'"
  fi
}

# Windows Environment && Compile
function windows {
  export PATH=$OLD_PATH
  . ./environ.sh 5
  if test $WINDEV; then
    make clean BUILD_WIN=1
		#first remove old resource file and update with build number from build_number.h.
		if test -e "resources/OpenBOR.res"; then
			rm "resources/OpenBOR.res";
		fi

		# if it's cross-compile for Windows from Linux, then
		# find a proper tool
		if test -e "/usr/bin/i686-w64-mingw32-gcc" && test -e "/usr/bin/i686-w64-mingw32-windres"; then
			i686-w64-mingw32-windres resources/OpenBOR.rc -o resources/OpenBOR.res -O coff
		else
			windres.exe resources/OpenBOR.rc -o resources/OpenBOR.res -O coff
		fi
    make BUILD_WIN=1
    if test -f "./OpenBOR.exe"; then
      if test ! -e "./releases/WINDOWS" ; then
        mkdir ./releases/WINDOWS
        mkdir ./releases/WINDOWS/OpenBOR
        mkdir ./releases/WINDOWS/OpenBOR/Logs
        mkdir ./releases/WINDOWS/OpenBOR/Paks
        mkdir ./releases/WINDOWS/OpenBOR/Saves
        mkdir ./releases/WINDOWS/OpenBOR/ScreenShots
      fi
      mv OpenBOR.exe ./releases/WINDOWS/OpenBOR
    fi
    make clean BUILD_WIN=1
  fi
}

# Wii Environment && Compile
function wii {
  export PATH=$OLD_PATH
  . ./environ.sh 7
  if test $DEVKITPPC; then
    make clean BUILD_WII=1
    make BUILD_WII=1
    if test -f "./boot.dol"; then
      if test ! -e "./releases/WII" ; then
        mkdir ./releases/WII
        mkdir ./releases/WII/OpenBOR
        mkdir ./releases/WII/OpenBOR/Logs
        mkdir ./releases/WII/OpenBOR/Paks
        mkdir ./releases/WII/OpenBOR/Saves
        mkdir ./releases/WII/OpenBOR/ScreenShots
      fi
      mv boot.dol ./releases/WII/OpenBOR/
      cp ./resources/meta.xml ./releases/WII/OpenBOR
      cp ./resources/OpenBOR_Icon_128x48.png ./releases/WII/OpenBOR/icon.png
    fi
    make clean BUILD_WII=1
  fi
}

# Darwin Environment && Compile
function darwin {
  export PATH=$OLD_PATH
  . ./environ.sh 10
  if test $DWNDEV; then
    make clean BUILD_DARWIN=1
    make BUILD_DARWIN=1
    if test -f "./OpenBOR"; then
      if test ! -e "./releases/DARWIN"; then
        mkdir ./releases/DARWIN
        mkdir ./releases/DARWIN/OpenBOR.app
        mkdir ./releases/DARWIN/OpenBOR.app/Contents
        mkdir ./releases/DARWIN/OpenBOR.app/Contents/MacOS
        mkdir ./releases/DARWIN/OpenBOR.app/Contents/Resources
        mkdir ./releases/DARWIN/OpenBOR.app/Contents/Libraries
      fi
      mv OpenBOR ./releases/DARWIN/OpenBOR.app/Contents/MacOS
      cp ./resources/PkgInfo ./releases/DARWIN/OpenBOR.app/Contents
      cp ./resources/Info.plist ./releases/DARWIN/OpenBOR.app/Contents
      cp ./resources/OpenBOR.icns ./releases/DARWIN/OpenBOR.app/Contents/Resources
      if [ "${DWNDEV}" != "/opt/mac" ]; then
        ./darwin.sh
      fi
    fi
    make clean BUILD_DARWIN=1
  fi
}

# Android Compile
function android {
  export PATH=$OLD_PATH
    if test -f "./android/app/build/outputs/apk/debug/OpenBOR.apk"; then
      if test ! -e "./releases/Android/"; then
		rm -rf ./releases/ANDROID
        mkdir ./releases/ANDROID
      fi
      cp ./android/app/build/outputs/apk/debug/OpenBOR.apk ./releases/ANDROID
		echo "Android Build Copied!"
    fi
}


function build_all {
  clean
  version
  if test -e "buildspec.sh"; then
    . ./buildspec.sh
  else
    linux_x86
    linux_amd64
    windows
    wii
    darwin
	android
  fi
  #distribute -- 2023-01-04 DC Throws series of errors. Needs a closer look.
}

function print_help {
  echo
  echo "Run $0 with one of the below targets"
  echo "-------------------------------------------------------"
  echo "    0 = Distribute"
  echo "    4 = Linux (x86, amd64) Example: $0 4 amd64"
  echo "    5 = Windows"
  echo "    7 = Wii"
  echo "  all = build for all applicable targets"
  echo "-------------------------------------------------------"
  echo "Example: $0 10"
  echo
}

case $1 in
  0)
    version
    distribute
    ;;

  4)
    version
    linux_something $2
    ;;

  5)
    version
    windows
    ;;

  7)
    version
    wii
    ;;

  ?)
    version
    print_help
    ;;

  all)
    build_all
    ;;

  *)
    print_help
    ;;
esac

