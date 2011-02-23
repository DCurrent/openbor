#
# OpenBOR - http://www.LavaLit.com
# -----------------------------------------------------------------------
# Licensed under the BSD license, see LICENSE in OpenBOR root for details.
#
# Copyright (c) 2004 - 2011 OpenBOR Team
#

#!/bin/bash
# Script acquires the verison number from SVN Repository and creates 
# a version.h as well as the environment variable to be used.

function check_svn_bin {
if [ `echo $(uname -s) | grep -o "windows"` ]; then
  if [ ! -d "tools/svn/bin" ]; then
    echo "-------------------------------------------------------"
    echo "           SVN - Not Found, Installing SVN!"
    echo "-------------------------------------------------------"
    7za x -y tools/svn/svn-win32-1.6.6.7z -otools/svn/
    echo
    echo "-------------------------------------------------------"
    echo "           SVN - Installation Has Completed!"
    echo "-------------------------------------------------------"
  fi
fi
}

# Support the Bazaar VCS as an alternative to SVN through the bzr-svn plugin
function get_revnum {
  if test -d ".svn"; then
    VERSION_BUILD=`svn info | grep "Last Changed Rev" | sed s/Last\ Changed\ Rev:\ //g`
  elif test -d ".bzr"; then
    VERSION_BUILD=`bzr version-info | grep "svn-revno" | sed 's/svn-revno: //g'`
    if [ ! $VERSION_BUILD ]; then # use non-SVN revision number if "svn-revno" property not available
      REVNO=`bzr version-info | grep "revno:" | sed 's/revno: //g'`
      BRANCH=`bzr version-info | grep "branch-nick:" | sed 's/branch-nick: //g'`
      VERSION_BUILD=$REVNO-bzr-$BRANCH
    fi
  fi
}

function read_version {
check_svn_bin
get_revnum
VERSION_NAME="OpenBOR"
VERSION_MAJOR=3
VERSION_MINOR=0
VERSION_DATE=`date '+%Y%m%d%H%M%S'`
export VERSION="v$VERSION_MAJOR.$VERSION_MINOR Build $VERSION_BUILD"
}

function write_version {
rm -rf version.h
echo "/*
 * OpenBOR - http://www.LavaLit.com
 * -----------------------------------------------------------------------
 * Licensed under the BSD license, see LICENSE in OpenBOR root for details.
 *
 * Copyright (c) 2004 - 2011 OpenBOR Team
 */

#ifndef VERSION_H
#define VERSION_H

#define VERSION_NAME \"$VERSION_NAME\"
#define VERSION_MAJOR \"$VERSION_MAJOR\"
#define VERSION_MINOR \"$VERSION_MINOR\"
#define VERSION_BUILD \"$VERSION_BUILD\"
#define VERSION (\"v\"VERSION_MAJOR\".\"VERSION_MINOR\" Build \"VERSION_BUILD)

#endif" >> version.h
rm -rf resources/meta.xml
echo "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>
<app version=\"1\">
	<name>$VERSION_NAME</name>
	<version>$VERSION_MAJOR.$VERSION_MINOR.$VERSION_BUILD</version>
	<release_date>$VERSION_DATE</release_date>
	<coder>Plombo, SX</coder>
	<short_description>Ultimate 2D Game Engine</short_description>
	<long_description>OpenBOR is a highly advanced continuation of Senile Team's semi-2D game engine, Beats Of Rage.  Visit LavaLit.com for all news, events, and releases of the engine and game modules.
</app>" >> resources/meta.xml
}

function archive_release {
svn log --verbose > ./releases/VERSION_INFO.txt
7za a -t7z -mx9 -r -x!.svn "./releases/OpenBOR $VERSION.7z" ./releases/*
}

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

