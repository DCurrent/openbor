/*
 * OpenBOR - http://www.LavaLit.com
 * -----------------------------------------------------------------------
 * Licensed under the BSD license, see LICENSE in OpenBOR root for details.
 *
 * Copyright (c) 2004 - 2011 OpenBOR Team
 */

// see texture.c, this file uses 32bit palette and screen
#include <string.h>
#include "types.h"

//static int distortion[256];
extern float _sinfactors[256];
extern float _amp;
#define distortion(x) ((int)(_sinfactors[x]*_amp+0.5))


void texture_wavex8p32(s_screen *screen, int x, int y, int width, int height, int offsx, int offsy, s_bitmap *bitmap, int offsd, int step, unsigned* pal32){

	unsigned char *src, *sp;
	unsigned *dest, *dp;
	int s, i;
	int sy;
	int twidth;
	int tx;
    unsigned* thepal = pal32?pal32:(unsigned*)bitmap->palette;

    if(thepal==NULL) return;
	// Check dimensions
	if(x >= screen->width) return;
	if(y >= screen->height) return;
	if(x<0){
		width += x;
		x = 0;
	}
	if(y<0){
		height += y;
		y=0;
	}
	if(x+width > screen->width){
		width = screen->width - x;
	}
	if(y+height > screen->height){
		height = screen->height - y;
	}
	if(width<=0) return;
	if(height<=0) return;

	// Dest ptr
	dest = ((unsigned*)screen->data) + ((y * screen->width) + x);

	// Fill area
	do{
		// Source line ptr
		sy = offsy % bitmap->height;
		src = bitmap->data + (sy * bitmap->width);
		offsy++;

		// Adjust distortion stuff
		offsd &= 255;
		s = (offsx + distortion(offsd)) % bitmap->width;
		offsd += step;

		// Copy loop
		tx = 0;
		twidth = bitmap->width - s;
		if(twidth > width) twidth = width;
		while(twidth > 0){
            // apply texture to 24bit screen
            dp = dest+tx;
            sp = src+s;
            for(i=0; i<twidth; i++) dp[i] = thepal[sp[i]];
			s = 0;
			tx += twidth;
			twidth = width - tx;
			if(twidth > bitmap->width) twidth = bitmap->width;
		}

		// Advance destination line pointer
		dest += screen->width;
	}while(--height);

}



static void draw_plane_line(unsigned *destline, unsigned char *srcline, int destlen, int srclen, int stretchto, int texture_offset, unsigned* table){
	int i;
	unsigned int s, s_pos, s_step;
	int center_offset = destlen / 2;

	s_pos = texture_offset + (256 * srclen);
	s_step = srclen * 256 / stretchto;
	s_pos -= center_offset * s_step;

	for(i=0; i<destlen; i++){
		s = s_pos >> 8;
		if(s > srclen){
			s %= srclen;
			s_pos = (s_pos & 0xFF) | (s << 8);
		}
        destline[i] = table[srcline[s]];
		s_pos += s_step;
	}
}



// Draw a plane (like the sea)
void texture_planex8p32(s_screen *screen, int x, int y, int width, int height, int fixp_offs, int factor, s_bitmap *bitmap, unsigned* pal32){

	int i;
	unsigned *dest;
	unsigned char *src;
	int sy;
	unsigned* thepal = pal32?pal32:(unsigned*)bitmap->palette;

	if(!thepal) return;

	if(factor < 0) return;
	factor++;

	// Check dimensions
	if(x >= screen->width) return;
	if(y >= screen->height) return;
	if(x<0){
		width += x;
		x = 0;
	}
	if(y<0){
		height += y;
		y=0;
	}
	if(x+width > screen->width){
		width = screen->width - x;
	}
	if(y+height > screen->height){
		height = screen->height - y;
	}
	if(width<=0) return;
	if(height<=0) return;


	dest = ((unsigned*)screen->data) + ((y*screen->width) + x);
	sy = 0;
	for(i=0; i<height; i++){

		sy = i % bitmap->height;
		src = bitmap->data + (sy * bitmap->width);

		draw_plane_line(dest,src, width,bitmap->width, bitmap->width + ((bitmap->width * i) / factor), fixp_offs, thepal);

		dest += screen->width;
	}
}




