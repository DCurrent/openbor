/*
 * OpenBOR - http://www.chronocrash.com
 * -----------------------------------------------------------------------
 * All rights reserved, see LICENSE in OpenBOR root for details.
 *
 * Copyright (c)  OpenBOR Team
 */

#include "sdlport.h"
#include "globals.h"
#include "video.h"
#include "savedata.h"
#include "gfx.h"
#include "videocommon.h"
#include "gpu.h"

static SDL_Surface *screen = NULL;
static SDL_Surface *bscreen = NULL;
static SDL_Surface *bscreen2 = NULL;
u8 pDeltaBuffer[480 * 2592];

static s_videosurface videoSurface;

/*
 * Damon V. Caskey
 * 2026-11-15
 * Prepare optional output filtering for the sole RGB32 scene format. Existing
 * CPU filters operate on private BGR565 SDL surfaces; these intermediates do
 * not create or expose another engine scene format.
 */
s_videomodes setupPreBlitProcessing(s_videomodes videomodes)
{
	assert(videomodes.pixel == 4);
	
	if(screen) { SDL_FreeSurface(screen); screen=NULL; }
	if(bscreen) { SDL_FreeSurface(bscreen); bscreen=NULL; }
	if(bscreen2) { SDL_FreeSurface(bscreen2); bscreen2=NULL; }
	
	// set scale factors to 1 by default
	videomodes.hScale = savedata.hwscale;
	videomodes.vScale = savedata.hwscale;
	
	/* Keep the filter's input, padded working image and output separate from
	 * the RGB32 scene. Ordinary native presentation allocates none of them. */
	if(savedata.swfilter && (savedata.hwscale >= 2.0 || savedata.fullscreen))
	{
		screen = SDL_CreateRGBSurface(0, videomodes.hRes*2, videomodes.vRes*2, 16, 0x1F, 0x07E0, 0xF800, 0);
		bscreen = SDL_CreateRGBSurface(0, videomodes.hRes, videomodes.vRes, 32, 0xFF, 0xFF00, 0xFF0000, 0);
		bscreen2 = SDL_CreateRGBSurface(0, videomodes.hRes+4, videomodes.vRes+8, 16, 0x1F, 0x07E0, 0xF800, 0);
		if(!screen || !bscreen || !bscreen2)
		{
			gpu_log("ERROR: CPU output filter surfaces could not initialize: %s.", SDL_GetError());
			if(screen) { SDL_FreeSurface(screen); screen=NULL; }
			if(bscreen) { SDL_FreeSurface(bscreen); bscreen=NULL; }
			if(bscreen2) { SDL_FreeSurface(bscreen2); bscreen2=NULL; }
			videomodes.pixel = 0;
			return videomodes;
		}
		memset(pDeltaBuffer, 0, sizeof(pDeltaBuffer));

		videomodes.hRes *= 2;
		videomodes.vRes *= 2;
		videomodes.hScale /= 2;
		videomodes.vScale /= 2;
		videomodes.pixel = 2;
	}
	
	return videomodes;
}

/*
 * Damon V. Caskey
 * 2026-11-12
 * 2026-11-15 - Accept RGB32 scenes only and isolate private CPU filter images.
 * Expose a CPU-readable presentation image. Resolve newer GPU screen contents
 * before accessing pixels, then perform configured software output scaling
 * while respecting SDL surface pitch.
 */
s_videosurface *getVideoSurface(s_screen *src)
{
	unsigned char *sp;
	char *dp;
	int width, height, linew, slinew;
	int h;
	SDL_Surface* ds = NULL;
	SDL_Rect rectsrc;

	/* Indexed artwork is expanded while drawing, never at presentation. */
	if(!src || src->pixelformat != PIXEL_32)
	{
		return NULL;
	}

	/*
	 * Conversion, output filters and texture uploads all read CPU storage.
	 * Screens produced by native drawing must first update that shadow;
	 * screens whose CPU copy is already current need no readback.
	 */
	gpu_sync_cpu(src);

	if(bscreen)
	{
		/* Limit the row copy to the overlapping source and conversion surfaces. */
		width = bscreen->w;
		if(width > src->width)
		{
			width = src->width;
		}
		height = bscreen->h;
		if(height > src->height)
		{
			height = src->height;
		}
		if(!width || !height)
		{
			return NULL;
		}
		h = height;

		/* Software filters require a padded intermediate image around the source. */
		rectsrc.x = 2;
		rectsrc.y = 4;
		rectsrc.w = width;
		rectsrc.h = height;

		sp = (unsigned char*)src->data;
		ds = bscreen;
		dp = ds->pixels;

		linew = width*4;
		slinew = src->width*4;

		/* Engine rows are tightly packed; SDL rows advance by their surface pitch. */
		do
		{
			memcpy(dp, sp, linew);
			sp += slinew;
			dp += ds->pitch;
		}
		while(--h);

		/* Convert into the padded format expected by the selected CPU filter. */
		SDL_BlitSurface(bscreen, NULL, bscreen2, &rectsrc);

		(*GfxBlitters[savedata.swfilter])((u8*)bscreen2->pixels+bscreen2->pitch*4+4, bscreen2->pitch, pDeltaBuffer+bscreen2->pitch, (u8*)screen->pixels, screen->pitch, screen->w/2, screen->h/2);
		
		videoSurface.width = screen->w;
		videoSurface.height = screen->h;
		videoSurface.pitch = screen->pitch;
		videoSurface.data = screen->pixels;
	}
	else
	{
		/* Compatible CPU images can be presented directly without an extra copy. */
		videoSurface.width = src->width;
		videoSurface.height = src->height;
		videoSurface.pitch = src->width * 4;
		videoSurface.data = src->data;
	}
	return &videoSurface;
}

