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

#ifdef OPENGL

// OpenGL video functions
int video_gl_set_mode(s_videomodes);
int video_gl_copy_screen(s_screen*);
void video_gl_clearscreen();
void video_gl_fullscreen_flip();
void video_gl_setpalette(unsigned char*);
void video_gl_set_color_correction(int, int);

#else

// define dummy macros for OpenGL video functions if OpenGL is not supported
#define video_gl_set_mode(X)					0
#define video_gl_copy_screen(X)					0
#define video_gl_clearscreen()
#define video_gl_fullscreen_flip()
#define video_gl_setpalette(X)
#define video_gl_set_color_correction(X,Y)

#endif
#endif

