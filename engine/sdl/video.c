/*
 * OpenBOR - http://www.chronocrash.com
 * -----------------------------------------------------------------------
 * All rights reserved, see LICENSE in OpenBOR root for details.
 *
 * Copyright (c) 2004 - 2014 OpenBOR Team
 */
#if ANDROID

// CRxTRDude - changed the directory for neatness.
#include "android/jni/openbor/video.c"

#else

#include "sdlport.h"
#include "SDL2_framerate.h"

#include <math.h>
#include "types.h"
#include "video.h"
#include "vga.h"
#include "screen.h"
#include "opengl.h"
#include "openbor.h"
#include "gfxtypes.h"
#include "gfx.h"
#include "videocommon.h"

extern int videoMode;

#if GP2X || DARWIN || OPENDINGUX || WII
#define SKIP_CODE
#endif

#ifndef SKIP_CODE
#include "pngdec.h"
#include "../resources/OpenBOR_Icon_32x32_png.h"
#endif

SDL_Window *window = NULL;
SDL_Renderer *renderer = NULL;
SDL_Texture *texture = NULL;
SDL_Palette *screenPalette = NULL;

FPSmanager framerate_manager;

s_videomodes stored_videomodes;
SDL_Color colors[256];
int stretch = 0;
int opengl = 0; // OpenGL backend currently in use?
int nativeWidth, nativeHeight; // monitor resolution used in fullscreen mode
int brightness = 0;

void initSDL()
{
	SDL_DisplayMode video_info;
	int init_flags = SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER | SDL_INIT_JOYSTICK;

	if(SDL_Init(init_flags) < 0)
	{
		printf("SDL Failed to Init!!!! (%s)\n", SDL_GetError());
		borExit(0);
	}
	SDL_ShowCursor(SDL_DISABLE);
	atexit(SDL_Quit);

#if WIN || LINUX && !DARWIN
	if(SDL_GL_LoadLibrary(NULL) < 0)
	{
		printf("Warning: couldn't load OpenGL library (%s)\n", SDL_GetError());
	}
#endif

	screenPalette = SDL_AllocPalette(256);

	SDL_GetCurrentDisplayMode(0, &video_info);
	nativeWidth = video_info.w;
	nativeHeight = video_info.h;
	printf("debug:nativeWidth, nativeHeight, bpp, Hz  %d, %d, %d, %d\n", nativeWidth, nativeHeight, SDL_BITSPERPIXEL(video_info.format), video_info.refresh_rate);

	SDL_initFramerate(&framerate_manager);
	SDL_setFramerate(&framerate_manager, 200);
}

void video_set_window_title(const char* title)
{
	if(window) SDL_SetWindowTitle(window, title);
}

static unsigned pixelformats[4] = {SDL_PIXELFORMAT_INDEX8, SDL_PIXELFORMAT_BGR565, SDL_PIXELFORMAT_BGR888, SDL_PIXELFORMAT_ABGR8888};

int SetVideoMode(int w, int h, int bpp, bool gl)
{
	int x = SDL_WINDOWPOS_UNDEFINED;
	int y = SDL_WINDOWPOS_UNDEFINED;
	int flags = SDL_WINDOW_SHOWN | SDL_WINDOW_INPUT_FOCUS;
	static bool last_gl = false;
	if(gl) flags |= SDL_WINDOW_OPENGL;
	if(savedata.fullscreen) flags |= SDL_WINDOW_FULLSCREEN_DESKTOP;

	if(window && gl != last_gl)
	{
		if (!(SDL_GetWindowFlags(window) & SDL_WINDOW_FULLSCREEN_DESKTOP))
			SDL_GetWindowPosition(window, &x, &y);
		SDL_DestroyWindow(window);
		window = NULL;
	}
	last_gl = gl;

	if(window)
	{
		if(savedata.fullscreen)
		{
			SDL_SetWindowFullscreen(window, SDL_WINDOW_FULLSCREEN_DESKTOP);
		}
		else
		{
			if(SDL_GetWindowFlags(window) & SDL_WINDOW_FULLSCREEN_DESKTOP)
				SDL_HideWindow(window);
			SDL_SetWindowFullscreen(window, 0);
			SDL_SetWindowSize(window, w, h);
			SDL_ShowWindow(window);
		}
	}
	else
	{
		window = SDL_CreateWindow("OpenBOR", x, y, w, h, flags); // FIXME: use previous window title
		if(!window) return 0;
		renderer = SDL_CreateRenderer(window, -1, 0);
		if(!renderer) return 0;
		SDL_Surface* icon = (SDL_Surface*)pngToSurface((void*)openbor_icon_32x32_png.data);
		SDL_SetWindowIcon(window, icon);
		SDL_FreeSurface(icon);
		SDL_GetWindowSize(window, &w, &h);
	}
	
	if(!gl && !renderer)
	{
		renderer = SDL_CreateRenderer(window, -1, 0);
	}
	else if(gl)
	{
		if(renderer) SDL_DestroyRenderer(renderer);
		if(texture)  SDL_DestroyTexture(texture);
		renderer = NULL;
		texture = NULL;
	}

	return 1;
}

int video_set_mode(s_videomodes videomodes)
{
	stored_videomodes = videomodes;

	// try OpenGL initialization first
	if(savedata.usegl[savedata.fullscreen] && video_gl_set_mode(videomodes)) return 1;
	else opengl = 0;

	// FIXME: OpenGL surfaces aren't freed when switching from OpenGL to SDL

	if(videomodes.hRes==0 && videomodes.vRes==0)
	{
		Term_Gfx();
		return 0;
	}

	videomodes = setupPreBlitProcessing(videomodes);

	if(!SetVideoMode(videomodes.hRes * videomodes.hScale,
	                 videomodes.vRes * videomodes.vScale,
	                 videomodes.pixel * 8, false))
	{
		return 0;
	}

	SDL_RenderSetLogicalSize(renderer, videomodes.hRes, videomodes.vRes);
	texture = SDL_CreateTexture(renderer,
	                            pixelformats[videomodes.pixel-1],
	                            SDL_TEXTUREACCESS_STREAMING,
	                            videomodes.hRes, videomodes.vRes);

	SDL_ShowCursor(SDL_DISABLE);
	SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);

	return 1;
}

void video_fullscreen_flip()
{
	savedata.fullscreen ^= 1;
	if(window) video_set_mode(stored_videomodes);
}

int video_copy_screen(s_screen* src)
{
	// use video_gl_copy_screen if in OpenGL mode
	if(opengl) return video_gl_copy_screen(src);

	s_videosurface *surface = getVideoSurface(src);

	SDL_UpdateTexture(texture, NULL, surface->data, surface->pitch);

	SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0);
	SDL_RenderClear(renderer);
	SDL_RenderCopy(renderer, texture, NULL, NULL);

	if (brightness > 0)
		SDL_SetRenderDrawColor(renderer, 255, 255, 255, brightness-1);
	else if (brightness < 0)
		SDL_SetRenderDrawColor(renderer, 0, 0, 0, (-brightness)-1);
	SDL_RenderFillRect(renderer, NULL);

	SDL_RenderPresent(renderer);

#if WIN || LINUX
	SDL_framerateDelay(&framerate_manager);
#endif

	return 1;
}

void video_clearscreen()
{
	if(opengl) { video_gl_clearscreen(); return; }
	
	SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0);
	SDL_RenderClear(renderer);
	SDL_RenderPresent(renderer);
}

void video_stretch(int enable)
{
	stretch = enable;
	if(window && !opengl)
	{
		if(stretch)
			SDL_RenderSetLogicalSize(renderer, 0, 0);
		else
			SDL_RenderSetLogicalSize(renderer, stored_videomodes.hRes, stored_videomodes.vRes);
	}
}

void video_set_color_correction(int gm, int br)
{
	brightness = br;
	if(opengl) video_gl_set_color_correction(gm, br);
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
		SDL_SetPaletteColors(screenPalette, colors, 0, 256);
	}
}

#endif
