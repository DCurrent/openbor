/*
 * OpenBOR - http://www.chronocrash.com
 * -----------------------------------------------------------------------
 * Licensed under a BSD-style license, see LICENSE in OpenBOR root for details.
 *
 * Copyright (c) 2004 - 2017 OpenBOR Team
 */

#ifndef VIDEO_H
#define VIDEO_H

#include "types.h"

void  video_init(void);
int   video_set_mode(s_videomodes);
int   video_copy_screen(s_screen*);
void  video_clearscreen(void);
void  video_set_color_correction(int,int);
void  video_exit(void);

#if 0
// for WebM video playback
int video_setup_yuv_overlay(const yuv_video_mode*);
int video_prepare_yuv_frame(yuv_frame*);
int video_display_yuv_frame(void);
#endif

#endif

