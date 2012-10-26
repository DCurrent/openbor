/*
 * OpenBOR - http://www.LavaLit.com
 * -----------------------------------------------------------------------
 * All rights reserved, see LICENSE in OpenBOR root for details.
 *
 * Copyright (c) 2004 - 2012 OpenBOR Team
 */

/**
 * Scales images by 2x using nearest neighbor filtering.  Replaces the old
 * "Simple 2x" filter used in OpenBOR before October 2012, which was licensed
 * under the GPL.
 */

#include "gfx.h"
#include "types.h"

void Simple2x(u8 *srcPtr, u32 srcPitch, u8 *deltaPtr, u8 *dstPtr, u32 dstPitch, int width, int height)
{
	u16 *srcPixels;
	u16 *dstTop, *dstBottom;
	int x, y;
	
	for (y=0; y<height; y++)
	{
		srcPixels = (u16*)(srcPtr + y*srcPitch);
		dstTop = (u16*)dstPtr;
		dstBottom = (u16*)(dstPtr + dstPitch);
		for(x=0; x<width; x++)
		{
			*dstTop++ = *srcPixels;
			*dstTop++ = *srcPixels;
			*dstBottom++ = *srcPixels;
			*dstBottom++ = *srcPixels;
			srcPixels++;
		}
		dstPtr += dstPitch << 1;
	}
}

void Simple2x32(u8 *srcPtr, u32 srcPitch, u8 *deltaPtr, u8 *dstPtr, u32 dstPitch, int width, int height)
{
	u32 *srcPixels;
	u32 *dstTop, *dstBottom;
	int x, y;
	
	for (y=0; y<height; y++)
	{
		srcPixels = (u32*)(srcPtr + y*srcPitch);
		dstTop = (u32*)dstPtr;
		dstBottom = (u32*)(dstPtr + dstPitch);
		for(x=0; x<width; x++)
		{
			*dstTop++ = *srcPixels;
			*dstTop++ = *srcPixels;
			*dstBottom++ = *srcPixels;
			*dstBottom++ = *srcPixels;
			srcPixels++;
		}
		dstPtr += dstPitch << 1;
	}
}

