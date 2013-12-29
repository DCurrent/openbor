/*
 * OpenBOR - http://www.chronocrash.com
 * -----------------------------------------------------------------------
 * All rights reserved, see LICENSE in OpenBOR root for details.
 *
 * Copyright (c) 2004 - 2014 OpenBOR Team
 */

#ifndef __GFX_H__
#define __GFX_H__

#include "gfxtypes.h"

#define  BLITTER_SIMPLE2X	  0
#define  BLITTER_BILINEAR	  1
#define  BLITTER_2XSAI	  	  2
#define  BLITTER_SUPER2XSAI	  3
#define  BLITTER_SUPEREAGLE	  4
#define  BLITTER_ADMAME2X	  5
#define  BLITTER_LQ2X		  6
#define  BLITTER_HQ2X		  7
#define  BLITTER_SCANLINES	  8
#define  BLITTER_SCANLINESTV  9
#define  BLITTER_TV2X         10
#define  BLITTER_DOTMATRIX    11
#define  BLITTER_MAX          12

typedef void (*GfxBlitterTypes)(u8 *srcPtr, u32 srcPitch, u8 *deltaPtr, u8 *dstPtr, u32 dstPitch, int width, int height);
extern GfxBlitterTypes GfxBlitters[BLITTER_MAX * 2];
extern char *GfxBlitterNames[(BLITTER_MAX * 2) + 1];

extern bool GetMMX   ();
extern int  Init_Gfx (u32 BitFormat, u32 ColorDepth);
extern void Term_Gfx ();

#endif
