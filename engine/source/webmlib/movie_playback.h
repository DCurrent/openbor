/*
 * OpenBOR - http://www.chronocrash.com
 * -----------------------------------------------------------------------
 * All rights reserved. See LICENSE in OpenBOR root for license details.
 *
 * Copyright (c) OpenBOR Team
 */

#ifndef MOVIE_PLAYBACK_H
#define MOVIE_PLAYBACK_H

#include <stdbool.h>
#include <stdint.h>

#include "types.h"
#include "yuv.h"

#define MOVIE_CHANNEL_COUNT 64U
#define MOVIE_CHANNEL_AUTO (-1)
#define MOVIE_SPEED_MIN (-16.0)
#define MOVIE_SPEED_MAX 16.0

typedef enum e_movie_loading_mode {
    MOVIE_LOADING_STREAM,
    MOVIE_LOADING_CACHE,
    MOVIE_LOADING_END
} e_movie_loading_mode;

struct webm_context;

/*
* Caskey, Damon V.
* 2026-08-12
*
* Stable script-visible movie playback object. The source,
* decoder, frame, and clock members remain engine-managed.
*/
typedef struct s_movie_playback {
    e_object_type object_type;
    int index;
    int active;
    int paused;
    int repeat;
    int interrupt;
    int black_filter;
    int offset_x;
    int offset_y;
    int width;
    int height;
    int sound_channel;
    int source_id;
    float speed;
    uint64_t duration;
    uint64_t position;
    s_screen *screen;

    struct webm_context *context;
    yuv_video_mode video_mode;
    yuv_frame *current_frame;
    yuv_frame *next_frame;
    s_screen *rgb_frame;
    s_screen *scaled_frame;
    uint64_t clock_anchor;
    uint64_t position_anchor;
    int volume;
    bool replace_all_audio;
    int frame_dirty;
    int reverse_pending;
    int terminal_pending;
} s_movie_playback;

bool movie_playback_init(void);
void movie_playback_shutdown(void);

int movie_source_load(const char *path, e_movie_loading_mode loading_mode);
bool movie_source_unload(int source_id);

s_movie_playback *movie_playback_play(
    int source_id,
    int movie_channel,
    int volume,
    bool replace_all_audio
);
s_movie_playback *movie_playback_get(int channel);
int movie_playback_get_index(const s_movie_playback *playback);
bool movie_playback_get_snapshot(
    const s_movie_playback *playback,
    s_movie_playback *snapshot
);
uint64_t movie_playback_get_active_mask(void);
void movie_playback_update(int interrupt_requested);
void movie_playback_render_subscreens(void);
bool movie_playback_render_main(s_screen *screen);
void movie_playback_stop(s_movie_playback *playback);
void movie_playback_stop_all(void);
void movie_playback_detach_screen(const s_screen *screen);

bool movie_playback_set_height(s_movie_playback *playback, int height);
bool movie_playback_set_black_filter(
    s_movie_playback *playback,
    int black_filter
);
bool movie_playback_set_interrupt(s_movie_playback *playback, int interrupt);
bool movie_playback_set_offset_x(s_movie_playback *playback, int offset_x);
bool movie_playback_set_offset_y(s_movie_playback *playback, int offset_y);
bool movie_playback_set_paused(s_movie_playback *playback, int paused);
bool movie_playback_set_position(
    s_movie_playback *playback,
    uint64_t position,
    int volume
);
bool movie_playback_set_repeat(s_movie_playback *playback, int repeat);
bool movie_playback_set_screen(s_movie_playback *playback, s_screen *screen);
bool movie_playback_set_sound_channel(
    s_movie_playback *playback,
    int sound_channel,
    int volume
);
bool movie_playback_set_speed(s_movie_playback *playback, double speed);
bool movie_playback_set_width(s_movie_playback *playback, int width);

#endif
