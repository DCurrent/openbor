#
# OpenBOR - http://www.LavaLit.com
# -----------------------------------------------------------------------
# Licensed under the BSD license, see LICENSE in OpenBOR root for details.
#
# Copyright (c) 2004 - 2011 OpenBOR Team
#

#!/bin/bash
# Building script for all platforms
# build.sh by SX (SumolX@gmail.com)

# Display Version
function version {
  source ./version.sh
  make version
  cp README ./releases/README.txt
  cp LICENSE ./releases/LICENSE.txt
  cp COMPILING ./releases/COMPILING.txt
}

# CleanUp Releases
function clean {
  make clean-releases
}

# Distribute Releases
function distribute {
  ERROR=0
  echo ------------------------------------------------------
  echo "          Validating Platforms Built w/Bash"
  echo ------------------------------------------------------
  echo
  
  if ! test "releases/PSP/OpenBOR/EBOOT.PBP"; then
    echo "PSP Platform Failed To Build!"
    ERROR=1
  fi
  if ! test -e "releases/GP2X/OpenBOR/OpenBOR.gpe"; then
    echo "GP2X Platform Failed To Build!"
    ERROR=0
  fi
  if ! test -e "releases/WIZ/OpenBOR/OpenBOR.gpe"; then
    echo "WIZ Platform Failed To Build!"
    ERROR=0
  fi
  if ! test -e "releases/DC/OpenBOR/1ST_READ.BIN"; then
    echo "Dreamcast Platform Failed To Build!"
    ERROR=1
  fi
  if ! test -e "releases/WINDOWS/OpenBOR/OpenBOR.exe"; then
    echo "Windows Platform Failed To Build!"
    ERROR=1
  fi
  if ! test -e "releases/WII/OpenBOR/boot.dol"; then
    echo "Wii Platform Failed To Build!"
    ERROR=1
  fi
  if ! test -e "releases/DINGOO/OpenBOR/OpenBOR.dge"; then
    echo "Dingoo Platform Failed To Build!"
    ERROR=0
  fi
  if ! test -e "releases/LINUX/OpenBOR/OpenBOR"; then
    if [ `echo $HOST_PLATFORM | grep -o "Linux"` ]; then
      echo "Linux Platform Failed To Build!"
      ERROR=1
    fi
  fi
  if ! test -e "releases/DARWIN/OpenBOR.app/Contents/MacOS/OpenBOR"; then
    if [ `echo $HOST_PLATFORM | grep -o "Darwin"` ]; then
      echo "Darwin Platform Failed To Build!"
      ERROR=1
    fi
  fi
  
  if [ $ERROR = 0 ]; then
    echo "All Platforms Created Successfully"
    if ! test "$BUILDBATCH"; then
      svn log --verbose > ./releases/VERSION_INFO.txt
      7za a -t7z -mx9 -r -x!.svn "./releases/OpenBOR $VERSION.7z" ./releases/*
    fi
  fi
  echo
}

# PSP Environment && Compile
function psp {
  source ./environ.sh 1
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

# PS2 Environment && Compile
function ps2 {
  source ./environ.sh 2
  if test $PS2DEV; then
    make clean BUILD_PS2=1
    make BUILD_PS2=1
    if test -f "./EBOOT.PBP"; then
      if test ! -e "./releases/PS2"; then
        mkdir ./releases/PS2
        mkdir ./releases/PS2/OpenBOR
        mkdir ./releases/PS2/OpenBOR/Images
        mkdir ./releases/PS2/OpenBOR/Logs
        mkdir ./releases/PS2/OpenBOR/Paks
        mkdir ./releases/PS2/OpenBOR/Saves
        mkdir ./releases/PS2/OpenBOR/ScreenShots
      fi
      mv EBOOT.PBP ./releases/PS2/OpenBOR/
      cp ./ps2/data/Menu.png ./releases/PS2/OpenBOR/Images/
    fi
    make clean BUILD_PS2=1
  fi 
}

# Gp2x Environment && Compile
function gp2x {
  source ./environ.sh 3
  if test $GP2XDEV; then
    make clean BUILD_GP2X=1
    make BUILD_GP2X=1
    if test -f "./OpenBOR.gpe"; then
      if test ! -e "./releases/GP2X"; then
        mkdir ./releases/GP2X
        mkdir ./releases/GP2X/OpenBOR
        mkdir ./releases/GP2X/OpenBOR/Logs
        mkdir ./releases/GP2X/OpenBOR/Paks  
        mkdir ./releases/GP2X/OpenBOR/Saves
        mkdir ./releases/GP2X/OpenBOR/ScreenShots
      fi
      cp ./sdl/gp2x/modules/mmuhack.o ./releases/GP2X/OpenBOR/
      mv OpenBOR.gpe ./releases/GP2X/OpenBOR/
    fi
    make clean BUILD_GP2X=1
  fi
}

# Linux Environment && Compile (common to all architectures)
function linux {
  export GCC_TARGET=$1
  export TARGET_ARCH=$2
  source ./environ.sh 4
  if test $LNXDEV; then
    if [[ ! $BUILD_DEBUG ]] ; then
	    make clean BUILD_LINUX=1
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
  source ./environ.sh 5
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

# Dreamcast Environment && Compile
function dreamcast {
  source ./environ.sh 6
  if test $KOS_BASE; then
    make clean BUILD_DC=1
    make BUILD_DC=1
    if test -f "./1ST_READ.BIN"; then
      if test ! -e "./releases/DC" ; then
        mkdir ./releases/DC
        mkdir ./releases/DC/OpenBOR
      fi
      mv 1ST_READ.BIN ./releases/DC/OpenBOR/
    fi
    make clean BUILD_DC=1
  fi
}

# Wii Environment && Compile
function wii {
  source ./environ.sh 7
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

# Dingoo Environment && Compile
function dingoo {
  source ./environ.sh 8
  if test $DINGUX_TOOLCHAIN; then
    make clean BUILD_DINGOO=1
    make BUILD_DINGOO=1
    if test -f "OpenBOR.dge"; then
      if test ! -e "./releases/DINGOO" ; then
        mkdir ./releases/DINGOO
        mkdir ./releases/DINGOO/OpenBOR
        mkdir ./releases/DINGOO/OpenBOR/Logs
        mkdir ./releases/DINGOO/OpenBOR/Paks
        mkdir ./releases/DINGOO/OpenBOR/Saves
        mkdir ./releases/DINGOO/OpenBOR/ScreenShots
      fi
      mv OpenBOR.dge ./releases/DINGOO/OpenBOR/
    fi
    make clean BUILD_DINGOO=1
  fi
}

# WIZ Environment && Compile
function wiz {
  source ./environ.sh 9
  if test $WIZDEV; then
    make clean BUILD_WIZ=1
    make BUILD_WIZ=1
    if test -f "./OpenBOR.gpe"; then
      if test ! -e "./releases/WIZ"; then
        mkdir ./releases/WIZ
        mkdir ./releases/WIZ/OpenBOR
        mkdir ./releases/WIZ/OpenBOR/Logs
        mkdir ./releases/WIZ/OpenBOR/Paks
        mkdir ./releases/WIZ/OpenBOR/Saves
        mkdir ./releases/WIZ/OpenBOR/ScreenShots
      fi  
      mv OpenBOR.gpe ./releases/WIZ/OpenBOR/
      if [ `echo $HOST_PLATFORM | grep -o "SVN"` ]; then
        cp $SDKPATH/lib/target/libSDL-1.2.so.0.11.2 ./releases/WIZ/OpenBOR/
        cp $SDKPATH/lib/target/libSDL_gfx.so.0.0.17 ./releases/WIZ/OpenBOR/libSDL_gfx.so.0
      else
        cp $SDKPATH/lib/warm_2.6.24.ko ./releases/WIZ/OpenBOR/
        cp $SDKPATH/lib/libSDL-1.2.so.0.11.2 ./releases/WIZ/OpenBOR/
        cp $SDKPATH/lib/libSDL_gfx.so.0.0.17 ./releases/WIZ/OpenBOR/libSDL_gfx.so.0
      fi
    fi  
    make clean BUILD_WIZ=1
  fi
}

# Darwin Environment && Compile
function darwin {
  source ./environ.sh 10 
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
      cp /opt/local/lib/libSDL-1.2.0.dylib ./releases/DARWIN/OpenBOR.app/Contents/Libraries
      cp /opt/local/lib/libSDL_gfx.13.dylib ./releases/DARWIN/OpenBOR.app/Contents/Libraries
      cp /opt/local/lib/libogg.0.dylib ./releases/DARWIN/OpenBOR.app/Contents/Libraries
      cp /opt/local/lib/libvorbisfile.3.dylib ./releases/DARWIN/OpenBOR.app/Contents/Libraries
      cp /opt/local/lib/libvorbis.0.dylib ./releases/DARWIN/OpenBOR.app/Contents/Libraries
      ./darwin.sh
    fi
    make clean BUILD_DARWIN=1
  fi
}

function build_all {
  clean
  version
  if test -e "buildspec.sh"; then
    source ./buildspec.sh
  else
    psp
    #ps2
    gp2x
    linux_x86
    linux_amd64
    windows
    dreamcast
    wii
    dingoo
    wiz
    darwin
  fi
  distribute
}

function print_help {
  echo
  echo "Run $0 with one of the below targets"
  echo "-------------------------------------------------------"
  echo "    0 = Distribute"
  echo "    1 = PSP"
  echo "    2 = PS2"
  echo "    3 = Gp2x"
  echo "    4 = Linux (x86, amd64) Example: $0 4 amd64"
  echo "    5 = Windows"
  echo "    6 = Dreamcast"
  echo "    7 = Wii"
  echo "    8 = Dingoo"
  echo "    9 = Wiz"
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
    ps2
    ;;

  3)
    version
    gp2x
    ;;

  4)
    version
    linux_something $2
    ;;

  5)
    version
    windows
    ;;

  6)
    version
    dreamcast
    ;;

  7)
    version
    wii
    ;;

  8)
    version
    dingoo
    ;;

  9)
    version
    wiz
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

