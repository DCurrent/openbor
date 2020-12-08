#
# OpenBOR - http://www.ChronoCrash.com
# -----------------------------------------------------------------------
# All rights reserved. See LICENSE in OpenBOR root for details.
#
# Copyright (c) 2004 - 2014 OpenBOR Team
#

#!/bin/bash
# Script acquires the verison number from GIT Repository and creates
# a version.h as well as the environment variable to be used.

function check_git {
HOST_PLATFORM=$(uname -s)
if [ `echo $HOST_PLATFORM | grep -o "windows"` ]; then
  if [ ! -d "../tools/mingit/mingw32" ]; then
    echo "-------------------------------------------------------"
    echo "           GIT - Not Found, Installing GIT!"
    echo "-------------------------------------------------------"
    7za x -y ../tools/mingit/MinGit-2.21.0-32-bit.7z -o../tools/mingit/
    echo
    echo "-------------------------------------------------------"
    echo "           GIT - Installation Has Completed!"
    echo "-------------------------------------------------------"
  fi
fi
}

function get_revnum {
  if test -d "../.git" || test -d ".git"; then
    VERSION_BUILD=`git rev-list --count HEAD`
    # get commit hash, 7 chars in length is enough, and still work when supply as URL on github.com
    VERSION_COMMIT=`git rev-parse HEAD | cut -c -7`
  else
	VERSION_BUILD=0000
	VERSION_COMMIT=0000000
  fi
}

function read_version {
check_git
get_revnum
VERSION_NAME="OpenBOR"
VERSION_MAJOR=3
VERSION_MINOR=0
VERSION_DATE=`date '+%Y%m%d%H%M%S'`

# if there's no commit hash then set string as usual way
# otherwise include it in the version string
if [ -z "${VERSION_COMMIT}" ]; then
  export VERSION="v$VERSION_MAJOR.$VERSION_MINOR Build $VERSION_BUILD"
else
  export VERSION="v$VERSION_MAJOR.$VERSION_MINOR Build $VERSION_BUILD (commit hash: ${VERSION_COMMIT})"
fi
}

function write_version {
rm -rf version.h
echo "/*
 * OpenBOR - http://www.ChronoCrash.com
 * -----------------------------------------------------------------------
 * All rights reserved, see LICENSE in OpenBOR root for details.
 *
 * Copyright (c) 2004 - 2014 OpenBOR Team
 */

#ifndef VERSION_H
#define VERSION_H

#define VERSION_NAME \"$VERSION_NAME\"
#define VERSION_MAJOR \"$VERSION_MAJOR\"
#define VERSION_MINOR \"$VERSION_MINOR\"
#define VERSION_BUILD \"$VERSION_BUILD\"
#define VERSION_BUILD_INT $VERSION_BUILD" >> version.h

if [ -z "${VERSION_COMMIT}" ]; then
  echo "#define VERSION \"v\"VERSION_MAJOR\".\"VERSION_MINOR\" Build \"VERSION_BUILD

#endif" >> version.h
else
  echo "#define VERSION_COMMIT \"${VERSION_COMMIT}\"
#define VERSION \"v\"VERSION_MAJOR\".\"VERSION_MINOR\" Build \"VERSION_BUILD\" (commit hash: \"VERSION_COMMIT\")\"

#endif" >> version.h
fi

rm -rf resources/meta.xml
echo "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>
<app version=\"1\">
	<name>$VERSION_NAME</name>
	<version>$VERSION_MAJOR.$VERSION_MINOR.$VERSION_BUILD</version>
	<release_date>$VERSION_DATE</release_date>
	<coder>Damon Caskey, Plombo, SX, Utunnels, White Dragon</coder>
	<short_description>The Ultimate 2D Game Engine</short_description>
	<long_description>OpenBOR is a highly advanced continuation of Senile Team's semi-2D game engine, Beats Of Rage.  Visit http://www.ChronoCrash.com for all news, events, and releases of the engine and game modules.</long_description>
</app>" >> resources/meta.xml

rm -rf resources/Info.plist
echo "<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0">
<dict>
  <key>Author</key>
  <string>Damon Caskey</string>
  <key>Description</key>
  <string>The Ultimate 2D Game Engine</string>
  <key>ExtendedDescription</key>
  <string>OpenBOR is a highly advanced continuation of Senile Team's semi-2D game engine, Beats Of Rage.  Visit http://www.ChronoCrash.com for all news, events, and releases of the engine and game modules.</string>
  <key>CFBundleIdentifier</key>
  <string>com.ChronoCrash.openbor</string>
  <key>CFBundleShortVersionString</key>
  <string>$VERSION_MAJOR.$VERSION_MINOR</string>
  <key>NSHumanReadableCopyright</key>
  <string>The Ultimate 2D Game Engine. Presented by Damon V. Caskey.

Beats of Rage © SenileTeam
OpenBOR © ChronoCrash
All Rights Reserved</string>
  <key>CFBundlePackageType</key>
  <string>APPL</string>
  <key>CFBundleSignature</key>
  <string>OBOR</string>
  <key>CFBundleName</key>
  <string>OpenBOR</string>
  <key>CFBundleExecutable</key>
  <string>OpenBOR</string>
  <key>CFBundleVersion</key>
  <string>$VERSION_BUILD</string>
  <key>CFBundleDevelopmentRegion</key>
  <string>English</string>
  <key>CFBundleInfoDictionaryVersion</key>
  <string>6.0</string>
  <key>LSRequiresCarbon</key>
  <true/>
  <key>LSMinimumSystemVersion</key>
  <string>10.5</string>
  <key>LSMultipleInstancesProhibited</key>
  <true/>
  <key>CFBundleIconFile</key>
  <string>OpenBOR</string>
  <key>CFBundleDocumentTypes</key>
  <array>
    <dict>
      <key>CFBundleTypeExtensions</key>
      <array>
        <string>pak</string>
        <string>spk</string>
      </array>
      <key>CFBundleTypeIconFile</key>
      <string>OpenBOR.icns</string>
      <key>CFBundleTypeName</key>
      <string>PAK File</string>
      <key>CFBundleTypeOSTypes</key>
        <array>
          <string>pak</string>
          <string>spk</string>
        </array>
      <key>CFBundleTypeRole</key>
      <string>Viewer</string>
    </dict>
    <dict>
      <key>CFBundleTypeIconFile</key>
      <string>OpenBOR.icns</string>
      <key>CFBundleTypeName</key>
      <string>SPK File</string>
      <key>CFBundleTypeRole</key>
      <string>Viewer</string>
    </dict>
  </array>
</dict>
</plist>" >> resources/Info.plist
}

function archive_release {
#TRIMMED_URL=`svn info | grep "URL:" | sed s/URL:\ svn\+ssh//g`
#if test -n $TRIMMED_URL;  then
#  TRIMMED_URL="svn"$TRIMMED_URL
#fi
#svn log  $TRIMMED_URL --verbose > ./releases/VERSION_INFO.txt
7za a -t7z -mx9 -r -x!.svn "./releases/OpenBOR $VERSION.7z" ./releases/*
}

export LC_MESSAGES=en_US.UTF-8
export LC_TIME=en_US.UTF-8
export LC_ALL=en_US.UTF-8

case $1 in
1)
    read_version
    echo ------------------------------------------------------
    echo "      Creating Archive OpenBOR $VERSION.7z"
    echo ------------------------------------------------------
    archive_release
    ;;
*)
    read_version
    write_version
    ;;
esac

