/*
 * OpenBOR - http://www.chronocrash.com
 * -----------------------------------------------------------------------
 * All rights reserved, see LICENSE in OpenBOR root for details.
 *
 * Copyright (c)  OpenBOR Team
 */

#ifndef VIDPLAY_H
#define VIDPLAY_H

#include <stdint.h>

#include "yuv.h"

struct webm_context;
typedef struct webm_context webm_context;

webm_context *webm_start_playback(const char *path, int volume);
webm_context *webm_start_playback_ex(
    const char *path,
    int volume,
    int sound_channel,
    int cache,
    uint64_t seek_timestamp,
    int replace_all_audio
);
void webm_get_video_info(webm_context *ctx, yuv_video_mode *dims);
uint64_t webm_get_duration(webm_context *ctx);
void webm_set_audio_paused(webm_context *ctx, int paused);
void webm_set_audio_speed(webm_context *ctx, unsigned int speed);
void webm_close(webm_context *ctx);
yuv_frame *webm_get_next_frame(webm_context *ctx);
int webm_try_get_next_frame(webm_context *ctx, yuv_frame **frame);

#endif
