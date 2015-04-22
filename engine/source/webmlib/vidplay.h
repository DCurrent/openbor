/*
 * OpenBOR - http://www.chronocrash.com
 * -----------------------------------------------------------------------
 * All rights reserved, see LICENSE in OpenBOR root for details.
 *
 * Copyright (c) 2004 - 2015 OpenBOR Team
 */

#ifndef VIDPLAY_H
#define VIDPLAY_H

#include "yuv.h"

struct webm_context;
typedef struct webm_context webm_context;

webm_context *webm_start_playback(const char *path, int volume);
void webm_get_video_info(webm_context *ctx, yuv_video_mode *dims);
void webm_close(webm_context *ctx);
yuv_frame *webm_get_next_frame(webm_context *ctx);

#endif

