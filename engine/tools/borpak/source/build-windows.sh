#!/bin/sh
# Compiles borpak for Windows using MinGW.
# 
# You must have the GCC compiler from MinGW in your PATH for this to work.

gcc -o borpak.exe borpak.c stristr.c scandir.c
