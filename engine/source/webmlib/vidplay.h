/*
 * OpenBOR - http://www.chronocrash.com
 * -----------------------------------------------------------------------
 * All rights reserved, see LICENSE in OpenBOR root for details.
 *
 * Copyright (c)  OpenBOR Team
 */

#ifndef VIDPLAY_H
#define VIDPLAY_H

#include <stddef.h>
#include <stdint.h>

#include "yuv.h"

struct webm_context;
typedef struct webm_context webm_context;

/*
* Caskey, Damon V.
* 2026-08-17
*
* Describe each externally observable stage of the asynchronous WebM
* decoder lifecycle. Callers poll these states without joining decoder
* workers or performing container I/O on the engine thread.
*/
typedef enum e_webm_decoder_state {
    WEBM_DECODER_STATE_OPENING,
    WEBM_DECODER_STATE_SEEKING,
    WEBM_DECODER_STATE_PREROLLING,
    WEBM_DECODER_STATE_READY,
    WEBM_DECODER_STATE_FAILED,
    WEBM_DECODER_STATE_CLOSING,
    WEBM_DECODER_STATE_CLOSED
} e_webm_decoder_state;

int webm_lifecycle_init(void);
void webm_lifecycle_shutdown(void);
webm_context *webm_start_playback_ex(
    const char *path,
    int volume,
    int sound_channel,
    const unsigned char *cache_buffer,
    size_t cache_size,
    uint64_t seek_timestamp,
    int play_audio,
    int replace_all_audio
);
e_webm_decoder_state webm_get_decoder_state(webm_context *ctx);
void webm_get_video_info(webm_context *ctx, yuv_video_mode *dims);
uint64_t webm_get_duration(webm_context *ctx);
int webm_get_audio_playback_position(
    webm_context *ctx,
    uint64_t *position
);
void webm_set_audio_paused(webm_context *ctx, int paused);
void webm_set_audio_speed(webm_context *ctx, double speed);
void webm_request_close(webm_context *ctx);
int webm_poll_closed(webm_context *ctx);
void webm_close(webm_context *ctx);
int webm_get_pending_frame_count(webm_context *ctx);
int webm_try_get_next_frame(webm_context *ctx, yuv_frame **frame);

#endif
