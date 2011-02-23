/*
 * OpenBOR - http://www.LavaLit.com
 * -----------------------------------------------------------------------
 * Licensed under the BSD license, see LICENSE in OpenBOR root for details.
 *
 * Copyright (c) 2004 - 2011 OpenBOR Team
 */

// Simple bitmap code. Not fast, but useful nonetheless...
// 25-jan-2003

#include <stdio.h>
#include "types.h"
#include "tracemalloc.h"


#define		TRANS_INDEX		0x00


s_bitmap * allocbitmap(int width, int height, int format){
	s_bitmap *b;
	int psize, extrab;
	if(width * height == 0) return NULL;
	psize = width*height*pixelbytes[(int)format];
	extrab = (4-psize%4)%4;
	if(format==PIXEL_x8)
        b = (s_bitmap *)tracemalloc("allocbitmap",4*sizeof(int) + psize + extrab +PAL_BYTES);
    else b = (s_bitmap *)tracemalloc("allocbitmap#2",4*sizeof(int) + psize);
	if(b){
		b->width = width;
		b->height = height;
		b->pixelformat = format;
		if(format==PIXEL_x8) b->palette = ((unsigned char*)b->data) + psize + extrab;
		else                 b->palette = NULL;
	}
	return b;
}

void freebitmap(s_bitmap *bitmap){
	if(bitmap != NULL){
		tracefree(bitmap);
		bitmap = NULL;
	}
}



// Sluggish getbitmap function. Should be redone in ASM.
void getbitmap(int x, int y, int width, int height, s_bitmap *bitmap, s_screen *screen){

	int s, d;
	int i, j;

	// Clip width and height
	if(x<0){
		width += x;
		x = 0;
	}
	if(y<0){
		height += y;
		y = 0;
	}
	if(x+width > screen->width){
		width = screen->width - x;
	}
	if(y+height > screen->height){
		height = screen->height - y;
	}
	if(width<=0 || height<=0){
		bitmap->width = 0;
		bitmap->height = 0;
		return;
	}

	bitmap->width = width;
	bitmap->height = height;

	d = 0;
	for(j=0; j<height; j++){
		s = x + (y+j) * screen->width;
		for(i=0; i<width; i++){
			bitmap->data[d] = screen->data[s];
			++d;
			++s;
		}
	}
}



// Clipped putbitmap. Slow.
void putbitmap(int x, int y, s_bitmap *bitmap, s_screen *screen){
	int skipleft = 0;
	int skiptop = 0;
	int width = bitmap->width;
	int height = bitmap->height;
	int s, d;
	int i;

	// Clip width and height
	if(x<0){
		skipleft = -x;
		width -= skipleft;
		x = 0;
	}
	if(y<0){
		skiptop = -y;
		height -= skiptop;
		y = 0;
	}
	if(x+width > screen->width){
		width = screen->width - x;
	}
	if(y+height > screen->height){
		height = screen->height - y;
	}
	if(width<=0 || height<=0) return;

	d = (y*screen->width) + x;

	do{
		s = skiptop * bitmap->width + skipleft;
		++skiptop;
		for(i=0; i<width; i++){
			screen->data[d] = bitmap->data[s];
			++d;
			++s;
		}
		d += screen->width;
		d -= width;
	}while(--height);
}




// Flip horizontally
void flipbitmap(s_bitmap *bitmap){
	int x, xo, y;
	unsigned char t;
	int xsize = bitmap->width;
	int ysize = bitmap->height;

	for(y=0; y<ysize; y++){
		for(x=0, xo=xsize-1; x<xsize/2; x++, xo--){
			t = bitmap->data[y*xsize+x];
			bitmap->data[y*xsize+x] = bitmap->data[y*xsize+xo];
			bitmap->data[y*xsize+xo] = t;
		}
	}
}





// Clipbitmap: cuts off transparent edges to optimize a bitmap.
void clipbitmap(s_bitmap *bitmap, int *clipl, int *clipr, int *clipt, int *clipb){

	int x, y, i;
	int clip, clear;
	int xsize = bitmap->width;
	int ysize = bitmap->height;
	int tclipmove = 0;
	int lclipmove = 0;
	int bclipmove = 0;
	int rclipmove = 0;



	// Determine size of empty top
	clip = 0;
	for(y=0; y<ysize; y++){
		clear = 1;
		for(x=0; x<xsize && clear; x++){
			if(bitmap->data[y*xsize+x] != TRANS_INDEX) clear = 0;
		}
		if(clear) ++clip;
		else break;
	}

	if(clip){
		// Cut off empty top
		ysize -= clip;
		if(ysize<1){
			// If nothing is left of the bitmap, return...
			if(clipl) *clipl = 0;
			if(clipr) *clipr = 0;
			if(clipt) *clipt = 0;
			if(clipb) *clipb = 0;
			bitmap->width = 0;
			bitmap->height = 0;
			return;
		}
		bitmap->height = ysize;
		for(i=0; i<xsize*ysize; i++) bitmap->data[i] = bitmap->data[i+(clip*xsize)];
		tclipmove = clip;
	}



	// Determine size of empty bottom
	clip = 0;
	for(y=ysize-1; y>=0; y--){
		clear = 1;
		for(x=0; x<xsize && clear; x++){
			if(bitmap->data[y*xsize+x] != TRANS_INDEX) clear = 0;
		}
		if(clear) ++clip;
		else break;
	}

	// Cut off empty bottom
	ysize -= clip;
	bitmap->height = ysize;
	bclipmove = clip;


	// Determine size of empty left side
	clip = 2000000000;
	for(y=0; y<ysize; y++){
		clear = 0;
		for(x=0; x<xsize; x++){
			if(bitmap->data[y*xsize+x] != TRANS_INDEX) break;
			++clear;
		}
		if(clear < clip) clip = clear;
	}

	// Cut off empty pixels on the left side
	if(clip){
		xsize -= clip;
		bitmap->width = xsize;
		for(y=0; y<ysize; y++){
			for(x=0; x<xsize; x++) bitmap->data[y*xsize+x] = bitmap->data[y*xsize+x+(clip*(y+1))];
		}
		lclipmove = clip;
	}

	// Determine size of empty right side
	clip = 2000000000;
	for(y=0; y<ysize; y++){
		clear = 0;
		for(x=xsize-1; x>=0; x--){
			if(bitmap->data[y*xsize+x] != TRANS_INDEX) break;
			++clear;
		}
		if(clear < clip) clip = clear;
	}

	// Cut off empty pixels on the right side
	if(clip){
		xsize -= clip;
		bitmap->width = xsize;
		for(y=0; y<ysize; y++){
			for(x=0; x<xsize; x++) bitmap->data[y*xsize+x] = bitmap->data[y*xsize+x+(clip*y)];
		}
		rclipmove = clip;
	}

	if(clipl) *clipl = lclipmove;
	if(clipr) *clipr = rclipmove;
	if(clipt) *clipt = tclipmove;
	if(clipb) *clipb = bclipmove;
}



