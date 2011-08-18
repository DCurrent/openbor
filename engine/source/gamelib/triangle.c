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


#if 1
//////////////////////////////////////////2d quad test/////////////////////////////////////////////

#define swapVertices(va, vb) {vsw=va;va=vb;vb=vsw;}
#define P unsigned char

/**

 draw a pixel from source gfx surface to destination screen
 complex

*/

void draw_pixel_dummy(s_screen* dest, gfx_entry* src, int dx, int dy, int sx, int sy,  s_drawmethod* drawmethod)  
{
	int pb = pixelbytes[(int)dest->pixelformat];
	unsigned char* pd = ((unsigned char*)(dest->data)) + (dx + dy*dest->width)*pb; 
	memset(pd, 0, pb);
}

void draw_pixel_screen(s_screen* dest, gfx_entry* src, int dx, int dy, int sx, int sy,  s_drawmethod* drawmethod)
{

}


void draw_pixel_bitmap(s_screen* dest, gfx_entry* src, int dx, int dy, int sx, int sy,  s_drawmethod* drawmethod)
{

}

void draw_pixel_sprite(s_screen* dest, gfx_entry* src, int dx, int dy, int sx, int sy,  s_drawmethod* drawmethod)
{

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

	void (*drawfp)(s_screen* dest, gfx_entry* src, int dx, int dy, int sx, int sy,  s_drawmethod* drawmethod) = draw_pixel_dummy;

	vrect.ulx = vrect.uly = 0;
	vrect.lrx = dest->width;
	vrect.lry = dest->height;

	
	for (i=0; i<triangleCount; ++i)
	{
		v1 = &vertices[i*3];
		v2 = &vertices[i*3+1];
		v3 = &vertices[i*3+2];

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
						drawfp(dest, src, hSpanBegin, targetY, (int)spanTx, (int)spanTy, drawmethod);

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


