/*
 * OpenBOR - http://www.chronocrash.com
 * -----------------------------------------------------------------------
 * All rights reserved, see LICENSE in OpenBOR root for details.
 *
 * Copyright (c)  OpenBOR Team
 */

/**
 * This is an implementation of the Scale2x algorithm, also known as
 * AdvanceMAME2x. Before October 2012, OpenBOR contained a version of the
 * Scale2x filter licensed under the GPL. This implementation was written
 * from scratch by Plombo based on the description of the algorithm on the
 * Scale2x website at:
 *     http://scale2x.sourceforge.net/algorithm.html
 */

#include "gfx.h"
#include "types.h"

static inline void scale2x_16_pixel_c(void *src0v, void *src1v, void *src2v, void *dst0v, void *dst1v)
{
    u16 *src0 = src0v, *src1 = src1v, *src2 = src2v, *dst0 = dst0v, *dst1 = dst1v;
    u16 D = *(src0 - 1), E = *src0, F = *(src0 + 1), B = *src1, H = *src2;
    u16 R1, R2, R3, R4;
    if (B != H && D != F)
    {
        R1 = D == B ? D : E;
        R2 = F == B ? F : E;
        R3 = D == H ? D : E;
        R4 = F == H ? F : E;
    }
    else
    {
        R1 = R2 = R3 = R4 = E;
    }

    *dst0 = R1;
    *(dst0 + 1) = R2;
    *dst1 = R3;
    *(dst1 + 1) = R4;
}

static inline void scale2x_32_pixel_c(void *src0v, void *src1v, void *src2v, void *dst0v, void *dst1v)
{
    u32 *src0 = src0v, *src1 = src1v, *src2 = src2v, *dst0 = dst0v, *dst1 = dst1v;
    u32 D = *(src0 - 1), E = *src0, F = *(src0 + 1), B = *src1, H = *src2;
    u32 R1, R2, R3, R4;
    if (B != H && D != F)
    {
        R1 = D == B ? D : E;
        R2 = F == B ? F : E;
        R3 = D == H ? D : E;
        R4 = F == H ? F : E;
    }
    else
    {
        R1 = R2 = R3 = R4 = E;
    }

    *dst0 = R1;
    *(dst0 + 1) = R2;
    *dst1 = R3;
    *(dst1 + 1) = R4;
}

void AdMame2x(u8 *srcPtr, u32 srcPitch, u8 *deltaPtr, u8 *dstPtr, u32 dstPitch, int width, int height)
{
    int x, y;
    for (y = 0; y < height; y++)
    {
        u8 *src0 = srcPtr + srcPitch * y;
        u8 *src1 = y == 0 ? src0 : src0 - srcPitch;
        u8 *src2 = (y == height - 1) ? src0 : src1 + srcPitch;
        u8 *dst0 = dstPtr + dstPitch * y * 2;
        u8 *dst1 = dst0 + dstPitch;

        for (x = 0; x < width; x++)
        {
            scale2x_16_pixel_c(src0 + 2 * x, src1 + 2 * x, src2 + 2 * x, dst0 + 4 * x, dst1 + 4 * x);
        }
    }
}

void AdMame2x32(u8 *srcPtr, u32 srcPitch, u8 *deltaPtr, u8 *dstPtr, u32 dstPitch, int width, int height)
{
    int x, y;
    for (y = 0; y < height; y++)
    {
        u8 *src0 = srcPtr + srcPitch * y;
        u8 *src1 = y == 0 ? src0 : src0 - srcPitch;
        u8 *src2 = (y == height - 1) ? src0 : src1 + srcPitch;
        u8 *dst0 = dstPtr + dstPitch * y * 2;
        u8 *dst1 = dst0 + dstPitch;

        for (x = 0; x < width; x++)
        {
            scale2x_32_pixel_c(src0 + 4 * x, src1 + 4 * x, src2 + 4 * x, dst0 + 8 * x, dst1 + 8 * x);
        }
    }
}
