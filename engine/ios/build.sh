#!/bin/sh
#build Xcode project unsigned using command line tools

cd "$( dirname "${BASH_SOURCE[0]}" )"

rm -r Build/*

xcodebuild clean -project "OpenBOR.xcodeproj" -scheme "OpenBOR"

xcodebuild archive -project OpenBOR.xcodeproj \
		   -scheme OpenBOR \
		   -archivePath "$PWD/Build/unsigned.xcarchive" \
		   -configuration Release CODE_SIGN_IDENTITY="" \
					  CODE_SIGNING_REQUIRED=NO \
					  CODE_SIGNING_ALLOWED=NO

if [ -f "$PWD/Build/OpenBOR.xcarchive" ]; then
   echo "archive missing aborting." 
   exit
fi

mkdir ./Build/Payload
cp -r ./Build/unsigned.xcarchive/Products/Applications/OpenBOR.app ./Build/Payload
cd ./Build
zip -r ./OpenBOR.zip ./Payload
mv ./OpenBOR.zip ./OpenBOR.ipa
rm -r ./Payload

#xcodebuild -exportArchive \
#           -exportOptionsPlist "$PWD/test.plist" \
#           -archivePath "$PWD/Build/unsigned.xcarchive" \
#           -exportPath "$PWD/Build"
#
#if [ -f "$PWD/Build/OpenBOR.xcarchive" ]; then
#   rm "$PWD/Build/OpenBOR.xcarchive"
#fi