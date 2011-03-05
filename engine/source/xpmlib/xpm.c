/*
 * OpenBOR - http://www.LavaLit.com
 * -----------------------------------------------------------------------
 * Licensed under the BSD license, see LICENSE in OpenBOR root for details.
 *
 * Copyright (c) 2004 - 2011 OpenBOR Team
 */

/*
 * Code to decode XPM images used for OpenBOR's menu graphics.
 * Author: Plombo
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "types.h"
#include "screen.h"

#ifdef SDL
#include <SDL.h>
#endif

typedef struct {
	char chars[4];
	int rgb;
} XpmColor;

/*
 * Decodes an XPM image from its array form. Supports only the specific subset 
 * of the XPM format used by OpenBOR.
 */
s_screen* xpmToScreen(char *array[])
{
	int x, y, i;
	int width, height, numcolors, cpp;
	char *str;
	XpmColor *colors = NULL;
	s_screen *screen = NULL;
	int *dest;

	// read width, height, number of colors, and chars per pixel
	if(sscanf(*array++, "%i %i %i %i", &width, &height, &numcolors, &cpp)!=4) goto fail;

	// read color list into array
	colors = malloc(numcolors*sizeof(XpmColor));
	for(i=0; i<numcolors; i++)
	{
		str = *array++;
		memcpy(colors[i].chars, str, cpp);
		str += cpp + 1;
		if(sscanf(str, "c #%x", &(colors[i].rgb)))
		{
			colors[i].rgb |= 0xff000000; // opaque
		}
		else if(strcmp(str, "c None")==0) 
			colors[i].rgb = 0; // transparent
		else goto fail;
	}

	// initialize screen
	screen = allocscreen(width, height, PIXEL_32);
	dest = (int*)screen->data;
	
	// decode image
	for(y=0; y<height; y++)
	{
		str = *array++;
		for(x=0; x<width; x++)
		{
			for(i=0; i<numcolors; i++)
			{
				if(!memcmp(str, colors[i].chars, cpp)) break;
			}
			if(i==numcolors) goto fail;
			*dest++ = colors[i].rgb;
			str += cpp;
		}
	}
	return screen;
	
fail:
	free(colors);
	freescreen(&screen);
	return NULL;
}

#ifdef SDL
SDL_Surface* xpmToSurface(char *array[])
{
	unsigned char *sp;
    char *dp;
    int width, height, linew;
    int h;
    SDL_Surface* ds = NULL;
    s_screen* src = xpmToScreen(array);

	if(src == NULL) return NULL;
	
    width = src->width;
    height = src->height;
    h = height;

    sp = (unsigned char*)src->data;
    ds = SDL_AllocSurface(SDL_SWSURFACE, width, height, 32, 0,0,0,0);
    dp = ds->pixels;

    linew = width*4;

    do{
        memcpy(dp, sp, linew);
        sp += linew;
        dp += ds->pitch;
    }while(--h);
    
    freescreen(&src);

    return ds;
}
#endif

