/*
 * OpenBOR - http://www.LavaLit.com
 * -----------------------------------------------------------------------
 * Licensed under the BSD license, see LICENSE in OpenBOR root for details.
 *
 * Copyright (c) 2004 - 2011 OpenBOR Team
 */

#include <SDL.h>
#include "types.h"
#include "video.h"
#include "vga.h"
#include "screen.h"
#include "openbor.h"
#include "filecache.h"

static SDL_Surface *screen = NULL;
static SDL_Surface *surface = NULL;
static SDL_Color colors[256];
static int width=0, height=0, bpp=0, mode=0;

static unsigned masks[4][4] = {{0x00000000,0x00000000,0x00000000,0x00000000},
							   {0x0000001F,0x000007E0,0x0000F800,0x00000000},
							   {0x000000FF,0x0000FF00,0x00FF0000,0x00000000},
							   {0x000000FF,0x0000FF00,0x00FF0000,0x00000000}};

int video_set_mode(s_videomodes videomodes)
{
	bpp = videomodes.pixel;
	width = videomodes.hRes;
	height = videomodes.vRes;
	if(videomodes.hRes==480) mode = 1;
	if(screen) SDL_FreeSurface(screen);
	screen = SDL_SetVideoMode(mode?640:width,mode?480:height,8*bpp,SDL_HWSURFACE|SDL_doubleBUF|SDL_FULLSCREEN);
	if(screen==NULL) return 0;
	if(bpp>1)
	{
		if(surface) SDL_FreeSurface(surface);
		surface = SDL_AllocSurface(SDL_SWSURFACE, videomodes.hRes, videomodes.vRes, 8*bpp, masks[bpp-1][0], masks[bpp-1][1], masks[bpp-1][2], masks[bpp-1][3]);
		if(surface==NULL) return 0;
	}
	video_clearscreen();
	SDL_ShowCursor(SDL_DISABLE);
	return 1;
}

int video_copy_screen(s_screen* src)
{
	unsigned char *sp;
	char *dp;
	int i, linew, slinew;
	int w=width;
	int h=height;
	SDL_Surface* ds = bpp>1?surface:screen;

	filecache_process();

	// Copy to linear video ram
	sp = (unsigned char*)src->data;
	dp = ds->pixels;

	linew = w*bpp;
	slinew = src->width*bpp;

	for(i=0; i<h; i++) {
		memcpy(dp, sp, linew);
		sp += slinew;
		dp += ds->pitch;
	}

	SDL_Rect rect;
	rect.x = mode?80:0; rect.y = mode?120:0;
	rect.w = src->width; rect.h = src->height;
	if(bpp>1) SDL_BlitSurface(surface, NULL, screen, &rect);
	SDL_Flip(screen);
	return 1;
}

void video_clearscreen()
{
	memset(screen->pixels, 0, screen->pitch*screen->h);
}

void vga_setpalette(char* palette)
{
	int i;
	if(bpp>1) return;
	for(i=0;i<256;i++){
		colors[i].r=palette[0];
		colors[i].g=palette[1];
		colors[i].b=palette[2];
		palette+=3;
	}
	SDL_SetColors(screen,colors,0,256);
}

void video_fullscreen_flip()
{
}

void vga_vwait(void)
{
}

