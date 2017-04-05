#!/bin/sh
# Compiles borpak for the following platforms...
# Windows (MinGW), Mac OS X and Linux

TARGET_PLATFORM=$1
HOST_PLATFORM=$(uname -s)
EXTENSION=
SOURCE=
PREFIX=
CFLAGS=

if [ $# -ne 1 ]; then
  echo "Usage: '`basename $0` mac' == Mac OS X"
  echo "\t\t win  == Windows"
  echo "\t\t lin  == Linux"
  exit 1
fi

# Target is Windows
if [ `echo $TARGET_PLATFORM | grep "win"` ]; then
  EXTENSION=".exe"
  SOURCE="scandir.c "
  if [ `echo $HOST_PLATFORM | grep "Darwin"` ]; then
    PREFIX="i386-mingw32-"
    PATH="$PATH:/usr/local/i386-mingw32-4.3.0/bin"
  elif [ `echo $HOST_PLATFORM | grep "Linux"` ]; then
    PREFIX="i586-mingw32msvc-"
    PATH="$PATH:/usr/i586-mingw32msvc/bin"
  fi
fi

# Target is Linux
if [ `echo $TARGET_PLATFORM | grep "lin"` ]; then
  if [ `echo $HOST_PLATFORM | grep "Darwin"` ]; then
    PREFIX="i386-linux-"
    PATH="$PATH:/usr/local/i386-linux-4.1.1/bin"
  fi
fi

# Target is Mac OS X
if [ `echo $TARGET_PLATFORM | grep "mac"` ]; then
  if [ `echo $HOST_PLATFORM | grep "Darwin"` ]; then
    CFLAGS+="-arch x86_64 -arch i386 -arch ppc"
  fi
fi

TARGET="borpak"
CC=${PREFIX}gcc
RM="rm -rf"
SOURCE="$SOURCE borpak.c stristr.c"

$RM $TARGET$EXTENSION *.o
$CC $CFLAGS -o $TARGET$EXTENSION $SOURCE
