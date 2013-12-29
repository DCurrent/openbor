/*
 * OpenBOR - http://www.chronocrash.com
 * -----------------------------------------------------------------------
 * All rights reserved, see LICENSE in OpenBOR root for details.
 *
 * Copyright (c) 2004 - 2012 OpenBOR Team
 */

/**
 * This is an implementation of the Scale2x algorithm, also known as
 * AdvanceMAME2x.  Before October 2012, OpenBOR contained a version of the
 * Scale2x filter licensed under the GPL.  Both implementations in this version
 * (C and MMX) were written from scratch by Plombo based on the description of
 * the algorithm on the Scale2x website at:
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

#if MMX
static inline void scale2x_16_pixel_mmx(void *src0, void *src1, void *src2, void *dst0, void *dst1)
{
    __asm__ __volatile__ (
        "# load pixels surrounding input pixel\n"
        "movq -2(%%eax),%%mm0                      # mm0 := D\n"
        "movq 2(%%eax),%%mm1                       # mm1 := F\n"
        "movq (%%edi),%%mm2                        # mm2 := B\n"
        "movq (%%ecx),%%mm3                        # mm3 := H\n"
        "\n"
        "# mm4 := ~((B==H)|(D==F))\n"
        "movq %%mm2,%%mm4\n"
        "pcmpeqw %%mm3,%%mm4\n"
        "movq %%mm0,%%mm5\n"
        "pcmpeqw %%mm1,%%mm5\n"
        "por %%mm5,%%mm4\n"
        "pxor %%mm7,%%mm7\n"
        "pcmpeqw %%mm7,%%mm4\n"
        "\n"
        "# calculate boolean conditions\n"
        "movq %%mm0,%%mm5\n"
        "pcmpeqw %%mm2,%%mm5\n"
        "pand %%mm4,%%mm5                          # mm5 := (D == B) & mm4\n"
        "movq %%mm1,%%mm6\n"
        "pcmpeqw %%mm2,%%mm6\n"
        "pand %%mm4,%%mm6                          # mm6 := (F == B) & mm4\n"
        "movq %%mm0,%%mm7\n"
        "pcmpeqw %%mm3,%%mm7\n"
        "pand %%mm4,%%mm7                          # mm7 := (D == H) & mm4\n"
        "pcmpeqw %%mm1,%%mm3\n"
        "pand %%mm4,%%mm3                          # mm3 := (F == H) & mm4\n"
        "\n"
        "# fetch input pixel E\n"
        "movq (%%eax),%%mm2                        # mm2 := E\n"
        "\n"
        "# calculate output pixel values\n"
        "movq %%mm5,%%mm4\n"
        "pandn %%mm2,%%mm4\n"
        "pand %%mm0,%%mm5\n"
        "por %%mm4,%%mm5                           # mm5 := R0\n"
        "movq %%mm6,%%mm4\n"
        "pandn %%mm2,%%mm4\n"
        "pand %%mm1,%%mm6\n"
        "por %%mm4,%%mm6                           # mm6 := R1\n"
        "movq %%mm7,%%mm4\n"
        "pandn %%mm2,%%mm4\n"
        "pand %%mm0,%%mm7\n"
        "por %%mm4,%%mm7                           # mm7 := R2\n"
        "movq %%mm3,%%mm4\n"
        "pandn %%mm2,%%mm4\n"
        "pand %%mm1,%%mm3\n"
        "por %%mm4,%%mm3                           # mm3 := R3\n"
        "\n"
        "# write the R0 pixels to memory\n"
        "movd %%mm5,%%eax\n"
        "movw %%ax,(%%edx)                         # far left pixel\n"
        "shrl $16,%%eax\n"
        "movw %%ax,4(%%edx)                        # middle left pixel\n"
        "psrlq $32,%%mm5\n"
        "movd %%mm5,%%eax\n"
        "movw %%ax,8(%%edx)                        # middle right pixel\n"
        "shrl $16,%%eax\n"
        "movw %%ax,12(%%edx)                       # far right pixel\n"
        "\n"
        "# write the R1 pixels to memory\n"
        "movd %%mm6,%%eax\n"
        "movw %%ax,2(%%edx)                        # far left pixel\n"
        "shrl $16,%%eax\n"
        "movw %%ax,6(%%edx)                        # middle left pixel\n"
        "psrlq $32,%%mm6\n"
        "movd %%mm6,%%eax\n"
        "movw %%ax,10(%%edx)                       # middle right pixel\n"
        "shrl $16,%%eax\n"
        "movw %%ax,14(%%edx)                       # far right pixel\n"
        "\n"
        "# write the R2 pixels to memory\n"
        "movd %%mm7,%%eax\n"
        "movw %%ax,(%%esi)                         # far left pixel\n"
        "shrl $16,%%eax\n"
        "movw %%ax,4(%%esi)                        # middle left pixel\n"
        "psrlq $32,%%mm7\n"
        "movd %%mm7,%%eax\n"
        "movw %%ax,8(%%esi)                        # middle right pixel\n"
        "shrl $16,%%eax\n"
        "movw %%ax,12(%%esi)                       # far right pixel\n"
        "\n"
        "# write the R3 pixels to memory\n"
        "movd %%mm3,%%eax\n"
        "movw %%ax,2(%%esi)                        # far left pixel\n"
        "shrl $16,%%eax\n"
        "movw %%ax,6(%%esi)                        # middle left pixel\n"
        "psrlq $32,%%mm3\n"
        "movd %%mm3,%%eax\n"
        "movw %%ax,10(%%esi)                       # middle right pixel\n"
        "shrl $16,%%eax\n"
        "movw %%ax,14(%%esi)                       # far right pixel\n"
        : "=a" (src0)
        : "a" (src0), "D" (src1), "c" (src2), "d" (dst0), "S" (dst1)
        : "cc", "memory"
    );
}

static inline void scale2x_32_pixel_mmx(void *src0, void *src1, void *src2, void *dst0, void *dst1)
{
    __asm__ __volatile__ (
        "# load pixels surrounding input pixel\n"
        "movq -4(%%eax),%%mm0                      # mm0 := D\n"
        "movq 4(%%eax),%%mm1                       # mm1 := F\n"
        "movq (%%edi),%%mm2                        # mm2 := B\n"
        "movq (%%ecx),%%mm3                        # mm3 := H\n"
        "\n"
        "# mm4 := ~((B==H)|(D==F))\n"
        "movq %%mm2,%%mm4\n"
        "pcmpeqd %%mm3,%%mm4\n"
        "movq %%mm0,%%mm5\n"
        "pcmpeqd %%mm1,%%mm5\n"
        "por %%mm5,%%mm4\n"
        "pxor %%mm7,%%mm7\n"
        "pcmpeqd %%mm7,%%mm4\n"
        "\n"
        "# calculate boolean conditions\n"
        "movq %%mm0,%%mm5\n"
        "pcmpeqd %%mm2,%%mm5\n"
        "pand %%mm4,%%mm5                          # mm5 := (D == B) & mm4\n"
        "movq %%mm1,%%mm6\n"
        "pcmpeqd %%mm2,%%mm6\n"
        "pand %%mm4,%%mm6                          # mm6 := (F == B) & mm4\n"
        "movq %%mm0,%%mm7\n"
        "pcmpeqd %%mm3,%%mm7\n"
        "pand %%mm4,%%mm7                          # mm7 := (D == H) & mm4\n"
        "pcmpeqd %%mm1,%%mm3\n"
        "pand %%mm4,%%mm3                          # mm3 := (F == H) & mm4\n"
        "\n"
        "# fetch input pixel E\n"
        "movq (%%eax),%%mm2                        # mm2 := E\n"
        "\n"
        "# calculate output pixel values\n"
        "movq %%mm5,%%mm4\n"
        "pandn %%mm2,%%mm4\n"
        "pand %%mm0,%%mm5\n"
        "por %%mm4,%%mm5                           # mm5 := R0\n"
        "movq %%mm6,%%mm4\n"
        "pandn %%mm2,%%mm4\n"
        "pand %%mm1,%%mm6\n"
        "por %%mm4,%%mm6                           # mm6 := R1\n"
        "movq %%mm7,%%mm4\n"
        "pandn %%mm2,%%mm4\n"
        "pand %%mm0,%%mm7\n"
        "por %%mm4,%%mm7                           # mm7 := R2\n"
        "movq %%mm3,%%mm4\n"
        "pandn %%mm2,%%mm4\n"
        "pand %%mm1,%%mm3\n"
        "por %%mm4,%%mm3                           # mm3 := R3\n"
        "\n"
        "# write the R0 pixels to memory\n"
        "movd %%mm5,%%eax\n"
        "movl %%eax,(%%edx)                        # left pixel\n"
        "psrlq $32,%%mm5\n"
        "movd %%mm5,%%eax\n"
        "movl %%eax,8(%%edx)                       # right pixel\n"
        "\n"
        "# write the R1 pixels to memory\n"
        "movd %%mm6,%%eax\n"
        "movl %%eax,4(%%edx)                       # left pixel\n"
        "psrlq $32,%%mm6\n"
        "movd %%mm6,%%eax\n"
        "movl %%eax,12(%%edx)                      # right pixel\n"
        "\n"
        "# write the R2 pixels to memory\n"
        "movd %%mm7,%%eax\n"
        "movl %%eax,(%%esi)                        # left pixel\n"
        "psrlq $32,%%mm7\n"
        "movd %%mm7,%%eax\n"
        "movl %%eax,8(%%esi)                       # right pixel\n"
        "\n"
        "# write the R3 pixels to memory\n"
        "movd %%mm3,%%eax\n"
        "movl %%eax,4(%%esi)                       # left pixel\n"
        "psrlq $32,%%mm3\n"
        "movd %%mm3,%%eax\n"
        "movl %%eax,12(%%esi)                      # right pixel\n"
        : "=a" (src0)
        : "a" (src0), "D" (src1), "c" (src2), "d" (dst0), "S" (dst1)
        : "cc", "memory"
    );
}
#endif

#if MMX
#define scale2x_16_pixel scale2x_16_pixel_mmx
#define scale2x_32_pixel scale2x_32_pixel_mmx
#define increment16 4
#define increment32 2
#else
#define scale2x_16_pixel scale2x_16_pixel_c
#define scale2x_32_pixel scale2x_32_pixel_c
#define increment16 1
#define increment32 1
#endif

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

        for (x = 0; x < width; x += increment16)
        {
            scale2x_16_pixel(src0 + 2 * x, src1 + 2 * x, src2 + 2 * x, dst0 + 4 * x, dst1 + 4 * x);
        }
    }

#if MMX
    // done with MMX instructions, so tell the processor to restore floating-point state
    __asm__ __volatile__ ("emms");
#endif
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

        for (x = 0; x < width; x += increment32)
        {
            scale2x_32_pixel(src0 + 4 * x, src1 + 4 * x, src2 + 4 * x, dst0 + 8 * x, dst1 + 8 * x);
        }
    }
#if MMX
    // done with MMX instructions, so tell the processor to restore floating-point state
    __asm__ __volatile__ ("emms");
#endif
}

