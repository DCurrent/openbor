#
# OpenBOR - http://www.ChronoCrash.com
# -----------------------------------------------------------------------
# All rights reserved, see LICENSE in OpenBOR root for details.
#
# Copyright (c) 2004 - 2018 OpenBOR Team
#

#!/bin/bash
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

  if ! test "releases/PSP/OpenBOR/EBOOT.PBP"; then
    echo "PSP Platform Failed To Build!"
    exit 1
  fi

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
  if ! test -e "releases/OPENDINGUX/OpenBOR/OpenBOR.dge"; then
    echo "OpenDingux Platform Failed To Build!"
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
    if [ `echo $HOST_PLATFORM | grep -o "Linux"` ]; then
      echo "Linux Platform Failed To Build!"
      exit 1
    fi
  fi
  if test -e "releases/DARWIN/OpenBOR.app/Contents/MacOS/OpenBOR"; then
    cd ../tools/borpak/source/
    . build.sh mac
    cp borpak ../../../engine/releases/DARWIN/OpenBOR.app/Contents/Resources/
    cp ../scripts/packer ../../../engine/releases/DARWIN/OpenBOR.app/Contents/Resources/
    cp ../scripts/paxplode ../../../engine/releases/DARWIN/OpenBOR.app/Contents/Resources/
    cd ../../../engine
  else
    echo "Darwin Platform Failed To Build!"
    exit 1
  fi

  echo "All Platforms Created Successfully"
  if ! test "$BUILDBATCH"; then
    TRIMMED_URL=`svn info | grep "URL:" | sed s/URL:\ svn\+ssh//g`
    if test -n $TRIMMED_URL;  then
      TRIMMED_URL="svn"$TRIMMED_URL
    fi
    svn log $TRIMMED_URL --verbose > ./releases/VERSION_INFO.txt
    7za a -t7z -mx9 -r -x!.svn "./releases/OpenBOR $VERSION.7z" ./releases/*
  fi
  echo
}

# PSP Environment && Compile
function psp {
  export PATH=$OLD_PATH
  . ./environ.sh 1
  if test $PSPDEV; then
    make clean BUILD_PSP=1
    make BUILD_PSP=1
    if test -f "./EBOOT.PBP"; then
      if test ! -e "./releases/PSP"; then
        mkdir ./releases/PSP
        mkdir ./releases/PSP/OpenBOR
        mkdir ./releases/PSP/OpenBOR/Images
        mkdir ./releases/PSP/OpenBOR/Logs
        mkdir ./releases/PSP/OpenBOR/Paks
        mkdir ./releases/PSP/OpenBOR/Saves
        mkdir ./releases/PSP/OpenBOR/Modules
      fi
      mv EBOOT.PBP ./releases/PSP/OpenBOR/
      mv OpenBOR.elf ./releases/PSP/OpenBOR/Modules/
      cp ./psp/dvemgr/dvemgr.prx ./releases/PSP/OpenBOR/Modules/
      cp ./psp/kernel/kernel.prx ./releases/PSP/OpenBOR/Modules/
      cp ./psp/control/control.prx ./releases/PSP/OpenBOR/Modules/
      cp ./psp/exception/exception.prx ./releases/PSP/OpenBOR/Modules/
      cp ./resources/OpenBOR_Menu_480x272_Sony.png ./releases/PSP/OpenBOR/Images/Menu.png
      cp ./resources/OpenBOR_Logo_480x272.png ./releases/PSP/OpenBOR/Images/Loading.png
    fi
    make clean BUILD_PSP=1
  fi
}

# PS Vita Environment && Compile
function vita {
  export PATH=$OLD_PATH
  . ./environ.sh 2
  if test $VITASDK; then
    make clean BUILD_VITA=1
    make BUILD_VITA=1
    if test -f "./OpenBOR.vpk"; then
      if test ! -e "./releases/VITA"; then
        mkdir ./releases/VITA
      fi
      mv OpenBOR.vpk ./releases/VITA/
    fi
    make clean BUILD_VITA=1
  fi
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
  if [ `uname -s | grep -o "Linux"` ]; then
    linux i.86-.*linux.* x86 LINUX || # try standard 32-bit GCC
    [ `gcc -dumpmachine | grep -o x86_64-.*linux.*` ] && [ `gcc -print-multi-lib | grep -o '@m32'` ] && # check for x86_64 GCC with 32-bit multilib
    linux x86_64-.*linux.* x86 LINUX  # try 64-bit compiler with multilib
  fi
}

function linux_amd64 {
  if [ `uname -s | grep -o "Linux"` ]; then
    linux x86_64-.*linux.* amd64 LINUX_AMD64 || # try standard 64-bit GCC
    [ `gcc -dumpmachine | grep -o i.86-.*linux.*` ] && [ `gcc -print-multi-lib | grep -o '@m64'` ] && # check for x86 GCC with 64-bit multilib
    linux i.86-.*linux.* amd64 LINUX_AMD64 # try 32-bit compiler with multilib
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
    if test -f "./android/bin/OpenBOR-debug.apk"; then
      if test ! -e "./releases/Android/"; then
		rm -rf ./releases/ANDROID
        mkdir ./releases/ANDROID
      fi
      cp ./android/bin/OpenBOR-debug.apk ./releases/ANDROID
		echo "Android Build Copied!"
    fi
}


function build_all {
  clean
  version
  if test -e "buildspec.sh"; then
    . ./buildspec.sh
  else
    psp
    vita
    linux_x86
    linux_amd64
    windows
    wii
    darwin
	android
  fi
  distribute
}

function print_help {
  echo
  echo "Run $0 with one of the below targets"
  echo "-------------------------------------------------------"
  echo "    0 = Distribute"
  echo "    1 = PSP"
  echo "    2 = PS Vita"
  echo "    4 = Linux (x86, amd64) Example: $0 4 amd64"
  echo "    5 = Windows"
  echo "    7 = Wii"
  echo "   10 = Darwin"
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

  1)
    version
    psp
    ;;

  2)
    version
    vita
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

  10)
    version
    darwin
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

