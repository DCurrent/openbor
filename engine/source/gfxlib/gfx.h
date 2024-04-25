/*
 * OpenBOR - http://www.chronocrash.com
 * -----------------------------------------------------------------------
 * All rights reserved, see LICENSE in OpenBOR root for details.
 *
 * Copyright (c)  OpenBOR Team
 */

#ifndef __GFX_H__
#define __GFX_H__

#include "gfxtypes.h"

#define  BLITTER_SIMPLE2X	  0
#define  BLITTER_BILINEAR	  1
#define  BLITTER_ADMAME2X	  2
#define  BLITTER_SCANLINES	  3
#define  BLITTER_SCANLINESTV  4
#define  BLITTER_TV2X         5
#define  BLITTER_DOTMATRIX    6
#define  BLITTER_MAX          7

typedef void (*GfxBlitterTypes)(u8 *srcPtr, u32 srcPitch, u8 *deltaPtr, u8 *dstPtr, u32 dstPitch, int width, int height);
extern GfxBlitterTypes GfxBlitters[BLITTER_MAX * 2];
extern char *GfxBlitterNames[(BLITTER_MAX * 2) + 1];

#endif
