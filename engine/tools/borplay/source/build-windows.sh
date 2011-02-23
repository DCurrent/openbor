#!/bin/sh

# Compiles borplay on Windows. You must have the MinGW toolchain 
# installed.

#compiler="i586-mingw32msvc-gcc"
compiler="mingw32-gcc"
$compiler src/adpcm.c src/borplay.c src/mylibaow32.c src/stristr.c -lwinmm \
	-o ../borplay.c

