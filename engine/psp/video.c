/*
 * OpenBOR - http://www.LavaLit.com
 * -----------------------------------------------------------------------
 * Licensed under the BSD license, see LICENSE in OpenBOR root for details.
 *
 * Copyright (c) 2004 - 2011 OpenBOR Team
 */

#include <pspdisplay.h>
#include <pspkernel.h>
#include <pspsdk.h>
#include <psprtc.h>
#include <string.h>
#include "vga.h"
#include "types.h"
#include "video.h"
#include "graphics.h"
#include "tracemalloc.h"

static int screen_w, screen_h;
int scaleVideo;

int video_set_mode(s_videomodes videomodes)
{
	if(videomodes.hRes==0 && videomodes.vRes==0) return 0;
	screen_w = videomodes.hRes;
	screen_h = videomodes.vRes;
	setGraphicsScreen(displayFormat[(int)videomodes.mode], videomodes.pixel, videomodes.filter);
	video_clearscreen();
	return 1;
}

int video_copy_screen(s_screen* src)
{
	blitScreenToScreen(screen_w, screen_h, src);
	flipScreen();
	return 1;
}

void vga_vwait(void)
{
	sceDisplayWaitVblankStart();
}


// Set VGA-type palette
void vga_setpalette(unsigned char* pal)
{
	int i;
	for(i=0;i<256;i++)
	{
		palette[i] = ((pal[0]) | ((pal[1]) << 8) | ((pal[2]) << 16));
		pal+=3;
	}
}

void video_clearscreen()
{
	clearScreen(0x00000000);
	flipScreen();
}

