/*
 * OpenBOR - http://www.chronocrash.com
 * -----------------------------------------------------------------------
 * All rights reserved. See LICENSE in OpenBOR root for license details.
 *
 * Copyright (c) OpenBOR Team
 */

#include <inttypes.h>
#include <limits.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*
* Caskey, Damon V.
* 2026-08-13
*
* Allocate an optional movie cache without the engine's fatal
* allocation wrapper. Automatic mode must be able to recover from
* allocation pressure by falling back to streamed playback.
*/
static void *movie_cache_allocate_nonfatal(size_t size)
{
    return malloc(size);
}

#include "globals.h"
#include "ram.h"
#include "sound_channel.h"
#include "soundmix.h"
#include "timer.h"
#include "video.h"
#include "vidplay.h"
#include "movie_playback.h"
#include "gpu.h"

#define MOVIE_SOURCE_CAPACITY_INITIAL 64U
#define MOVIE_CACHE_READ_CHUNK_SIZE (1024U * 1024U)
#define MOVIE_CACHE_RESERVE_MINIMUM (UINT64_C(128) * UINT64_C(1024) * UINT64_C(1024))
#define MOVIE_SPEED_ROUNDING_FACTOR 1000.0
#define MOVIE_VIDEO_LATE_TOLERANCE_NANOSECONDS UINT64_C(50000000)
#define MOVIE_PRESENTATION_DROP_REPORT_INTERVAL UINT64_C(16)

typedef enum e_movie_cache_result {
    MOVIE_CACHE_RESULT_SUCCESS,
    MOVIE_CACHE_RESULT_MEMORY,
    MOVIE_CACHE_RESULT_IO
} e_movie_cache_result;

typedef struct s_movie_source {
    char *path;
    unsigned char *cache_buffer;
    size_t cache_size;
    unsigned int references;
    e_movie_loading_mode loading_mode;
    int active;
    int id;
} s_movie_source;

/*
* Caskey, Damon V.
* 2026-08-17
*
* Own asynchronous decoder lifecycle independently from the script-visible
* playback record. Each decoder holds its movie source through CLOSED.
*/
typedef struct s_movie_decoder {
    webm_context *context;
    int source_id;
    int channel;
    int ready;
    struct s_movie_decoder *next;
} s_movie_decoder;

typedef struct s_movie_decoder_request {
    uint64_t position;
    int source_id;
    int volume;
    int pending;
} s_movie_decoder_request;

typedef struct s_movie_playback_pool {
    s_movie_playback channel[MOVIE_CHANNEL_COUNT];
    s_movie_decoder *decoder[MOVIE_CHANNEL_COUNT];
    s_movie_decoder_request decoder_request[MOVIE_CHANNEL_COUNT];
    unsigned int retiring_decoder_count[MOVIE_CHANNEL_COUNT];
    s_movie_decoder *retired_decoder;
    s_movie_source *source;
    size_t source_capacity;
    int next_source_id;
    uint64_t active_mask;
    uint64_t cached_bytes;
    int initialized;
} s_movie_playback_pool;

static s_movie_playback_pool movie_playback_pool;

/*
* Caskey, Damon V.
* 2026-08-18
*
* Track exclusive hardware YUV presentation independently from decoder
* channels. The overlay replaces the normal game texture until playback
* ends, at which point the configured game video mode is restored.
*/
static int movie_yuv_channel = -1;
static yuv_video_mode movie_yuv_mode;

/*
* Caskey, Damon V.
* 2026-08-17
*
* Convert creator-supplied forward playback speed to one canonical float.
* Reject negative rates because backward navigation is provided by explicit
* seeks, then clamp the upper range and round to thousandths so video timing,
* audio resampling, property reads, and decoder reopens share one rate.
*/
static bool movie_playback_sanitize_speed(
    double requested_speed,
    float *applied_speed
)
{
    if(!applied_speed || !isfinite(requested_speed)) {
        return false;
    }

    if(requested_speed < MOVIE_SPEED_MIN) {
        return false;
    }
    if(requested_speed > MOVIE_SPEED_MAX) {
        requested_speed = MOVIE_SPEED_MAX;
    }
    requested_speed = round(
        requested_speed * MOVIE_SPEED_ROUNDING_FACTOR
    ) / MOVIE_SPEED_ROUNDING_FACTOR;

    *applied_speed = (float)requested_speed;
    if(*applied_speed == 0.0f) {
        *applied_speed = 0.0f;
    }
    return true;
}

/*
* Caskey, Damon V.
* 2026-08-12
*
* Validate RGB32 destination screens. Movie rectangle dimensions no longer
* require a CPU RGB allocation or fixed-point scaling representation.
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
    return width > 0 && height > 0;
}

/*
* Caskey, Damon V.
* 2026-08-15
*
* Resolve each creator-selected composition dimension. Zero follows the
* destination screen, MOVIE_SIZE_NATIVE follows the decoded video, and an
* explicit positive value requests that exact processing size.
*/
static bool movie_playback_resolve_dimensions(
    const s_movie_playback *playback,
    const s_screen *destination,
    int *width,
    int *height
)
{
    uint64_t resolved_width = playback->width;
    uint64_t resolved_height = playback->height;

    if(resolved_width == 0) {
        resolved_width = (uint64_t)destination->width;
    } else if(resolved_width == MOVIE_SIZE_NATIVE) {
        resolved_width = (uint64_t)playback->current_frame->width;
    }
    if(resolved_height == 0) {
        resolved_height = (uint64_t)destination->height;
    } else if(resolved_height == MOVIE_SIZE_NATIVE) {
        resolved_height = (uint64_t)playback->current_frame->height;
    }

    if(resolved_width > (uint64_t)INT_MAX ||
       resolved_height > (uint64_t)INT_MAX) {
        return false;
    }
    *width = (int)resolved_width;
    *height = (int)resolved_height;
    return movie_playback_dimensions_valid(*width, *height);
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
* 2026-08-17
*
* Reap completed decoder retirement during runtime updates. The wait option
* provides synchronous shutdown after playback has stopped. Decoder source
* references remain valid through lifecycle completion.
*/
static void movie_playback_reap_retired_decoders(int wait)
{
    s_movie_decoder **cursor = &movie_playback_pool.retired_decoder;

    while(*cursor) {
        s_movie_decoder *decoder = *cursor;
        int closed;

        if(wait) {
            webm_close(decoder->context);
            closed = 1;
        }
        else {
            closed = webm_poll_closed(decoder->context);
        }
        if(!closed) {
            cursor = &decoder->next;
            continue;
        }

        *cursor = decoder->next;
        if(decoder->channel >= 0 &&
           (unsigned int)decoder->channel < MOVIE_CHANNEL_COUNT &&
           movie_playback_pool.retiring_decoder_count[
               decoder->channel
           ]) {
            --movie_playback_pool.retiring_decoder_count[
                decoder->channel
            ];
        }
        movie_source_release(decoder->source_id);
        free(decoder);
    }
}

/*
* Caskey, Damon V.
* 2026-08-17
*
* Queue the channel decoder for asynchronous retirement. Per-channel counts
* gate decoder creation, and one request slot coalesces repeated pending seeks.
*/
static void movie_playback_retire_decoder(s_movie_playback *playback)
{
    int channel;
    s_movie_decoder *decoder;

    if(!playback) {
        return;
    }
    channel = playback->index;
    if(channel >= 0 &&
       (unsigned int)channel < MOVIE_CHANNEL_COUNT &&
       &movie_playback_pool.channel[channel] == playback &&
       (decoder = movie_playback_pool.decoder[channel])) {
        movie_playback_pool.decoder[channel] = NULL;
        playback->context = NULL;
        decoder->channel = channel;
        ++movie_playback_pool.retiring_decoder_count[channel];
        webm_request_close(decoder->context);
        decoder->next = movie_playback_pool.retired_decoder;
        movie_playback_pool.retired_decoder = decoder;
    }
}

/*
* Caskey, Damon V.
* 2026-08-17
*
* Retire the current decoder and cancel pending replacement intent when the
* owning playback itself closes. Its queued look-ahead frame is released
* immediately because it belongs to the canceled decode sequence.
*/
static void movie_playback_close_decoder(s_movie_playback *playback)
{
    int channel;

    if(!playback) {
        return;
    }
    channel = playback->index;
    movie_playback_retire_decoder(playback);
    if(channel >= 0 && (unsigned int)channel < MOVIE_CHANNEL_COUNT) {
        memset(
            &movie_playback_pool.decoder_request[channel],
            0,
            sizeof(movie_playback_pool.decoder_request[channel])
        );
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
    gpu_release_yuv(playback);
    yuv_frame_destroy(playback->current_frame);
    playback->current_frame = NULL;
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
    playback->resample = DC_RESAMPLE_MOVIE;
}

/*
* Caskey, Damon V.
* 2026-08-12
*
* Initialize one stable bank of 64 movie playback objects and dynamic source
* storage. Decoded YUV planes remain unchanged until GPU rendering.
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

    if(!webm_lifecycle_init() ||
       !movie_source_reserve(MOVIE_SOURCE_CAPACITY_INITIAL)) {
        webm_lifecycle_shutdown();
        memset(&movie_playback_pool, 0, sizeof(movie_playback_pool));
        return false;
    }

    return true;
}

/*
* Caskey, Damon V.
* 2026-08-12
*
* Stop every playback and release loaded sources during script or engine
* shutdown. Resetting the playback records also releases their GPU YUV storage.
*/
void movie_playback_shutdown(void)
{
    size_t source_id;

    if(!movie_playback_pool.initialized) {
        return;
    }

    movie_playback_stop_all();
    movie_playback_reap_retired_decoders(1);
    for(source_id = 0;
        source_id < movie_playback_pool.source_capacity;
        source_id++) {
        free(movie_playback_pool.source[source_id].path);
        free(movie_playback_pool.source[source_id].cache_buffer);
    }
    free(movie_playback_pool.source);
    webm_lifecycle_shutdown();
    memset(&movie_playback_pool, 0, sizeof(movie_playback_pool));
}

/*
* Caskey, Damon V.
* 2026-08-13
*
* Determine whether an automatic movie cache preserves both its
* share of total memory and the required free-memory cushion.
* The budget scales without an upper cap for high-memory systems.
*/
static bool movie_source_auto_cache_fits(
    uint64_t file_size,
    uint64_t total_memory,
    uint64_t available_memory,
    uint64_t cached_bytes
)
{
    uint64_t cache_budget;
    uint64_t memory_reserve;

    if(!file_size ||
       !total_memory ||
       !available_memory ||
       file_size > (uint64_t)SIZE_MAX) {
        return false;
    }

    cache_budget = total_memory / 4U;
    memory_reserve = total_memory / 8U;
    if(memory_reserve < MOVIE_CACHE_RESERVE_MINIMUM) {
        memory_reserve = MOVIE_CACHE_RESERVE_MINIMUM;
    }

    if(file_size > cache_budget ||
       cached_bytes > cache_budget - file_size ||
       available_memory <= memory_reserve ||
       file_size > available_memory - memory_reserve) {
        return false;
    }
    return true;
}

/*
* Caskey, Damon V.
* 2026-08-13
*
* Read one validated movie source into a nonfatal allocation using
* bounded packfile reads. Allocation pressure is distinguished from
* I/O failure so automatic mode only falls back for memory limits.
*/
static e_movie_cache_result movie_source_read_cache(
    int handle,
    uint64_t file_size,
    unsigned char **cache_buffer,
    size_t *cache_size
)
{
    unsigned char *buffer;
    uint64_t read_position = 0;

    if(handle < 0 ||
       !file_size ||
       file_size > (uint64_t)SIZE_MAX ||
       !cache_buffer ||
       !cache_size) {
        return MOVIE_CACHE_RESULT_IO;
    }

    buffer = movie_cache_allocate_nonfatal((size_t)file_size);
    if(!buffer) {
        return MOVIE_CACHE_RESULT_MEMORY;
    }

    while(read_position < file_size) {
        uint64_t bytes_remaining = file_size - read_position;
        int read_size = bytes_remaining > MOVIE_CACHE_READ_CHUNK_SIZE
            ? (int)MOVIE_CACHE_READ_CHUNK_SIZE
            : (int)bytes_remaining;

        if(readpackfile(
            handle,
            buffer + (size_t)read_position,
            read_size
        ) != read_size) {
            free(buffer);
            return MOVIE_CACHE_RESULT_IO;
        }
        read_position += (uint64_t)read_size;
    }

    *cache_buffer = buffer;
    *cache_size = (size_t)file_size;
    return MOVIE_CACHE_RESULT_SUCCESS;
}

/*
* Caskey, Damon V.
* 2026-08-13
*
* Load one reusable movie source according to its stream, cache, or automatic
* memory policy. Forced cache reports failure; automatic cache may select
* streaming when memory requirements are not met.
*/
int movie_source_load(const char *path, e_movie_loading_mode loading_mode)
{
    s_movie_source source = { 0 };
    e_movie_cache_result cache_result = MOVIE_CACHE_RESULT_SUCCESS;
    packfile_signed_offset_t file_size;
    bool attempt_cache;
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

    handle = openpackfile(path, packfile);
    if(handle < 0) {
        free(source.path);
        return -1;
    }
    file_size = seekpackfile64(handle, 0, SEEK_END);
    if(file_size <= 0 || seekpackfile64(handle, 0, SEEK_SET) < 0) {
        closepackfile(handle);
        free(source.path);
        return -1;
    }

    attempt_cache = loading_mode == MOVIE_LOADING_CACHE ||
        (loading_mode == MOVIE_LOADING_AUTO &&
         movie_source_auto_cache_fits(
             (uint64_t)file_size,
             getSystemRam(BYTES),
             getFreeRam(BYTES),
             movie_playback_pool.cached_bytes
         ));
    if(attempt_cache) {
        cache_result = movie_source_read_cache(
            handle,
            (uint64_t)file_size,
            &source.cache_buffer,
            &source.cache_size
        );
    }
    closepackfile(handle);

    if(cache_result == MOVIE_CACHE_RESULT_IO ||
       (cache_result == MOVIE_CACHE_RESULT_MEMORY &&
        loading_mode == MOVIE_LOADING_CACHE)) {
        free(source.path);
        free(source.cache_buffer);
        return -1;
    }
    source.loading_mode = source.cache_buffer
        ? MOVIE_LOADING_CACHE
        : MOVIE_LOADING_STREAM;

    source_id = movie_playback_pool.next_source_id;
    if(!movie_source_reserve((size_t)source_id + 1U)) {
        free(source.path);
        free(source.cache_buffer);
        return -1;
    }

    source.id = source_id;
    source.active = 1;
    movie_playback_pool.source[source_id] = source;
    movie_playback_pool.cached_bytes += (uint64_t)source.cache_size;
    ++movie_playback_pool.next_source_id;
    return source_id;
}

/*
* Caskey, Damon V.
* 2026-08-17
*
* Unload a source only after active and asynchronously retiring decoders
* release it. Each call first reaps completed teardown without waiting.
*/
bool movie_source_unload(int source_id)
{
    s_movie_source *source;

    movie_playback_reap_retired_decoders(0);
    source = movie_source_get(source_id);

    if(!source || source->references) {
        return false;
    }

    if((uint64_t)source->cache_size <= movie_playback_pool.cached_bytes) {
        movie_playback_pool.cached_bytes -= (uint64_t)source->cache_size;
    } else {
        movie_playback_pool.cached_bytes = 0;
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
* 2026-08-17
*
* Test whether the current channel decoder has crossed its asynchronous
* READY boundary. Property mutations may update pending playback intent
* before this point without touching partially initialized decoder fields.
*/
static int movie_playback_decoder_ready(
    const s_movie_playback *playback
)
{
    int channel;
    s_movie_decoder *decoder;

    if(!playback) {
        return 0;
    }
    channel = playback->index;
    if(channel < 0 || (unsigned int)channel >= MOVIE_CHANNEL_COUNT) {
        return 0;
    }
    decoder = movie_playback_pool.decoder[channel];
    return decoder && decoder->ready;
}

/*
* Caskey, Damon V.
* 2026-08-17
*
* Use consumed PCM time as the master during forward playback. Each
* successful owned-channel snapshot also reanchors the monotonic fallback,
* preserving continuity for silent media and replaced audio channels.
*/
static uint64_t movie_playback_position_now(
    s_movie_playback *playback,
    uint64_t now
)
{
    uint64_t audio_position;
    double elapsed;
    double position;

    if(playback->paused ||
       playback->speed == 0.0 ||
       !movie_playback_decoder_ready(playback)) {
        return playback->position_anchor;
    }
    if(playback->speed > 0.0 &&
       webm_get_audio_playback_position(
           playback->context,
           &audio_position
       )) {
        playback->position_anchor = audio_position;
        playback->clock_anchor = now;
        return audio_position;
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
* 2026-08-17
*
* Start the latest pending decoder request after the retiring decoder reaches
* CLOSED. One pending slot coalesces repeated seeks during teardown.
*/
static int movie_playback_start_pending_decoder(
    s_movie_playback *playback
)
{
    s_movie_decoder_request *request;
    s_movie_source *source;
    s_movie_decoder *decoder;
    webm_context *context;
    int channel = playback->index;

    if(channel < 0 || (unsigned int)channel >= MOVIE_CHANNEL_COUNT) {
        return -1;
    }
    request = &movie_playback_pool.decoder_request[channel];
    if(!request->pending) {
        return movie_playback_pool.decoder[channel] ? 1 : -1;
    }
    if(movie_playback_pool.retiring_decoder_count[channel]) {
        return 0;
    }
    source = movie_source_get(request->source_id);
    if(!source || playback->source_id != request->source_id) {
        request->pending = 0;
        return -1;
    }

    decoder = calloc(1, sizeof(*decoder));
    if(!decoder) {
        request->pending = 0;
        return -1;
    }
    ++source->references;
    context = webm_start_playback_ex(
        source->path,
        request->volume,
        playback->sound_channel,
        source->cache_buffer,
        source->cache_size,
        request->position,
        playback->speed > 0.0,
        playback->replace_all_audio
    );
    if(!context) {
        movie_source_release(request->source_id);
        request->pending = 0;
        free(decoder);
        return -1;
    }

    decoder->context = context;
    decoder->source_id = request->source_id;
    decoder->channel = channel;
    movie_playback_pool.decoder[channel] = decoder;
    playback->context = context;
    request->pending = 0;
    return 1;
}

/*
* Caskey, Damon V.
* 2026-08-17
*
* Begin an asynchronous WebM open or seek from a reusable source while
* preserving the last frame until replacement is ready. Decoder ownership
* retains an additional source reference through asynchronous teardown.
*/
static bool movie_playback_begin_media(
    s_movie_playback *playback,
    uint64_t position,
    int volume
)
{
    s_movie_source *source = movie_source_get(playback->source_id);
    s_movie_decoder_request *request;
    int channel = playback->index;

    if(!source ||
       channel < 0 ||
       (unsigned int)channel >= MOVIE_CHANNEL_COUNT) {
        return false;
    }

    movie_playback_retire_decoder(playback);
    yuv_frame_destroy(playback->next_frame);
    playback->next_frame = NULL;
    playback->current_frame_presented = playback->current_frame != NULL;
    request = &movie_playback_pool.decoder_request[channel];
    request->position = position;
    request->source_id = playback->source_id;
    request->volume = volume;
    request->pending = 1;
    playback->volume = volume;
    playback->position_anchor = position;
    playback->position = position / UINT64_C(1000000);
    playback->terminal_pending = 0;
    playback->failed = 0;
    return movie_playback_start_pending_decoder(playback) >= 0;
}

/*
* Caskey, Damon V.
* 2026-08-17
*
* Poll an asynchronous decoder and commit its immutable media metadata once
* READY is published. Movie clocks begin at this boundary, preventing open,
* seek, and preroll latency from advancing the presentation timeline.
*/
static int movie_playback_poll_decoder(s_movie_playback *playback)
{
    s_movie_decoder *decoder;
    e_webm_decoder_state state;
    yuv_video_mode video_mode = { 0 };
    uint64_t duration;
    int channel = playback->index;

    if(channel < 0 ||
       (unsigned int)channel >= MOVIE_CHANNEL_COUNT ||
       !(decoder = movie_playback_pool.decoder[channel])) {
        return -1;
    }
    if(decoder->ready) {
        return 1;
    }

    state = webm_get_decoder_state(decoder->context);
    if(state == WEBM_DECODER_STATE_FAILED ||
       state == WEBM_DECODER_STATE_CLOSING ||
       state == WEBM_DECODER_STATE_CLOSED) {
        return -1;
    }
    if(state != WEBM_DECODER_STATE_READY) {
        return 0;
    }

    webm_get_video_info(decoder->context, &video_mode);
    duration = webm_get_duration(decoder->context);
    if((video_mode.width & 1) ||
       (video_mode.height & 1) ||
       !movie_playback_dimensions_valid(
           video_mode.width,
           video_mode.height
       )) {
        printf(
            "Error: Invalid movie frame dimensions %d*%d.\n",
            video_mode.width,
            video_mode.height
        );
        return -1;
    }

    if(playback->video_mode.width != video_mode.width ||
       playback->video_mode.height != video_mode.height) {
        yuv_frame_destroy(playback->current_frame);
        playback->current_frame = NULL;
        playback->current_frame_presented = 0;
    }
    playback->video_mode = video_mode;
    playback->duration = duration / UINT64_C(1000000);
    if(duration && playback->position_anchor >= duration) {
        playback->position_anchor = duration - 1U;
    }
    playback->clock_anchor = timer_uticks();
    playback->position =
        playback->position_anchor / UINT64_C(1000000);
    playback->terminal_pending = 0;
    decoder->ready = 1;

    if(playback->speed > 0.0) {
        webm_set_audio_speed(decoder->context, playback->speed);
    }
    webm_set_audio_paused(
        decoder->context,
        playback->paused || playback->speed == 0.0
    );
    return 1;
}

/*
* Caskey, Damon V.
* 2026-08-12
*
* Create one playback from a reusable source. Automatic mode takes the lowest
* free slot; explicit selection replaces that channel. Exclusive audio mode
* stops other channel playback for fullscreen ownership.
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
    if(!movie_playback_begin_media(playback, 0, volume)) {
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
* Stop individual or grouped playbacks and release their source
* references and fixed channel slots.
*/
void movie_playback_stop(s_movie_playback *playback)
{
    int channel = movie_playback_get_index(playback);

    if(channel < 0) {
        return;
    }
    if(movie_yuv_channel == channel) {
        movie_yuv_channel = -1;
        memset(&movie_yuv_mode, 0, sizeof(movie_yuv_mode));
        video_restore_mode();
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

/*
 * Damon V. Caskey
 * 2026-11-15
 * Describe immutable retained decoder planes using a stable playback owner and
 * a promotion generation. Legacy field cr contains U, and cb contains V.
 * Presentation and draw-property changes do not change the uploaded pixels.
 */
static gpu_yuv_source movie_playback_frame_source(
    const s_movie_playback *playback
)
{
    const yuv_frame *frame = playback->current_frame;
    gpu_yuv_source source = {
        .owner = playback,
        .generation = playback->frame_generation,
        .width = frame->width,
        .height = frame->height,
        .y = frame->lum,
        .u = frame->cr,
        .v = frame->cb
    };
    return source;
}

/*
 * Damon V. Caskey
 * 2026-11-15
 * Compose retained YUV directly into an RGB32 GPU screen. Keep opaque movie
 * writes, exact dimensions, signed clipping and call-order layering independent
 * of global drawmethod state. Display correction belongs to final presentation.
 */
static bool movie_playback_render_to(
    s_movie_playback *playback,
    s_screen *destination
)
{
    gpu_yuv_source source;
    gpu_movie_draw draw = { 0 };

    if(!movie_playback_screen_valid(destination)) {
        return false;
    }
    if(!playback->current_frame) {
        return true;
    }
    if(!movie_playback_resolve_dimensions(
        playback,
        destination,
        &draw.width,
        &draw.height
    )) {
        return false;
    }

    source = movie_playback_frame_source(playback);
    draw.x = playback->offset_x;
    draw.y = playback->offset_y;
    draw.resample = playback->resample;
    draw.black_filter = playback->black_filter;
    if(!gpu_draw_yuv(destination, &source, &draw)) {
        return false;
    }
    playback->frame_dirty = 0;
    playback->current_frame_presented = 1;
    return true;
}

/*
* Caskey, Damon V.
* 2026-08-18
*
* Record decoded frames omitted after exceeding the audio-master lateness
* tolerance. Track maximum lateness and rate-limit diagnostic output.
*/
static void movie_playback_record_presentation_drop(
    s_movie_playback *playback,
    uint64_t frame_timestamp,
    uint64_t position
)
{
    uint64_t lateness = position >= frame_timestamp
        ? position - frame_timestamp
        : 0;

    if(playback->presentation_drop_count < UINT64_MAX) {
        ++playback->presentation_drop_count;
    }
    if(lateness > playback->maximum_presentation_lateness) {
        playback->maximum_presentation_lateness = lateness;
    }
    if(playback->presentation_drop_count == 1U ||
       !(playback->presentation_drop_count %
            MOVIE_PRESENTATION_DROP_REPORT_INTERVAL)) {
        printf(
            "\nWarning: Movie channel %d discarded %" PRIu64
            " late decoded video frame%s for audio synchronization "
            "(maximum lateness=%" PRIu64 " ms).\n",
            playback->index,
            playback->presentation_drop_count,
            playback->presentation_drop_count == 1U ? "" : "s",
            playback->maximum_presentation_lateness / UINT64_C(1000000)
        );
    }
}

/*
* Caskey, Damon V.
* 2026-08-18
*
* Transfer the look-ahead frame into presentation ownership and mark it pending
* until composition acknowledges it.
*/
static void movie_playback_promote_next_frame(
    s_movie_playback *playback
)
{
    yuv_frame_destroy(playback->current_frame);
    playback->current_frame = playback->next_frame;
    playback->next_frame = NULL;
    ++playback->frame_generation;
    playback->frame_dirty = 1;
    playback->current_frame_presented = 0;
}

/*
* Caskey, Damon V.
* 2026-08-18
*
* Promote one due frame per presentation opportunity under the audio master.
* Protect frames within the lateness tolerance and bound work to the initial
* decoded-queue snapshot.
*/
static void movie_playback_poll_frames(
    s_movie_playback *playback,
    uint64_t position,
    int *terminal
)
{
    int remaining = webm_get_pending_frame_count(playback->context);
    if(playback->next_frame) {
        ++remaining;
    }
    while(remaining-- > 0) {
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

        if(playback->current_frame &&
           !playback->current_frame_presented) {
            uint64_t current_timestamp =
                playback->current_frame->timestamp;

            if(playback->next_frame->timestamp > position ||
               current_timestamp > position ||
               position - current_timestamp <=
                    MOVIE_VIDEO_LATE_TOLERANCE_NANOSECONDS) {
                break;
            }
            movie_playback_record_presentation_drop(
                playback,
                current_timestamp,
                position
            );
            yuv_frame_destroy(playback->current_frame);
            playback->current_frame = NULL;
            playback->frame_dirty = 0;
        }

        if(playback->next_frame->timestamp > position) {
            if(!playback->current_frame) {
                movie_playback_promote_next_frame(playback);
            }
            break;
        }

        if(position - playback->next_frame->timestamp >
                MOVIE_VIDEO_LATE_TOLERANCE_NANOSECONDS &&
           remaining > 1) {
            movie_playback_record_presentation_drop(
                playback,
                playback->next_frame->timestamp,
                position
            );
            yuv_frame_destroy(playback->next_frame);
            playback->next_frame = NULL;
            continue;
        }

        movie_playback_promote_next_frame(playback);
        break;
    }
}

/*
* Caskey, Damon V.
* 2026-08-17
*
* Advance playback clocks and decoder state independently from composition.
* Lifecycle polling keeps open, seek, preroll, and teardown work off the
* engine thread. Backward navigation remains an explicit seek operation.
*/
void movie_playback_update(int interrupt_requested)
{
    uint64_t active_mask;
    uint64_t now;
    int channel;

    if(!movie_playback_pool.initialized) {
        return;
    }

    movie_playback_reap_retired_decoders(0);
    now = timer_uticks();
    active_mask = movie_playback_pool.active_mask;
    while((channel = sound_channel_mask_first(active_mask)) >= 0) {
        s_movie_playback *playback = &movie_playback_pool.channel[channel];
        uint64_t position;
        int decoder_result;
        int terminal = 0;

        active_mask &= ~(UINT64_C(1) << channel);
        if(interrupt_requested && playback->interrupt) {
            movie_playback_stop(playback);
            continue;
        }
        decoder_result = movie_playback_start_pending_decoder(playback);
        if(decoder_result > 0) {
            decoder_result = movie_playback_poll_decoder(playback);
        }
        if(decoder_result < 0) {
            movie_playback_stop(playback);
            playback->failed = 1;
            continue;
        }
        if(decoder_result == 0) {
            continue;
        }
        if(playback->terminal_pending) {
            if(playback->repeat) {
                if(!movie_playback_begin_media(
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

        if((playback->speed > 0.0 && !playback->paused) ||
           !playback->current_frame) {
            movie_playback_poll_frames(playback, position, &terminal);
        }

        if(terminal) {
            if(playback->frame_dirty) {
                playback->terminal_pending = 1;
            } else if(playback->repeat) {
                if(!movie_playback_begin_media(
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
* 2026-08-13
*
* Draw the retained frame from one active movie channel to a
* creator-selected 32-bit screen. Playback and composition remain
* independent so script call order controls clearing and layering.
*/
bool movie_playback_draw_to_screen(s_screen *screen, int channel)
{
    s_movie_playback *playback;

    if(!movie_playback_pool.initialized ||
       channel < 0 ||
       (unsigned int)channel >= MOVIE_CHANNEL_COUNT ||
       !movie_playback_screen_valid(screen)) {
        return false;
    }

    playback = &movie_playback_pool.channel[channel];
    if(!playback->active ||
       !(movie_playback_pool.active_mask & (UINT64_C(1) << channel))) {
        return false;
    }
    return movie_playback_render_to(playback, screen);
}

/*
* Caskey, Damon V.
* 2026-08-18
*
* Present one retained decoder frame through the hardware YUV path. This is
* an exclusive final-output operation, not a compositing operation. Width and
* height select the overlay's logical display size; zero and native both use
* the WebM display dimensions. Playback timing and transport remain owned by
* the channel and therefore work identically to screen composition.
*/
bool movie_playback_draw_to_yuv(int channel)
{
    s_movie_playback *playback;
    yuv_video_mode mode;
    gpu_yuv_source source;

    if(!movie_playback_pool.initialized ||
       channel < 0 ||
       (unsigned int)channel >= MOVIE_CHANNEL_COUNT) {
        return false;
    }
    playback = &movie_playback_pool.channel[channel];
    if(!playback->active ||
       !(movie_playback_pool.active_mask & (UINT64_C(1) << channel))) {
        return false;
    }
    if(!playback->current_frame) {
        return true;
    }

    mode = playback->video_mode;
    if(playback->width && playback->width != MOVIE_SIZE_NATIVE) {
        mode.display_width = (int)playback->width;
    }
    if(playback->height && playback->height != MOVIE_SIZE_NATIVE) {
        mode.display_height = (int)playback->height;
    }
    if(movie_yuv_channel != channel ||
       memcmp(&movie_yuv_mode, &mode, sizeof(mode))) {
        if(!video_setup_yuv_overlay(&mode)) {
            return false;
        }
        movie_yuv_channel = channel;
        movie_yuv_mode = mode;
    }
    source = movie_playback_frame_source(playback);
    if(!video_prepare_yuv_frame(&source, playback->resample) ||
       !video_display_yuv_frame()) {
        return false;
    }
    playback->frame_dirty = 0;
    playback->current_frame_presented = 1;
    return true;
}

/*
 * Damon V. Caskey
 * 2026-11-15
 * Capture the retained frame with the hardware movie color treatment. Explicit
 * capture does not acknowledge presentation or alter playback scheduling.
 */
bool movie_playback_capture_frame(s_screen *screen, int channel)
{
    s_movie_playback *playback = movie_playback_get(channel);
    gpu_yuv_source source;

    if(!playback || !playback->active ||
       !(movie_playback_pool.active_mask & (UINT64_C(1) << channel)) ||
       !playback->current_frame || !movie_playback_screen_valid(screen)) {
        return false;
    }
    source = movie_playback_frame_source(playback);
    return video_capture_yuv_frame(screen, &source, playback->resample) != 0;
}

/*
* Caskey, Damon V.
* 2026-08-12
*
* Property mutation helpers preserve decoder, timing, audio,
* scaling, signed-rate, and external screen invariants.
*/
bool movie_playback_set_height(s_movie_playback *playback, uint64_t height)
{
    if(movie_playback_get_index(playback) < 0 ||
       (height != MOVIE_SIZE_NATIVE && height > (uint64_t)INT_MAX)) {
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
    if(movie_playback_decoder_ready(playback)) {
        webm_set_audio_paused(
            playback->context,
            paused || playback->speed == 0.0
        );
    }
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
    if(!movie_playback_begin_media(playback, position_ns, volume)) {
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

/*
 * Damon V. Caskey
 * 2026-11-15
 * Select one defined GPU reconstruction filter without changing decoded planes
 * or transport state. A retained paused frame becomes ready for redraw.
 */
bool movie_playback_set_resample(
    s_movie_playback *playback,
    e_drawmethod_resample resample
)
{
    if(movie_playback_get_index(playback) < 0 ||
       (unsigned int)resample >= (unsigned int)DC_RESAMPLE_END) {
        return false;
    }
    playback->resample = resample;
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
    if(!movie_playback_begin_media(playback, position, volume)) {
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
    float applied_speed;

    if(movie_playback_get_index(playback) < 0 ||
       !playback->active ||
       !movie_playback_sanitize_speed(speed, &applied_speed)) {
        return false;
    }
    if(playback->speed == applied_speed) {
        return true;
    }

    now = timer_uticks();
    position = movie_playback_position_now(playback, now);
    previous_speed = playback->speed;
    playback->position_anchor = position;
    playback->clock_anchor = now;
    playback->speed = applied_speed;
    playback->position = position / UINT64_C(1000000);

    if(playback->speed > 0.0 && previous_speed == 0.0) {
        if(!movie_playback_begin_media(playback, position, playback->volume)) {
            movie_playback_stop(playback);
            return false;
        }
    } else if(playback->speed > 0.0 &&
              movie_playback_decoder_ready(playback)) {
        webm_set_audio_speed(playback->context, playback->speed);
        webm_set_audio_paused(playback->context, playback->paused);
    } else if(playback->speed == 0.0) {
        playback->terminal_pending = 0;
        if(movie_playback_decoder_ready(playback)) {
            webm_set_audio_paused(playback->context, 1);
        }
    }
    return true;
}

bool movie_playback_set_width(s_movie_playback *playback, uint64_t width)
{
    if(movie_playback_get_index(playback) < 0 ||
       (width != MOVIE_SIZE_NATIVE && width > (uint64_t)INT_MAX)) {
        return false;
    }
    playback->width = width;
    playback->frame_dirty = 1;
    return true;
}
