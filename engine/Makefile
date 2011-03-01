#
# OpenBOR - http://www.LavaLit.com
# ---------------------------------------------------------------------------------------------------
# Licensed under the BSD license, see LICENSE in OpenBOR root for details.
#
# Copyright (c) 2004 - 2011 OpenBOR Team
#

#----------------------------------------------------------------------------------------------------
#
#               	OpenBOR Makefile for ALL TARGET_PLATFORMs 
#
#             PSP, PS3, Wii, Dreamcast, GP2X, WIZ, Pandora, Dingoo, Windows, Darwin & Linux
#
#----------------------------------------------------------------------------------------------------

ifndef VERSION_NAME
VERSION_NAME = OpenBOR
endif

#----------------------------------------------------------------------------------------------------
# Defines
#----------------------------------------------------------------------------------------------------


ifdef BUILD_PSP
TARGET          = $(VERSION_NAME)
TARGET_FINAL    = EBOOT.PBP
TARGET_PLATFORM = PSP
PBPNAME_STR     = $(TARGET)
BUILD_TREMOR    = 1
BUILDING        = 1
ifeq ($(BUILD_PSP), 0)
BUILD_DEBUG     = 1
endif
endif


ifdef BUILD_WIN
TARGET          = $(VERSION_NAME).elf
TARGET_FINAL    = $(VERSION_NAME).exe
TARGET_PLATFORM = WIN
TARGET_ARCH     = x86
TARGET_RESOURCE = resources/$(VERSION_NAME).res
OBJTYPE         = win32
BUILD_SDL       = 1
BUILD_GFX       = 1
BUILD_STATIC    = 1
BUILD_SDL_IO    = 1
BUILD_OPENGL    = 1
BUILD_LOADGL    = 1
BUILD_VORBIS    = 1
BUILDING        = 1
YASM 	        = yasm$(EXTENSION)
CC              = $(WINDEV)/$(PREFIX)gcc$(EXTENSION)
INCLUDES        = $(SDKPATH)/include \
                  $(SDKPATH)/include/SDL
LIBRARIES       = $(SDKPATH)/lib
ARCHFLAGS       = -m32
ifeq ($(findstring 86, $(TARGET_ARCH)), 86)
BUILD_MMX       = 1
endif
ifeq ($(BUILD_WIN), 0)
BUILD_DEBUG     = 1
endif
endif


ifdef BUILD_LINUX
TARGET 	        = $(VERSION_NAME).elf
TARGET_FINAL    = $(VERSION_NAME)
TARGET_PLATFORM = LINUX
BUILD_SDL       = 1
BUILD_GFX       = 1
BUILD_PTHREAD   = 1
BUILD_SDL_IO    = 1
BUILD_OPENGL    = 1
BUILD_LOADGL    = 1
BUILD_VORBIS    = 1
BUILDING        = 1
YASM 	        = yasm
CC  	        = $(LNXDEV)/$(PREFIX)gcc
OBJTYPE         = elf
INCLUDES        = $(SDKPATH)/include \
                  $(SDKPATH)/include/SDL
ifeq ($(findstring 64, $(TARGET_ARCH)), 64)
BUILD_AMD64     = 1
ARCHFLAGS       = -m64
LIBRARIES       = $(SDKPATH)/lib64
else
ARCHFLAGS       = -m32
LIBRARIES       = $(SDKPATH)/lib32
endif
ifeq ($(findstring 86, $(TARGET_ARCH)), 86)
BUILD_MMX       = 1
endif
ifeq ($(BUILD_LINUX), 0)
BUILD_DEBUG     = 1
endif
endif

ifdef BUILD_DARWIN
TARGET          = $(VERSION_NAME).elf
TARGET_FINAL    = $(VERSION_NAME)
TARGET_PLATFORM = DARWIN
TARGET_ARCH     = x86
BUILD_SDL       = 1
BUILD_GFX       = 1
BUILD_LINUX     = 1
BUILD_PTHREAD   = 1
BUILD_SDL_IO    = 1
BUILD_OPENGL    = 1
BUILD_LOADGL    = 1
BUILD_VORBIS    = 1
BUILDING        = 1
YASM            = yasm
CC              = gcc
OBJTYPE         = macho
INCLUDES        = $(DWNDEV)/include \
                  $(DWNDEV)/include/SDL \
                  $(SDKPATH)/usr/include/malloc
LIBRARIES       = $(DWNDEV)/lib
ARCHFLAGS       = -m32
ifeq ($(findstring 86, $(TARGET_ARCH)), 86)
BUILD_MMX       = 1
endif
ifeq ($(BUILD_DARWIN), 0)
BUILD_DEBUG     = 1
endif
endif

ifdef BUILD_PANDORA
TARGET 	        = $(VERSION_NAME).elf
TARGET_FINAL    = $(VERSION_NAME)
TARGET_PLATFORM = PANDORA
BUILD_LINUX     = 1
BUILD_SDL       = 1
BUILD_GFX       = 1
BUILD_PTHREAD   = 1
BUILD_SDL_IO    = 1
BUILD_TREMOR    = 1
BUILDING        = 1
CC  	        = $(PNDDEV)/bin/arm-none-linux-gnueabi-gcc
INCLUDES        = $(PNDDEV)/include \
                  $(PNDDEV)/include/SDL
OBJTYPE         = elf
LIBRARIES       = $(PNDDEV)/lib
ifeq ($(BUILD_PANDORA), 0)
BUILD_DEBUG     = 1
endif
endif

ifdef BUILD_GP2X
TARGET 	        = $(VERSION_NAME).elf
TARGET_FINAL    = $(VERSION_NAME).gpe
TARGET_PLATFORM = GP2X
BUILD_SDL       = 1
BUILD_GFX       = 1
BUILD_STATIC    = 1
BUILD_PTHREAD   = 1
BUILD_SDL_IO    = 1
BUILD_TREMOR    = 1
BUILDING        = 1
CC              = $(GP2XDEV)/arm-open2x-linux-gcc
UNAME          := $(shell uname)
INCLUDES        = $(SDKPATH)/include \
                  $(SDKPATH)/include/SDL
LIBRARIES       = $(SDKPATH)/lib
ifeq ($(BUILD_GP2X), 0)
BUILD_DEBUG     = 1
endif
endif

ifdef BUILD_WIZ
TARGET 	        = $(VERSION_NAME).elf
TARGET_FINAL    = $(VERSION_NAME).gpe
TARGET_PLATFORM = WIZ
BUILD_SDL       = 1
BUILD_GFX       = 1
BUILD_PTHREAD   = 1
BUILD_SDL_IO    = 1
BUILD_TREMOR    = 1
BUILDING        = 1
CC              = $(WIZDEV)/$(PREFIX)gcc$(EXTENSION)
UNAME          := $(shell uname)
INCLUDES        = $(SDKPATH)/include \
                  $(SDKPATH)/include/SDL
LIBRARIES       = $(SDKPATH)/lib
ifeq ($(findstring wiz-sdk, $(SDKPATH)), wiz-sdk)
INCLUDES       += $(SDKPATH)/../include
LIBRARIES      += $(SDKPATH)/../lib/target \
                  $(SDKPATH)/lib/target
endif
ifeq ($(BUILD_WIZ), 0)
BUILD_DEBUG     = 1
endif
endif

ifdef BUILD_DINGOO
TARGET 	        = $(VERSION_NAME).elf
TARGET_FINAL    = $(VERSION_NAME).dge
TARGET_PLATFORM = DINGOO
BUILD_SDL       = 1
BUILD_GFX       = 1
BUILD_STATIC    = 1
BUILD_PTHREAD   = 1
BUILD_SDL_IO    = 1
BUILD_TREMOR    = 1
BUILDING        = 1
CC              = $(DINGUX_TOOLCHAIN_PREFIX)/bin/mipsel-linux-gcc
INCLUDES        = $(DINGUX_TOOLCHAIN_PREFIX)/include \
                  $(DINGUX_TOOLCHAIN_PREFIX)/include/SDL
LIBRARIES       = $(DINGUX_TOOLCHAIN_PREFIX)/lib
ifeq ($(BUILD_DINGOO), 0)
BUILD_DEBUG     = 1
endif
endif

ifdef BUILD_DC
TARGET 	        = $(VERSION_NAME).elf
TARGET_FINAL    = 1ST_READ.BIN
TARGET_PLATFORM = DC
BUILDING        = 1
INCLUDES        = $(KOS_BASE)/../kos-ports/include/SDL \
                  $(KOS_BASE)/../kos-ports/include/SDL-1.2.9 \
                  $(KOS_BASE)/../kos-ports/include/png \
                  $(KOS_BASE)/../kos-ports/include/zlib \
                  $(KOS_BASE)/../kos-ports/include/liboggvorbis \
                  $(KOS_BASE)/../kos-ports/libtremor/xiph
ifeq ($(BUILD_DC), 0)
BUILD_DEBUG     = 1
endif
endif

ifdef BUILD_WII
TARGET 	        = $(VERSION_NAME).elf
TARGET_MAP      = $(TARGET).map
TARGET_FINAL    = boot.dol
TARGET_PLATFORM = WII
BUILD_TREMOR    = 1
BUILD_ELM       = 1
BUILDING        = 1
INCLUDES        = $(DEVKITPRO)/portlibs/ppc/include \
                  $(DEVKITPRO)/libogc/include
LIBRARIES       = $(DEVKITPRO)/portlibs/ppc/lib \
                  $(DEVKITPRO)/libogc/lib/wii
ifeq ($(BUILD_WII), 0)
BUILD_DEBUG     = 1
endif
endif


STRIP           = cp $(TARGET) $(TARGET_FINAL)
ifndef BUILD_DEBUG
ifdef BUILD_WIN
STRIP 	        = $(WINDEV)/$(PREFIX)strip$(EXTENSION) $(TARGET) -o $(TARGET_FINAL)
endif
ifdef BUILD_LINUX
STRIP 	        = $(LNXDEV)/$(PREFIX)strip $(TARGET) -o $(TARGET_FINAL)
endif
ifdef BUILD_DARWIN
STRIP           = strip $(TARGET) -o $(TARGET_FINAL)
endif
ifdef BUILD_PANDORA
STRIP 	        = $(PNDDEV)/bin/arm-none-linux-gnueabi-strip $(TARGET) -o $(TARGET_FINAL)
endif
ifdef BUILD_GP2X
STRIP 	        = $(GP2XDEV)/arm-open2x-linux-strip $(TARGET) -o $(TARGET_FINAL)
endif
ifdef BUILD_WIZ
STRIP 	        = $(WIZDEV)/$(PREFIX)strip$(EXTENSION) $(TARGET) -o $(TARGET_FINAL)
endif
ifdef BUILD_DINGOO
STRIP           = $(DINGUX_TOOLCHAIN_PREFIX)/bin/mipsel-linux-strip $(TARGET) -o $(TARGET_FINAL)
endif
ifdef BUILD_WII
STRIP           = elf2dol $< $@
endif
endif


#----------------------------------------------------------------------------------------------------
# Directories
#----------------------------------------------------------------------------------------------------

ifdef BUILD_PSP
INCS           += psp
endif


ifdef BUILD_DC
INCS           += dc
endif


ifdef BUILD_WII
INCS           += wii
endif


ifdef BUILD_SDL
INCS           += sdl
endif


ifdef BUILD_LINUX
INCS           += 'libpng-config --prefix'/include/libpng
endif


ifdef BUILD_GP2X
INCS 	       += sdl/gp2x
endif

ifdef BUILD_WIZ
INCS 	       += sdl/gp2x
endif

ifdef BUILD_DINGOO
INCS           += '$(DINGUX_TOOLCHAIN_PREFIX)/bin/libpng-config --prefix'/include/libpng sdl/dingoo
endif


INCS 	       += .                                                                                 \
                  source                                                                            \
                  source/adpcmlib                                                                   \
                  source/gamelib                                                                    \
                  source/preprocessorlib                                                            \
                  source/ramlib                                                                     \
                  source/randlib                                                                    \
                  source/scriptlib                                                                  \
                  source/tracelib                                                                   \
                  source/xpmlib

ifndef BUILD_DC
INCS 	       += source/pcxlib
endif



ifdef BUILD_GFX
INCS 	       += source/gfxlib
endif

INCS += $(INCLUDES)

#----------------------------------------------------------------------------------------------------
# Objects
#----------------------------------------------------------------------------------------------------

ADPCM 	        = source/adpcmlib/adpcm.o

ifdef BUILD_GFX
GFX 	        = source/gfxlib/2xSaI.o                                                             \
                  source/gfxlib/bilinear.o                                                          \
                  source/gfxlib/dotmatrix.o                                                         \
                  source/gfxlib/gfx.o                                                               \
                  source/gfxlib/hq2x.o                                                              \
                  source/gfxlib/motionblur.o                                                        \
                  source/gfxlib/scale2x.o                                                           \
                  source/gfxlib/scanline.o                                                          \
                  source/gfxlib/simple2x.o                                                          \
                  source/gfxlib/tv2x.o
endif
		  
ifdef BUILD_MMX
GFX 	       += source/gfxlib/2xSaImmx.o                                                          \
                  source/gfxlib/bilinearmmx.o                                                       \
                  source/gfxlib/hq2x16mmx.o
endif

GAME	        = source/gamelib/draw.o                                                             \
                  source/gamelib/draw16.o                                                           \
                  source/gamelib/draw32.o                                                           \
                  source/gamelib/font.o                                                             \
                  source/gamelib/anigif.o                                                           \
                  source/gamelib/bitmap.o 	                                                        \
                  source/gamelib/screen.o                                                           \
                  source/gamelib/screen16.o                                                         \
                  source/gamelib/screen32.o                                                         \
                  source/gamelib/loadimg.o                                                          \
                  source/gamelib/palette.o                                                          \
                  source/gamelib/packfile.o                                                         \
                  source/gamelib/filecache.o                                                        \
                  source/gamelib/pixelformat.o                                                      \
                  source/gamelib/soundmix.o                                                         \
                  source/gamelib/spritef.o                                                          \
                  source/gamelib/spriteq.o                                                          \
                  source/gamelib/spritex8p16.o                                                      \
                  source/gamelib/spritex8p32.o                                                      \
                  source/gamelib/texture.o                                                          \
                  source/gamelib/texture16.o                                                        \
                  source/gamelib/texture32.o
SCRIPT          = source/scriptlib/StackedSymbolTable.o                                             \
                  source/scriptlib/ScriptVariant.o                                                  \
                  source/scriptlib/SymbolTable.o                                                    \
                  source/scriptlib/Instruction.o                                                    \
                  source/scriptlib/Interpreter.o                                                    \
                  source/scriptlib/ParserSet.o                                                      \
                  source/scriptlib/Parser.o                                                         \
                  source/scriptlib/Lexer.o                                                          \
                  source/scriptlib/Stack.o                                                          \
                  source/scriptlib/List.o                                                           \
                  source/preprocessorlib/pp_lexer.o                                                 \
                  source/preprocessorlib/pp_parser.o
RAM             = source/ramlib/ram.o
RAND	        = source/randlib/rand32.o
TRACE           = source/tracelib/tracemalloc.o
XPM             = source/xpmlib/xpm.o
SOURCE	        = source/stringptr.o                                                                \
				  source/utils.o                                                                    \
                  source/stristr.o


ifndef BUILD_DC
PCX             = source/pcxlib/savepcx.o
endif


ifdef BUILD_PSP
GAME_CONSOLE    = psp/control/control.o                                                             \
                  psp/dvemgr/dvemgr.o                                                               \
                  psp/kernel/kernel.o                                                               \
                  psp/graphics.o                                                                    \
                  psp/audiodrv.o                                                                    \
                  psp/sblaster.o                                                                    \
                  psp/control.o                                                                     \
                  psp/vertex.o                                                                      \
                  psp/timer.o                                                                       \
                  psp/video.o                                                                       \
                  psp/image.o                                                                       \
                  psp/menu.o                                                                        \
                  psp/pspport.o
endif


ifdef BUILD_DC
GAME_CONSOLE    = dc/dcport.o                                                                       \
                  dc/bios.o                                                                         \
                  dc/gdrom.o                                                                        \
                  dc/timer.o                                                                        \
                  dc/sblaster.o                                                                     \
                  dc/control.o                                                                      \
                  dc/video.o
endif


ifdef BUILD_WII
GAME_CONSOLE    = wii/control.o                                                                     \
                  wii/sblaster.o                                                                    \
                  wii/timer.o                                                                       \
                  wii/video.o                                                                       \
                  wii/menu.o                                                                        \
                  wii/wiiport.o
endif


ifdef BUILD_SDL
GAME	       += source/gamelib/filters.o
endif



ifdef BUILD_SDL_IO
GAME_CONSOLE   += sdl/joysticks.o                                                                   \
                  sdl/control.o                                                                     \
                  sdl/sblaster.o                                                                    \
                  sdl/timer.o                                                                       \
                  sdl/sdlport.o                                                                     \
                  sdl/video.o                                                                       \
                  sdl/menu.o
endif


ifdef BUILD_OPENGL
GAME_CONSOLE   += sdl/opengl.o
endif


ifdef BUILD_LOADGL
GAME_CONSOLE   += sdl/loadgl.o
endif


ifdef BUILD_GP2X
GAME_CONSOLE   += sdl/gp2x/gp2xport.o
endif


ifdef BUILD_WIZ
GAME_CONSOLE   += sdl/gp2x/gp2xport.o
endif



MAIN            = openborscript.o					                                                \
                  openbor.o

OBJS            = $(GAME_CONSOLE)                                                                        \
                  $(ADPCM)                                                                          \
                  $(GFX)                                                                            \
                  $(GAME)                                                                           \
                  $(PCX)                                                                            \
                  $(SOURCE)                                                                         \
                  $(SCRIPT)                                                                         \
                  $(RAM)                                                                            \
                  $(RAND)                                                                           \
                  $(TRACE)                                                                          \
                  $(XPM)                                                                            \
                  $(MAIN)
		  
#----------------------------------------------------------------------------------------------------
# Compiler Flags
#----------------------------------------------------------------------------------------------------

CFLAGS 	       += $(addprefix -I", $(addsuffix ", $(INCS))) $(ARCHFLAGS) -D$(TARGET_PLATFORM)
CFLAGS 	       += -g -Wall -Werror -fsigned-char 


ifndef BUILD_DEBUG
ifdef BUILD_DC
CFLAGS 	       += -O9
else
CFLAGS 	       += -O2
endif
CFLAGS 	       += -fno-ident -freorder-blocks 
ifndef BUILD_AMD64
CFLAGS         += -fomit-frame-pointer 
endif
else
CFLAGS 	       += -DDEBUG -O0
ifdef NO_RAM_DEBUGGER
CFLAGS         += -DNO_RAM_DEBUGGER 
endif
endif


ifdef BUILD_PSP
CFLAGS         += -G0
endif


ifdef BUILD_SDL
CFLAGS 	       += -DSDL
endif


ifdef BUILD_DARWIN
CFLAGS 	       += -DLINUX -headerpad_max_install_names -arch i386 -isysroot $(SDKPATH)
endif


ifdef BUILD_PANDORA
CFLAGS         += -DLINUX
endif


ifdef BUILD_WII
CFLAGS 	       += -D__ppc__ $(MACHDEP) -Wl,-Map,$(TARGET_MAP)
ifdef BUILD_ELM
CFLAGS         += -DUSE_LIBELM
endif
endif


ifdef BUILD_WIZ
CFLAGS 	       += -DGP2X
endif


ifdef BUILD_DINGOO
CFLAGS 	       += -D_REENTRANT
endif


ifdef BUILD_MMX
CFLAGS 	       += -DMMX
endif


ifdef BUILD_VORBIS
CFLAGS         += -DOV_EXCLUDE_STATIC_CALLBACKS
endif


ifdef BUILD_TREMOR
CFLAGS         += -DTREMOR
endif


ifdef BUILD_OPENGL
CFLAGS         += -DOPENGL
endif


ifdef BUILD_LOADGL
CFLAGS         += -DLOADGL
endif


ifdef BUILD_GLES
CFLAGS         += -DGLES
endif


ifdef BUILD_VERBOSE
CFLAGS         += -DVERBOSE
endif


CXXFLAGS        = $(CFLAGS) -fno-exceptions -fno-rtti
ASFLAGS         = $(CFLAGS)

#----------------------------------------------------------------------------------------------------
# Library
#----------------------------------------------------------------------------------------------------

LIBS            = $(addprefix -L", $(addsuffix ", $(LIBRARIES)))


ifdef BUILD_PSP
LIBS 	       += -lpspgu -lpspaudio -lpsppower -lpsprtc
endif


ifdef BUILD_DARWIN
LIBS           += -Wl,-syslibroot,$(SDKPATH) \
                  -framework Cocoa \
                  -framework OpenGL \
                  -framework Carbon \
                  -framework AudioUnit \
                  -framework IOKit \
                  -lSDLmain
endif


ifdef BUILD_SDL
ifdef BUILD_WIZ
LIBS           += -lSDL -lSDL_gfx -lts
else
LIBS           += -Wl,-rpath,$(LIBRARIES) -lSDL -lSDL_gfx
endif
endif


ifdef BUILD_WIN
LIBS           += -luser32 -lgdi32 -lwinmm -ldxguid -lpsapi -lopengl32 -mwindows
endif


ifdef BUILD_PTHREAD
LIBS           += -lpthread 
endif


ifdef BUILD_WII
ifdef BUILD_ELM
LIBS           += -lelm -lwiiuse -lbte -logc
else
LIBS           += -lwiiuse -lbte -lfat -logc
endif
endif


ifdef BUILD_STATIC
LIBS           += -static
endif


ifdef BUILD_DC
LIBS           += -lc -lgcc -lSDL_129 -ltremor
endif


ifdef BUILD_VORBIS
LIBS           += -lvorbisfile -lvorbis -logg
endif


ifdef BUILD_TREMOR
LIBS           += -lvorbisidec
endif


LIBS           += -lpng -lz -lm

#----------------------------------------------------------------------------------------------------
# Rules to manage Files and Libraries for PSP
#----------------------------------------------------------------------------------------------------
       
ifdef BUILD_PSP
%.o : %.c
	@echo Compiling $(TARGET_PLATFORM) Port: $<...
	@$(CC) $(CFLAGS) -c $< -o $@
%.o : %.S
	@echo Compiling $(TARGET_PLATFORM) Port: $<...
	@$(CC) $(CFLAGS) -c $< -o $@
INCDIR          = $(INCS)
PSP_EBOOT_TITLE = $(VERSION_NAME) $(VERSION)
PSP_EBOOT_ICON 	= resources/OpenBOR_Icon_144x80.png
PSP_EBOOT_PIC1	= resources/OpenBOR_Logo_480x272.png
PSP_FW_VERSION  = 371
PSP_LARGE_MEMORY= 1
BUILD_PRX       = 1
include psp/build.mak
endif


#----------------------------------------------------------------------------------------------------
# Rules to manage Files and Libraries for Dreamcast
#----------------------------------------------------------------------------------------------------

ifdef BUILD_DC
all : $(TARGET) $(TARGET_FINAL)
KOS_LOCAL_CFLAGS = -I$(KOS_BASE)/../kos-ports/include -ffast-math
include $(KOS_BASE)/Makefile.rules
%.o : %.c
	@echo Compiling $(TARGET_PLATFORM) Port: $<...
	@$(KOS_CC) $(KOS_CFLAGS) $(CFLAGS) -c $< -o $@
%.o : %.s
	@echo Compiling $(TARGET_PLATFORM) Port: $<...
	@$(KOS_AS) $(KOS_AFLAGS) $< -o $@
%.o : %.S
	@echo Compiling $(TARGET_PLATFORM) Port: $<...
	@$(KOS_AS) $(KOS_AFLAGS) $< -o $@
$(TARGET) : $(OBJS)
	@echo
	@echo Linking $(TARGET_PLATFORM) Port: $(TARGET)...
	@$(KOS_CC) $(KOS_CFLAGS) $(KOS_LOCAL_CFLAGS) $(KOS_LDFLAGS) -o $@ $(KOS_START) $^ $(LIBS) $(KOS_LIBS)
$(TARGET_FINAL) : $(TARGET)
	@echo Creating $(TARGET_PLATFORM) Port: $(TARGET_FINAL)...
	@$(KOS_OBJCOPY) -R .stack -O binary $(TARGET) $(TARGET_FINAL)
	@echo
	@echo Completed $(TARGET_PLATFORM) Port!
	@echo $(TARGET_FINAL) is now ready!
endif


#----------------------------------------------------------------------------------------------------
# Rules to manage Files and Libraries for SDL
#----------------------------------------------------------------------------------------------------

ifdef BUILD_SDL
ifdef BUILD_WII
SOURCES = $(INCS)
include $(DEVKITPPC)/wii_rules
endif
all : $(TARGET) $(TARGET_FINAL)
%.o : %.asm
	@echo Compiling $(TARGET_PLATFORM) Port: $<...
	@$(YASM) -D $(TARGET_PLATFORM) -f $(OBJTYPE) -m $(TARGET_ARCH) -o $@ $<
%.o : %.c
	@echo Compiling $(TARGET_PLATFORM) Port: $<...
	@$(CC) $(CFLAGS) -c $< -o $@
$(TARGET) : $(OBJS) $(RES)
	@echo
	@echo Linking $(TARGET_PLATFORM) Port: $(TARGET)...
	@$(CC) $(CFLAGS) -o $(TARGET) $(OBJS) $(TARGET_RESOURCE) $(LIBS) 
$(TARGET_FINAL) : $(TARGET)
	@echo Stripping $(TARGET_PLATFORM) Port: $(TARGET_FINAL)...
	@$(STRIP)
	@echo
	@echo Completed $(TARGET_PLATFORM) Port!
	@echo $(TARGET_FINAL) is now ready!
endif

#----------------------------------------------------------------------------------------------------
# Rules to manage Files and Libraries for Wii
#----------------------------------------------------------------------------------------------------

ifndef BUILD_SDL
ifdef BUILD_WII
SOURCES = $(INCS)
include $(DEVKITPPC)/wii_rules
all : $(TARGET) $(TARGET_FINAL)
%.o : %.c
	@echo Compiling $(TARGET_PLATFORM) Port: $<...
	@$(CC) $(CFLAGS) -c $< -o $@
$(TARGET) : $(OBJS) $(RES)
	@echo
	@echo Linking $(TARGET_PLATFORM) Port: $(TARGET)...
	@$(CC) $(CFLAGS) -o $(TARGET) $(OBJS) $(TARGET_RESOURCE) $(LIBS) 
$(TARGET_FINAL) : $(TARGET)
	@echo Stripping $(TARGET_PLATFORM) Port: $(TARGET_FINAL)...
	@$(STRIP)
	@echo
	@echo Completed $(TARGET_PLATFORM) Port!
	@echo $(TARGET_FINAL) is now ready!
endif
endif


#----------------------------------------------------------------------------------------------------
# Rules to CleanUp Files for All Platforms
#----------------------------------------------------------------------------------------------------

ifndef BUILDING
all:
	@echo
	@echo Build A TARGET_PLATFORM:
	@echo
	@echo make BUILD_DC=1
	@echo make BUILD_PSP=1
	@echo make BUILD_PS2=1
	@echo make BUILD_WII=1
	@echo make BUILD_WIN=1
	@echo make BUILD_GP2X=1
	@echo make BUILD_WIZ=1
	@echo make BUILD_PANDORA=1
	@echo make BUILD_LINUX=1
	@echo make BUILD_DINGOO=1
	@echo
	@echo Cleanup Intermediate Files:
	@echo 
	@echo make clean
	@echo
	@echo Remove All Files:
	@echo 
	@echo make clean-all
	@echo
endif


ifndef BUILD_PSP
clean-all: clean-releases clean

clean-releases:
	@rm -rf releases/* 
	
clean:
	@echo
	@echo "Removing All $(TARGET_PLATFORM) Files..."
	@rm -f $(TARGET) $(TARGET_FINAL) $(TARGET_MAP) PARAM.SFO linkmap $(OBJS)
	@echo Done!
	@echo
endif

version:
	@echo "-------------------------------------------------------"
	@echo "OpenBOR $(VERSION) - http://www.LavaLit.com"
	@echo 
	@echo "Licensed under the BSD license."
	@echo "See LICENSE and README within OpenBOR root for details."
	@echo 
	@echo "Copyright (c) 2004 - 2011 OpenBOR Team"
	@echo "-------------------------------------------------------"

