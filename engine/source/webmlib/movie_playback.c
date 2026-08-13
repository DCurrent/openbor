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

#include "globals.h"
#include "screen.h"
#include "sound_channel.h"
#include "soundmix.h"
#include "timer.h"
#include "vidplay.h"
#include "movie_playback.h"

#define MOVIE_SOURCE_CAPACITY_INITIAL 64U
#define MOVIE_REVERSE_SEEK_INTERVAL UINT64_C(33333333)

extern int buffer_pakfile(const char *filename, char **pbuffer, size_t *psize);

typedef struct s_movie_source {
    char *path;
    unsigned char *cache_buffer;
    size_t cache_size;
    unsigned int references;
    e_movie_loading_mode loading_mode;
    int active;
    int id;
} s_movie_source;

typedef struct s_movie_playback_pool {
    s_movie_playback channel[MOVIE_CHANNEL_COUNT];
    s_movie_source *source;
    size_t source_capacity;
    int next_source_id;
    uint64_t active_mask;
    int initialized;
    int yuv_initialized;
} s_movie_playback_pool;

static s_movie_playback_pool movie_playback_pool;

/*
* Caskey, Damon V.
* 2026-08-12
*
* Validate screens and allocate exact logical dimensions
* over the engine's four-pixel-aligned screen storage.
*/
static bool movie_playback_screen_valid(const s_screen *screen)
{
    return screen &&
        screen->magic == screen_magic &&
        screen->pixelformat == PIXEL_32 &&
        screen->width > 0 &&
        screen->height > 0;
}

static bool movie_playback_dimensions_valid(int width, int height)
{
    int allocation_width;

    if(width < 1 || height < 1 || width > INT_MAX - 3) {
        return false;
    }
    allocation_width = (width + 3) & ~3;
    return allocation_width <=
        INT_MAX / height / (int)sizeof(uint32_t);
}

static s_screen *movie_playback_alloc_screen(int width, int height)
{
    s_screen *screen;

    if(!movie_playback_dimensions_valid(width, height)) {
        return NULL;
    }

    screen = allocscreen((width + 3) & ~3, height, PIXEL_32);
    if(screen) {
        screen->width = width;
    }
    return screen;
}

static char *movie_copy_path(const char *path)
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

/*
* Caskey, Damon V.
* 2026-08-12
*
* Grow stable integer-addressed movie source storage. IDs are
* never reused during a session, so unloaded IDs stay invalid.
*/
static bool movie_source_reserve(size_t required_capacity)
{
    s_movie_source *source;
    size_t previous_capacity;
    size_t capacity;

    if(required_capacity <= movie_playback_pool.source_capacity) {
        return true;
    }

    previous_capacity = movie_playback_pool.source_capacity;
    capacity = previous_capacity
        ? previous_capacity
        : MOVIE_SOURCE_CAPACITY_INITIAL;
    while(capacity < required_capacity) {
        if(capacity > SIZE_MAX / 2U) {
            return false;
        }
        capacity *= 2U;
    }
    if(capacity > SIZE_MAX / sizeof(*source)) {
        return false;
    }

    source = realloc(
        movie_playback_pool.source,
        capacity * sizeof(*source)
    );
    if(!source) {
        return false;
    }
    memset(
        source + previous_capacity,
        0,
        (capacity - previous_capacity) * sizeof(*source)
    );
    movie_playback_pool.source = source;
    movie_playback_pool.source_capacity = capacity;
    return true;
}

static s_movie_source *movie_source_get(int source_id)
{
    s_movie_source *source;

    if(!movie_playback_pool.initialized ||
       source_id < 0 ||
       (size_t)source_id >= movie_playback_pool.source_capacity) {
        return NULL;
    }

    source = &movie_playback_pool.source[source_id];
    return source->active && source->id == source_id ? source : NULL;
}

static void movie_source_release(int source_id)
{
    s_movie_source *source = movie_source_get(source_id);

    if(source && source->references) {
        --source->references;
    }
}

/*
* Caskey, Damon V.
* 2026-08-12
*
* Close decoder-owned resources separately from the retained
* current frame so seeks can display the old frame until ready.
*/
static void movie_playback_close_decoder(s_movie_playback *playback)
{
    if(playback->context) {
        webm_close(playback->context);
        playback->context = NULL;
    }
    yuv_frame_destroy(playback->next_frame);
    playback->next_frame = NULL;
}

static void movie_playback_close_media(s_movie_playback *playback)
{
    if(!playback) {
        return;
    }

    movie_playback_close_decoder(playback);
    yuv_frame_destroy(playback->current_frame);
    playback->current_frame = NULL;
    if(playback->rgb_frame) {
        freescreen(&playback->rgb_frame);
    }
    if(playback->scaled_frame) {
        freescreen(&playback->scaled_frame);
    }
}

static void movie_playback_reset_record(s_movie_playback *playback)
{
    e_object_type object_type;
    int index;
    int source_id;

    if(!playback) {
        return;
    }

    object_type = playback->object_type;
    index = playback->index;
    source_id = playback->source_id;
    movie_playback_close_media(playback);
    movie_source_release(source_id);
    memset(playback, 0, sizeof(*playback));
    playback->object_type = object_type;
    playback->index = index;
    playback->source_id = -1;
    playback->sound_channel = SOUND_CHANNEL_MUSIC_DEFAULT;
    playback->speed = 1.0;
    playback->interrupt = 1;
}

/*
* Caskey, Damon V.
* 2026-08-12
*
* Initialize one stable bank of 64 movie playback objects,
* dynamic source storage, and shared 32-bit YUV conversion.
*/
bool movie_playback_init(void)
{
    unsigned int channel;

    if(movie_playback_pool.initialized) {
        return true;
    }

    memset(&movie_playback_pool, 0, sizeof(movie_playback_pool));
    movie_playback_pool.initialized = 1;
    for(channel = 0; channel < MOVIE_CHANNEL_COUNT; channel++) {
        movie_playback_pool.channel[channel].object_type =
            OBJECT_TYPE_MOVIE_PLAYBACK;
        movie_playback_pool.channel[channel].index = (int)channel;
        movie_playback_pool.channel[channel].source_id = -1;
        movie_playback_reset_record(&movie_playback_pool.channel[channel]);
    }

    if(!movie_source_reserve(MOVIE_SOURCE_CAPACITY_INITIAL)) {
        memset(&movie_playback_pool, 0, sizeof(movie_playback_pool));
        return false;
    }

    yuv_init(4);
    movie_playback_pool.yuv_initialized = 1;
    return true;
}

/*
* Caskey, Damon V.
* 2026-08-12
*
* Stop every playback, release loaded sources, and clear the
* shared converter during script or engine shutdown.
*/
void movie_playback_shutdown(void)
{
    size_t source_id;

    if(!movie_playback_pool.initialized) {
        return;
    }

    movie_playback_stop_all();
    for(source_id = 0;
        source_id < movie_playback_pool.source_capacity;
        source_id++) {
        free(movie_playback_pool.source[source_id].path);
        free(movie_playback_pool.source[source_id].cache_buffer);
    }
    free(movie_playback_pool.source);
    if(movie_playback_pool.yuv_initialized) {
        yuv_clear();
    }
    memset(&movie_playback_pool, 0, sizeof(movie_playback_pool));
}

/*
* Caskey, Damon V.
* 2026-08-12
*
* Load one reusable movie source. Stream mode validates the
* path; cache mode retains one shared read-only byte buffer.
*/
int movie_source_load(const char *path, e_movie_loading_mode loading_mode)
{
    s_movie_source source = { 0 };
    int handle = -1;
    int source_id;

    if(!movie_playback_init() ||
       !path || !path[0] ||
       loading_mode < 0 || loading_mode >= MOVIE_LOADING_END ||
       movie_playback_pool.next_source_id == INT_MAX) {
        return -1;
    }

    source.path = movie_copy_path(path);
    if(!source.path) {
        return -1;
    }

    if(loading_mode == MOVIE_LOADING_CACHE) {
        if(buffer_pakfile(
            path,
            (char**)&source.cache_buffer,
            &source.cache_size
        ) != 1 || !source.cache_size) {
            free(source.path);
            free(source.cache_buffer);
            return -1;
        }
    } else {
        handle = openpackfile(path, packfile);
        if(handle < 0) {
            free(source.path);
            return -1;
        }
        closepackfile(handle);
    }

    source_id = movie_playback_pool.next_source_id;
    if(!movie_source_reserve((size_t)source_id + 1U)) {
        free(source.path);
        free(source.cache_buffer);
        return -1;
    }

    source.id = source_id;
    source.loading_mode = loading_mode;
    source.active = 1;
    movie_playback_pool.source[source_id] = source;
    ++movie_playback_pool.next_source_id;
    return source_id;
}

/*
* Caskey, Damon V.
* 2026-08-12
*
* Unload an inactive source. Live playbacks retain their
* source until stopped and therefore reject premature unload.
*/
bool movie_source_unload(int source_id)
{
    s_movie_source *source = movie_source_get(source_id);

    if(!source || source->references) {
        return false;
    }

    free(source->path);
    free(source->cache_buffer);
    memset(source, 0, sizeof(*source));
    source->id = -1;
    return true;
}

/*
* Caskey, Damon V.
* 2026-08-12
*
* Validate playback pointers by exact address arithmetic and
* stable channel metadata without dereferencing foreign data.
*/
s_movie_playback *movie_playback_get(int channel)
{
    if(!movie_playback_pool.initialized ||
       channel < 0 ||
       (unsigned int)channel >= MOVIE_CHANNEL_COUNT) {
        return NULL;
    }
    return &movie_playback_pool.channel[channel];
}

int movie_playback_get_index(const s_movie_playback *playback)
{
    const s_movie_playback *indexed_playback;
    uintptr_t base_address;
    uintptr_t playback_address;
    uintptr_t playback_offset;
    unsigned int channel;

    if(!movie_playback_pool.initialized || !playback) {
        return -1;
    }

    base_address = (uintptr_t)(const void*)&movie_playback_pool.channel[0];
    playback_address = (uintptr_t)(const void*)playback;
    if(playback_address < base_address) {
        return -1;
    }

    playback_offset = playback_address - base_address;
    if(playback_offset >= sizeof(movie_playback_pool.channel) ||
       playback_offset % sizeof(movie_playback_pool.channel[0])) {
        return -1;
    }

    channel = (unsigned int)(
        playback_offset / sizeof(movie_playback_pool.channel[0])
    );
    indexed_playback = &movie_playback_pool.channel[channel];
    if((uintptr_t)(const void*)indexed_playback != playback_address ||
       indexed_playback->object_type != OBJECT_TYPE_MOVIE_PLAYBACK ||
       indexed_playback->index != (int)channel) {
        return -1;
    }
    return (int)channel;
}

bool movie_playback_get_snapshot(
    const s_movie_playback *playback,
    s_movie_playback *snapshot
)
{
    int channel = movie_playback_get_index(playback);

    if(channel < 0 || !snapshot) {
        return false;
    }
    *snapshot = movie_playback_pool.channel[channel];
    return true;
}

uint64_t movie_playback_get_active_mask(void)
{
    return movie_playback_pool.initialized
        ? movie_playback_pool.active_mask
        : UINT64_C(0);
}

/*
* Caskey, Damon V.
* 2026-08-12
*
* Resolve signed playback rate into a bounded nanosecond
* position, including stationary and reverse playback.
*/
static uint64_t movie_playback_position_now(
    const s_movie_playback *playback,
    uint64_t now
)
{
    double elapsed;
    double position;

    if(playback->paused || playback->speed == 0.0) {
        return playback->position_anchor;
    }

    elapsed = now >= playback->clock_anchor
        ? (double)(now - playback->clock_anchor) * 1000.0 * playback->speed
        : 0.0;
    position = (double)playback->position_anchor + elapsed;
    if(position <= 0.0) {
        return 0;
    }
    if(position >= (double)UINT64_MAX) {
        return UINT64_MAX;
    }
    return (uint64_t)position;
}

/*
* Caskey, Damon V.
* 2026-08-12
*
* Open or seek the WebM backend from a reusable movie source
* while preserving the last frame until replacement is ready.
*/
static bool movie_playback_open_media(
    s_movie_playback *playback,
    uint64_t position,
    int volume
)
{
    s_movie_source *source = movie_source_get(playback->source_id);
    yuv_video_mode previous_mode = playback->video_mode;
    uint64_t duration;
    unsigned int speed_percent;

    if(!source) {
        return false;
    }

    movie_playback_close_decoder(playback);
    playback->volume = volume;
    playback->context = webm_start_playback_ex(
        source->path,
        volume,
        playback->sound_channel,
        source->cache_buffer,
        source->cache_size,
        position,
        playback->speed > 0.0,
        playback->replace_all_audio
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
       !movie_playback_dimensions_valid(
           playback->video_mode.width,
           playback->video_mode.height
       )) {
        printf(
            "Error: Movie frame dimensions %d*%d cannot be represented by the 32-bit screen renderer.\n",
            playback->video_mode.width,
            playback->video_mode.height
        );
        movie_playback_close_decoder(playback);
        return false;
    }

    if(previous_mode.width != playback->video_mode.width ||
       previous_mode.height != playback->video_mode.height) {
        yuv_frame_destroy(playback->current_frame);
        playback->current_frame = NULL;
        if(playback->rgb_frame) freescreen(&playback->rgb_frame);
        if(playback->scaled_frame) freescreen(&playback->scaled_frame);
    }
    if(!playback->rgb_frame) {
        playback->rgb_frame = movie_playback_alloc_screen(
            playback->video_mode.width,
            playback->video_mode.height
        );
    }
    if(!playback->rgb_frame) {
        movie_playback_close_decoder(playback);
        return false;
    }

    playback->position_anchor = position;
    playback->clock_anchor = timer_uticks();
    playback->position = position / UINT64_C(1000000);
    playback->reverse_pending = playback->speed < 0.0;
    playback->terminal_pending = 0;

    if(playback->speed > 0.0) {
        speed_percent = (unsigned int)(playback->speed * 100.0 + 0.5);
        if(speed_percent < 1U) speed_percent = 1U;
        webm_set_audio_speed(playback->context, speed_percent);
    }
    webm_set_audio_paused(
        playback->context,
        playback->paused || playback->speed <= 0.0
    );
    return true;
}

/*
* Caskey, Damon V.
* 2026-08-12
*
* Create one playback from a reusable source. Automatic mode
* takes the lowest free slot; explicit selection replaces it.
* Legacy callers may request their historical audio takeover.
*/
s_movie_playback *movie_playback_play(
    int source_id,
    int movie_channel,
    int volume,
    bool replace_all_audio
)
{
    s_movie_source *source;
    s_movie_playback *playback;
    uint64_t available_mask;
    int channel;

    if(!movie_playback_init() ||
       !(source = movie_source_get(source_id)) ||
       (movie_channel != MOVIE_CHANNEL_AUTO &&
        (movie_channel < 0 ||
         (unsigned int)movie_channel >= MOVIE_CHANNEL_COUNT))) {
        return NULL;
    }

    if(movie_channel == MOVIE_CHANNEL_AUTO) {
        available_mask = ~movie_playback_pool.active_mask;
        channel = sound_channel_mask_first(available_mask);
        if(channel < 0 || (unsigned int)channel >= MOVIE_CHANNEL_COUNT) {
            return NULL;
        }
    } else {
        channel = movie_channel;
    }

    playback = &movie_playback_pool.channel[channel];
    movie_playback_stop(playback);
    playback->source_id = source_id;
    playback->replace_all_audio = replace_all_audio;
    ++source->references;
    if(!movie_playback_open_media(playback, 0, volume)) {
        movie_playback_reset_record(playback);
        return NULL;
    }

    playback->active = 1;
    movie_playback_pool.active_mask |= UINT64_C(1) << channel;
    return playback;
}

/*
* Caskey, Damon V.
* 2026-08-12
*
* Stop individual, grouped, or screen-detached playbacks and
* release their source references and fixed channel slots.
*/
void movie_playback_stop(s_movie_playback *playback)
{
    int channel = movie_playback_get_index(playback);

    if(channel < 0) {
        return;
    }
    movie_playback_pool.active_mask &= ~(UINT64_C(1) << channel);
    movie_playback_reset_record(&movie_playback_pool.channel[channel]);
}

void movie_playback_stop_all(void)
{
    uint64_t active_mask;
    int channel;

    if(!movie_playback_pool.initialized) {
        return;
    }

    active_mask = movie_playback_pool.active_mask;
    while((channel = sound_channel_mask_first(active_mask)) >= 0) {
        active_mask &= ~(UINT64_C(1) << channel);
        movie_playback_stop(&movie_playback_pool.channel[channel]);
    }
}

void movie_playback_detach_screen(const s_screen *screen)
{
    uint64_t active_mask;
    int channel;

    if(!screen || !movie_playback_pool.initialized) {
        return;
    }

    active_mask = movie_playback_pool.active_mask;
    while((channel = sound_channel_mask_first(active_mask)) >= 0) {
        s_movie_playback *playback = &movie_playback_pool.channel[channel];
        active_mask &= ~(UINT64_C(1) << channel);
        if(playback->screen == screen) {
            movie_playback_stop(playback);
        }
    }
}

/*
* Caskey, Damon V.
* 2026-08-12
*
* Convert and scale a current frame only when its pixels or
* destination dimensions change, then reuse it for composition.
*/
static bool movie_playback_prepare_frame(
    s_movie_playback *playback,
    const s_screen *destination,
    s_screen **render_frame
)
{
    int refresh = playback->frame_dirty;
    int width = playback->width ? playback->width : destination->width;
    int height = playback->height ? playback->height : destination->height;

    if(!playback->current_frame || !playback->rgb_frame) {
        return false;
    }
    if(width < 1 || height < 1 ||
       !movie_playback_dimensions_valid(width, height)) {
        return false;
    }

    if(refresh) {
        size_t pixel_count;
        size_t pixel_index;
        uint32_t *pixel;
        uint32_t protected_black;

        yuv_to_rgb(playback->current_frame, playback->rgb_frame);
        if(playback->black_filter) {
            pixel_count =
                (size_t)playback->rgb_frame->width *
                (size_t)playback->rgb_frame->height;
            pixel = (uint32_t*)playback->rgb_frame->data;
            protected_black = colour32(1, 1, 1);
            for(pixel_index = 0;
                pixel_index < pixel_count;
                pixel_index++) {
                if(pixel[pixel_index] == 0) {
                    pixel[pixel_index] = protected_black;
                }
            }
        }
    }
    if(width == playback->rgb_frame->width &&
       height == playback->rgb_frame->height) {
        playback->frame_dirty = 0;
        *render_frame = playback->rgb_frame;
        return true;
    }

    if(playback->scaled_frame &&
       (playback->scaled_frame->width != width ||
        playback->scaled_frame->height != height)) {
        freescreen(&playback->scaled_frame);
    }
    if(!playback->scaled_frame) {
        playback->scaled_frame = movie_playback_alloc_screen(width, height);
        refresh = 1;
    }
    if(!playback->scaled_frame) {
        return false;
    }
    if(refresh) {
        scalescreen32(playback->scaled_frame, playback->rgb_frame);
    }

    playback->frame_dirty = 0;
    *render_frame = playback->scaled_frame;
    return true;
}

/*
* Caskey, Damon V.
* 2026-08-12
*
* Copy a prepared 32-bit movie frame with signed offsets and
* destination clipping independent of global draw state.
*/
static void movie_playback_copy_frame(
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
        if(clipped_width >= width) return;
        source_x = (int)clipped_width;
        width -= source_x;
        destination_x = 0;
    }
    if(destination_y < 0) {
        int64_t clipped_height = -destination_y;
        if(clipped_height >= height) return;
        source_y = (int)clipped_height;
        height -= source_y;
        destination_y = 0;
    }
    if(destination_x >= destination->width ||
       destination_y >= destination->height) {
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

static bool movie_playback_render_to(
    s_movie_playback *playback,
    s_screen *destination
)
{
    s_screen *render_frame;

    if(!movie_playback_screen_valid(destination)) {
        return false;
    }
    if(!playback->current_frame) {
        return true;
    }
    if(!movie_playback_prepare_frame(
        playback,
        destination,
        &render_frame
    )) {
        return false;
    }

    movie_playback_copy_frame(
        destination,
        render_frame,
        playback->offset_x,
        playback->offset_y
    );
    return true;
}

/*
* Caskey, Damon V.
* 2026-08-12
*
* Poll forward-decoded frames without blocking. Reverse seeks
* retain pending state until reaching the requested timestamp.
*/
static void movie_playback_poll_frames(
    s_movie_playback *playback,
    uint64_t position,
    int *terminal
)
{
    int promoted = 0;

    while(1) {
        int result;

        if(!playback->next_frame) {
            result = webm_try_get_next_frame(
                playback->context,
                &playback->next_frame
            );
            if(result == 0) break;
            if(result < 0) {
                *terminal = 1;
                break;
            }
        }

        if(playback->next_frame->timestamp > position) {
            if((!playback->current_frame || playback->reverse_pending) &&
               !promoted) {
                yuv_frame_destroy(playback->current_frame);
                playback->current_frame = playback->next_frame;
                playback->next_frame = NULL;
                playback->frame_dirty = 1;
                promoted = 1;
            }
            if(playback->reverse_pending) {
                playback->reverse_pending = 0;
            }
            break;
        }

        yuv_frame_destroy(playback->current_frame);
        playback->current_frame = playback->next_frame;
        playback->next_frame = NULL;
        playback->frame_dirty = 1;
        promoted = 1;

        if(playback->speed < 0.0 &&
           playback->current_frame->timestamp == position) {
            playback->reverse_pending = 0;
            break;
        }
    }

    if(*terminal && playback->reverse_pending) {
        playback->reverse_pending = 0;
    }
}

/*
* Caskey, Damon V.
* 2026-08-12
*
* Advance playback clocks and decoder state independently from
* composition. Reverse rates periodically seek and decode forward.
*/
void movie_playback_update(int interrupt_requested)
{
    uint64_t active_mask;
    uint64_t now;
    int channel;

    if(!movie_playback_pool.initialized) {
        return;
    }

    now = timer_uticks();
    active_mask = movie_playback_pool.active_mask;
    while((channel = sound_channel_mask_first(active_mask)) >= 0) {
        s_movie_playback *playback = &movie_playback_pool.channel[channel];
        uint64_t position;
        int terminal = 0;

        active_mask &= ~(UINT64_C(1) << channel);
        if(interrupt_requested && playback->interrupt) {
            movie_playback_stop(playback);
            continue;
        }
        if(playback->terminal_pending && !playback->frame_dirty) {
            if(playback->repeat) {
                if(!movie_playback_open_media(
                    playback,
                    0,
                    playback->volume
                )) {
                    movie_playback_stop(playback);
                }
            } else {
                movie_playback_stop(playback);
            }
            continue;
        }

        position = movie_playback_position_now(playback, now);
        playback->position = position / UINT64_C(1000000);
        if(playback->duration && playback->position > playback->duration) {
            playback->position = playback->duration;
        }

        if(playback->speed < 0.0 &&
           !playback->paused &&
           !playback->reverse_pending &&
           playback->position_anchor >= position &&
           playback->position_anchor - position >=
               MOVIE_REVERSE_SEEK_INTERVAL) {
            if(!movie_playback_open_media(
                playback,
                position,
                playback->volume
            )) {
                movie_playback_stop(playback);
                continue;
            }
        }

        if((playback->speed > 0.0 && !playback->paused) ||
           !playback->current_frame ||
           playback->reverse_pending) {
            movie_playback_poll_frames(playback, position, &terminal);
        }

        if(playback->speed < 0.0 &&
           position == 0 &&
           !playback->reverse_pending) {
            if(playback->repeat && playback->duration) {
                uint64_t repeat_position =
                    (playback->duration - 1U) * UINT64_C(1000000);
                if(!movie_playback_open_media(
                    playback,
                    repeat_position,
                    playback->volume
                )) {
                    movie_playback_stop(playback);
                }
            } else {
                movie_playback_stop(playback);
            }
            continue;
        }

        if(terminal && playback->speed >= 0.0) {
            if(playback->frame_dirty) {
                playback->terminal_pending = 1;
            } else if(playback->repeat) {
                if(!movie_playback_open_media(
                    playback,
                    0,
                    playback->volume
                )) {
                    movie_playback_stop(playback);
                }
            } else {
                movie_playback_stop(playback);
            }
        }
    }
}

/*
* Caskey, Damon V.
* 2026-08-12
*
* Compose bound subscreens before creator scripts, and compose
* unbound playback over the main screen at its presentation stage.
*/
void movie_playback_render_subscreens(void)
{
    uint64_t active_mask = movie_playback_pool.active_mask;
    int channel;

    while((channel = sound_channel_mask_first(active_mask)) >= 0) {
        s_movie_playback *playback = &movie_playback_pool.channel[channel];
        active_mask &= ~(UINT64_C(1) << channel);
        if(playback->screen &&
           !movie_playback_render_to(playback, playback->screen)) {
            movie_playback_stop(playback);
        }
    }
}

bool movie_playback_render_main(s_screen *screen)
{
    uint64_t active_mask;
    int channel;
    bool result = true;

    if(!movie_playback_screen_valid(screen)) {
        return false;
    }

    active_mask = movie_playback_pool.active_mask;
    while((channel = sound_channel_mask_first(active_mask)) >= 0) {
        s_movie_playback *playback = &movie_playback_pool.channel[channel];
        active_mask &= ~(UINT64_C(1) << channel);
        if(!playback->screen && !movie_playback_render_to(playback, screen)) {
            movie_playback_stop(playback);
            result = false;
        }
    }
    return result;
}

/*
* Caskey, Damon V.
* 2026-08-12
*
* Property mutation helpers preserve decoder, timing, audio,
* scaling, signed-rate, and external screen invariants.
*/
bool movie_playback_set_height(s_movie_playback *playback, int height)
{
    if(movie_playback_get_index(playback) < 0 || height < 0) {
        return false;
    }
    playback->height = height;
    playback->frame_dirty = 1;
    return true;
}

/*
* Caskey, Damon V.
* 2026-08-12
*
* Protect exact-black movie pixels from the subscreen color key
* without modifying any nonzero near-black shades.
*/
bool movie_playback_set_black_filter(
    s_movie_playback *playback,
    int black_filter
)
{
    if(movie_playback_get_index(playback) < 0) {
        return false;
    }
    playback->black_filter = black_filter != 0;
    playback->frame_dirty = 1;
    return true;
}

bool movie_playback_set_interrupt(s_movie_playback *playback, int interrupt)
{
    if(movie_playback_get_index(playback) < 0) {
        return false;
    }
    playback->interrupt = interrupt != 0;
    return true;
}

bool movie_playback_set_offset_x(s_movie_playback *playback, int offset_x)
{
    if(movie_playback_get_index(playback) < 0) {
        return false;
    }
    playback->offset_x = offset_x;
    return true;
}

bool movie_playback_set_offset_y(s_movie_playback *playback, int offset_y)
{
    if(movie_playback_get_index(playback) < 0) {
        return false;
    }
    playback->offset_y = offset_y;
    return true;
}

bool movie_playback_set_paused(s_movie_playback *playback, int paused)
{
    uint64_t now;

    if(movie_playback_get_index(playback) < 0 || !playback->active) {
        return false;
    }
    paused = paused != 0;
    if(playback->paused == paused) {
        return true;
    }

    now = timer_uticks();
    if(paused) {
        playback->position_anchor = movie_playback_position_now(playback, now);
        playback->position =
            playback->position_anchor / UINT64_C(1000000);
    } else {
        playback->clock_anchor = now;
    }
    playback->paused = paused;
    webm_set_audio_paused(
        playback->context,
        paused || playback->speed <= 0.0
    );
    return true;
}

bool movie_playback_set_position(
    s_movie_playback *playback,
    uint64_t position,
    int volume
)
{
    uint64_t position_ns;

    if(movie_playback_get_index(playback) < 0 || !playback->active) {
        return false;
    }
    if(playback->duration && position >= playback->duration) {
        position = playback->duration - 1U;
    }
    if(position > UINT64_MAX / UINT64_C(1000000)) {
        return false;
    }

    position_ns = position * UINT64_C(1000000);
    if(!movie_playback_open_media(playback, position_ns, volume)) {
        movie_playback_stop(playback);
        return false;
    }
    return true;
}

bool movie_playback_set_repeat(s_movie_playback *playback, int repeat)
{
    if(movie_playback_get_index(playback) < 0) {
        return false;
    }
    playback->repeat = repeat != 0;
    return true;
}

bool movie_playback_set_screen(s_movie_playback *playback, s_screen *screen)
{
    if(movie_playback_get_index(playback) < 0 ||
       (screen && !movie_playback_screen_valid(screen))) {
        return false;
    }
    playback->screen = screen;
    playback->frame_dirty = 1;
    return true;
}

bool movie_playback_set_sound_channel(
    s_movie_playback *playback,
    int sound_channel,
    int volume
)
{
    uint64_t position;

    if(movie_playback_get_index(playback) < 0 ||
       !playback->active ||
       sound_channel < 0 ||
       (unsigned int)sound_channel >= SOUND_CHANNEL_COUNT_MAX) {
        return false;
    }
    if(playback->sound_channel == sound_channel) {
        return true;
    }

    position = movie_playback_position_now(playback, timer_uticks());
    playback->sound_channel = sound_channel;
    if(!movie_playback_open_media(playback, position, volume)) {
        movie_playback_stop(playback);
        return false;
    }
    return true;
}

bool movie_playback_set_speed(s_movie_playback *playback, double speed)
{
    uint64_t now;
    uint64_t position;
    double previous_speed;
    unsigned int speed_percent;

    if(movie_playback_get_index(playback) < 0 ||
       !playback->active ||
       !isfinite(speed) ||
       speed < MOVIE_SPEED_MIN ||
       speed > MOVIE_SPEED_MAX) {
        return false;
    }
    if(playback->speed == speed) {
        return true;
    }

    now = timer_uticks();
    position = movie_playback_position_now(playback, now);
    previous_speed = playback->speed;
    playback->position_anchor = position;
    playback->clock_anchor = now;
    playback->speed = speed;
    playback->position = position / UINT64_C(1000000);

    if(speed > 0.0 && previous_speed <= 0.0) {
        if(!movie_playback_open_media(playback, position, playback->volume)) {
            movie_playback_stop(playback);
            return false;
        }
    } else if(speed > 0.0) {
        speed_percent = (unsigned int)(speed * 100.0 + 0.5);
        if(speed_percent < 1U) speed_percent = 1U;
        webm_set_audio_speed(playback->context, speed_percent);
        webm_set_audio_paused(playback->context, playback->paused);
    } else {
        playback->reverse_pending = 0;
        playback->terminal_pending = 0;
        webm_set_audio_paused(playback->context, 1);
    }
    return true;
}

bool movie_playback_set_width(s_movie_playback *playback, int width)
{
    if(movie_playback_get_index(playback) < 0 || width < 0) {
        return false;
    }
    playback->width = width;
    playback->frame_dirty = 1;
    return true;
}
