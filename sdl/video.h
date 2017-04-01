/*
 * OpenBOR - http://www.chronocrash.com
 * -----------------------------------------------------------------------
 * All rights reserved, see LICENSE in OpenBOR root for details.
 *
 * Copyright (c) 2004 - 2014 OpenBOR Team
 */

#ifndef VIDEO_H
#define VIDEO_H

#include "gfxtypes.h"
#include "types.h"
#include "yuv.h"
#include "SDL.h"

extern u8 pDeltaBuffer[480 * 2592];
extern int opengl;

int SetVideoMode(int, int, int, bool);

// Frees all VESA shit when returning to textmode
int video_set_mode(s_videomodes);
int video_copy_screen(s_screen*);
void video_clearscreen();
void video_fullscreen_flip();
void video_stretch(int);
void video_set_window_title(const char*);
void video_set_color_correction(int, int);

// for WebM video playback
int video_setup_yuv_overlay(const yuv_video_mode*);
int video_prepare_yuv_frame(yuv_frame*);
int video_display_yuv_frame(void);

#endif

