/*
 * OpenBOR - http://www.LavaLit.com
 * -----------------------------------------------------------------------
 * All rights reserved, see LICENSE in OpenBOR root for details.
 *
 * Copyright (c) 2004 - 2011 OpenBOR Team
 */

#include <stdio.h>
#include <string.h>
#include "types.h"

//with remap, work only under 8bit pixel format
void putscreenx8p32(s_screen * dest, s_screen * src, int x, int y, int key, u32* remap, u32(*blendfp)(u32,u32))
{
	unsigned char *sp = src->data;
	u32 *dp = (u32*)dest->data;
	int i;
	int sw = src->width;
	int sh = src->height;
	int dw = dest->width;
	int cw = sw, ch = sh;
	int sox, soy;
	int xmin=useclip?clipx1:0,
		xmax=useclip?clipx2:dest->width,
		ymin=useclip?clipy1:0,
		ymax=useclip?clipy2:dest->height;

	// Copy anything at all?
	if(x >= xmax) return;
	if(sw+x <= xmin) return;
	if(y >= ymax) return;
	if(sh+y <= ymin) return;

	sox = 0;
	soy = 0;

	// Clip?
	if(x<xmin) {sox = xmin-x; cw -= sox;}
	if(y<ymin) {soy = ymin-y; ch -= soy;}

	if(x+sw > xmax) cw -= (x+sw) - xmax;
	if(y+sh > ymax) ch -= (y+sh) - ymax;

	if(x<xmin) x = xmin;
	if(y<ymin) y = ymin;

	sp += (soy*sw + sox);
	dp += (y*dw + x);

	if(!remap) remap = (u32*)src->palette;

	if(!remap) return;

	if(blendfp)
	{
		if(key) // color key, should be slower
		{
		    // blend
			do{
				i=cw-1;
				do
				{
				   if(!sp[i])continue;
				   dp[i] = blendfp(remap[sp[i]], dp[i]);
				}while(i--);
				sp += sw;
				dp += dw;
			}while(--ch);
		}
		else // without colorkey
		{
		    // blend
			do{
				i=cw-1;
				do
				{
				   dp[i] = blendfp(remap[sp[i]], dp[i]);
				}while(i--);
				sp += sw;
				dp += dw;
			}while(--ch);
		}
	}
	else //without blend
	{
		if(key) // with color key
		{
			// Copy data
			do{
				i=cw-1;
				do
				{
				   if(!sp[i])continue;
				   dp[i] = remap[sp[i]];
				}while(i--);
				sp += sw;
				dp += dw;
			}while(--ch);
		}
		else // without colorkey
		{
			// Copy data
			do{
				//u32pcpy(dp, sp, remap, cw);
				i=cw-1;
				do
				{
				   dp[i] = remap[sp[i]];
				}while(i--);
				sp += sw;
				dp += dw;
			}while(--ch);
		}
	}
}

//32 to 32
void blendscreen32(s_screen * dest, s_screen * src, int x, int y, int key, u32(*blendfp)(u32, u32))
{
	u32 *sp = (u32*)src->data;
	u32 *dp = (u32*)dest->data;
	int i;
	int sw = src->width;
	int sh = src->height;
	int dw = dest->width;
	int cw = sw, ch = sh;
	int sox, soy;
	int xmin=useclip?clipx1:0,
		xmax=useclip?clipx2:dest->width,
		ymin=useclip?clipy1:0,
		ymax=useclip?clipy2:dest->height;

	// Copy anything at all?
	if(x >= xmax) return;
	if(sw+x <= xmin) return;
	if(y >= ymax) return;
	if(sh+y <= ymin) return;

	sox = 0;
	soy = 0;

	// Clip?
	if(x<xmin) {sox = xmin-x; cw -= sox;}
	if(y<ymin) {soy = ymin-y; ch -= soy;}

	if(x+sw > xmax) cw -= (x+sw) - xmax;
	if(y+sh > ymax) ch -= (y+sh) - ymax;

	if(x<xmin) x = xmin;
	if(y<ymin) y = ymin;

	sp += (soy*sw + sox);
	dp += (y*dw + x);

	if(blendfp)
	{
		if(key) // with colour key
		{
			// Copy data
			do{
				i=cw-1;
				do
				{
				   if(sp[i]==0) continue;
				   dp[i] = blendfp(sp[i], dp[i]);
				}while(i--);
				sp += sw;
				dp += dw;
			}while(--ch);
		}
		else //without colour key
		{
			// Copy data
			do{
				i=cw-1;
				do
				{
				   dp[i] = blendfp(sp[i], dp[i]);
				}while(i--);
				sp += sw;
				dp += dw;
			}while(--ch);
		}
	}
	else // without blend
	{
		if(key) // with colour key
		{
			// Copy data
			do{
				i=cw-1;
				do
				{
				   if(sp[i]==0) continue;
				   dp[i] = sp[i];
				}while(i--);
				sp += sw;
				dp += dw;
			}while(--ch);
		}
		else //without colour key
		{
			// Copy data
			do{
				memcpy(dp, sp, cw<<2);
				sp += sw;
				dp += dw;
			}while(--ch);
		}
	}

}

// Scale screen
void scalescreen32(s_screen * dest, s_screen * src)
{
	int sw, sh;
	int dw, dh;
	int dx, dy;
	u32 *sp;
	u32 *dp;
	u32 *lineptr;
	unsigned int xstep, ystep, xpos, ypos;
	int pixelformat = src->pixelformat;

	//if(dest->pixelformat!=pixelformat || pixelformat!=PIXEL_16) return;
	if(dest->pixelformat!=pixelformat) return;

	if(src==NULL || dest==NULL) return;
	sp = (u32*)src->data;
	dp = (u32*)dest->data;

	sw = src->width;
	sh = src->height;
	dw = dest->width;
	dh = dest->height;

	xstep = (sw<<16) / dw;
	ystep = (sh<<16) / dh;

	ypos = 0;
	for(dy=0; dy<dh; dy++)
	{
		lineptr = sp + ((ypos>>16) * sw);
		ypos += ystep;
		xpos = 0;
		for(dx=0; dx<dw; dx++)
		{
			*dp = lineptr[xpos>>16];
			++dp;
			xpos += xstep;
		}
	}
}
