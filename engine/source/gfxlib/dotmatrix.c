/*
 * OpenBOR - http://www.LavaLit.com
 * -----------------------------------------------------------------------
 * All rights reserved, see LICENSE in OpenBOR root for details.
 *
 * Copyright (c) 2004 - 2011 OpenBOR Team
 */

#include "gfxtypes.h"

static inline u16 DOT_16(u16 c, int j, int i)
{
    static const u16 dotmatrix[16] =
    {
        0x01E0, 0x0007, 0x3800, 0x0000,
        0x39E7, 0x0000, 0x39E7, 0x0000,
        0x3800, 0x0000, 0x01E0, 0x0007,
        0x39E7, 0x0000, 0x39E7, 0x0000
    };
    return c - ((c >> 2) & *(dotmatrix + ((j & 3) << 2) + (i & 3)));
}

void DotMatrix(u8 *srcPtr, u32 srcPitch, u8 *deltaPtr, u8 *dstPtr, u32 dstPitch, int width, int height)
{
    u32 nextlineSrc = srcPitch / sizeof(u16);
    u32 nextlineDst = dstPitch / sizeof(u16);
    u16 *p = (u16 *)srcPtr;
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
