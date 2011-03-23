/*
 * OpenBOR - http://www.LavaLit.com
 * -----------------------------------------------------------------------
 * Licensed under the BSD license, see LICENSE in OpenBOR root for details.
 *
 * Copyright (c) 2004 - 2011 OpenBOR Team
 */

#ifndef VIDEO_H
#define VIDEO_H

#include "types.h"
#include <gctypes.h>

void  video_init();
int   video_set_mode(s_videomodes);
int   video_copy_screen(s_screen*);
void  video_clearscreen();
void  video_draw_quad(int,int,int,int);
void  video_stretch(int);
void  video_swizzle_simple(const void*,void*,int,int);
void  video_exit();

void  copyscreen8(s_screen*);
void  copyscreen16(s_screen*);
void  copyscreen32(s_screen*);

#endif

