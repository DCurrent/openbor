#!/bin/sh
# Compiles borpak for the Linux platform. Also works with Cygwin on Windows, 
# although I don't know why you'd be using Cygwin instead of MinGW.
# 
# This should also work on Mac OS X, although it hasn't been tested because 
# Plombo doesn't own a Mac.

gcc -o borpak borpak.c stristr.c

