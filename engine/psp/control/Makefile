#
# OpenBOR - http://www.LavaLit.com
# -----------------------------------------------------------------------
# Licensed under the BSD license, see LICENSE in OpenBOR root for details.
#
# Copyright (c) 2004 - 2011 OpenBOR Team
#

TARGET = control
OBJS = main.o

INCDIR =
CFLAGS = -O2 -G0 -Wall
CXXFLAGS = $(CFLAGS) -fno-exceptions -fno-rtti
ASFLAGS = $(CFLAGS)

BUILD_PRX = 1
PRX_EXPORTS = $(TARGET).exp

USE_KERNEL_LIBC=1
USE_KERNEL_LIBS=1

LIBDIR =
LDFLAGS = -mno-crt0 -nostartfiles
LIBS = -lpspsdk

%.o : %.c
	@echo Compiling $(PLATFORM) Port: $<...
	@$(CC) $(CFLAGS) -c $< -o $@
%.o : %.S
	@echo Compiling $(PLATFORM) Port: $<...
	@$(CC) $(CFLAGS) -c $< -o $@

PSPSDK=$(shell psp-config --pspsdk-path)
include $(PSPSDK)/lib/build.mak


all:
	psp-build-exports -s $(PRX_EXPORTS)
