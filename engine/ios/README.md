# OpenBOR for iOS

OpenBOR is a beat-em-up engine:
https://github.com/DCurrent/openbor

Work-in-progress for iOS port of OpenBOR. It uses SDL2 so using the SDL2 library for iOS arm64.

Eventually, I'd like to contribute back to the mainline OpenBOR with this port so that it'd be included in future releases. This is here just because I wanted to get something up and working first.

Delete the download source by running:

xattr -d com.apple.metadata:kMDItemWhereFroms ./build.sh

To also remove the quarantine flag use:

xattr -d com.apple.quarantine ./build.sh