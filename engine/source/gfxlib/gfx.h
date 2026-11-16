/*
 * OpenBOR - http://www.chronocrash.com
 * -----------------------------------------------------------------------
 * All rights reserved, see LICENSE in OpenBOR root for details.
 *
 * Copyright (c)  OpenBOR Team
 */

#ifndef __GFX_H__
#define __GFX_H__

/* Stable savedata selectors for the GPU output-filter pass. */
#define  BLITTER_NONE	      0
#define  BLITTER_BILINEAR	  1
#define  BLITTER_ADMAME2X	  2
#define  BLITTER_SCANLINES	  3
#define  BLITTER_SCANLINESTV  4
#define  BLITTER_TV2X         5
#define  BLITTER_DOTMATRIX    6
#define  BLITTER_MAX          7

extern char *const GfxBlitterNames[BLITTER_MAX];

#endif
