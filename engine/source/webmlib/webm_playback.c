/*
 * OpenBOR - http://www.chronocrash.com
 * -----------------------------------------------------------------------
 * All rights reserved. See LICENSE in OpenBOR root for license details.
 *
 * Copyright (c) OpenBOR Team
 */

#include <limits.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "screen.h"
#include "sound_channel.h"
#include "soundmix.h"
#include "timer.h"
#include "vidplay.h"
#include "webm_playback.h"

typedef struct s_webm_playback_pool {
    s_webm_playback channel[WEBM_PLAYBACK_CHANNEL_COUNT];
    uint64_t active_mask;
    int initialized;
    int yuv_initialized;
} s_webm_playback_pool;

static s_webm_playback_pool webm_playback_pool;

/*
* Caskey, Damon V.
* 2026-08-12
*
* Private validation, allocation, and cleanup helpers for
* records retained by the fixed playback bank.
*/
static bool webm_playback_screen_valid(const s_screen *screen)
{
    return screen &&
        screen->magic == screen_magic &&
        screen->pixelformat == PIXEL_32 &&
        screen->width > 0 &&
        screen->height > 0;
}

static bool webm_playback_dimensions_valid(int width, int height)
{
    int allocation_width;

    if(width < 1 || height < 1 || width > INT_MAX - 3) {
        return false;
    }
    allocation_width = (width + 3) & ~3;
    return allocation_width <=
        INT_MAX / height / (int)sizeof(uint32_t);
}

static s_screen *webm_playback_alloc_screen(int width, int height)
{
    s_screen *screen;

    if(!webm_playback_dimensions_valid(width, height)) {
        return NULL;
    }

    screen = allocscreen((width + 3) & ~3, height, PIXEL_32);
    if(screen) {
        /* Retain the logical width while keeping aligned allocation space. */
        screen->width = width;
    }
    return screen;
}

static char *webm_playback_copy_path(const char *path)
{
    char *copy;
    size_t length;

    if(!path || !path[0]) {
        return NULL;
    }

    length = strlen(path) + 1;
    copy = malloc(length);
    if(copy) {
        memcpy(copy, path, length);
    }
    return copy;
}

static void webm_playback_close_media(s_webm_playback *playback)
{
    if(!playback) {
        return;
    }

    if(playback->context) {
        webm_close(playback->context);
        playback->context = NULL;
    }
    yuv_frame_destroy(playback->current_frame);
    yuv_frame_destroy(playback->next_frame);
    playback->current_frame = NULL;
    playback->next_frame = NULL;
    if(playback->rgb_frame) {
        freescreen(&playback->rgb_frame);
    }
    if(playback->scaled_frame) {
        freescreen(&playback->scaled_frame);
    }
}

static void webm_playback_reset_record(s_webm_playback *playback)
{
    e_object_type object_type;
    int index;

    if(!playback) {
        return;
    }

    object_type = playback->object_type;
    index = playback->index;
    webm_playback_close_media(playback);
    free(playback->path);
    memset(playback, 0, sizeof(*playback));
    playback->object_type = object_type;
    playback->index = index;
    playback->sound_channel = SOUND_CHANNEL_MUSIC_DEFAULT;
    playback->speed = 1.0;
    playback->interrupt = 1;
    playback->display_mode = WEBM_DISPLAY_FILL;
    playback->loading_mode = WEBM_LOADING_STREAM;
}

/*
* Caskey, Damon V.
* 2026-08-12
*
* Initialize one stable bank of 64 video objects and
* the shared 32-bit YUV conversion tables.
*/
bool webm_playback_init(void)
{
    unsigned int channel;

    if(webm_playback_pool.initialized) {
        return true;
    }

    memset(&webm_playback_pool, 0, sizeof(webm_playback_pool));
    for(channel = 0; channel < WEBM_PLAYBACK_CHANNEL_COUNT; channel++) {
        webm_playback_pool.channel[channel].object_type = OBJECT_TYPE_WEBM;
        webm_playback_pool.channel[channel].index = (int)channel;
        webm_playback_reset_record(&webm_playback_pool.channel[channel]);
    }

    yuv_init(4);
    webm_playback_pool.yuv_initialized = 1;
    webm_playback_pool.initialized = 1;
    return true;
}

/*
* Caskey, Damon V.
* 2026-08-12
*
* Stop the playback bank and release its shared converter.
*/
void webm_playback_shutdown(void)
{
    if(!webm_playback_pool.initialized) {
        return;
    }

    webm_playback_stop_all();
    if(webm_playback_pool.yuv_initialized) {
        yuv_clear();
    }
    memset(&webm_playback_pool, 0, sizeof(webm_playback_pool));
}

/*
* Caskey, Damon V.
* 2026-08-12
*
* Restore 32-bit conversion after legacy fullscreen playback
* temporarily uses the shared YUV converter in 16-bit mode.
*/
void webm_playback_restore_yuv(void)
{
    if(webm_playback_pool.initialized) {
        yuv_init(4);
        webm_playback_pool.yuv_initialized = 1;
    }
}

/*
* Caskey, Damon V.
* 2026-08-12
*
* Return a stable object from the fixed channel bank.
*/
s_webm_playback *webm_playback_get(int channel)
{
    if(!webm_playback_pool.initialized ||
       channel < 0 ||
       (unsigned int)channel >= WEBM_PLAYBACK_CHANNEL_COUNT) {
        return NULL;
    }
    return &webm_playback_pool.channel[channel];
}

/*
* Caskey, Damon V.
* 2026-08-12
*
* Validate a caller-provided object by exact address and
* stable channel metadata without dereferencing it first.
*/
int webm_playback_get_index(const s_webm_playback *playback)
{
    const s_webm_playback *indexed_playback;
    uintptr_t base_address;
    uintptr_t playback_address;
    uintptr_t playback_offset;
    unsigned int channel;

    if(!webm_playback_pool.initialized || !playback) {
        return -1;
    }

    base_address = (uintptr_t)(const void*)&webm_playback_pool.channel[0];
    playback_address = (uintptr_t)(const void*)playback;
    if(playback_address < base_address) {
        return -1;
    }

    playback_offset = playback_address - base_address;
    if(playback_offset >= sizeof(webm_playback_pool.channel) ||
       playback_offset % sizeof(webm_playback_pool.channel[0])) {
        return -1;
    }

    channel = (unsigned int)(playback_offset / sizeof(webm_playback_pool.channel[0]));
    indexed_playback = &webm_playback_pool.channel[channel];
    if((uintptr_t)(const void*)indexed_playback != playback_address ||
       indexed_playback->object_type != OBJECT_TYPE_WEBM ||
       indexed_playback->index != (int)channel) {
        return -1;
    }

    return (int)channel;
}

/*
* Caskey, Damon V.
* 2026-08-12
*
* Copy stable public state and expose the active channel mask
* without publishing mutable pool internals.
*/
bool webm_playback_get_snapshot(
    const s_webm_playback *playback,
    s_webm_playback *snapshot
)
{
    int channel = webm_playback_get_index(playback);

    if(channel < 0 || !snapshot) {
        return false;
    }
    *snapshot = webm_playback_pool.channel[channel];
    return true;
}

uint64_t webm_playback_get_active_mask(void)
{
    return webm_playback_pool.initialized
        ? webm_playback_pool.active_mask
        : UINT64_C(0);
}

/*
* Caskey, Damon V.
* 2026-08-12
*
* Resolve wall-clock position and open or reopen decoder media
* while retaining the surrounding playback object's settings.
*/
static uint64_t webm_playback_position_now(
    const s_webm_playback *playback,
    uint64_t now
)
{
    double elapsed;
    double position;

    if(playback->paused) {
        return playback->position_anchor;
    }

    elapsed = now >= playback->clock_anchor
        ? (double)(now - playback->clock_anchor) * 1000.0 * playback->speed
        : 0.0;
    position = (double)playback->position_anchor + elapsed;
    if(position >= (double)UINT64_MAX) {
        return UINT64_MAX;
    }
    return (uint64_t)position;
}

static bool webm_playback_open_media(
    s_webm_playback *playback,
    uint64_t position,
    int volume
)
{
    uint64_t duration;
    unsigned int speed_percent;

    webm_playback_close_media(playback);
    playback->volume = volume;
    playback->context = webm_start_playback_ex(
        playback->path,
        volume,
        playback->sound_channel,
        playback->loading_mode == WEBM_LOADING_CACHE,
        position,
        0
    );
    if(!playback->context) {
        return false;
    }

    webm_get_video_info(playback->context, &playback->video_mode);
    duration = webm_get_duration(playback->context);
    playback->duration = duration / UINT64_C(1000000);
    if((playback->video_mode.width & 1) ||
       (playback->video_mode.height & 1) ||
       playback->video_mode.width > (INT_MAX >> 16) ||
       playback->video_mode.height > (INT_MAX >> 16) ||
       !webm_playback_dimensions_valid(
           playback->video_mode.width,
           playback->video_mode.height
       )) {
        printf(
            "Error: WebM frame dimensions %d*%d cannot be represented by the 32-bit screen renderer.\n",
            playback->video_mode.width,
            playback->video_mode.height
        );
        webm_playback_close_media(playback);
        return false;
    }
    playback->rgb_frame = webm_playback_alloc_screen(
        playback->video_mode.width,
        playback->video_mode.height
    );
    if(!playback->rgb_frame) {
        webm_playback_close_media(playback);
        return false;
    }

    speed_percent = (unsigned int)(playback->speed * 100.0 + 0.5);
    if(speed_percent < 1U) {
        speed_percent = 1U;
    }
    webm_set_audio_speed(playback->context, speed_percent);
    if(playback->paused) {
        webm_set_audio_paused(playback->context, 1);
    }

    playback->position_anchor = position;
    playback->clock_anchor = timer_uticks();
    playback->position = position / UINT64_C(1000000);
    playback->render_dirty = 1;
    return true;
}

/*
* Caskey, Damon V.
* 2026-08-12
*
* Acquire the lowest free video channel and return its
* stable object after decoder and destination validation.
*/
s_webm_playback *webm_playback_open(
    const char *path,
    s_screen *screen,
    int sound_channel,
    int interrupt,
    e_webm_loading_mode loading_mode,
    int volume
)
{
    s_webm_playback *playback;
    uint64_t available_mask;
    int channel;

    if(!webm_playback_init() ||
       !path || !path[0] ||
       !webm_playback_screen_valid(screen) ||
       sound_channel < 0 ||
       (unsigned int)sound_channel >= SOUND_CHANNEL_COUNT_MAX ||
       loading_mode < 0 || loading_mode >= WEBM_LOADING_END) {
        return NULL;
    }

    available_mask = ~webm_playback_pool.active_mask;
    channel = sound_channel_mask_first(available_mask);
    if(channel < 0 || (unsigned int)channel >= WEBM_PLAYBACK_CHANNEL_COUNT) {
        return NULL;
    }

    playback = &webm_playback_pool.channel[channel];
    webm_playback_reset_record(playback);
    playback->path = webm_playback_copy_path(path);
    if(!playback->path) {
        return NULL;
    }
    playback->screen = screen;
    playback->sound_channel = sound_channel;
    playback->interrupt = interrupt != 0;
    playback->loading_mode = loading_mode;

    if(!webm_playback_open_media(playback, 0, volume)) {
        webm_playback_reset_record(playback);
        return NULL;
    }

    playback->active = 1;
    webm_playback_pool.active_mask |= UINT64_C(1) << channel;
    return playback;
}

/*
* Caskey, Damon V.
* 2026-08-12
*
* Stop individual, grouped, or screen-detached playback records
* and return their fixed slots to the available mask.
*/
void webm_playback_stop(s_webm_playback *playback)
{
    int channel = webm_playback_get_index(playback);

    if(channel < 0) {
        return;
    }

    webm_playback_pool.active_mask &= ~(UINT64_C(1) << channel);
    webm_playback_reset_record(&webm_playback_pool.channel[channel]);
}

void webm_playback_stop_all(void)
{
    uint64_t active_mask;
    int channel;

    if(!webm_playback_pool.initialized) {
        return;
    }

    active_mask = webm_playback_pool.active_mask;
    while((channel = sound_channel_mask_first(active_mask)) >= 0) {
        active_mask &= ~(UINT64_C(1) << channel);
        webm_playback_stop(&webm_playback_pool.channel[channel]);
    }
}

void webm_playback_detach_screen(const s_screen *screen)
{
    uint64_t active_mask;
    int channel;

    if(!screen || !webm_playback_pool.initialized) {
        return;
    }

    active_mask = webm_playback_pool.active_mask;
    while((channel = sound_channel_mask_first(active_mask)) >= 0) {
        s_webm_playback *playback = &webm_playback_pool.channel[channel];
        active_mask &= ~(UINT64_C(1) << channel);
        if(playback->screen == screen) {
            webm_playback_stop(playback);
        }
    }
}

/*
* Caskey, Damon V.
* 2026-08-12
*
* Prepare scaled output and copy it into a creator-owned screen
* with bounds independent from global draw clipping.
*/
static bool webm_playback_scale_frame(
    s_webm_playback *playback,
    int width,
    int height,
    s_screen **render_frame
)
{
    if(width == playback->rgb_frame->width &&
       height == playback->rgb_frame->height) {
        *render_frame = playback->rgb_frame;
        return true;
    }

    if(!webm_playback_dimensions_valid(width, height)) {
        return false;
    }

    if(playback->scaled_frame &&
       (playback->scaled_frame->width != width ||
        playback->scaled_frame->height != height)) {
        freescreen(&playback->scaled_frame);
    }
    if(!playback->scaled_frame) {
        playback->scaled_frame = webm_playback_alloc_screen(width, height);
    }
    if(!playback->scaled_frame) {
        return false;
    }

    scalescreen32(playback->scaled_frame, playback->rgb_frame);
    *render_frame = playback->scaled_frame;
    return true;
}

/*
* Caskey, Damon V.
* 2026-08-12
*
* Copy a 32-bit video frame with destination clipping
* independent of the engine's current draw-method clip state.
*/
static void webm_playback_copy_frame(
    s_screen *destination,
    const s_screen *source,
    int offset_x,
    int offset_y
)
{
    const unsigned char *source_row;
    unsigned char *destination_row;
    int64_t destination_x = offset_x;
    int64_t destination_y = offset_y;
    int source_x = 0;
    int source_y = 0;
    int width = source->width;
    int height = source->height;
    int row;

    if(destination_x < 0) {
        int64_t clipped_width = -destination_x;
        if(clipped_width >= width) {
            return;
        }
        source_x = (int)clipped_width;
        width -= source_x;
        destination_x = 0;
    }
    if(destination_y < 0) {
        int64_t clipped_height = -destination_y;
        if(clipped_height >= height) {
            return;
        }
        source_y = (int)clipped_height;
        height -= source_y;
        destination_y = 0;
    }
    if(destination_x >= destination->width || destination_y >= destination->height) {
        return;
    }
    if(width > destination->width - destination_x) {
        width = (int)(destination->width - destination_x);
    }
    if(height > destination->height - destination_y) {
        height = (int)(destination->height - destination_y);
    }
    if(width <= 0 || height <= 0) {
        return;
    }

    source_row = source->data +
        ((source_y * source->width + source_x) * (int)sizeof(uint32_t));
    destination_row = destination->data +
        (((int)destination_y * destination->width + (int)destination_x) *
         (int)sizeof(uint32_t));
    for(row = 0; row < height; row++) {
        memcpy(destination_row, source_row, (size_t)width * sizeof(uint32_t));
        source_row += source->width * (int)sizeof(uint32_t);
        destination_row += destination->width * (int)sizeof(uint32_t);
    }
}

static bool webm_playback_render(s_webm_playback *playback)
{
    s_screen *render_frame;
    int width;
    int height;

    if(!playback->current_frame ||
       !playback->rgb_frame ||
       !webm_playback_screen_valid(playback->screen)) {
        return false;
    }

    yuv_to_rgb(playback->current_frame, playback->rgb_frame);
    width = playback->width;
    height = playback->height;
    if(width == 0) {
        width = playback->display_mode == WEBM_DISPLAY_FILL
            ? playback->screen->width
            : (playback->video_mode.display_width > 0
                ? playback->video_mode.display_width
                : playback->video_mode.width);
    }
    if(height == 0) {
        height = playback->display_mode == WEBM_DISPLAY_FILL
            ? playback->screen->height
            : (playback->video_mode.display_height > 0
                ? playback->video_mode.display_height
                : playback->video_mode.height);
    }
    if(width < 1 || height < 1 ||
       !webm_playback_scale_frame(playback, width, height, &render_frame)) {
        return false;
    }

    webm_playback_copy_frame(
        playback->screen,
        render_frame,
        playback->offset_x,
        playback->offset_y
    );
    playback->render_dirty = 0;
    return true;
}

/*
* Caskey, Damon V.
* 2026-08-12
*
* Advance all active objects without blocking on decoder
* queues, render due frames, and recycle terminal channels.
*/
void webm_playback_update(int interrupt_requested)
{
    uint64_t active_mask;
    uint64_t now;
    int channel;

    if(!webm_playback_pool.initialized) {
        return;
    }

    now = timer_uticks();
    active_mask = webm_playback_pool.active_mask;
    while((channel = sound_channel_mask_first(active_mask)) >= 0) {
        s_webm_playback *playback = &webm_playback_pool.channel[channel];
        uint64_t position;
        int terminal = 0;

        active_mask &= ~(UINT64_C(1) << channel);
        if(interrupt_requested && playback->interrupt) {
            webm_playback_stop(playback);
            continue;
        }
        if(!webm_playback_screen_valid(playback->screen)) {
            webm_playback_stop(playback);
            continue;
        }

        position = webm_playback_position_now(playback, now);
        playback->position = position / UINT64_C(1000000);
        if(playback->duration && playback->position > playback->duration) {
            playback->position = playback->duration;
        }

        if(!playback->paused) {
            while(1) {
                int result;

                if(!playback->next_frame) {
                    result = webm_try_get_next_frame(
                        playback->context,
                        &playback->next_frame
                    );
                    if(result == 0) {
                        break;
                    }
                    if(result < 0) {
                        terminal = 1;
                        break;
                    }
                }

                if(playback->next_frame->timestamp > position) {
                    break;
                }

                yuv_frame_destroy(playback->current_frame);
                playback->current_frame = playback->next_frame;
                playback->next_frame = NULL;
                playback->render_dirty = 1;
            }
        }

        if(playback->render_dirty && playback->current_frame &&
           !webm_playback_render(playback)) {
            webm_playback_stop(playback);
            continue;
        }

        if(terminal) {
            if(playback->repeat) {
                if(!webm_playback_open_media(playback, 0, playback->volume)) {
                    webm_playback_stop(playback);
                }
            } else {
                webm_playback_stop(playback);
            }
        }
    }
}

/*
* Caskey, Damon V.
* 2026-08-12
*
* Property mutation helpers preserve decoder, timing, audio,
* scaling, and externally owned screen invariants.
*/
bool webm_playback_set_display_mode(
    s_webm_playback *playback,
    e_webm_display_mode display_mode
)
{
    if(webm_playback_get_index(playback) < 0 ||
       display_mode < 0 || display_mode >= WEBM_DISPLAY_END) {
        return false;
    }
    playback->display_mode = display_mode;
    playback->render_dirty = 1;
    return true;
}

bool webm_playback_set_height(s_webm_playback *playback, int height)
{
    if(webm_playback_get_index(playback) < 0 || height < 0) {
        return false;
    }
    playback->height = height;
    playback->render_dirty = 1;
    return true;
}

bool webm_playback_set_interrupt(s_webm_playback *playback, int interrupt)
{
    if(webm_playback_get_index(playback) < 0) {
        return false;
    }
    playback->interrupt = interrupt != 0;
    return true;
}

bool webm_playback_set_loading_mode(
    s_webm_playback *playback,
    e_webm_loading_mode loading_mode,
    int volume
)
{
    uint64_t position;
    e_webm_loading_mode previous_mode;

    if(webm_playback_get_index(playback) < 0 ||
       !playback->active ||
       loading_mode < 0 || loading_mode >= WEBM_LOADING_END) {
        return false;
    }
    if(playback->loading_mode == loading_mode) {
        return true;
    }

    position = webm_playback_position_now(playback, timer_uticks());
    previous_mode = playback->loading_mode;
    playback->loading_mode = loading_mode;
    if(!webm_playback_open_media(playback, position, volume)) {
        playback->loading_mode = previous_mode;
        webm_playback_stop(playback);
        return false;
    }
    return true;
}

bool webm_playback_set_offset_x(s_webm_playback *playback, int offset_x)
{
    if(webm_playback_get_index(playback) < 0) {
        return false;
    }
    playback->offset_x = offset_x;
    playback->render_dirty = 1;
    return true;
}

bool webm_playback_set_offset_y(s_webm_playback *playback, int offset_y)
{
    if(webm_playback_get_index(playback) < 0) {
        return false;
    }
    playback->offset_y = offset_y;
    playback->render_dirty = 1;
    return true;
}

bool webm_playback_set_paused(s_webm_playback *playback, int paused)
{
    uint64_t now;

    if(webm_playback_get_index(playback) < 0 || !playback->active) {
        return false;
    }
    paused = paused != 0;
    if(playback->paused == paused) {
        return true;
    }

    now = timer_uticks();
    if(paused) {
        playback->position_anchor = webm_playback_position_now(playback, now);
        playback->position = playback->position_anchor / UINT64_C(1000000);
    } else {
        playback->clock_anchor = now;
    }
    playback->paused = paused;
    webm_set_audio_paused(playback->context, paused);
    return true;
}

bool webm_playback_set_position(
    s_webm_playback *playback,
    uint64_t position,
    int volume
)
{
    uint64_t position_ns;

    if(webm_playback_get_index(playback) < 0 || !playback->active) {
        return false;
    }
    if(playback->duration && position >= playback->duration) {
        position = playback->duration - 1;
    }
    if(position > UINT64_MAX / UINT64_C(1000000)) {
        return false;
    }

    position_ns = position * UINT64_C(1000000);
    if(!webm_playback_open_media(playback, position_ns, volume)) {
        webm_playback_stop(playback);
        return false;
    }
    return true;
}

bool webm_playback_set_repeat(s_webm_playback *playback, int repeat)
{
    if(webm_playback_get_index(playback) < 0) {
        return false;
    }
    playback->repeat = repeat != 0;
    return true;
}

bool webm_playback_set_screen(s_webm_playback *playback, s_screen *screen)
{
    if(webm_playback_get_index(playback) < 0 ||
       !webm_playback_screen_valid(screen)) {
        return false;
    }
    playback->screen = screen;
    playback->render_dirty = 1;
    return true;
}

bool webm_playback_set_sound_channel(
    s_webm_playback *playback,
    int sound_channel,
    int volume
)
{
    uint64_t position;
    int previous_channel;

    if(webm_playback_get_index(playback) < 0 ||
       !playback->active ||
       sound_channel < 0 ||
       (unsigned int)sound_channel >= SOUND_CHANNEL_COUNT_MAX) {
        return false;
    }
    if(playback->sound_channel == sound_channel) {
        return true;
    }

    position = webm_playback_position_now(playback, timer_uticks());
    previous_channel = playback->sound_channel;
    playback->sound_channel = sound_channel;
    if(!webm_playback_open_media(playback, position, volume)) {
        playback->sound_channel = previous_channel;
        webm_playback_stop(playback);
        return false;
    }
    return true;
}

bool webm_playback_set_speed(s_webm_playback *playback, double speed)
{
    uint64_t now;
    unsigned int speed_percent;

    if(webm_playback_get_index(playback) < 0 ||
       !playback->active ||
       !isfinite(speed) ||
       speed < WEBM_PLAYBACK_SPEED_MIN ||
       speed > WEBM_PLAYBACK_SPEED_MAX) {
        return false;
    }

    now = timer_uticks();
    playback->position_anchor = webm_playback_position_now(playback, now);
    playback->clock_anchor = now;
    playback->speed = speed;
    speed_percent = (unsigned int)(speed * 100.0 + 0.5);
    if(speed_percent < 1U) {
        speed_percent = 1U;
    }
    webm_set_audio_speed(playback->context, speed_percent);
    return true;
}

bool webm_playback_set_width(s_webm_playback *playback, int width)
{
    if(webm_playback_get_index(playback) < 0 ||
       width < 0) {
        return false;
    }
    playback->width = width;
    playback->render_dirty = 1;
    return true;
}
