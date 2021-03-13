#!/bin/bash
#
# build the android apk !
# 

cd $(dirname $(readlink -f $0))
cd ../
./version.sh
cd android
./gradlew clean
./gradlew assembleDebug
