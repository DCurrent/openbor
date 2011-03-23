/*
 * OpenBOR - http://www.LavaLit.com
 * -----------------------------------------------------------------------
 * Licensed under the BSD license, see LICENSE in OpenBOR root for details.
 *
 * Copyright (c) 2004 - 2011 OpenBOR Team
 */

#include "xboxport.h"
#include "video.h"
#include "vga.h"

static int screen_w, screen_h;
int scaleVideo;

int video_set_mode(s_videomodes videomodes)
{
	if(videomodes.hRes==0 && videomodes.vRes==0) return 0;
	screen_w = videomodes.hRes;
	screen_h = videomodes.vRes;
	return 1;
}

int video_copy_screen(s_screen * src)
{
	xbox_put_image(screen_w, screen_h, src);
	return 1 ;
}

void video_clearscreen()
{
	xbox_clear_screen();
}

void vga_vwait(void)
{
}

void vga_setpalette(unsigned char *pal)
{
	xbox_set_palette( pal ) ;
}
