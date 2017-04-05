/*
 * OpenBOR - http://www.chronocrash.com
 * -----------------------------------------------------------------------
 * All rights reserved, see LICENSE in OpenBOR root for details.
 *
 * Copyright (c) 2004 - 2014 OpenBOR Team
 */

#ifndef __FILTERS_H__
#define __FILTERS_H__

#include "gfxtypes.h"

extern void filter_tv2x      (u8 *, u32, u8 *, u8 *, u32, int, int);
extern void filter_normal2x  (u8 *, u32, u8 *, u8 *, u32, int, int);
extern void filter_dotmatrix (u8 *, u32, u8 *, u8 *, u32, int, int);
extern void filter_bicubic   (u8 *, u32, u8 *, u8 *, u32, int, int);

#endif
