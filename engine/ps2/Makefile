PS2VERSION = 0.1b

include $(PS2SDK)/Defs.make

EE_CPPFLAGS = -G0 -Wall -Werror -fomit-frame-pointer -fno-common -funsigned-char \
	          -I$(PS2SDK)/ee/include -I$(PS2SDK)/common/include -I$(PS2DEV)/gsKit/include \
	          -I$(PS2DEV)/isjpcm/include -D_EE -DPS2VERSION=\"$(PS2VERSION)\"
	      
EE_LDFLAGS  = -s -nostartfiles -T $(PS2SDK)/ee/startup/linkfile \
	          -L $(PS2SDK)/ee/lib -L $(PS2DEV)/gsKit/lib -L. \
	          -L $(PS2DEV)/isjpcm/lib -lgskit -lsjpcm -ldmakit -lcdvd -lpad \
	          -lmc -lm -lc -lkernel -lc -lsyscall $(PS2SDK)/ee/startup/crt0.o

CC = $(EE_CC)
AS = $(EE_CC) -c
LD = $(EE_LD)

CPPFLAGS = $(EE_CPPFLAGS)
LDFLAGS = $(EE_LDFLAGS)

BIN2S = $(PS2SDK)/bin/bin2s

TARGET      = bor
OBJS        = \
	      tracemalloc.o \
	      ps2port.o borstartup.o consfont.o ps2pak.o filecache.o \
	      video.o vga.o screen.o control.o soundmix.o timer.o adpcm.o \
	      rand32.o \
	      draw.o sprite.o spriteq.o font.o bitmap.o loadimg.o palette.o texture.o anigif.o \
	      bor.o \
\
	      gsvga.o ps2sdr.o \
\
	      sjpcm_irx_embed.o icon_icn.o

all: $(TARGET).elf

release: $(TARGET).elf
	zip -9 ps2bor-src-$(PS2VERSION).zip *.c *.h *.s *.icn Makefile README.txt
	zip -9 ps2bor-$(PS2VERSION).zip $(TARGET).elf README.txt

$(TARGET).elf: $(OBJS) $(PS2DEV)/isjpcm/bin/isjpcm.irx
	$(CC) -o $@ $(OBJS) $(LDFLAGS)

clean:
	rm -f *.o $(TARGET).elf

rebuild: clean all

sjpcm_irx_embed.o: sjpcm_irx_embed
icon_icn.o: icon_icn

sjpcm_irx_embed: $(PS2DEV)/isjpcm/bin/isjpcm.irx
	$(BIN2S) $< $@.s $@
	$(EE_CC) -c $@.s
	rm $@.s

icon_icn: icon.icn
	$(BIN2S) $< $@.s $@
	$(EE_CC) -c $@.s
	rm $@.s
