/*
 * OpenBOR - http://www.LavaLit.com
 * -----------------------------------------------------------------------
 * All rights reserved, see LICENSE in OpenBOR root for details.
 *
 * Copyright (c) 2004 - 2011 OpenBOR Team
 */

///////////////////////////////////////////////////////////////////////////
//         This file defines some commmon methods used by the gamelib
////////////////////////////////////////////////////////////////////////////

#include "globals.h"
#include "types.h"

#ifndef TRANSPARENT_IDX
#define		TRANSPARENT_IDX		0x00
#endif

static transpixelfunc pfp;
static unsigned int fillcolor;
static blend16fp pfp16;
static blend32fp pfp32;
static unsigned char* table;

/*transpixelfunc, 8bit*/
static unsigned char remapcolor(unsigned char* table, unsigned char color, unsigned char unused)
{
	return table[color];
}

static unsigned char blendcolor(unsigned char* table, unsigned char color1, unsigned char color2)
{
	if(!table) return color1;
	return table[color1<<8|color2];
}

static unsigned char blendfillcolor(unsigned char* table, unsigned char unused, unsigned char color)
{
	if(!table) return fillcolor;
	return table[fillcolor<<8|color];
}

#if 1
//////////////////////////////////////////2d quad test/////////////////////////////////////////////

#define swapVertices(va, vb) {vsw=va;va=vb;vb=vsw;}
#define P unsigned char

/**

 draw a pixel from source gfx surface to destination screen
 complex

*/

void draw_pixel_dummy(s_screen* dest, gfx_entry* src, int dx, int dy, int sx, int sy)  
{
	int pb = pixelbytes[(int)dest->pixelformat];
	unsigned char* pd = ((unsigned char*)(dest->data)) + (dx + dy*dest->width)*pb; 
	memset(pd, 0, pb);
}

void draw_pixel_screen(s_screen* dest, gfx_entry* src, int dx, int dy, int sx, int sy)
{
	unsigned char *ptrd8, *ptrs8, ps8;
	unsigned short *ptrd16, *ptrs16, pd16, ps16;
	unsigned int *ptrd32, *ptrs32, pd32, ps32;
	switch(dest->pixelformat)
	{
		case PIXEL_8:
			ptrd8 = ((unsigned char*)dest->data) + dx + dy * dest->width;
			ps8 = *(((unsigned char*)src->screen->data) + sx + sy * src->screen->width);
			if(!ps8) return;
			*ptrd8 =pfp?pfp(table, ps8, *ptrd8):ps8;
			break;
		case PIXEL_16:
			ptrd16 = ((unsigned short*)dest->data) + dx + dy * dest->width;
			pd16 = *ptrd16;
			if(fillcolor) ps16 = fillcolor;
			else
			{
				switch(src->screen->pixelformat)
				{
				case PIXEL_16:
					ptrs16 = ((unsigned short*)src->screen->data) + sx + sy * src->screen->width;
					ps16 = *ptrs16;
					break;
				case PIXEL_x8:
					ptrs8 = ((unsigned char*)src->screen->data) + sx + sy * src->screen->width;
					ps16 = table?((unsigned short*)table)[*ptrs8]:((unsigned short*)src->screen->palette)[*ptrs8];
					break;
				default:
					ps16 = 0;
					break;
				}
			}
			if(!ps16) return;
			if(!pfp16) *ptrd16 = ps16;
			else       *ptrd16 = pfp16(ps16, pd16);
			break;
		case PIXEL_32:
			ptrd32 = ((unsigned int*)dest->data) + dx + dy * dest->width;
			pd32 = *ptrd32;
			if(fillcolor) ps32 = fillcolor;
			else
			{
				switch(src->screen->pixelformat)
				{
				case PIXEL_32:
					ptrs32 = ((unsigned int*)src->screen->data) + sx + sy * src->screen->width;
					ps32 = *ptrs32;
					break;
				case PIXEL_x8:
					ptrs8 = ((unsigned char*)src->screen->data) + sx + sy * src->screen->width;
					ps32 = table?((unsigned int*)table)[*ptrs8]:((unsigned int*)src->screen->palette)[*ptrs8];
					break;
				default:
					ps32 = 0;
					break;
				}
			}
			if(!ps32) return;
			if(!pfp32) *ptrd32 = ps32;
			else       *ptrd32 = pfp32(ps32, pd32);
			break;

	}
}


void draw_pixel_bitmap(s_screen* dest, gfx_entry* src, int dx, int dy, int sx, int sy)
{
	//stub
}

// get a pixel from specific sprite
// should be fairly slow due to the RLE compression
unsigned char sprite_get_pixel(s_sprite* sprite, int x, int y){
	int *linetab;
	register int lx = 0;
	unsigned char * data;

	x += sprite->centerx;
	y += sprite->centery;
	
	//should we check? 
	if(y<0 || y>=sprite->height || x<0 || x>=sprite->width)
		return 0;


	linetab = ((int*)sprite->data) + y;

	data = ((unsigned char*)linetab) + (*linetab);

	while(1) {
		register int count = *data++;
		if(count == 0xFF) break;
		if(lx+count>x) return 0; // transparent pixel
		lx += count;
		count = *data++;
		if(!count) continue;
		if(lx + count > x)
		{
			return data[x-lx]; // not transparent pixel
		}
		lx+=count;
		data+=count;
	}

	return 0; // should not happen, just in case

}

void draw_pixel_sprite(s_screen* dest, gfx_entry* src, int dx, int dy, int sx, int sy)
{
	unsigned char *ptrd8, ps8;
	unsigned short *ptrd16, pd16, ps16;
	unsigned int *ptrd32, pd32, ps32;
	switch(dest->pixelformat)
	{
		case PIXEL_8:
			ptrd8 = ((unsigned char*)dest->data) + dx + dy * dest->width;
			ps8 = sprite_get_pixel(src->sprite, sx, sy);
			if(!ps8) return;
			*ptrd8 =pfp?pfp(table, ps8, *ptrd8):ps8;
			break;
		case PIXEL_16:
			ptrd16 = ((unsigned short*)dest->data) + dx + dy * dest->width;
			pd16 = *ptrd16;
			if(fillcolor) ps16 = fillcolor;
			else
			{
				ps8 = sprite_get_pixel(src->sprite, sx, sy);
				ps16 = table?((unsigned short*)table)[ps8]:((unsigned short*)src->sprite->palette)[ps8];
			}
			if(!ps16) return;
			if(!pfp16) *ptrd16 = ps16;
			else       *ptrd16 = pfp16(ps16, pd16);
			break;
		case PIXEL_32:
			ptrd32 = ((unsigned int*)dest->data) + dx + dy * dest->width;
			pd32 = *ptrd32;
			if(fillcolor) ps32 = fillcolor;
			else
			{
				ps8 = sprite_get_pixel(src->sprite, sx, sy);
				ps32 = table?((unsigned int*)table)[ps8]:((unsigned int*)src->sprite->palette)[ps8];
			}
			if(!ps32) return;
			if(!pfp32) *ptrd32 = ps32;
			else       *ptrd32 = pfp32(ps32, pd32);
			break;

	}
}


void draw_triangle_list(vert2d* vertices, s_screen *dest, gfx_entry *src, s_drawmethod* drawmethod, int triangleCount)
{
	vert2d *v1, *v2, *v3, *vsw = NULL;

	int i, triangleHalf;
	float tmpDiv; // temporary division factor
	float longest; // saves the longest span
	int height; // saves height of triangle
	int targetY; // target pointer where to plot pixels
	int spanEnd; // saves end of spans
	float leftdeltaxf; // amount of pixels to increase on left side of triangle
	float rightdeltaxf; // amount of pixels to increase on right side of triangle
	int leftx, rightx, temp; // position where we are 
	float leftxf, rightxf; // same as above, but as float values
	int span; // current span
	int	hSpanBegin, hSpanEnd; // pointer used when plotting pixels
	float leftTx, rightTx, leftTy, rightTy; // texture interpolating values
	float leftTxStep, rightTxStep, leftTyStep, rightTyStep; // texture interpolating values
	float spanTx, spanTy, spanTxStep, spanTyStep; // values of Texturecoords when drawing a span
	rect2d trect, vrect; //triangle rect

	int spf = 0; //source pixel format

	void (*drawfp)(s_screen* dest, gfx_entry* src, int dx, int dy, int sx, int sy) = draw_pixel_dummy;

	switch(src->type)
	{
	case gfx_screen:
		spf = src->screen->pixelformat;
		drawfp = draw_pixel_screen;
		break;
	case gfx_bitmap:
		spf = src->bitmap->pixelformat;
		drawfp = draw_pixel_bitmap;
		break;
	case gfx_sprite:
		spf = src->sprite->pixelformat;
		drawfp = draw_pixel_sprite;
		break;
	default:
		return;
	}

	switch(dest->pixelformat)
	{
	case PIXEL_8:
		if(drawmethod->fillcolor) fillcolor = drawmethod->fillcolor&0xFF;
		else fillcolor = 0;

		table = NULL;

		if(drawmethod->table)
		{
			table = drawmethod->table;
			pfp = remapcolor;
		}
		else if(drawmethod->alpha>0)
		{
			table = blendtables[drawmethod->alpha-1];
			pfp = (fillcolor==TRANSPARENT_IDX?blendcolor:blendfillcolor);
		}
		else pfp = (fillcolor==TRANSPARENT_IDX?NULL:blendfillcolor);
		break;
	case PIXEL_16:
		fillcolor = drawmethod->fillcolor;
		if(drawmethod->alpha>0) pfp16 = blendfunctions16[drawmethod->alpha-1];
		else pfp16 = NULL;
		table = drawmethod->table;
		break;
	case PIXEL_32:
		fillcolor = drawmethod->fillcolor;
		if(drawmethod->alpha>0) pfp32 = blendfunctions32[drawmethod->alpha-1];
		else pfp32 = NULL;
		table = drawmethod->table;
		break;
	default: 
		return;
	}


	vrect.ulx = vrect.uly = 0;
	vrect.lrx = dest->width;
	vrect.lry = dest->height;
	
	for (i=0; i<triangleCount; ++i)
	{
		v1 = &vertices[i];
		v2 = &vertices[i+1];
		v3 = &vertices[i+2];

		// sort for width for inscreen clipping

		if (v1->x > v2->x)	swapVertices(v1, v2);
		if (v1->x > v3->x)	swapVertices(v1, v3);
		if (v2->x > v3->x)	swapVertices(v2, v3);

		if ((v1->x - v3->x) == 0)
			continue;

		trect.ulx = v1->x;
		trect.lrx = v3->x;

		// sort for height for faster drawing.

		if (v1->y > v2->y)	swapVertices(v1, v2);
		if (v1->y > v3->y)	swapVertices(v1, v3);
		if (v2->y > v3->y)	swapVertices(v2, v3);

		trect.uly = v1->y;
		trect.lry = v3->y;

		if (trect.ulx>vrect.lrx || trect.uly>vrect.lry || trect.lrx<vrect.ulx || trect.lry<vrect.uly)
			continue;

		// calculate height of triangle
		height = v3->y - v1->y;
		if (!height)
			continue;

		// calculate longest span

		longest = (v2->y - v1->y) / (float)height * (v3->x - v1->x) + (v1->x - v2->x);

		spanEnd = v2->y;
		span = v1->y;
		leftxf = (float)v1->x;
		rightxf = (float)v1->x;

		leftTx = rightTx = v1->tx;
		leftTy = rightTy = v1->ty;

		targetY = span;

		if (longest < 0.0f)
		{
			tmpDiv = 1.0f / (float)(v2->y - v1->y);
			rightdeltaxf = (v2->x - v1->x) * tmpDiv;
			rightTxStep = (v2->tx - rightTx) * tmpDiv;
			rightTyStep = (v2->ty - rightTy) * tmpDiv;

			tmpDiv = 1.0f / (float)height;
			leftdeltaxf = (v3->x - v1->x) * tmpDiv;
			leftTxStep = (v3->tx - leftTx) * tmpDiv;
			leftTyStep = (v3->ty - leftTy) * tmpDiv;
		}
		else
		{
			tmpDiv = 1.0f / (float)height;
			rightdeltaxf = (v3->x - v1->x) * tmpDiv;
			rightTxStep = (v3->tx - rightTx) * tmpDiv;
			rightTyStep = (v3->ty - rightTy) * tmpDiv;

			tmpDiv = 1.0f / (float)(v2->y - v1->y);
			leftdeltaxf = (v2->x - v1->x) * tmpDiv;
			leftTxStep = (v2->tx - leftTx) * tmpDiv;
			leftTyStep = (v2->ty - leftTy) * tmpDiv;
		}


		// do it twice, once for the first half of the triangle,
		// end then for the second half.

		for (triangleHalf=0; triangleHalf<2; ++triangleHalf)
		{
			if (spanEnd > vrect.lry)
				spanEnd = vrect.lry;

			// if the span <0, than we can skip these spans, 
			// and proceed to the next spans which are really on the screen.
			if (span < vrect.uly)
			{
				// we'll use leftx as temp variable
				if (spanEnd < vrect.uly)
				{
					temp = spanEnd - span;
					span = spanEnd;
				}
				else
				{
					temp = vrect.uly - span; 
					span = vrect.uly;
				}

				leftxf += leftdeltaxf*temp;
				rightxf += rightdeltaxf*temp;
				targetY += temp;

				leftTx += leftTxStep*temp;
				leftTy += leftTyStep*temp;
				rightTx += rightTxStep*temp;
				rightTy += rightTyStep*temp;
			}


			// the main loop. Go through every span and draw it.

			while (span < spanEnd)
			{
				leftx = (int)(leftxf);
				rightx = (int)(rightxf + 0.5f);

				// perform some clipping

				// TODO: clipping is not correct when leftx is clipped.

				if (leftx<vrect.ulx)
					leftx = vrect.ulx;
				else
					if (leftx>vrect.lrx)
						leftx = vrect.lrx;

				if (rightx<vrect.ulx)
					rightx = vrect.ulx;
				else
					if (rightx>vrect.lrx)
						rightx = vrect.lrx;

				// draw the span

				if (rightx - leftx != 0)
				{
					tmpDiv = 1.0f / (rightx - leftx);

					hSpanBegin = leftx;
					hSpanEnd = rightx;

					spanTx = (float)leftTx;	
					spanTy = (float)leftTy;
					spanTxStep = (rightTx - leftTx) * tmpDiv;
					spanTyStep = (rightTy - leftTy) * tmpDiv;

					//printf("==%f ==%f\n", spanTxStep, spanTyStep);

					while (hSpanBegin < hSpanEnd)
					{
						//*hSpanBegin = srcp[(int)spanTy * src->width + (int)spanTx];
						drawfp(dest, src, hSpanBegin, targetY, (int)spanTx, (int)spanTy);

						spanTx += spanTxStep;
						spanTy += spanTyStep;
						
						++hSpanBegin;
					}
				}

				leftxf += leftdeltaxf;
				rightxf += rightdeltaxf;
				++span;
				++targetY;

				leftTx += leftTxStep;
				leftTy += leftTyStep;
				rightTx += rightTxStep;
				rightTy += rightTyStep;

				//printf("rightTyStep %d  leftTyStep %d\n", rightTyStep, leftTyStep);
			}

			if (triangleHalf>0) // break, we've gout only two halves
				break;


			// setup variables for second half of the triangle.

			if (longest < 0.0f)
			{
				tmpDiv = 1.0f / (v3->y - v2->y);

				rightdeltaxf = (v3->x - v2->x) * tmpDiv;
				rightxf = (float)v2->x;

				rightTx = v2->tx;
				rightTy = v2->ty;
				rightTxStep = (v3->tx - rightTx) * tmpDiv;
				rightTyStep = (v3->ty - rightTy) * tmpDiv;
			}
			else
			{
				tmpDiv = 1.0f / (v3->y - v2->y);

				leftdeltaxf = (v3->x - v2->x) * tmpDiv;
				leftxf = (float)v2->x;

				leftTx = v2->tx;
				leftTy = v2->ty;
				leftTxStep = (v3->tx - leftTx) * tmpDiv;
				leftTyStep = (v3->ty - leftTy) * tmpDiv;
			}


			spanEnd = v3->y;
		}

	}

}
#undef P
#undef swapVertices
////////////////////////////////////////////////////////////////
#endif


