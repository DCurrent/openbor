/*
 * OpenBOR - http://www.ChronoCrash.com
 * -----------------------------------------------------------------------
 * Licensed under the BSD license, see LICENSE in OpenBOR root for details.
 *
 * Copyright (c) 2004 - 2015 OpenBOR Team
 */

#ifndef VIDEO_H
#define VIDEO_H

#include "types.h"
#include "yuv.h"
#include <gctypes.h>

void  video_init();
int   video_set_mode(s_videomodes);
int   video_copy_screen(s_screen*);
void  video_clearscreen();
void  video_draw_quad(int,int,int,int);
void  video_stretch(int);
void  video_set_color_correction(int,int);
void  video_exit();

void  video_swizzle_simple(const void*,void*,int,int);
void  copyscreen32(s_screen*);

// for WebM video playback
int video_setup_yuv_overlay(const yuv_video_mode*);
int video_prepare_yuv_frame(yuv_frame*);
int video_display_yuv_frame(void);

#endif

