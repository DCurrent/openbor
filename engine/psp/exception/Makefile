#
# OpenBOR - http://www.LavaLit.com
# -----------------------------------------------------------------------
# Licensed under the BSD license, see LICENSE in OpenBOR root for details.
#
# Copyright (c) 2004 - 2011 OpenBOR Team
#

TARGET = exception
OBJS = main.o exception_asm.o

# Define to build this as a prx (instead of a static elf)
BUILD_PRX=1
# Define the name of our custom exports (minus the .exp extension)
PRX_EXPORTS=exception.exp

USE_KERNEL_LIBC = 1
USE_KERNEL_LIBS = 1

INCDIR =
CFLAGS = -Os -G0 -Wall  -nostartfiles
CXXFLAGS = $(CFLAGS) -fno-exceptions -fno-rtti
ASFLAGS = $(CFLAGS)

LIBDIR =

%.o : %.c
	@echo Compiling $(PLATFORM) Port: $<...
	@$(CC) $(CFLAGS) -c $< -o $@
%.o : %.S
	@echo Compiling $(PLATFORM) Port: $<...
	@$(CC) $(CFLAGS) -c $< -o $@
	
PSPSDK=$(shell psp-config --pspsdk-path)
include $(PSPSDK)/lib/build.mak
