/*
 * OpenBOR - http://www.chronocrash.com
 * -----------------------------------------------------------------------
 * All rights reserved, see LICENSE in OpenBOR root for details.
 *
 * Copyright (c)  OpenBOR Team
 */

#include "gfx.h"

u32 GfxColorMask = 0xF7DEF7DE;
u32 GfxQColorMask = 0xE79CE79C;
u32 GfxRedBlueMask = 0xF81F;
u32 GfxGreenMask = 0x7E0;

extern void AdMame2x       (u8 *srcPtr, u32 srcPitch, u8 *deltaPtr, u8 *dstPtr, u32 dstPitch, int width, int height);
extern void AdMame2x32     (u8 *srcPtr, u32 srcPitch, u8 *deltaPtr, u8 *dstPtr, u32 dstPitch, int width, int height);
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
	x" Advance Mame2x", \
	x" Scanlines",      \
	x" Scanlines TV",   \
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
    AdMame2x,
    Scanlines,
    ScanlinesTV,
    Tv2x,
    DotMatrix,
    Simple2x32,
    Bilinear,
    AdMame2x32,
    Scanlines32,
    ScanlinesTV,
    Tv2x,
    DotMatrix,
};
