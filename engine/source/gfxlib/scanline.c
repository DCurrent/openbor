/*
 * OpenBOR - http://www.LavaLit.com
 * -----------------------------------------------------------------------
 * All rights reserved, see LICENSE in OpenBOR root for details.
 *
 * Copyright (c) 2004 - 2012 OpenBOR Team
 */

/**
 * Scales images by 2x while emulating scanlines.  The "Scanlines" function
 * is nearest neighbor filtering with every other line blacked out.  The 
 * "ScanlinesTV" function more closely emulates the scanlines on an actual
 * interlaced TV.  Replaces the old "Scanline 2x" and "Scanline TV 2x" filters
 * licensed under the GPL that were used in OpenBOR before October 2012.
 */

#include <string.h>
#include "gfx.h"
#include "types.h"

void Scanlines(u8 *srcPtr, u32 srcPitch, u8 *deltaPtr, u8 *dstPtr, u32 dstPitch, int width, int height)
{
	u16 *src;
	u16 *dst;
	int x, y;
	
	for (y=0; y<height; y++)
	{
		src = (u16*)srcPtr;
		dst = (u16*)dstPtr;
		for(x=0; x<width; x++)
		{
			*dst++ = *src;
			*dst++ = *src++;
		}
		srcPtr += srcPitch;
		dstPtr += dstPitch;
		memset(dstPtr, 0, dstPitch);
		dstPtr += dstPitch;
	}
}

void Scanlines32(u8 *srcPtr, u32 srcPitch, u8 *deltaPtr, u8 *dstPtr, u32 dstPitch, int width, int height)
{
	u32 *src;
	u32 *dst;
	int x, y;
	
	for (y=0; y<height; y++)
	{
		src = (u32*)srcPtr;
		dst = (u32*)dstPtr;
		for(x=0; x<width; x++)
		{
			*dst++ = *src;
			*dst++ = *src++;
		}
		srcPtr += srcPitch;
		dstPtr += dstPitch;
		memset(dstPtr, 0, dstPitch);
		dstPtr += dstPitch;
	}
}

void ScanlinesTV(u8 *srcPtr, u32 srcPitch, u8 *deltaPtr, u8 *dstPtr, u32 dstPitch, int width, int height)
{
	u16 *src;
	u32 *dstTop, *dstBottom;
	int x, y;
	
	for (y=0; y<height; y++)
	{
		src = (u16*)srcPtr;
		dstTop = (u32*)dstPtr;
		dstBottom = (u32*)(dstPtr + dstPitch);
		for(x=0; x<width; x++)
		{
			// look at the source pixel and the one to its right
			u16 a = *src, b = *(++src);
			u16 bhalf = (b & GfxColorMask) >> 1;
			u32 result;
			*dstTop++ = result = a | (((a & GfxColorMask) >> 1) + bhalf) << 16;
			*dstBottom++ = ((result & GfxQColorMask) >> 2) * 3;
		}
		dstPtr += dstPitch << 1;
		srcPtr += srcPitch;
	}
}

