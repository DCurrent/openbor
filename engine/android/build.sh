#!/bin/bash
#
# build the android apk !
# 

# make sdk directory if missing
if [ ! -d ~/.android ];
then
	cd ~/
	mkdir .android
fi

#get sdk if missing
if [ ! -d ~/.android/cmdline-tools ];
then
cd ~/.android

	for i in {1..5}
	do

		if [ $i == 5 ];
		then 
			echo "error: failed to download cmdline-tools.tar.xz"
			exit
		fi

		if [ ! -f ~/.android/cmdline-tools.tar.xz ];
		then
			wget --no-check-certificate "https://onedrive.live.com/download?cid=CFF1642CDA1859A3&resid=CFF1642CDA1859A3%21654&authkey=ANAEsZyPlQwHLMQ" -O ~/.android/cmdline-tools.tar.xz
		fi
	
		if md5sum ~/.android/cmdline-tools.tar.xz | grep -i d7edd7ea7900fece56ffcfc37400e5be > /dev/null;
		then 
			tar -xf ~/.android/cmdline-tools.tar.xz
			break
		else 
			rm ~/.android/cmdline-tools.tar.xz
		fi
	done
fi

#sign all licenses
if [ ! -d ~/.android/licenses ];
then
	cd ~/.android
	bash cmdline-tools/bin/sdkmanager --sdk_root=./ --licenses
fi

#set build version, android home temp variable and build
export ANDROID_SDK_ROOT=~/.android
cd $(dirname $(readlink -f $0))
cd ../
./version.sh
cd android
./gradlew clean
./gradlew assembleDebug
