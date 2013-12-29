/*
 * OpenBOR - http://www.chronocrash.com
 * -----------------------------------------------------------------------
 * All rights reserved, see LICENSE in OpenBOR root for details.
 *
 * Copyright (c) 2004 - 2014 OpenBOR Team
 */

#include "gfx.h"

extern void Super2xSaI     (u8 *srcPtr, u32 srcPitch, u8 *deltaPtr, u8 *dstPtr, u32 dstPitch, int width, int height);
extern void Super2xSaI32   (u8 *srcPtr, u32 srcPitch, u8 *deltaPtr, u8 *dstPtr, u32 dstPitch, int width, int height);
extern void SuperEagle     (u8 *srcPtr, u32 srcPitch, u8 *deltaPtr, u8 *dstPtr, u32 dstPitch, int width, int height);
extern void SuperEagle32   (u8 *srcPtr, u32 srcPitch, u8 *deltaPtr, u8 *dstPtr, u32 dstPitch, int width, int height);
extern void _2xSaI         (u8 *srcPtr, u32 srcPitch, u8 *deltaPtr, u8 *dstPtr, u32 dstPitch, int width, int height);
extern void _2xSaI32       (u8 *srcPtr, u32 srcPitch, u8 *deltaPtr, u8 *dstPtr, u32 dstPitch, int width, int height);
extern void AdMame2x       (u8 *srcPtr, u32 srcPitch, u8 *deltaPtr, u8 *dstPtr, u32 dstPitch, int width, int height);
extern void AdMame2x32     (u8 *srcPtr, u32 srcPitch, u8 *deltaPtr, u8 *dstPtr, u32 dstPitch, int width, int height);
extern void Hq2x           (u8 *srcPtr, u32 srcPitch, u8 *deltaPtr, u8 *dstPtr, u32 dstPitch, int width, int height);
extern void Hq2x32         (u8 *srcPtr, u32 srcPitch, u8 *deltaPtr, u8 *dstPtr, u32 dstPitch, int width, int height);
extern void Lq2x           (u8 *srcPtr, u32 srcPitch, u8 *deltaPtr, u8 *dstPtr, u32 dstPitch, int width, int height);
extern void Lq2x32         (u8 *srcPtr, u32 srcPitch, u8 *deltaPtr, u8 *dstPtr, u32 dstPitch, int width, int height);
extern void Scanlines      (u8 *srcPtr, u32 srcPitch, u8 *deltaPtr, u8 *dstPtr, u32 dstPitch, int width, int height);
extern void Scanlines32    (u8 *srcPtr, u32 srcPitch, u8 *deltaPtr, u8 *dstPtr, u32 dstPitch, int width, int height);
extern void ScanlinesTV    (u8 *srcPtr, u32 srcPitch, u8 *deltaPtr, u8 *dstPtr, u32 dstPitch, int width, int height);
extern void ScanlinesTV32  (u8 *srcPtr, u32 srcPitch, u8 *deltaPtr, u8 *dstPtr, u32 dstPitch, int width, int height);
extern void Simple2x       (u8 *srcPtr, u32 srcPitch, u8 *deltaPtr, u8 *dstPtr, u32 dstPitch, int width, int height);
extern void Simple2x32     (u8 *srcPtr, u32 srcPitch, u8 *deltaPtr, u8 *dstPtr, u32 dstPitch, int width, int height);
extern void Bilinear       (u8 *srcPtr, u32 srcPitch, u8 *deltaPtr, u8 *dstPtr, u32 dstPitch, int width, int height);
extern void Bilinear32     (u8 *srcPtr, u32 srcPitch, u8 *deltaPtr, u8 *dstPtr, u32 dstPitch, int width, int height);
extern void Tv2x           (u8 *srcPtr, u32 srcPitch, u8 *deltaPtr, u8 *dstPtr, u32 dstPitch, int width, int height);
extern void DotMatrix      (u8 *srcPtr, u32 srcPitch, u8 *deltaPtr, u8 *dstPtr, u32 dstPitch, int width, int height);

char *GfxBlitterNames[(BLITTER_MAX * 2) + 1] =
{
#define BIT_NAMES(x) \
	x" Simple 2x",      \
	x" Bilinear",       \
	x" 2xSaI",          \
	x" Super 2xSaI",    \
	x" Super Eagle",    \
	x" Advance Mame2x", \
	x" Lq2x",           \
	x" Hq2x",           \
	x" ScanLines",      \
	x" ScanLines TV",   \
	x" TV 2x",          \
	x" Dot Matrix",
    BIT_NAMES("16-Bit")
    BIT_NAMES("32-Bit")
    "Unknown"
};

GfxBlitterTypes GfxBlitters[BLITTER_MAX * 2] =
{
    Simple2x,
    Bilinear,
    _2xSaI,
    Super2xSaI,
    SuperEagle,
    AdMame2x,
    Lq2x,
    Hq2x,
    Scanlines,
    ScanlinesTV,
    Tv2x,
    DotMatrix,
    Simple2x32,
    Bilinear,
    _2xSaI32,
    Super2xSaI32,
    SuperEagle32,
    AdMame2x32,
    Lq2x32,
    Hq2x32,
    Scanlines32,
    ScanlinesTV,
    Tv2x,
    DotMatrix,
};
