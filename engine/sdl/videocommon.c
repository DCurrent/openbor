/*
 * OpenBOR - http://www.chronocrash.com
 * -----------------------------------------------------------------------
 * All rights reserved, see LICENSE in OpenBOR root for details.
 *
 * Copyright (c) 2004 - 2015 OpenBOR Team
 */

#include "sdlport.h"
#include "globals.h"
#include "video.h"
#include "openbor.h"
#include "videocommon.h"

static SDL_Surface *screen = NULL;
static SDL_Surface *bscreen = NULL;
static SDL_Surface *bscreen2 = NULL;
u8 pDeltaBuffer[480 * 2592];

static s_videosurface videoSurface;
static unsigned masks[4][4] = {{0,0,0,0},{0x1F,0x07E0,0xF800,0},{0xFF,0xFF00,0xFF0000,0},{0xFF,0xFF00,0xFF0000,0}};
static int bytes_per_pixel;

extern int videoMode;
extern SDL_Color colors[256];
extern SDL_Palette *screenPalette;

s_videomodes setupPreBlitProcessing(s_videomodes videomodes)
{
	bytes_per_pixel = videomodes.pixel;
	
	if(screen) { SDL_FreeSurface(screen); screen=NULL; }
	if(bscreen) { SDL_FreeSurface(bscreen); bscreen=NULL; }
	if(bscreen2) { SDL_FreeSurface(bscreen2); bscreen2=NULL; }
	
	// set scale factors to 1 by default
	videomodes.hScale = 1;
	videomodes.vScale = 1;
	
	// set up indexed to RGB conversion
	if(videomodes.pixel == 1)
	{
		bscreen = SDL_CreateRGBSurface(0, videomodes.hRes, videomodes.vRes, 8, 0,0,0,0);
		screen = SDL_CreateRGBSurface(0, videomodes.hRes, videomodes.vRes, 32, masks[3][0], masks[3][1], masks[3][2], masks[3][3]);
		SDL_SetSurfacePalette(bscreen, screenPalette);
		SDL_SetPaletteColors(screenPalette, colors, 0, 256);
		videomodes.pixel = 4;
	}
	
	if(savedata.screen[videoMode][0] == 2)
	{
		if (screen) SDL_FreeSurface(screen);
		screen = SDL_CreateRGBSurface(0, videomodes.hRes*2, videomodes.vRes*2, 16, masks[1][0], masks[1][1], masks[1][2], masks[1][3]);
		if (!bscreen) bscreen = SDL_CreateRGBSurface(0, videomodes.hRes, videomodes.vRes, 8*bytes_per_pixel, masks[bytes_per_pixel-1][0], masks[bytes_per_pixel-1][1], masks[bytes_per_pixel-1][2], masks[bytes_per_pixel-1][3]); // 24bit mask
		bscreen2 = SDL_CreateRGBSurface(0, videomodes.hRes+4, videomodes.vRes+8, 16, masks[1][0], masks[1][1], masks[1][2], masks[1][3]);
		Init_Gfx(565, 16);
		memset(pDeltaBuffer, 0x00, 1244160);
		
		assert(bscreen);
		assert(bscreen2);

		videomodes.hRes *= 2;
		videomodes.vRes *= 2;
		videomodes.pixel = 2;
	}
	else if(savedata.screen[videoMode][0])
	{
		videomodes.hScale = savedata.screen[videoMode][0];
		videomodes.vScale = savedata.screen[videoMode][0];
	}
	
	return videomodes;
}

s_videosurface *getVideoSurface(s_screen *src)
{
	unsigned char *sp;
	char *dp;
	int width, height, linew, slinew;
	int h;
	SDL_Surface* ds = NULL;
	SDL_Rect rectdes, rectsrc;

	if(bscreen)
	{
		width = screen->w;
		if(width > src->width) width = src->width;
		height = screen->h;
		if(height > src->height) height = src->height;
		if(!width || !height) return NULL;
		h = height;

		rectdes.x = rectdes.y = 0;
		rectdes.w = width*savedata.screen[videoMode][0]; rectdes.h = height*savedata.screen[videoMode][0];
		if(bscreen2) {rectsrc.x = 2; rectsrc.y = 4;}
		else         {rectsrc.x = 0; rectsrc.y = 0;}
		rectsrc.w = width; rectsrc.h = height;

		sp = (unsigned char*)src->data;
		ds = bscreen;
		dp = ds->pixels;

		linew = width*bytes_per_pixel;
		slinew = src->width*bytes_per_pixel;

		do{
			memcpy(dp, sp, linew);
			sp += slinew;
			dp += ds->pitch;
		}while(--h);

		if (bscreen2)
		{
			assert(savedata.screen[videoMode][0] == 2);
			SDL_BlitSurface(bscreen, NULL, bscreen2, &rectsrc);

			(*GfxBlitters[(int)savedata.screen[videoMode][1]])((u8*)bscreen2->pixels+bscreen2->pitch*4+4, bscreen2->pitch, pDeltaBuffer+bscreen2->pitch, (u8*)screen->pixels, screen->pitch, screen->w>>1, screen->h>>1);
		}
		else SDL_BlitSurface(bscreen, NULL, screen, NULL);
		
		videoSurface.width = screen->w;
		videoSurface.height = screen->h;
		videoSurface.pitch = screen->pitch;
		videoSurface.data = screen->pixels;
	}
	else
	{
		videoSurface.width = src->width;
		videoSurface.height = src->height;
		videoSurface.pitch = src->width * pixelbytes[(int)src->pixelformat];
		videoSurface.data = src->data;
	}
	return &videoSurface;
}


