/////////////////////////////////////////////////////////////////////////////
//
// gsvga - Emulate a 320x240x8bpp color-indexed framebuffer
//
/////////////////////////////////////////////////////////////////////////////

#include <stdio.h>
#include <gsKit.h>
#include <dmaKit.h>

#include "ps2port.h"
#include "ps2sdr.h"
#include "filecache.h"

GSGLOBAL gsGlobal __attribute__((aligned(64)));;
GSTEXTURE Tex __attribute__((aligned(64)));;
u64 Black __attribute__((aligned(64)));;
struct { u8 red, green, blue, alpha; } clut[256] __attribute__((aligned(64)));;
u8 screen[320 * 240] __attribute__((aligned(64)));;

void gsvga_init(void) {
    dmaKit_init(D_CTRL_RELE_ON,D_CTRL_MFD_OFF, D_CTRL_STS_UNSPEC, D_CTRL_STD_OFF, D_CTRL_RCYC_8, 1 << DMA_CHANNEL_GIF);
    dmaKit_chan_init(DMA_CHANNEL_GIF);

    Black = GS_SETREG_RGBAQ(0x00,0x00,0x00,0x00,0x00);

    /* Generic Values */
    gsGlobal.Mode = GS_MODE_NTSC;
    gsGlobal.Interlace = GS_INTERLACED;
    gsGlobal.Field = GS_FIELD;
    gsGlobal.Aspect = GS_ASPECT_4_3;
    gsGlobal.Width = 640;
    gsGlobal.Height = 448;
    gsGlobal.OffsetX = 2048;
    gsGlobal.OffsetY = 2048;
    gsGlobal.StartX = 0;
    gsGlobal.StartY = 0;
    gsGlobal.PSM = GS_PSM_CT32;
    gsGlobal.PSMZ = GS_PSMZ_16;
    gsGlobal.ActiveBuffer = 1;
    gsGlobal.PrimFogEnable = 0;
    gsGlobal.PrimAAEnable = 1;
    gsGlobal.PrimAlphaEnable = 1;
    gsGlobal.PrimAlpha = 1;
    gsGlobal.PrimContext = 0;

    /* BGColor Register Values */
    gsGlobal.BGColor->Red = 0x00;
    gsGlobal.BGColor->Green = 0x00;
    gsGlobal.BGColor->Blue = 0x00;

    /* TEST Register Values */
    gsGlobal.Test->ATE = 0;
    gsGlobal.Test->ATST = 1;
    gsGlobal.Test->AREF = 0x80;
    gsGlobal.Test->AFAIL = 0;
    gsGlobal.Test->DATE = 0;
    gsGlobal.Test->DATM = 0;
    gsGlobal.Test->ZTE = 1;
    gsGlobal.Test->ZTST = 2;

    gsGlobal.Clamp->WMS = GS_CMODE_REPEAT;
    gsGlobal.Clamp->WMT = GS_CMODE_REPEAT;

    gsKit_init_screen(&gsGlobal);
    gsKit_clear(&gsGlobal, Black);

    Tex.Width = 320;
    Tex.Height = 240;
    Tex.PSM = GS_PSM_T8;
    Tex.Mem = (void*)screen;
    Tex.Clut = (void*)clut;
    Tex.Vram = 0x3000 * 256;
    Tex.VramClut = 0x3a00 * 256;
    
    memset(clut, 0, 256 * 4);
    memset(screen, 0, 320 * 240);
}

void gsvga_tweak(int offset_X, int offset_Y, int interlace, int videomode) {
    gsGlobal.StartX = offset_X;
    gsGlobal.StartY = offset_Y;
    gsGlobal.Interlace = interlace ? GS_INTERLACED : GS_NONINTERLACED;
    gsGlobal.Mode = videomode ? GS_FIELD : GS_FRAME;
    
    gsKit_init_screen(&gsGlobal);
}

void gsvga_setpalette(unsigned char *pal) {
    int i, j, k;
#define COPYCOLOR {clut[k].red=pal[0];clut[k].green=pal[1];clut[k].blue=pal[2];clut[k].alpha=0x80;}
    k = 0;
    for(i = 0; i < 256; i += 32) {
	for(j = 0; j < 8; j++, pal += 3, k += 1) { COPYCOLOR; }
	for(j = 0; j < 8; j++, pal += 3, k += 1) { k += 8; COPYCOLOR; k -= 8; }
	for(j = 0; j < 8; j++, pal += 3, k += 1) { k -= 8; COPYCOLOR; k += 8; }
	for(j = 0; j < 8; j++, pal += 3, k += 1) { COPYCOLOR; }
    }
}

void gsvga_draw(unsigned char *frame) {
    memcpy(screen, frame, 320 * 240);
    gsKit_texture_upload(&gsGlobal, &Tex);
    gsKit_clear(&gsGlobal, Black);
    gsKit_prim_sprite_texture(&gsGlobal, &Tex, 0, 0, 0, 0, 640, 448, 320, 240, 0, 0x80808080);
    filecache_process();
    gsKit_sync_flip(&gsGlobal);
}

void gsvga_shutdown(const char *msg) {
    debug_printf("TODO: gsvga_shutdown()\n");
}

void gsvga_clear() {
    memset(screen, 0, 320 * 240);
    gsvga_draw(screen);
    gsvga_draw(screen);
}
