/*
 * OpenBOR - http://www.chronocrash.com
 * -----------------------------------------------------------------------
 * All rights reserved, see LICENSE in OpenBOR root for details.
 *
 * Copyright (c)  OpenBOR Team
 */

#ifndef YUV_H
#define YUV_H

#include "types.h"

typedef struct {
    u64 timestamp;
    int width;
    int height;
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

#endif
