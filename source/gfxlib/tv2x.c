/*
 * OpenBOR - http://www.chronocrash.com
 * -----------------------------------------------------------------------
 * All rights reserved, see LICENSE in OpenBOR root for details.
 *
 * Copyright (c) 2004 - 2014 OpenBOR Team
 */

#include "gfxtypes.h"

void Tv2x(u8 *srcPtr, u32 srcPitch, u8 *deltaPtr, u8 *dstPtr, u32 dstPitch, int width, int height)
{
    u32 nextlineSrc = srcPitch / sizeof(u16);
    u32 nextlineDst = dstPitch / sizeof(u16);
    u16 *p = (u16 *)srcPtr;
    u16 *q = (u16 *)dstPtr;

    while(height--)
    {
        int i = 0, j = 0;
        for(; i < width; ++i, j += 2)
        {
            u16 p1 = *(p + i);
            u32 pi;
            pi = (((p1 & GfxRedBlueMask) * 7) >> 3) & GfxRedBlueMask;
            pi |= (((p1 & GfxGreenMask) * 7) >> 3) & GfxGreenMask;
            *(q + j) = p1;
            *(q + j + 1) = p1;
            *(q + j + nextlineDst) = pi;
            *(q + j + nextlineDst + 1) = pi;
        }
        p += nextlineSrc;
        q += nextlineDst << 1;
    }
}
