/*
 * OpenBOR - http://www.LavaLit.com
 * -----------------------------------------------------------------------
 * All rights reserved, see LICENSE in OpenBOR root for details.
 *
 * Copyright (c) 2004 - 2011 OpenBOR Team
 */

#include <math.h>
#include "filters.h"

/* 2X SAI Filter */

#define BLUE_MASK565 0x001F001F
#define RED_MASK565 0xF800F800
#define GREEN_MASK565 0x07E007E0

#define BLUE_MASK555 0x001F001F
#define RED_MASK555 0x7C007C00
#define GREEN_MASK555 0x03E003E0

static u32 colorMask = 0xF7DEF7DE;
static u32 lowPixelMask = 0x08210821;
static u32 qcolorMask = 0xE79CE79C;
static u32 qlowpixelMask = 0x18631863;
static u32 redblueMask = 0xF81F;
static u32 greenMask = 0x7E0;


u32 INTERPOLATE(u32 A, u32 B)
{
	if(A != B) return (((A & colorMask) >> 1) + ((B & colorMask) >> 1) + (A & B & lowPixelMask));
	else return A;
}

u32 Q_INTERPOLATE(u32 A, u32 B, u32 C, u32 D)
{
	register u32 x = ((A & qcolorMask) >> 2) +	((B & qcolorMask) >> 2) + ((C & qcolorMask) >> 2) + ((D & qcolorMask) >> 2);
	register u32 y = (A & qlowpixelMask) +	(B & qlowpixelMask) + (C & qlowpixelMask) + (D & qlowpixelMask);
	y = (y >> 2) & qlowpixelMask;
	return x + y;
}



void filter_tv2x(u8 *srcPtr, u32 srcPitch, u8 *deltaPtr, u8 *dstPtr, u32 dstPitch, int width, int height)
{
	const u32 nextlineSrc = srcPitch / sizeof(u16);
	const u32 nextlineDst = dstPitch / sizeof(u16);
	const u16 *p = (u16 *)srcPtr;
	u16 *q = (u16 *)dstPtr;

	while(height--)
	{
	    int i = 0, j = 0;
		for(; i < width; ++i, j += 2)
		{
			u16 p1 = *(p + i);
		    u32 pi;
			pi = (((p1 & redblueMask) * 7) >> 3) & redblueMask;
			pi |= (((p1 & greenMask) * 7) >> 3) & greenMask;
	        *(q + j) = p1;
		    *(q + j + 1) = p1;
			*(q + j + nextlineDst) = pi;
			*(q + j + nextlineDst + 1) = pi;
	  }
	  p += nextlineSrc;
	  q += nextlineDst << 1;
	}
}

void filter_normal2x(u8 *srcPtr, u32 srcPitch, u8 *deltaPtr, u8 *dstPtr, u32 dstPitch, int width, int height)
{
	const u32 nextlineSrc = srcPitch / sizeof(u16);
	const u32 nextlineDst = dstPitch / sizeof(u16);
	const u16 *p = (u16 *)srcPtr;
	u16 *q = (u16 *)dstPtr;

	while(height--)
	{
	    int i = 0, j = 0;
		for(; i < width; ++i, j += 2)
		{
			u16 color = *(p + i);
			*(q + j) = color;
			*(q + j + 1) = color;
			*(q + j + nextlineDst) = color;
			*(q + j + nextlineDst + 1) = color;
		}
		p += nextlineSrc;
		q += nextlineDst << 1;
	}
}

void filter_scan50(u8 *srcPtr, u32 srcPitch, u8 *deltaPtr, u8 *dstPtr, u32 dstPitch, int width, int height)
{
	const u32 nextlineSrc = srcPitch / sizeof(u16);
	const u32 nextlineDst = dstPitch / sizeof(u16);
	const u16 *p = (u16 *)srcPtr;
	u16 *q = (u16 *)dstPtr;

	while(height--)
	{
		int i = 0, j = 0;
	    for(; i < width; ++i, j += 2)
		{
		    u16 p1 = *(p + i);
		    u16 p2 = *(p + i + nextlineSrc);
	 	    // 0111 1011 1110 1111 == 0x7BEF
		    u16 pm = ((p1 + p2) >> 2) & 0x7BEF;
		    *(q + j) = p1;
	        *(q + j + 1) = p1;
			*(q + j + nextlineDst) = pm;
			*(q + j + nextlineDst + 1) = pm;
		}
		p += nextlineSrc;
		q += nextlineDst << 1;
	}
}

void filter_scan100(u8 *srcPtr, u32 srcPitch, u8 *deltaPtr, u8 *dstPtr, u32 dstPitch, int width, int height)
{
	const u32 nextlineSrc = srcPitch / sizeof(u16);
	const u32 nextlineDst = dstPitch / sizeof(u16);
	const u16 *p = (u16 *)srcPtr;
	u16 *q = (u16 *)dstPtr;

	while(height--)
	{
		int i = 0, j = 0;
	    for(; i < width; ++i, j += 2)
		{
			*(q + j) = *(q + j + 1) = *(p + i);
	    }
		p += nextlineSrc;
		q += nextlineDst << 1;
	}
}

static u16 DOT_16(u16 c, int j, int i)
{
	static const u16 dotmatrix[16] = {
		0x01E0, 0x0007, 0x3800, 0x0000,
		0x39E7, 0x0000, 0x39E7, 0x0000,
		0x3800, 0x0000, 0x01E0, 0x0007,
	    0x39E7, 0x0000, 0x39E7, 0x0000
	};
	return c - ((c >> 2) & *(dotmatrix + ((j & 3) << 2) + (i & 3)));
}

void filter_dotmatrix(u8 *srcPtr, u32 srcPitch, u8 *deltaPtr, u8 *dstPtr, u32 dstPitch, int width, int height)
{
	const u32 nextlineSrc = srcPitch / sizeof(u16);
	const u32 nextlineDst = dstPitch / sizeof(u16);
	const u16 *p = (u16 *)srcPtr;
	u16 *q = (u16 *)dstPtr;
	int i, ii, j, jj;

	for(j = 0, jj = 0; j < height; ++j, jj += 2)
	{
		for(i = 0, ii = 0; i < width; ++i, ii += 2)
		{
			u16 c = *(p + i);
			*(q + ii) = DOT_16(c, jj, ii);
			*(q + ii + 1) = DOT_16(c, jj, ii + 1);
			*(q + ii + nextlineDst) = DOT_16(c, jj + 1, ii);
			*(q + ii + nextlineDst + 1) = DOT_16(c, jj + 1, ii + 1);
		}
		p += nextlineSrc;
		q += nextlineDst << 1;
	}
}

// NEED_OPTIMIZE
static void MULT(u16 c, float* r, float* g, float* b, float alpha)
{
	*r += alpha * ((c & RED_MASK565  ) >> 11);
	*g += alpha * ((c & GREEN_MASK565) >>  5);
	*b += alpha * ((c & BLUE_MASK565 ) >>  0);
}

static inline u16 MAKE_RGB565(float r, float g, float b)
{
	return
		((((unsigned char)r) << 11) & RED_MASK565  ) |
		((((unsigned char)g) <<  5) & GREEN_MASK565) |
		((((unsigned char)b) <<  0) & BLUE_MASK565 );
}

float CUBIC_WEIGHT(float x)
{
	// P(x) = { x, x>0 | 0, x<=0 }
	// P(x + 2) ^ 3 - 4 * P(x + 1) ^ 3 + 6 * P(x) ^ 3 - 4 * P(x - 1) ^ 3
	double r = 0.;
	if(x + 2 > 0) r +=      pow(x + 2, 3);
	if(x + 1 > 0) r += -4 * pow(x + 1, 3);
	if(x     > 0) r +=  6 * pow(x    , 3);
	if(x - 1 > 0) r += -4 * pow(x - 1, 3);
	return (float)r / 6;
}

void filter_bicubic(u8 *srcPtr, u32 srcPitch, u8 *deltaPtr, u8 *dstPtr, u32 dstPitch, int width, int height)
{
	const u32 nextlineSrc = srcPitch / sizeof(u16);
	const u32 nextlineDst = dstPitch / sizeof(u16);
	const u16 *p = (u16 *)srcPtr;
	u16 *q = (u16 *)dstPtr;
	int dx = width << 1, dy = height << 1;
	float fsx = (float)width / dx;
	float fsy = (float)height / dy;
	float v = 0.0f;
	int j = 0;

	for(; j < dy; ++j)
	{
		float u = 0.0f;
		int iv = (int)v;
		float decy = v - iv;
		int i = 0;

		for(; i < dx; ++i)
		{
			int iu = (int)u;
			float decx = u - iu;
			float r, g, b;
			int m;
			r = g = b = 0.;

			for(m = -1; m <= 2; ++m)
			{
				float r1 = CUBIC_WEIGHT(decy - m);
				int n;

				for(n = -1; n <= 2; ++n)
				{
					float r2 = CUBIC_WEIGHT(n - decx);
					const u16* pIn = p + (iu  + n) + (iv + m) * nextlineSrc;
					MULT(*pIn, &r, &g, &b, r1 * r2);
				}
			}
			*(q + i) = MAKE_RGB565(r, g, b);
			u += fsx;
		}
	    q += nextlineDst;
		v += fsy;
	}
}
