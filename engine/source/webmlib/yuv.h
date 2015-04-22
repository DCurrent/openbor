/*
 * OpenBOR - http://www.chronocrash.com
 * -----------------------------------------------------------------------
 * All rights reserved, see LICENSE in OpenBOR root for details.
 *
 * Copyright (c) 2004 - 2015 OpenBOR Team
 */

#ifndef YUV_H
#define YUV_H

#include "types.h"

typedef struct {
    u64 timestamp;
    void *lum;
    void *cb;
    void *cr;
} yuv_frame;

typedef struct {
    int width;
    int height;
    int display_width;
    int display_height;
} yuv_video_mode;

// Allocates a YUV frame
yuv_frame *yuv_frame_create(int width, int height);

// Frees a YUV frame
void yuv_frame_destroy(yuv_frame *frame);

// Intializes a context for YUV->RGB color conversion; bits should be 16 or 32
void yuv_init(int bits);

// Frees the context allocated for color conversion
void yuv_clear(void);

// Performs YUV->RGB color conversion on a single frame; requires initialized context
void yuv_to_rgb(yuv_frame *in, s_screen *out);

#endif
