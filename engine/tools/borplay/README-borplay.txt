Copyright (c) 2009 Bryan Cain ("Plombo")

borplay

Description
--------------------------------------------------------------------------------
Borplay is an updated version of Luigi Auriemma's borplay program. This update 
can play both mono and stereo BOR files, and it makes it more clear how to use 
the program's command line interface.

Instructions
--------------------------------------------------------------------------------
Windows: Use the borplay.exe file in the windows directory. If you want to build
from source, use buildwin.bat with the MinGW toolchain installed.

Linux: Extract the tar file in the linux directory and use the borplay program 
for 32-bit systems or the borplay_x64 program for 64-bit systems. To use either,
you must have libao or libao2 installed. To compile from source, run 
build-linux.sh in the source directory. You must have libao-dev installed to 
build it.

Mac: Compile the program by running build-linux.sh in the source directory. You 
must have libao (or libao2) installed to build it.
