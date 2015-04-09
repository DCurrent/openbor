/*
 * OpenBOR - http://www.chronocrash.com
 * -----------------------------------------------------------------------
 * All rights reserved, see LICENSE in OpenBOR root for details.
 *
 * Copyright (c) 2004 - 2014 OpenBOR Team
 */

#ifndef BOR_OPENGL_H
#define BOR_OPENGL_H

#include "types.h"
#include "yuv.h"
#include "videocommon.h"

#ifdef OPENGL

// OpenGL video functions
int video_gl_set_mode(s_videomodes);
int video_gl_copy_screen(s_videosurface*);
void video_gl_clearscreen();
void video_gl_set_color_correction(int, int);

// for WebM video playback
int video_gl_setup_yuv_overlay(int, int, int, int);
int video_gl_prepare_yuv_frame(yuv_frame*);
int video_gl_display_yuv_frame(void);

#else

// define dummy macros for OpenGL video functions if OpenGL is not supported
#define video_gl_set_mode(X)					0
#define video_gl_copy_screen(X)					0
#define video_gl_clearscreen()
#define video_gl_set_color_correction(X,Y)
#define video_gl_setup_yuv_overlay(X,Y,Z,W)     0
#define video_gl_prepare_yuv_frame(X)           0
#define video_gl_display_yuv_frame()            0

#endif
#endif

