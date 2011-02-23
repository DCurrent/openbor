#!/bin/sh
# Builds borplay. Note that it has a dependency on libao. To build, you must 
# have libao-dev installed.  To install it on Debian and Ubuntu, install the 
# package libao-dev.

gcc adpcm.c borplay.c kbhit.c -lao -o ../borplay

