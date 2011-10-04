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
#include "transform.h"

#define _swap(va, vb) {vsw=va;va=vb;vb=vsw;}
#define P unsigned char


void draw_triangle_list(vert2d* vertices, s_screen *dest, gfx_entry *src, s_drawmethod* drawmethod, int triangleCount)
{
	vert2d *v1, *v2, *v3, *vsw = NULL;

	int i, triangleHalf;
	float tmpDiv; // temporary division factor
	float spanLongest; // saves the spanLongest span
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
	unsigned char* shadow_buffer; // temporary fix to remove overlapping, relatively slow
	extern int trans_sw;

	init_gfx_global_draw_stuff(dest, src, drawmethod);
	if(!trans_sw) return;

	vrect.ulx = vrect.uly = 0;
	vrect.lrx = dest->width;
	vrect.lry = dest->height;

	shadow_buffer = malloc(dest->width*dest->height);

	if(!shadow_buffer) return;

	memset(shadow_buffer, 0, dest->width*dest->height);
/*
	if(triangleCount==8 && src->type == gfx_sprite){
		printf("%ld\n", src->sprite);
	}
	*/
	for (i=0; i<triangleCount; ++i)
	{
		v1 = &vertices[i];
		v2 = &vertices[i+1];
		v3 = &vertices[i+2];

		// sort in order of v1 <= v2 <= v3
		if (v1->x > v2->x)	_swap(v1, v2);
		if (v1->x > v3->x)	_swap(v1, v3);
		if (v2->x > v3->x)	_swap(v2, v3);

		if ((v1->x - v3->x) == 0)
			continue;

		trect.ulx = v1->x;
		trect.lrx = v3->x;

		// sort in order of v1 <= v2 <= v3
		if (v1->y > v2->y)	_swap(v1, v2);
		if (v1->y > v3->y)	_swap(v1, v3);
		if (v2->y > v3->y)	_swap(v2, v3);

		trect.uly = v1->y;
		trect.lry = v3->y;

		if (trect.ulx>vrect.lrx || trect.uly>vrect.lry || trect.lrx<vrect.ulx || trect.lry<vrect.uly)
			continue;

		// calculate height of triangle
		height = v3->y - v1->y;
		if (!height)
			continue;

		spanLongest = (v2->y - v1->y) / (float)height * (v3->x - v1->x) + (v1->x - v2->x);

		spanEnd = v2->y;
		span = v1->y;
		leftxf = (float)v1->x;
		rightxf = (float)v1->x;

		leftTx = rightTx = v1->tx;
		leftTy = rightTy = v1->ty;

		targetY = span;

		if (spanLongest < 0.0f)
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


		//draw upper and lower half of the triangle
		for (triangleHalf=0; triangleHalf<2; ++triangleHalf)
		{
			if (spanEnd > vrect.lry)
				spanEnd = vrect.lry;

			// if the span <0, than we can skip these spans, 
			// and proceed to the next spans which are really on the screen.
			if (span < vrect.uly)
			{
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


			// draw each line of the part
			while (span < spanEnd)
			{
				leftx = (int)(leftxf);
				rightx = (int)(rightxf + 0.5f);

				// TODO: perform clipping before going for the pixels
				if (rightx - leftx != 0)
				{
					tmpDiv = 1.0f / (rightxf - leftxf); // important: use float version of the right and left values to avoid sawtooth left edge

					hSpanBegin = leftx;
					hSpanEnd = rightx;

					spanTx = (float)leftTx;	
					spanTy = (float)leftTy;
					spanTxStep = (rightTx - leftTx) * tmpDiv;
					spanTyStep = (rightTy - leftTy) * tmpDiv;

					dest_seek(hSpanBegin, targetY);
					while (hSpanBegin < hSpanEnd)
					{
						if(hSpanBegin>=vrect.ulx && hSpanBegin<vrect.lrx)
						{
							temp = hSpanBegin + targetY*dest->width;
							//*hSpanBegin = srcp[(int)spanTy * src->width + (int)spanTx];
							if(!shadow_buffer[temp]) 
							{
								src_seek( (int)spanTx, (int)spanTy);
								//draw_pixel_gfx(dest, src, hSpanBegin, targetY, (int)spanTx, (int)spanTy);
								write_pixel();
								shadow_buffer[temp] = 1;
							}
						}

						spanTx += spanTxStep;
						spanTy += spanTyStep;
						dest_inc();
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

			if (spanLongest < 0.0f)
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
	free(shadow_buffer);
	shadow_buffer = NULL;
}
#undef P
#undef _swap
////////////////////////////////////////////////////////////////



