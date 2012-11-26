/*
 * OpenBOR - http://www.LavaLit.com
 * -----------------------------------------------------------------------
 * All rights reserved, see LICENSE in OpenBOR root for details.
 *
 * Copyright (c) 2004 - 2012 OpenBOR Team
 */

/**
 * Scales images by 2x using bilinear filtering.  Replaces the old bilinear
 * filter used in OpenBOR before November 2012, which was licensed under the
 * GPL.
 */

#include "gfx.h"
#include "types.h"

#ifdef MMX
extern void _BilinearMMX(u8 *srcPtr, u32 srcPitch, u8 *dstPtr, u32 dstPitch, int width, int height);
#endif

static inline u16 blend16_2(u16 a, u16 b)
{
	return ((a & GfxColorMask)>>1) + ((b & GfxColorMask)>>1);
}

static inline u16 blend16_4(u16 a, u16 b, u16 c, u16 d)
{
	return blend16_2(blend16_2(a, b), blend16_2(c, d));
}

void Bilinear(u8 *srcPtr, u32 srcPitch, u8 *deltaPtr, u8 *dstPtr, u32 dstPitch, int width, int height)
{
#ifdef MMX
	if (GetMMX())
	{
		_BilinearMMX(srcPtr, srcPitch, dstPtr, dstPitch, width, height);
		return;
	}
#endif
	
	while (height--)
	{
		u16 *src0 = (u16*)srcPtr, *src1 = (u16*)(srcPtr+srcPitch);
		u16 *dst0 = (u16*)dstPtr, *dst1 = (u16*)(dstPtr+dstPitch);
		u16 pix00, pix01 = *src0++, x;
		u16 pix10, pix11 = *src1++;
		for(x=width; x; x--)
		{
			pix00 = pix01;
			pix01 = *src0++;
			pix10 = pix11;
			pix11 = *src1++;
			*dst0++ = pix00;
			*dst0++ = blend16_2(pix00, pix01);
			*dst1++ = blend16_2(pix00, pix10);
			*dst1++ = blend16_4(pix00, pix01, pix10, pix11);
		}
		srcPtr += srcPitch;
		dstPtr += (dstPitch << 1);
	}
}

