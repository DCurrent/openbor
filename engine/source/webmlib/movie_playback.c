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
#include "screen.h"
#include "sound_channel.h"
#include "soundmix.h"
#include "timer.h"
#include "vidplay.h"
#include "movie_playback.h"

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
* Keep asynchronous decoder ownership outside the stable script-visible
* playback record. Each decoder retains its movie source until lifecycle
* teardown reaches CLOSED, including decoders retired by seeks or channel
* replacement while their worker is still running.
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
    int yuv_initialized;
} s_movie_playback_pool;

static s_movie_playback_pool movie_playback_pool;

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
* Poll decoder retirement without waiting during runtime updates. Shutdown
* may explicitly wait after every playback has stopped. Source references
* remain held until each lifecycle worker has released its borrowed path and
* cache storage.
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
* Move the channel's decoder directly to asynchronous retirement without
* allocating or joining. Per-channel retirement counts serialize a later
* replacement while allowing repeated pending seeks to collapse to one.
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

    if(!webm_lifecycle_init() ||
       !movie_source_reserve(MOVIE_SOURCE_CAPACITY_INITIAL)) {
        webm_lifecycle_shutdown();
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
    movie_playback_reap_retired_decoders(1);
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
* Load one reusable movie source. Streaming remains the default,
* forced cache fails cleanly, and automatic cache falls back to
* streaming when its memory policy or allocation cannot be met.
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
* Start the latest pending decoder request only after the previous decoder
* for this movie channel reaches CLOSED. Repeated seeks during teardown
* replace the request in place instead of accumulating lifecycle threads.
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
       video_mode.width > (INT_MAX >> 16) ||
       video_mode.height > (INT_MAX >> 16) ||
       !movie_playback_dimensions_valid(
           video_mode.width,
           video_mode.height
       )) {
        printf(
            "Error: Movie frame dimensions %d*%d cannot be represented by the 32-bit screen renderer.\n",
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
        if(playback->rgb_frame) {
            freescreen(&playback->rgb_frame);
        }
        if(playback->scaled_frame) {
            freescreen(&playback->scaled_frame);
        }
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
* Caskey, Damon V.
* 2026-08-15
*
* Protect absolute black after conversion when a movie frame will be drawn
* into a subscreen that reserves RGB 0,0,0 as its transparency mask.
*/
static void movie_playback_filter_black(
    const s_movie_playback *playback,
    s_screen *frame
)
{
    size_t pixel_count;
    size_t pixel_index;
    uint32_t *pixel;
    uint32_t protected_black;

    if(!playback->black_filter) {
        return;
    }

    pixel_count = (size_t)frame->width * (size_t)frame->height;
    pixel = (uint32_t *)frame->data;
    protected_black = colour32(1, 1, 1);
    for(pixel_index = 0; pixel_index < pixel_count; pixel_index++) {
        if(pixel[pixel_index] == 0) {
            pixel[pixel_index] = protected_black;
        }
    }
}

/*
* Caskey, Damon V.
* 2026-08-15
*
* Convert a current frame only when its pixels or destination dimensions
* change. Resized output converts directly from YUV at the requested size,
* avoiding a full-resolution RGB conversion and subsequent scaling pass.
*/
static bool movie_playback_prepare_frame(
    s_movie_playback *playback,
    const s_screen *destination,
    s_screen **render_frame
)
{
    int height;
    int width;

    if(!playback->current_frame) {
        return false;
    }
    if(!movie_playback_resolve_dimensions(
        playback,
        destination,
        &width,
        &height
    )) {
        return false;
    }

    if(playback->frame_dirty) {
        playback->rgb_frame_dirty = 1;
        playback->scaled_frame_dirty = 1;
    }

    if(width == playback->current_frame->width &&
       height == playback->current_frame->height) {
        if(!playback->rgb_frame) {
            playback->rgb_frame = movie_playback_alloc_screen(width, height);
            playback->rgb_frame_dirty = 1;
        }
        if(!playback->rgb_frame) {
            return false;
        }
        if(playback->rgb_frame_dirty) {
            yuv_to_rgb(playback->current_frame, playback->rgb_frame);
            movie_playback_filter_black(playback, playback->rgb_frame);
            playback->rgb_frame_dirty = 0;
        }
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
        playback->scaled_frame_dirty = 1;
    }
    if(!playback->scaled_frame) {
        return false;
    }
    if(playback->scaled_frame_dirty) {
        if(!yuv_to_rgb_scaled(
            playback->current_frame,
            playback->scaled_frame
        )) {
            return false;
        }
        movie_playback_filter_black(playback, playback->scaled_frame);
        playback->scaled_frame_dirty = 0;
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
    playback->current_frame_presented = 1;
    return true;
}

/*
* Caskey, Damon V.
* 2026-08-18
*
* Record decoded frames intentionally omitted from presentation after their
* lateness exceeds the bounded audio-master tolerance. Report the first drop
* and each subsequent group so video loss remains observable independently
* from healthy PCM reserves and compressed-packet recovery.
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
            "Warning: Movie channel %d discarded %" PRIu64
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
* Transfer the retained look-ahead frame into presentation ownership. One
* promotion remains pending until composition acknowledges it, preventing
* repeated engine updates from silently replacing an undisplayed frame.
*/
static void movie_playback_promote_next_frame(
    s_movie_playback *playback
)
{
    yuv_frame_destroy(playback->current_frame);
    playback->current_frame = playback->next_frame;
    playback->next_frame = NULL;
    playback->frame_dirty = 1;
    playback->current_frame_presented = 0;
}

/*
* Caskey, Damon V.
* 2026-08-18
*
* Promote one due frame per presentation opportunity and preserve ordinary
* scheduling jitter behind the audio master. Only frames beyond the lateness
* tolerance may be discarded, while the initial queue snapshot still prevents
* a decoder from extending one engine update without bound.
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
