/*
 * OpenBOR - http://www.LavaLit.com
 * -----------------------------------------------------------------------
 * Licensed under the BSD license, see LICENSE in OpenBOR root for details.
 *
 * Copyright (c) 2004 - 2011 OpenBOR Team
 */

// draw functions with 16bit pixel format, with alpha blending

#include <string.h>
#include "types.h"

#ifndef		NULL
#define		NULL	 ((void*)0)
#endif

#define		abso(x)		(x<0?-x:x)

#define __putpixel16(p) \
			if(blendfp )\
			{\
				*(p) = blendfp(colour, *(p));\
			}\
			else\
			{\
				*(p) = colour;\
			}

// same as the one in draw.c, this is 16bit version
// blendfp is the blending function pointer
void line16(int sx, int sy, int ex, int ey, unsigned short colour, s_screen *screen, int alpha)
{
	int diffx, diffy;
	int absdiffx, absdiffy;
	int xdir, ydir;
	int thres;
	int d;
	unsigned short* data;
	unsigned short(*blendfp)(unsigned short,unsigned short);

	// Some off-screen lines may slip through this test!
	if(sx<0 && ex<0) return;
	if(sy<0 && ey<0) return;
	if(sx>=screen->width && ex>=screen->width) return;
	if(sy>=screen->height && ey>=screen->height) return;


	// Check clipping and calculate new coords if necessary

	diffx = ex - sx;
	diffy = ey - sy;

	if(sx<0){
		sy -= (sx*diffy/diffx);
		sx = 0;
	}
	if(sy<0){
		sx -= (sy*diffx/diffy);
		sy = 0;
	}
	if(sx>=screen->width){
		sy -= ((sx-screen->width)*diffy/diffx);
		sx = screen->width-1;
	}
	if(sy>=screen->height){
		sx -= ((sy-screen->height)*diffx/diffy);
		sy = screen->height-1;
	}

	if(ex<0){
		ey -= (ex*diffy/diffx);
		ex = 0;
	}
	if(ey<0){
		ex -= (ey*diffx/diffy);
		ey = 0;
	}
	if(ex>=screen->width){
		ey -= ((ex-screen->width)*diffy/diffx);
		ex = screen->width-1;
	}
	if(ey>=screen->height){
		ex -= ((ey-screen->height)*diffx/diffy);
		ey = screen->height-1;
	}


	// Second test: the lines that passed test 1 won't pass this time!
	if(sx<0 || ex<0) return;
	if(sy<0 || ey<0) return;
	if(sx>=screen->width || ex>=screen->width) return;
	if(sy>=screen->height || ey>=screen->height) return;


	// Recalculate directions
	diffx = ex - sx;
	diffy = ey - sy;

	absdiffx = abso(diffx);
	absdiffy = abso(diffy);

	sy *= screen->width;
	ey *= screen->width;

	data = (unsigned short*)screen->data;

	blendfp = alpha>0?blendfunctions16[alpha-1]:NULL;

	if(absdiffx > absdiffy)
	{
		// Draw a flat line
		thres = absdiffx >> 1;
		xdir = 1;
		if(diffx<0) xdir = -xdir;
		ydir = screen->width;
		if(diffy<0) ydir = -ydir;
		while(sx!=ex)
		{
			d = sx+sy;
			__putpixel16(data+d);
			sx += xdir;
			if((thres-=absdiffy) <= 0){
				sy += ydir;
				thres += absdiffx;
			}
		}
		d = ex+ey;
		__putpixel16(data+d);
		return;
	}

	// Draw a high line
	thres = absdiffy >> 1;
	xdir = 1;
	if(diffx<0) xdir = -1;
	ydir = screen->width;
	if(diffy<0) ydir = -ydir;
	while(sy!=ey){
		d = sx+sy;
		__putpixel16(data+d);
		sy += ydir;
		if((thres-=absdiffx) <= 0){
			sx += xdir;
			thres += absdiffy;
		}
	}
	d = ex+ey;
	__putpixel16(data+d);
}




// drawbox, 16bit version
void drawbox16(int x, int y, int width, int height, unsigned short colour, s_screen *screen, int alpha)
{
	unsigned short *cp;
	unsigned short(*blendfp)(unsigned short,unsigned short);

	if(width<=0) return;
	if(height<=0) return;
	if(screen==NULL) return;

	if(x<0){
		if((width+=x)<=0) return;
		x = 0;
	}
	else if(x>=screen->width) return;
	if(y<0){
		if((height+=y)<=0) return;
		y = 0;
	}
	else if(y>=screen->height) return;
	if(x+width>screen->width) width = screen->width-x;
	if(y+height>screen->height) height = screen->height-y;

	cp = ((unsigned short*)screen->data) + (y*screen->width + x);

	blendfp = alpha>0?blendfunctions16[alpha-1]:NULL;

	while(--height>=0){
		for(x=0;x<width;x++){
			__putpixel16(cp);
			cp++;
		}
		cp += (screen->width - width);
	}
}



// Putpixel 16bit version
void putpixel16(unsigned x, unsigned y, unsigned short colour, s_screen *screen, int alpha){
	int pixind;
	unsigned short* data ;
	unsigned short(*blendfp)(unsigned short,unsigned short);
	if(x>screen->width || y>screen->height) return;
	pixind = x+y*screen->width;
	data = (unsigned short*)screen->data + pixind;
	blendfp = alpha>0?blendfunctions16[alpha-1]:NULL;
	__putpixel16(data);
}


