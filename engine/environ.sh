#
# OpenBOR - http://www.LavaLit.com
# -----------------------------------------------------------------------
# Licensed under the BSD license, see LICENSE in OpenBOR root for details.
#
# Copyright (c) 2004 - 2011 OpenBOR Team
#

#!/bin/bash
# Environments for Specific HOST_PLATFORMs
# environ.sh by SX (SumolX@gmail.com)

export HOST_PLATFORM=$(uname -s)
export MACHINENAME=$(uname -m)
export TOOLS=../tools/bin:../tools/7-Zip:../tools/svn/bin

if [ `echo $MACHINENAME | grep -o "ppc64"` ]; then
  export MACHINE=__ppc__
elif [ `echo $MACHINENAME | grep -o "powerpc"` ]; then
  export MACHINE=__powerpc__
elif [ `echo $MACHINENAME | grep -o "M680*[0-9]0"` ]; then
  export MACHINE=__${MACHINENAME}__
elif [ `echo $MACHINENAME | grep -o "i*[0-9]86"` ]; then
  export MACHINE=__${MACHINENAME%%-*}__
elif [ `echo $MACHINENAME | grep -o "x86"` ]; then
  export MACHINE=__${MACHINENAME%%-*}__
fi

case $1 in

############################################################################
#                                                                          #
#                           PSP Environment                                #
#                                                                          #
############################################################################
1) 
   if test -e "C:/pspsdk"; then
     export PSPDEV=C:/pspsdk
     export PATH=$PATH:$PSPDEV/bin
   elif test -e "c:/Cygwin/usr/local/pspdev"; then
     export PSPDEV=c:/Cygwin/usr/local/pspdev
     export PATH=$PATH:$PSPDEV/bin:C:/Cygwin/bin
   elif test -e "/usr/local/pspdev"; then
     export PSPDEV=/usr/local/pspdev
     export PATH=$PATH:$PSPDEV/bin
   elif [ `echo $HOST_PLATFORM | grep -o "windows"` ]; then
     if [ ! -d "../tools/psp-sdk/bin" ]; then
       echo "-------------------------------------------------------"
       echo "        PSP SDK - Not Found, Installing SDK!"
       echo "-------------------------------------------------------"
       ../tools/7-Zip/7za.exe x -y ../tools/psp-sdk/psp-sdk.7z -o../tools/psp-sdk/
       echo
       echo "-------------------------------------------------------"
       echo "        PSP SDK - Installation Has Completed!"
       echo "-------------------------------------------------------"
     fi
       export PSPDEV=../tools/psp-sdk
       export PATH=$TOOLS:$PSPDEV/bin
       HOST_PLATFORM="SVN";
   fi
   if test $PSPDEV; then
     export PSPSDK=$PSPDEV/psp/sdk
     echo "-------------------------------------------------------"
     echo "          PSP SDK ($HOST_PLATFORM) Environment Loaded!"
     echo "-------------------------------------------------------"
   else
     echo "-------------------------------------------------------"
     echo "            ERROR - PSP Environment Failed"
     echo "                   SDK Installed?"
     echo "-------------------------------------------------------"
   fi
   ;;

############################################################################
#                                                                          #
#                              PS2 Environment                             #
#                                                                          #
############################################################################
2)
   if test -e "c:/Cygwin/usr/local/ps2dev"; then
     export PS2DEV=c:/Cygwin/usr/local/ps2dev
     export PS2SDK=$PS2DEV/ps2sdk
   elif test -e "/usr/local/ps2dev"; then
     export PS2DEV=/usr/local/ps2dev
     export PS2SDK=$PS2DEV/ps2sdk
   elif [ `echo $HOST_PLATFORM | grep -o "windows"` ]; then
     if [ ! -d "../tools/ps2-sdk/bin" ]; then
       echo "-------------------------------------------------------"
       echo "        PS2 SDK - Not Found, Installing SDK!"
       echo "-------------------------------------------------------"
       ../tools/7-Zip/7za.exe x -y ../tools/ps2-sdk/ps2-sdk.7z -o../tools/ps2-sdk/
       echo
       echo "-------------------------------------------------------"
       echo "        PS2 SDK - Installation Has Completed!"
       echo "-------------------------------------------------------"
     fi
     export PS2DEV=../tools/ps2-sdk
     export PS2SDK=$PS2DEV
     HOST_PLATFORM="SVN"
   fi
   if test $PS2DEV; then
     export PATH=$PATH:$PS2DEV/bin
     export PATH=$PATH:$PS2DEV/ee/bin
     export PATH=$PATH:$PS2DEV/iop/bin
     export PATH=$PATH:$PS2DEV/dvp/bin
     if [ $HOST_PLATFORM = SVN ]; then 
       export PATH=$TOOLS:$PS2SDK/bin
     else
       export PATH=$PATH:$PS2SDK/bin
     fi
     echo "-------------------------------------------------------"
     echo "          PS2 SDK ($HOST_PLATFORM) Environment Loaded!"
     echo "-------------------------------------------------------"
   else
     echo "-------------------------------------------------------"
     echo "            ERROR - PS2 Environment Failed"
     echo "                   SDK Installed?"
     echo "-------------------------------------------------------"
   fi
   ;;

############################################################################
#                                                                          #
#                            GP2X Environment                              #
#                                                                          #
############################################################################
3)
   if test -e "c:/Cygwin/opt/open2x/gcc-4.1.1-glibc-2.3.6/bin/arm-open2x-linux-gcc.exe"; then
     export GP2XDEV=c:/Cygwin/opt/open2x/gcc-4.1.1-glibc-2.3.6/bin
     export SDKPATH=c:/Cygwin/opt/open2x/gcc-4.1.1-glibc-2.3.6
     export PATH=$PATH:$GP2XDEV
   elif test -e "/opt/open2x/gcc-4.1.1-glibc-2.3.6/bin/arm-open2x-linux-gcc"; then
     export GP2XDEV=/opt/open2x/gcc-4.1.1-glibc-2.3.6/bin
     export SDKPATH=/opt/open2x/gcc-4.1.1-glibc-2.3.6
     export PATH=$PATH:$GP2XDEV
   elif test -e "c:/Cygwin/opt/open2x/gcc-4.1.1-glibc-2.3.6/arm-open2x-linux/bin/arm-open2x-linux-gcc.exe"; then
     export GP2XDEV=/opt/open2x/gcc-4.1.1-glibc-2.3.6/arm-open2x-linux/bin
     export SDKPATH=/opt/open2x/gcc-4.1.1-glibc-2.3.6/arm-open2x-linux
     export PATH=$PATH:$GP2XDEV
   elif test -e "/opt/open2x/gcc-4.1.1-glibc-2.3.6/arm-open2x-linux/bin/arm-open2x-linux-gcc"; then
     export GP2XDEV=/opt/open2x/gcc-4.1.1-glibc-2.3.6/arm-open2x-linux/bin
     export SDKPATH=/opt/open2x/gcc-4.1.1-glibc-2.3.6/arm-open2x-linux
     export PATH=$PATH:$GP2XDEV
   elif [ `echo $HOST_PLATFORM | grep -o "windows"` ]; then
     if [ ! -d "../tools/gp2x-sdk/bin" ]; then
       echo "-------------------------------------------------------"
       echo "         GP2X SDK - Not Found, Installing SDK!"
       echo "-------------------------------------------------------"
       ../tools/7-Zip/7za.exe x -y ../tools/gp2x-sdk/gp2x-sdk.7z -o../tools/gp2x-sdk/
       echo
       echo "-------------------------------------------------------"
       echo "         GP2X SDK - Installation Has Completed!"
       echo "-------------------------------------------------------"
     fi
     export GP2XDEV=../tools/gp2x-sdk/bin
     export SDKPATH=../tools/gp2x-sdk/arm-open2x-linux
     export PATH=$TOOLS:$GP2XDEV
     HOST_PLATFORM="SVN"
   fi
   if test $GP2XDEV; then
     echo "-------------------------------------------------------"
     echo "           GP2X SDK ($HOST_PLATFORM) Environment Loaded!"
     echo "-------------------------------------------------------"
   else
     echo "-------------------------------------------------------"
     echo "            ERROR - GP2X Environment Failed"
     echo "                   SDK Installed?"
     echo "-------------------------------------------------------"
   fi
   ;;

############################################################################
#                                                                          #
#                           Linux Environment                              #
#                                                                          #
############################################################################
4)
   if [ `gcc -dumpmachine | grep -o $GCC_TARGET` ]; then
     export GCC_TARGET=`gcc -dumpmachine`
     export LNXDEV=`dirname \`which gcc\``
     export PREFIX=
     export SDKPATH=$LNXDEV/..
   elif [ `ls \`echo $PATH | sed 'y/:/ /'\` | grep -o "$GCC_TARGET-gcc" | tail -n 1` ]; then
     export TARGET_CC_NAME=`ls \`echo $PATH | sed 'y/:/ /'\` | grep -o $GCC_TARGET-gcc | tail -n 1`
     export TARGET_CC=`which $TARGET_CC_NAME`
     export GCC_TARGET=`$TARGET_CC -dumpmachine`
     export LNXDEV=`dirname $TARGET_CC`
     export PREFIX=`echo $TARGET_CC_NAME | sed 's/gcc$//'`
     export SDKPATH=$LNXDEV/..
   fi
   if test $LNXDEV; then
     echo "-------------------------------------------------------"
     echo "   Linux $TARGET_ARCH SDK ($GCC_TARGET) Environment Loaded!"
     echo "-------------------------------------------------------"
   else
     echo "-------------------------------------------------------"
     echo "     ERROR - Linux $TARGET_ARCH Environment Failed"
     echo "                 SDK Installed?"
     echo "-------------------------------------------------------"
   fi
   ;;

############################################################################
#                                                                          #
#                           Windows Environment                            #
#                                                                          #
############################################################################
5)
   if test -e "/usr/i586-mingw32msvc"; then
     export WINDEV=/usr/bin
     export SDKPATH=/usr/i586-mingw32msvc
     export PREFIX=i586-mingw32msvc-
     export PATH=$WINDEV:$PATH
   elif test -e "/usr/local/i386-mingw32-3.4.5"; then
     export WINDEV=/usr/local/i386-mingw32-3.4.5/bin
     export SDKPATH=/usr/local/i386-mingw32-3.4.5
     export PREFIX=i386-mingw32-
     export PATH=$WINDEV:$PATH
   elif test -e "/usr/local/i386-mingw32-4.3.0"; then
     export WINDEV=/usr/local/i386-mingw32-4.3.0/bin
     export SDKPATH=/usr/local/i386-mingw32-4.3.0
     export PREFIX=i386-mingw32-
     export PATH=$WINDEV:$PATH
   elif [ `echo $HOST_PLATFORM | grep -o "windows"` ]; then
     if [ ! -d "../tools/win-sdk/bin" ]; then
       echo "-------------------------------------------------------"
       echo "      Windows SDK - Not Found, Installing SDK!"
       echo "-------------------------------------------------------"
       ../tools/7-Zip/7za.exe x -y ../tools/win-sdk/MinGW.7z -o../tools/win-sdk/
       echo
       echo "-------------------------------------------------------"
       echo "      Windows SDK - Installation Has Completed!"
       echo "-------------------------------------------------------"
     fi
     export WINDEV=../tools/win-sdk/bin
     export SDKPATH=../tools/win-sdk
     export EXTENSION=.exe
     export PATH=$TOOLS:$WINDEV
     HOST_PLATFORM="SVN";
   fi
   if test $WINDEV; then
       echo "-------------------------------------------------------"
       echo "     Windows SDK ($HOST_PLATFORM) $MACHINENAME Environment Loaded!"
       echo "-------------------------------------------------------"
   else
       echo "-------------------------------------------------------"
       echo "          ERROR - Windows Environment Failed"
       echo "                   SDK Installed?"
       echo "-------------------------------------------------------"
   fi
   ;;

############################################################################
#                                                                          #
#                           Dreamcast Environment                          #
#                                                                          #
############################################################################
6)
   if test -e "/usr/local/dcdev/kos"; then
     . /usr/local/dcdev/kos/environ.sh
   elif [ `echo $HOST_PLATFORM | grep -o "windows"` ]; then
     if [ ! -d "../tools/dc-sdk/kos" ]; then
        echo "-------------------------------------------------------"
        echo "     Dreamcast SDK - Not Found, Installing SDK!"
        echo "-------------------------------------------------------"
        ../tools/7-Zip/7za.exe x -y ../tools/dc-sdk/kos-svn-698.7z -o../tools/dc-sdk/
        echo
        echo "-------------------------------------------------------"
        echo "     Dreamcast SDK - Installation Has Completed!"
        echo "-------------------------------------------------------"
     fi
     HOST_PLATFORM="SVN";
     export PATH=$TOOLS
     . ../tools/dc-sdk/kos/environ.sh     
   fi
   if test $KOS_BASE; then
     echo "-------------------------------------------------------"
     echo "          Dreamcast SDK ($HOST_PLATFORM) Environment Loaded!"
     echo "-------------------------------------------------------"
   else
     echo "-------------------------------------------------------"
     echo "         ERROR - Dreamcast Environment Failed"
     echo "                   SDK Installed?"
     echo "-------------------------------------------------------"
   fi
   ;;

############################################################################
#                                                                          #
#                             Wii Environment                              #
#                                                                          #
############################################################################
7)
   if test -e "/opt/devkitpro"; then
     export DEVKITPRO=/opt/devkitpro
     export DEVKITPPC=$DEVKITPRO/devkitPPC
     export PATH=$PATH:$DEVKITPPC/bin
   elif test -e "c:/devkitpro"; then
     export DEVKITPRO=c:/devkitpro
     export DEVKITPPC=$DEVKITPRO/devkitPPC
     export PATH=$PATH:$DEVKITPPC/bin
   elif [ `echo $HOST_PLATFORM | grep -o "windows"` ]; then
     if [ ! -d "../tools/devkitpro/devkitPPC" ]; then
        echo "-------------------------------------------------------"
        echo "         WII SDK - Not Found, Installing SDK!"
        echo "-------------------------------------------------------"
        ../tools/7-Zip/7za.exe x -y ../tools/devkitpro/devkitpro.7z -o../tools/devkitpro/
        echo
        echo "-------------------------------------------------------"
        echo "         WII SDK - Installation Has Completed!"
        echo "-------------------------------------------------------"
     fi
     HOST_PLATFORM="SVN";
     export DEVKITPRO=../tools/devkitpro
     export DEVKITPPC=$DEVKITPRO/devkitPPC
     export PATH=$TOOLS:$DEVKITPPC/bin
   fi
   if test $DEVKITPPC; then
     echo "-------------------------------------------------------"
     echo "         WII SDK ($HOST_PLATFORM) Environment Loaded!"
     echo "-------------------------------------------------------"
   else
     echo "-------------------------------------------------------"
     echo "            ERROR - WII Environment Failed"
     echo "                    SDK Installed?"
     echo "-------------------------------------------------------"
   fi
   ;;

############################################################################
#                                                                          #
#                          Dingoo Environment                              #
#                                                                          #
############################################################################
8)
   if test -e "/opt/mipsel-linux-uclibc"; then
     export DINGUX_TOOLCHAIN=/opt/mipsel-linux-uclibc
     export DINGUX_TOOLCHAIN_PREFIX=$DINGUX_TOOLCHAIN/usr
     export PATH=$PATH:$DINGUX_TOOLCHAIN/usr/bin
   fi
   if test $DINGUX_TOOLCHAIN; then
     echo "-------------------------------------------------------"
     echo "        DINGOO SDK ($HOST_PLATFORM) Environment Loaded!"
     echo "-------------------------------------------------------"
   else
     echo "-------------------------------------------------------"
     echo "            ERROR - DINGOO Environment Failed"
     echo "                    SDK Installed?"
     echo "-------------------------------------------------------"
   fi
   ;;
   
############################################################################
#                                                                          #
#                             WIZ Environment                              #
#                                                                          #
############################################################################
9)
   if test -e "/opt/openwiz/toolchain/arm-openwiz-linux-gnu"; then
     export WIZDEV=/opt/openwiz/toolchain/arm-openwiz-linux-gnu/bin
     export SDKPATH=/opt/openwiz/toolchain/arm-openwiz-linux-gnu
     export PREFIX=arm-openwiz-linux-gnu-
     export PATH=$PATH:$WIZDEV
   elif [ `echo $HOST_PLATFORM | grep -o "windows"` ]; then
     if [ ! -d "../tools/wiz-sdk/tools" ]; then
       echo "-------------------------------------------------------"
       echo "         WIZ SDK - Not Found, Installing SDK!"
       echo "-------------------------------------------------------"
       ../tools/7-Zip/7za.exe x -y ../tools/wiz-sdk/wiz-sdk.7z -o../tools/wiz-sdk/
       echo
       echo "-------------------------------------------------------"
       echo "         WIZ SDK - Installation Has Completed!"
       echo "-------------------------------------------------------"
     fi
     export WIZDEV=../tools/wiz-sdk/tools/gcc-4.0.2-glibc-2.3.6/arm-linux/bin
     export SDKPATH=../tools/wiz-sdk/DGE
     export PREFIX=arm-linux-
     export EXTENSION=.exe
     export PATH=$TOOLS:$WIZDEV
     HOST_PLATFORM="SVN"
   fi
   if test $WIZDEV; then
     echo "-------------------------------------------------------"
     echo "        WIZ SDK ($HOST_PLATFORM) Environment Loaded!"
     echo "-------------------------------------------------------"
   else
     echo "-------------------------------------------------------"
     echo "            ERROR - WIZ Environment Failed"
     echo "                    SDK Installed?"
     echo "-------------------------------------------------------"
   fi
   ;;

############################################################################
#                                                                          #
#                          Darwin Environment                              #
#                                                                          #
############################################################################
10)
   if test -e "/opt/mac"; then
     export DWNDEV=/opt/mac
     export SDKPATH=$DWNDEV/SDKs/MacOSX10.4u.sdk
     export PREFIX=i686-apple-darwin8-
     export PATH=$PATH:$DWNDEV/bin
   elif test -e "/sw/bin"; then
     export DWNDEV=/sw
     export SDKPATH=/Developer/SDKs/MacOSX10.6.sdk
     export PATH=$PATH:$DWNDEV/bin
   elif test -e "/opt/local/bin"; then
     export DWNDEV=/opt/local
     export SDKPATH=/Developer/SDKs/MacOSX10.6.sdk
     export PATH=$PATH:DWNDEV/bin
   fi
   if test $DWNDEV; then
     echo "-------------------------------------------------------"
     echo "        Darwin SDK $MACHINENAME Environment Loaded!"
     echo "-------------------------------------------------------"
   fi
   ;;

############################################################################
#                                                                          #
#                             Wrong value?                                 #
#                                                                          #
############################################################################
*)
   echo
   echo "-------------------------------------------------------"
   echo "    1 = PSP"
   echo "    2 = PS2"
   echo "    3 = Gp2x"
   echo "    4 = Linux"
   echo "    5 = Windows"
   echo "    6 = Dreamcast"
   echo "    7 = Nintendo Wii"
   echo "    8 = Dingoo-linux"
   echo "    9 = Wiz"
   echo "   10 = Darwin"
   echo "-------------------------------------------------------"
   echo
   ;;

esac
