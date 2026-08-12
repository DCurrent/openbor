/*
 * OpenBOR - http://www.chronocrash.com
 * -----------------------------------------------------------------------
 * All rights reserved. See LICENSE in OpenBOR root for license details.
 *
 * Copyright (c) OpenBOR Team
 */

#ifndef WEBM_PLAYBACK_H
#define WEBM_PLAYBACK_H

#include <stdbool.h>
#include <stdint.h>

#include "types.h"
#include "yuv.h"

#define WEBM_PLAYBACK_CHANNEL_COUNT 64U
#define WEBM_PLAYBACK_CHANNEL_AUTO (-1)
#define WEBM_PLAYBACK_SPEED_MIN 0.01
#define WEBM_PLAYBACK_SPEED_MAX 16.0

typedef enum e_webm_display_mode {
    WEBM_DISPLAY_FILL,            /* Zero-value default: scale to destination. */
    WEBM_DISPLAY_CLIP,
    WEBM_DISPLAY_END
} e_webm_display_mode;

typedef enum e_webm_loading_mode {
    WEBM_LOADING_STREAM,          /* Decode through ordinary packfile I/O. */
    WEBM_LOADING_CACHE,           /* Cache the complete WebM before decoding. */
    WEBM_LOADING_END
} e_webm_loading_mode;

struct webm_context;

/*
* Caskey, Damon V.
* 2026-08-12
*
* Stable script-visible WebM playback object. Decoder,
* frame, and clock members remain engine-managed.
*/
typedef struct s_webm_playback {
    e_object_type object_type;
    int index;
    int active;
    int paused;
    int repeat;
    int interrupt;
    int offset_x;
    int offset_y;
    int width;
    int height;
    int sound_channel;
    float speed;
    e_webm_display_mode display_mode;
    e_webm_loading_mode loading_mode;
    uint64_t duration;
    uint64_t position;
    s_screen *screen;

    char *path;
    struct webm_context *context;
    yuv_video_mode video_mode;
    yuv_frame *current_frame;
    yuv_frame *next_frame;
    s_screen *rgb_frame;
    s_screen *scaled_frame;
    uint64_t clock_anchor;
    uint64_t position_anchor;
    int volume;
    int render_dirty;
} s_webm_playback;

bool webm_playback_init(void);
void webm_playback_shutdown(void);
void webm_playback_restore_yuv(void);
s_webm_playback *webm_playback_open(
    const char *path,
    s_screen *screen,
    int video_channel,
    int sound_channel,
    int interrupt,
    e_webm_loading_mode loading_mode,
    int volume
);
s_webm_playback *webm_playback_get(int channel);
int webm_playback_get_index(const s_webm_playback *playback);
bool webm_playback_get_snapshot(
    const s_webm_playback *playback,
    s_webm_playback *snapshot
);
uint64_t webm_playback_get_active_mask(void);
void webm_playback_update(int interrupt_requested);
void webm_playback_stop(s_webm_playback *playback);
void webm_playback_stop_all(void);
void webm_playback_detach_screen(const s_screen *screen);

bool webm_playback_set_display_mode(
    s_webm_playback *playback,
    e_webm_display_mode display_mode
);
bool webm_playback_set_height(s_webm_playback *playback, int height);
bool webm_playback_set_interrupt(s_webm_playback *playback, int interrupt);
bool webm_playback_set_loading_mode(
    s_webm_playback *playback,
    e_webm_loading_mode loading_mode,
    int volume
);
bool webm_playback_set_offset_x(s_webm_playback *playback, int offset_x);
bool webm_playback_set_offset_y(s_webm_playback *playback, int offset_y);
bool webm_playback_set_paused(s_webm_playback *playback, int paused);
bool webm_playback_set_position(
    s_webm_playback *playback,
    uint64_t position,
    int volume
);
bool webm_playback_set_repeat(s_webm_playback *playback, int repeat);
bool webm_playback_set_screen(s_webm_playback *playback, s_screen *screen);
bool webm_playback_set_sound_channel(
    s_webm_playback *playback,
    int sound_channel,
    int volume
);
bool webm_playback_set_speed(s_webm_playback *playback, double speed);
bool webm_playback_set_width(s_webm_playback *playback, int width);

#endif
