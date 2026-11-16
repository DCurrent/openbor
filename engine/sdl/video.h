/*
 * OpenBOR - http://www.chronocrash.com
 * -----------------------------------------------------------------------
 * All rights reserved, see LICENSE in OpenBOR root for details.
 *
 * Copyright (c)  OpenBOR Team
 */

#ifndef VIDEO_H
#define VIDEO_H

#include "types.h"
#include "gpu.h"
#include "yuv.h"
#if SDL
#include "SDL.h"
#endif

extern int opengl;

int SetVideoMode(int, int);
void FramerateDelay();

// Initialize or release the supported RGB32 presentation mode.
int video_set_mode(s_videomodes);
int video_restore_mode(void);
int video_copy_screen(s_screen*);
void video_clearscreen();
void video_fullscreen_flip();
void video_stretch(int);
void video_set_window_title(const char*);
void video_set_color_correction(int, int);

// for WebM video playback
int video_setup_yuv_overlay(const yuv_video_mode*);
int video_prepare_yuv_frame(const gpu_yuv_source*, e_drawmethod_resample);
int video_capture_yuv_frame(s_screen*, const gpu_yuv_source*, e_drawmethod_resample);
int video_display_yuv_frame(void);

int video_current_refresh_rate();

#endif
