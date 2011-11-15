/*
 * OpenBOR - http://www.LavaLit.com
 * -----------------------------------------------------------------------
 * All rights reserved, see LICENSE in OpenBOR root for details.
 *
 * Copyright (c) 2004 - 2011 OpenBOR Team
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include "types.h"
#include "screen.h"
#include "loadimg.h"
#include "bitmap.h"
#include "sprite.h"
#include "spriteq.h"

#define		MAX_FONTS		8
#define		FONT_LAYER		0x0FFFFFFF


typedef struct{
	s_sprite *	token[256];
	int		token_width[256];
	int		font_loaded;
}s_font;

s_font fonts[MAX_FONTS];

int font_heights[MAX_FONTS];
int font_monowidths[MAX_FONTS];

void font_unload(int which){
	int i;

	which %= MAX_FONTS;

	for(i=0; i<256; i++){
		if(fonts[which].token[i] != NULL) free(fonts[which].token[i]);
		fonts[which].token[i] = NULL;
	}
	fonts[which].font_loaded = 0;
}



int font_load(int which, char *filename, char *packfile, int monospace){
	int x, y;
	int index = 0;
	int size;
	int cx = 0, cy = 0;
	s_bitmap *bitmap = NULL;
	s_screen *screen = NULL;
	int rval = 0;
	int tw, th;

	which %= MAX_FONTS;

	font_unload(which);

	if(!loadscreen(filename, packfile, NULL, pixelformat, &screen)) goto err;
	font_monowidths[which] = tw = screen->width/16;
	font_heights[which] = th = screen->height/16;
	if(!(bitmap = allocbitmap(tw,th,pixelformat))) goto err;

	if(bitmap->palette && screen->palette)
		memcpy(bitmap->palette, screen->palette, PAL_BYTES);

	// grab tokens
	for(y=0; y<16; y++){
		for(x=0; x<16; x++){
			getbitmap(x*tw, y*th, tw,th, bitmap, screen);
			clipbitmap(bitmap, &cx, NULL, &cy, NULL);
			if(index>0)  bitmap->palette=NULL;
			size = fakey_encodesprite(bitmap);
			fonts[which].token[index] = (s_sprite*)malloc(size);
			if(!fonts[which].token[index]){
				font_unload(which);
				goto err;
			}
			encodesprite(-cx,-cy, bitmap, fonts[which].token[index]);
			fonts[which].token_width[index] = monospace?tw:(fonts[which].token[index]->width+(tw/10));
			if(fonts[which].token_width[index] <= 1) fonts[which].token_width[index] = tw/3;
			if(index>0)
			{
				fonts[which].token[index]->palette = fonts[which].token[0]->palette ;
				fonts[which].token[index]->pixelformat = screen->pixelformat ;
			}
			++index;
		}
	}

	rval = 1;
	fonts[which].font_loaded = 1;

err:
	freebitmap(bitmap);
	freescreen(&screen);

	return rval;
}

static char b[1024];

int font_string_width(int which, char* format, ...)
{
	int w=0;
	char * buf = b;
	va_list arglist;

	which %= MAX_FONTS;

	if(!fonts[which].font_loaded || !format) return 0;

	va_start(arglist, format);
	vsprintf(buf, format, arglist);
	va_end(arglist);

	while(*buf)
	{
	    w += fonts[which].token_width[((int)(*buf)) & 0xFF];
	    buf++;
	}
	return w;
}

void font_printf(int x, int y, int which, int layeroffset,char *format, ...){
	char * buf = b;
	va_list arglist;

	which %= MAX_FONTS;

	if(!fonts[which].font_loaded) return;

	va_start(arglist, format);
	vsprintf(buf, format, arglist);
	va_end(arglist);

	while(*buf){
		spriteq_add_frame(x,y, FONT_LAYER + layeroffset, fonts[which].token[((int)(*buf)) & 0xFF], NULL, 0);
		x += fonts[which].token_width[((int)(*buf)) & 0xFF];
		if(*buf=='\n') y += fonts[which].token[0]->height;
		buf++;
	}
}


// Print to a screen rather than queueing the sprites
void screen_printf(s_screen * screen, int x, int y, int which, char *format, ...){
	char * buf = b;
	va_list arglist;
	int ox = x;

	which %= MAX_FONTS;

	if(!fonts[which].font_loaded) return;

	va_start(arglist, format);
	vsprintf(buf, format, arglist);
	va_end(arglist);

	while(*buf){
		if(*buf>=32)
		{
			putsprite(x, y, fonts[which].token[((int)(*buf)) & 0xFF], screen, NULL);
			x += fonts[which].token_width[((int)(*buf)) & 0xFF];
		}
		else if(*buf=='\n'){
			x = ox;
			y += fonts[which].token[0]->height;;
		}
		buf++;
	}
}



