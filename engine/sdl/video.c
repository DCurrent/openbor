/*
 * OpenBOR - http://www.LavaLit.com
 * -----------------------------------------------------------------------
 * All rights reserved, see LICENSE in OpenBOR root for details.
 *
 * Copyright (c) 2004 - 2011 OpenBOR Team
 */

#include <SDL_framerate.h>
#include <math.h>
#include "types.h"
#include "video.h"
#include "vga.h"
#include "screen.h"
#include "sdlport.h"
#include "opengl.h"
#include "openbor.h"
#include "gfxtypes.h"
#include "gfx.h"

extern int videoMode;

#if GP2X || DARWIN || OPENDINGUX || WII
#define SKIP_CODE
#endif

#ifndef SKIP_CODE
#include "pngdec.h"
#include "../resources/OpenBOR_Icon_32x32_png.h"
#endif

FPSmanager framerate_manager;
s_videomodes stored_videomodes;
static SDL_Surface *screen = NULL;
static SDL_Surface *bscreen = NULL;
static SDL_Surface *bscreen2 = NULL;
static SDL_Color colors[256];
static int bytes_per_pixel = 1;
int stretch = 0;
int opengl = 0; // OpenGL backend currently in use?
int nativeWidth, nativeHeight; // monitor resolution used in fullscreen mode
u8 pDeltaBuffer[480 * 2592];

void initSDL()
{
	const SDL_VideoInfo* video_info;
	int init_flags = SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER | SDL_INIT_JOYSTICK;

#ifdef CUSTOM_SIGNAL_HANDLER
	init_flags |= SDL_INIT_NOPARACHUTE;
#endif

	if(SDL_Init(init_flags) < 0)
	{
		printf("SDL Failed to Init!!!! (%s)\n", SDL_GetError());
		borExit(0);
	}
	SDL_ShowCursor(SDL_DISABLE);
	atexit(SDL_Quit);
#ifndef SKIP_CODE
	SDL_WM_SetCaption("OpenBOR", NULL);
	SDL_WM_SetIcon((SDL_Surface*)pngToSurface((void*)openbor_icon_32x32_png.data), NULL);
#endif
#if WIN || LINUX && !DARWIN && !defined(GLES)
	if(SDL_GL_LoadLibrary(NULL) < 0)
	{
		printf("Warning: couldn't load OpenGL library (%s)\n", SDL_GetError());
	}
#endif

	// Store the monitor's current resolution before setting the video mode for the first time
	video_info = SDL_GetVideoInfo();
#ifndef GP2X
	nativeWidth = video_info->current_w;
	nativeHeight = video_info->current_h;
#endif

	SDL_initFramerate(&framerate_manager);
	SDL_setFramerate(&framerate_manager, 200);
}

static unsigned masks[4][4] = {{0,0,0,0},{0x1F,0x07E0,0xF800,0},{0xFF,0xFF00,0xFF0000,0},{0xFF,0xFF00,0xFF0000,0}};

int video_set_mode(s_videomodes videomodes)
{
	stored_videomodes = videomodes;

	if(screen) SDL_FreeAndNullVideoSurface(screen);
	if(bscreen) { SDL_FreeSurface(bscreen); bscreen=NULL; }
	if(bscreen2) { SDL_FreeSurface(bscreen2); bscreen2=NULL; }

	// try OpenGL initialization first
	if((savedata.usegl[savedata.fullscreen]) && video_gl_set_mode(videomodes)) return 1;
	else opengl = 0;

	// FIXME: OpenGL surfaces aren't freed when switching from OpenGL to SDL

	bytes_per_pixel = videomodes.pixel;
	if(videomodes.hRes==0 && videomodes.vRes==0)
	{
		Term_Gfx();
		return 0;
	}

	if(savedata.screen[videoMode][0])
	{
#ifdef OPENDINGUX
		screen = SDL_SetVideoMode(videomodes.hRes*savedata.screen[videoMode][0],videomodes.vRes*savedata.screen[videoMode][0],16,savedata.fullscreen?(SDL_HWSURFACE|SDL_DOUBLEBUF|SDL_FULLSCREEN):(SDL_HWSURFACE|SDL_DOUBLEBUF));
#else
		screen = SDL_SetVideoMode(videomodes.hRes*savedata.screen[videoMode][0],videomodes.vRes*savedata.screen[videoMode][0],16,savedata.fullscreen?(SDL_SWSURFACE|SDL_DOUBLEBUF|SDL_FULLSCREEN):(SDL_SWSURFACE|SDL_DOUBLEBUF));
#endif
		SDL_ShowCursor(SDL_DISABLE);
		bscreen = SDL_AllocSurface(SDL_SWSURFACE, videomodes.hRes, videomodes.vRes, 8*bytes_per_pixel, masks[bytes_per_pixel-1][0], masks[bytes_per_pixel-1][1], masks[bytes_per_pixel-1][2], masks[bytes_per_pixel-1][3]); // 24bit mask
		bscreen2 = SDL_AllocSurface(SDL_SWSURFACE, videomodes.hRes+4, videomodes.vRes+8, 16, masks[1][2], masks[1][1], masks[1][0], masks[1][3]);
		Init_Gfx(565, 16);
		memset(pDeltaBuffer, 0x00, 1244160);
		if(bscreen==NULL || bscreen2==NULL) return 0;
	}
	else
	{
		if(bytes_per_pixel>1)
		{
			bscreen = SDL_AllocSurface(SDL_SWSURFACE, videomodes.hRes, videomodes.vRes, 8*bytes_per_pixel, masks[bytes_per_pixel-1][0], masks[bytes_per_pixel-1][1], masks[bytes_per_pixel-1][2], masks[bytes_per_pixel-1][3]); // 24bit mask
			if(!bscreen) return 0;
		}
#ifdef OPENDINGUX
		screen = SDL_SetVideoMode(videomodes.hRes,videomodes.vRes,8*bytes_per_pixel,savedata.fullscreen?(SDL_HWSURFACE|SDL_DOUBLEBUF|SDL_FULLSCREEN):(SDL_HWSURFACE|SDL_DOUBLEBUF));
#else
		screen = SDL_SetVideoMode(videomodes.hRes,videomodes.vRes,8*bytes_per_pixel,savedata.fullscreen?(SDL_SWSURFACE|SDL_DOUBLEBUF|SDL_FULLSCREEN):(SDL_SWSURFACE|SDL_DOUBLEBUF));
#endif
		SDL_ShowCursor(SDL_DISABLE);
	}

	if(bytes_per_pixel==1)
	{
		SDL_SetColors(screen,colors,0,256);
		if(bscreen) SDL_SetColors(bscreen,colors,0,256);
	}

	if(screen==NULL) return 0;

	video_clearscreen();
	return 1;
}

void video_fullscreen_flip()
{
	savedata.fullscreen ^= 1;

	video_set_mode(stored_videomodes);
}

//16bit, scale 2x 4x 8x ...
void _stretchblit(SDL_Surface *src, SDL_Rect *srcrect, SDL_Surface *dst, SDL_Rect *dstrect)
{
	SDL_Rect rect;
	int dst_x, dst_y, dst_w, dst_h, dst_row, src_row;
	int i;
	Uint16* psrc, *pdst;

	if(!srcrect)
	{
		rect.x = rect.y = 0;
		rect.w = src->w;
		rect.h = src->h;
		srcrect = &rect;
	}
	dst_w = savedata.screen[videoMode][0] * srcrect->w;
	dst_h = savedata.screen[videoMode][0] * srcrect->h;
	if(!dstrect)
	{
		dst_x = dst_y = 0;
		if(dst_w>dst->w) dst_w = dst->w;
		if(dst_h>dst->h) dst_h = dst->h;
	}
	else
	{
		dst_x = dstrect->x;
		dst_y = dstrect->y;
		if(dst_w>dstrect->w) dst_w = dstrect->w;
		if(dst_h>dstrect->h) dst_h = dstrect->h;
	}
	psrc = (Uint16*)src->pixels + srcrect->x + srcrect->y * src->pitch/2;
	pdst = (Uint16*)dst->pixels + dst_x + dst_y*dst->pitch/2;
	dst_row = dst->pitch/2;
	src_row = src->pitch/2;
	while(dst_h>0)
	{
		for(i=0; i<dst_w; i++)
		{
			*(pdst + i) = *(psrc+(i/savedata.screen[videoMode][0]));
		}

		for(i=1, pdst += dst_row; i<savedata.screen[videoMode][0] && dst_h; i++, dst_h--, pdst += dst_row)
		{
			memcpy(pdst, pdst-dst_row, dst_w<<1);
		}
		dst_h--;
		psrc += src_row;
	}
}

int video_copy_screen(s_screen* src)
{
	unsigned char *sp;
	char *dp;
	int width, height, linew, slinew;
	int h;
	SDL_Surface* ds = NULL;
	SDL_Rect rectdes, rectsrc;

	// use video_gl_copy_screen if in OpenGL mode
	if(opengl) return video_gl_copy_screen(src);

	width = screen->w;
	if(width > src->width) width = src->width;
	height = screen->h;
	if(height > src->height) height = src->height;
	if(!width || !height) return 0;
	h = height;

	if(bscreen)
	{
		rectdes.x = rectdes.y = 0;
		rectdes.w = width*savedata.screen[videoMode][0]; rectdes.h = height*savedata.screen[videoMode][0];
		if(bscreen2) {rectsrc.x = 2; rectsrc.y = 4;}
		else         {rectsrc.x = 0; rectsrc.y = 0;}
		rectsrc.w = width; rectsrc.h = height;
		if(SDL_MUSTLOCK(bscreen)) SDL_LockSurface(bscreen);
	}

	// Copy to linear video ram
	if(SDL_MUSTLOCK(screen)) SDL_LockSurface(screen);

	sp = (unsigned char*)src->data;
	ds = (bscreen?bscreen:screen);
	dp = ds->pixels;

	linew = width*bytes_per_pixel;
	slinew = src->width*bytes_per_pixel;

	do{
		memcpy(dp, sp, linew);
		sp += slinew;
		dp += ds->pitch;
	}while(--h);

	if(SDL_MUSTLOCK(screen)) SDL_UnlockSurface(screen);

	if(bscreen)
	{
		if(SDL_MUSTLOCK(bscreen)) SDL_UnlockSurface(bscreen);
		if(bscreen2)
		{
			SDL_BlitSurface(bscreen, NULL, bscreen2, &rectsrc);
			if(SDL_MUSTLOCK(bscreen2)) SDL_LockSurface(bscreen2);
			if(SDL_MUSTLOCK(screen)) SDL_LockSurface(screen);

			if(savedata.screen[videoMode][0]==2) (*GfxBlitters[(int)savedata.screen[videoMode][1]])((u8*)bscreen2->pixels+bscreen2->pitch*4+4, bscreen2->pitch, pDeltaBuffer+bscreen2->pitch, (u8*)screen->pixels, screen->pitch, screen->w>>1, screen->h>>1);
			else _stretchblit(bscreen2, &rectsrc, screen, &rectdes);

			if(SDL_MUSTLOCK(bscreen2)) SDL_UnlockSurface(bscreen2);
			if(SDL_MUSTLOCK(screen)) SDL_UnlockSurface(screen);
		}
		else
			SDL_BlitSurface(bscreen, NULL, screen, &rectsrc);
	}

	SDL_Flip(screen);


#if WIN || LINUX
	SDL_framerateDelay(&framerate_manager);
#endif

	return 1;
}

void video_clearscreen()
{
	if(opengl) { video_gl_clearscreen(); return; }

	if(SDL_MUSTLOCK(screen)) SDL_LockSurface(screen);
	memset(screen->pixels, 0, screen->pitch*screen->h);
	if(SDL_MUSTLOCK(screen)) SDL_UnlockSurface(screen);
	if(bscreen)
	{
		if(SDL_MUSTLOCK(bscreen)) SDL_LockSurface(bscreen);
		memset(bscreen->pixels, 0, bscreen->pitch*bscreen->h);
		if(SDL_MUSTLOCK(bscreen)) SDL_UnlockSurface(bscreen);
	}
}

void video_stretch(int enable)
{
	if(screen || opengl) video_clearscreen();
	stretch = enable;
}

void vga_vwait(void)
{
#ifdef GP2X
	gp2x_video_wait_vsync();
#else
	static int prevtick = 0;
	int now = SDL_GetTicks();
	int wait = 1000/60 - (now - prevtick);
	if (wait>0)
	{
		SDL_Delay(wait);
	}
	else SDL_Delay(1);
	prevtick = now;
#endif
}

void vga_setpalette(unsigned char* palette)
{
	int i;
	video_gl_setpalette(palette);
	for(i=0;i<256;i++){
		colors[i].r=palette[0];
		colors[i].g=palette[1];
		colors[i].b=palette[2];
		palette+=3;
	}
	if(!opengl)
	{
		SDL_SetColors(screen,colors,0,256);
		if(bscreen) SDL_SetColors(bscreen,colors,0,256);
	}
}

// TODO: give this function a boolean (int) return type denoting success/failure
void vga_set_color_correction(int gm, int br)
{
	if(opengl) video_gl_set_color_correction(gm, br);
}

