#!/bin/sh

cd test
if [ $# != 1 ]; then
	echo "usage: $0 file.h"
	exit 1
fi

./build.sh || exit 1

cd ..

NAME=`basename "$1"`
mcpp -V199901L -P -I/usr/include/i386-linux-gnu -I/usr/lib/gcc/i686-linux-gnu/4.6/include "$1" > mcpp_$NAME || exit 1
./pp_test "$1" > pp_$NAME || exit 1
./tokendiff pp_$NAME mcpp_$NAME

