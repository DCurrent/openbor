/*
 * OpenBOR - http://www.LavaLit.com
 * -----------------------------------------------------------------------
 * Licensed under the BSD license, see LICENSE in OpenBOR root for details.
 *
 * Copyright (c) 2004 - 2011 OpenBOR Team
 */

#ifndef VIDEO_H
#define VIDEO_H

#include "gfxtypes.h"
#include "types.h"
#include "SDL.h"

extern u8 pDeltaBuffer[480 * 2592];
extern int opengl;

// Frees all VESA shit when returning to textmode
int video_set_mode(s_videomodes);
int video_copy_screen(s_screen*);
void video_clearscreen();
void video_fullscreen_flip();
void video_stretch(int);

#endif

