/*
 * OpenBOR - http://www.chronocrash.com
 * -----------------------------------------------------------------------
 * All rights reserved, see LICENSE in OpenBOR root for details.
 *
 * Copyright (c)  OpenBOR Team
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdint.h>
#include <inttypes.h>
#include <string.h>
#include <assert.h>
#include <limits.h>
#include "nestegg/nestegg.h"

// libvpx
#define VPX_CODEC_DISABLE_COMPAT 1
#include "vpx/vpx_decoder.h"
#include "vpx/vp8dx.h"

// our headers
#include "vidplay.h"
#include "vorbis.h"
#include "yuv.h"
#include "threads.h"
#include "types.h"
#include "globals.h"
#include "borendian.h"
#include "soundmix.h"
#include "timer.h"

/*
* Caskey, Damon V.
* 2026-08-15
*
* Keep enough compressed packets ahead of both decoders that a full
* video queue does not stop the shared demuxer before the audio packet
* reserve fills. Video needs more packet slots because its packet rate
* may greatly exceed the audio rate. Queue allocation stores pointers;
* PCM output remains separately bounded by the generic stream ring.
*/
#define AUDIO_PACKET_QUEUE_SIZE 64
#define VIDEO_PACKET_QUEUE_SIZE 512
#define FRAME_QUEUE_SIZE 10
#define VIDEO_DECODER_THREAD_MAX 8U
#define VIDEO_PACKET_QUEUE_BACKPRESSURE_THRESHOLD \
    ((VIDEO_PACKET_QUEUE_SIZE * 3) / 4)
#define AUDIO_RECOVERY_PCM_BUFFER_COUNT \
    (SOUND_STREAM_BUFFER_COUNT / 2U)
#define AUDIO_PREROLL_BUFFER_COUNT ((SOUND_STREAM_BUFFER_COUNT * 3U) / 4U)
#define AUDIO_PREROLL_TIMEOUT_MICROSECONDS UINT64_C(2000000)

#define debug_printf(...) //printf(__VA_ARGS__)

/*
* Caskey, Damon V.
* 2026-08-12
*
* Decoder queues and track contexts retain a pointer to
* their owning playback's independent stop state.
*/
typedef struct {
    int start;
    int size;
    int max_size;
    uint64_t replacement_count;
    uint64_t resync_count;
    SDL_atomic_t *quit;
    bor_mutex *mutex;
    bor_cond *not_full;
    bor_cond *not_empty;
    void *data[ANYNUMBER];
} FixedSizeQueue;

typedef struct {
    FixedSizeQueue *packet_queue;
    FixedSizeQueue *video_packet_queue;
    FixedSizeQueue *video_frame_queue;
    vorbis_context vorbis_ctx;
    int frequency;
    int avail_samples;
    int last_samples;
    int channel;
    int stream_play_id;
    int output_active;
    int output_paused;
    int aligning;
    uint64_t observed_underrun_count;
    uint64_t reported_underrun_count;
    uint64_t playback_start_timestamp;
    uint64_t output_timestamp;
    uint64_t leading_silence_frames;
    SDL_atomic_t *quit;
    uint8_t pcm_buffer[SOUND_STREAM_BUFFER_SIZE];
} audio_context;

typedef struct {
    FixedSizeQueue *packet_queue;
    vpx_codec_ctx_t vpx_ctx;
    FixedSizeQueue *frame_queue;
    int width;
    int height;
    int display_width;
    int display_height;
    int playback_paused;
    uint64_t frame_delay;
    SDL_atomic_t *quit;
} video_context;

typedef struct {
    int packhandle;
    packfile_signed_offset_t stream_size;
    const unsigned char *cache_buffer;
    size_t cache_size;
    size_t cache_position;
} webm_io_context;

// see header for typedef
struct webm_context {
    webm_io_context io_ctx;
    nestegg *nestegg_ctx;
    audio_context audio_ctx;
    video_context video_ctx;
    FixedSizeQueue *audio_queue;
    int audio_track;
    int video_track;
    bor_thread *the_demux_thread;
    bor_thread *the_video_thread;
    bor_thread *the_audio_thread;
    bor_thread *the_lifecycle_thread;
    bor_mutex *lifecycle_mutex;
    bor_cond *lifecycle_condition;
    char *path;
    uint64_t duration;
    uint64_t seek_timestamp;
    e_webm_decoder_state decoder_state;
    int volume;
    int sound_channel;
    int play_audio;
    int replace_all_audio;
    int close_requested;
    int video_initialized;
    int audio_initialized;
    SDL_atomic_t quit;
};

static bor_mutex *webm_lifecycle_operation_mutex;

/*
* Caskey, Damon V.
* 2026-08-17
*
* Read the decoder stop flag with SDL atomics. Queue, demux, codec, and
* lifecycle workers share this flag without relying on volatile accesses
* that provide no cross-thread synchronization in C.
*/
static int webm_stop_is_requested(SDL_atomic_t *stop)
{
    return !stop || SDL_AtomicGet(stop) != 0;
}

/*
* Caskey, Damon V.
* 2026-08-17
*
* Publish decoder cancellation through the same SDL atomic consumed by every
* queue and worker, preserving one stop path across lifecycle transitions.
*/
static void webm_request_decoder_stop(SDL_atomic_t *stop)
{
    if(stop) {
        SDL_AtomicSet(stop, 1);
    }
}

/*
* Caskey, Damon V.
* 2026-08-17
*
* Serialize decoder resource acquisition and release on lifecycle workers.
* Independent decoding remains concurrent, while sound-channel replacement,
* container setup, and teardown cannot overtake one another during rapid
* asynchronous channel replacement.
*/
int webm_lifecycle_init(void)
{
    if(webm_lifecycle_operation_mutex) {
        return 1;
    }
    webm_lifecycle_operation_mutex = mutex_create();
    return webm_lifecycle_operation_mutex != NULL;
}

/*
* Caskey, Damon V.
* 2026-08-17
*
* Release shared lifecycle serialization after every decoder has reached
* CLOSED and its lifecycle thread has been reaped during engine shutdown.
*/
void webm_lifecycle_shutdown(void)
{
    if(webm_lifecycle_operation_mutex) {
        mutex_destroy(webm_lifecycle_operation_mutex);
        webm_lifecycle_operation_mutex = NULL;
    }
}

/*
* Caskey, Damon V.
* 2026-08-17
*
* Retain the source path for the lifetime of an asynchronous decoder open.
* The movie source registry may otherwise release its caller-owned string
* before the lifecycle worker has finished opening a streamed source.
*/
static char *webm_copy_path(const char *path)
{
    char *copy;
    size_t length;

    if(!path || !path[0]) {
        return NULL;
    }
    length = strlen(path) + 1U;
    copy = malloc(length);
    if(copy) {
        memcpy(copy, path, length);
    }
    return copy;
}

/*
* Caskey, Damon V.
* 2026-08-17
*
* Read asynchronous cancellation under the lifecycle mutex. The engine
* thread publishes only this request; the lifecycle worker remains the sole
* writer of the decoder stop flag consumed by demux and codec workers.
*/
static int webm_close_is_requested(webm_context *ctx)
{
    int close_requested;

    mutex_lock(ctx->lifecycle_mutex);
    close_requested = ctx->close_requested;
    mutex_unlock(ctx->lifecycle_mutex);
    return close_requested;
}

/*
* Caskey, Damon V.
* 2026-08-13
*
* Read or seek either a creator-requested memory cache or the
* ordinary packfile stream through the same Nestegg I/O. Streamed
* reads supply complete spans, while successful seeks return zero
* as required by the decoder callback contract.
*/
static int webm_io_read(void *buffer, size_t length, void *userdata)
{
    webm_io_context *io_ctx = userdata;
    packfile_signed_offset_t stream_position;
    size_t read_position = 0;
    int bytesRead;

    if(io_ctx->cache_buffer) {
        size_t remaining;

        if(io_ctx->cache_position > io_ctx->cache_size) {
            return -1;
        }
        remaining = io_ctx->cache_size - io_ctx->cache_position;
        if(remaining < length) {
            return 0;
        }
        memcpy(
            buffer,
            io_ctx->cache_buffer + io_ctx->cache_position,
            length
        );
        io_ctx->cache_position += length;
        return 1;
    }

    stream_position = seekpackfile64(io_ctx->packhandle, 0, SEEK_CUR);
    if(stream_position < 0 ||
       stream_position > io_ctx->stream_size ||
       (uint64_t)length >
           (uint64_t)(io_ctx->stream_size - stream_position)) {
        return stream_position < 0 ? -1 : 0;
    }

    while(read_position < length) {
        size_t remaining = length - read_position;
        int read_length = remaining > (size_t)INT_MAX
            ? INT_MAX
            : (int)remaining;

        bytesRead = readpackfile(
            io_ctx->packhandle,
            (unsigned char*)buffer + read_position,
            read_length
        );
        if(bytesRead <= 0) {
            return -1;
        }
        read_position += (size_t)bytesRead;
    }
    if(read_position != length) {
        return -1;
    }
    return 1;
}

static int webm_io_seek(int64_t offset, int whence, void *userdata)
{
    webm_io_context *io_ctx = userdata;

    if(io_ctx->cache_buffer) {
        int64_t base;
        int64_t position;

        switch(whence) {
            case SEEK_SET:
                base = 0;
                break;
            case SEEK_CUR:
                base = (int64_t)io_ctx->cache_position;
                break;
            case SEEK_END:
                base = (int64_t)io_ctx->cache_size;
                break;
            default:
                return -1;
        }

        position = base + offset;
        if(position < 0 || (uint64_t)position > (uint64_t)io_ctx->cache_size) {
            return -1;
        }
        io_ctx->cache_position = (size_t)position;
        return 0;
    }

    return seekpackfile64(
        io_ctx->packhandle,
        (packfile_signed_offset_t)offset,
        whence
    ) < 0 ? -1 : 0;
}

static int64_t webm_io_tell(void *userdata)
{
    webm_io_context *io_ctx = userdata;

    if(io_ctx->cache_buffer) {
        return (int64_t)io_ctx->cache_position;
    }
    return seekpackfile64(io_ctx->packhandle, 0, SEEK_CUR);
}

static FixedSizeQueue *queue_init(int max_size, SDL_atomic_t *quit)
{
    FixedSizeQueue *queue;

    if(max_size < 1 || !quit) {
        return NULL;
    }
    queue = malloc(sizeof(*queue) - sizeof(queue->data) + (max_size * sizeof(void *)));
    if(!queue) {
        return NULL;
    }
    queue->start = 0;
    queue->size = 0;
    queue->max_size = max_size;
    queue->replacement_count = 0;
    queue->resync_count = 0;
    queue->quit = quit;
    queue->mutex = mutex_create();
    queue->not_full = cond_create();
    queue->not_empty = cond_create();
    if(!queue->mutex || !queue->not_full || !queue->not_empty) {
        if(queue->not_full) cond_destroy(queue->not_full);
        if(queue->not_empty) cond_destroy(queue->not_empty);
        if(queue->mutex) mutex_destroy(queue->mutex);
        free(queue);
        return NULL;
    }
    return queue;
}

#define SPIT(fmt, ...) debug_printf("%s:%i(%p): " fmt, __func__, __LINE__, queue, __VA_ARGS__)

// returns 0 on success; <0 means that the caller should clean up
// and exit
int queue_insert(FixedSizeQueue *queue, void *data)
{
    mutex_lock(queue->mutex);
    //SPIT("size=%i\n", queue->size);
    if (queue->size == queue->max_size)
    {
        while(cond_wait_timed(queue->not_full, queue->mutex, 10) != 0)
        {
            if(webm_stop_is_requested(queue->quit))
            {
                mutex_unlock(queue->mutex);
                return -1;
            }
            else if (queue->size < queue->max_size) break;
        }
    }
    assert(queue->size < queue->max_size);
    int index = (queue->start + queue->size) % queue->max_size;
    queue->data[index] = data;
    ++queue->size;
    //SPIT("size=%i\n", queue->size);
    cond_signal(queue->not_empty);
    mutex_unlock(queue->mutex);
    return 0;
}

// returns pointer on success, NULL indicates that the caller
// should clean up and exit
void *queue_get(FixedSizeQueue *queue)
{
    mutex_lock(queue->mutex);
    //SPIT("size=%i\n", queue->size);
    if (queue->size == 0)
    {
        while (cond_wait_timed(queue->not_empty, queue->mutex, 10) != 0)
        {
            if(webm_stop_is_requested(queue->quit))
            {
                mutex_unlock(queue->mutex);
                return NULL;
            }
            else if (queue->size > 0) break;
        }
    }
    assert(queue->size > 0);
    void *data = queue->data[queue->start];
    --queue->size;
    queue->start = (queue->start + 1) % queue->max_size;
    //SPIT("size=%i\n", queue->size);
    cond_signal(queue->not_full);
    mutex_unlock(queue->mutex);
    return data;
}

/*
* Caskey, Damon V.
* 2026-08-12
*
* Remove one queued item without stalling the main
* engine thread when a decoder has not produced it yet.
*/
static int queue_try_get(FixedSizeQueue *queue, void **data)
{
    if(!queue || !data) {
        return 0;
    }

    mutex_lock(queue->mutex);
    if(queue->size == 0) {
        mutex_unlock(queue->mutex);
        return 0;
    }

    *data = queue->data[queue->start];
    --queue->size;
    queue->start = (queue->start + 1) % queue->max_size;
    cond_signal(queue->not_full);
    mutex_unlock(queue->mutex);
    return 1;
}

/*
* Caskey, Damon V.
* 2026-08-15
*
* Capture queue occupancy, replacement totals, and recovery events only when
* reporting exceptional state. Normal decoding and publication incur no
* diagnostic polling.
*/
static int queue_get_status(
    FixedSizeQueue *queue,
    uint64_t *replacement_count,
    uint64_t *resync_count
)
{
    int size;

    if(replacement_count) {
        *replacement_count = 0;
    }
    if(resync_count) {
        *resync_count = 0;
    }
    if(!queue) {
        return -1;
    }

    mutex_lock(queue->mutex);
    size = queue->size;
    if(replacement_count) {
        *replacement_count = queue->replacement_count;
    }
    if(resync_count) {
        *resync_count = queue->resync_count;
    }
    mutex_unlock(queue->mutex);
    return size;
}

/*
* Caskey, Damon V.
* 2026-08-15
*
* Identify a VP8 keyframe from its uncompressed frame tag and start code.
* WebM video packets may contain laced frames; recovery resumes only when
* the first frame in a packet establishes an independent decode sequence.
*/
static int video_packet_is_keyframe(nestegg_packet *packet)
{
    unsigned char *data;
    size_t data_size;

    if(!packet ||
       nestegg_packet_data(packet, 0, &data, &data_size) < 0 ||
       !data ||
       data_size < 10U) {
        return 0;
    }

    return !(data[0] & 1U) &&
        data[3] == 0x9dU &&
        data[4] == 0x01U &&
        data[5] == 0x2aU;
}

/*
* Caskey, Damon V.
* 2026-08-15
*
* Read playback pause state under the same frame-queue lock used by
* webm_set_audio_paused(). Paused movies retain their complete compressed
* and decoded queues because their audio reserve is intentionally idle.
*/
static int video_playback_is_paused(video_context *video_ctx)
{
    int paused;

    if(!video_ctx || !video_ctx->frame_queue) {
        return 1;
    }

    mutex_lock(video_ctx->frame_queue->mutex);
    paused = video_ctx->playback_paused;
    mutex_unlock(video_ctx->frame_queue->mutex);
    return paused;
}

/*
* Caskey, Damon V.
* 2026-08-15
*
* Detect genuine audio starvation risk before sacrificing video backlog.
* Empty compressed audio alone is harmless while the PCM ring remains
* healthy. Recovery begins only when both reserves are depleted and active
* playback would otherwise let a full video queue block the shared demuxer.
*/
static int webm_audio_reserve_is_critical(webm_context *ctx)
{
    s_sound_pcm_stream_status status;

    if(!ctx ||
       ctx->audio_track < 0 ||
       video_playback_is_paused(&ctx->video_ctx) ||
       queue_get_status(ctx->audio_ctx.packet_queue, NULL, NULL) != 0 ||
       !sound_get_channel_pcm_stream_status_owned(
           ctx->audio_ctx.channel,
           ctx->audio_ctx.stream_play_id,
           &status
       )) {
        return 0;
    }

    return !status.producer_finished &&
        status.ready_buffer_count <= AUDIO_RECOVERY_PCM_BUFFER_COUNT;
}

/*
* Caskey, Damon V.
* 2026-08-17
*
* Try to publish the first verified keyframe following a compressed-video
* queue reset without waiting. The demuxer is the queue's only producer, so
* capacity remains available after the stale backlog has been discarded.
*/
static int video_packet_queue_try_insert(
    FixedSizeQueue *queue,
    nestegg_packet *packet
)
{
    int index;

    if(!queue || !packet) {
        return -1;
    }

    mutex_lock(queue->mutex);
    if(webm_stop_is_requested(queue->quit)) {
        mutex_unlock(queue->mutex);
        return -1;
    }
    if(queue->size == queue->max_size) {
        mutex_unlock(queue->mutex);
        return 0;
    }
    index = (queue->start + queue->size) % queue->max_size;
    queue->data[index] = packet;
    queue->size++;
    cond_signal(queue->not_empty);
    mutex_unlock(queue->mutex);
    return 1;
}

/*
* Caskey, Damon V.
* 2026-08-17
*
* Atomically discard the stale compressed-video backlog and optionally make
* the packet that triggered recovery its new head when that packet is already
* a verified keyframe. Packet destruction occurs after releasing the queue
* lock so the decoder can resume immediately from the replacement boundary.
*/
static int video_packet_queue_begin_resync(
    FixedSizeQueue *queue,
    nestegg_packet *recovery_keyframe
)
{
    nestegg_packet *discarded_packets[VIDEO_PACKET_QUEUE_SIZE];
    int discarded_count;
    int i;

    if(!queue ||
       queue->max_size < 1 ||
       queue->max_size > VIDEO_PACKET_QUEUE_SIZE) {
        return -1;
    }

    mutex_lock(queue->mutex);
    if(webm_stop_is_requested(queue->quit)) {
        mutex_unlock(queue->mutex);
        return -1;
    }
    discarded_count = queue->size;
    for(i = 0; i < discarded_count; ++i) {
        int index = (queue->start + i) % queue->max_size;

        discarded_packets[i] = queue->data[index];
        queue->data[index] = NULL;
    }
    queue->start = 0;
    queue->size = 0;
    if((uint64_t)discarded_count >
       UINT64_MAX - queue->replacement_count) {
        queue->replacement_count = UINT64_MAX;
    }
    else {
        queue->replacement_count += (uint64_t)discarded_count;
    }
    if(queue->resync_count < UINT64_MAX) {
        queue->resync_count++;
    }
    if(recovery_keyframe) {
        queue->data[0] = recovery_keyframe;
        queue->size = 1;
        cond_signal(queue->not_empty);
    }
    cond_signal(queue->not_full);
    mutex_unlock(queue->mutex);

    for(i = 0; i < discarded_count; ++i) {
        if(discarded_packets[i]) {
            nestegg_free_packet(discarded_packets[i]);
        }
    }
    return 1;
}

/*
* Caskey, Damon V.
* 2026-08-17
*
* Record a newly demuxed packet discarded while waiting for the first
* verified keyframe after a compressed-video queue reset.
*/
static void video_packet_queue_record_drop(FixedSizeQueue *queue)
{
    if(!queue) {
        return;
    }

    mutex_lock(queue->mutex);
    if(queue->replacement_count < UINT64_MAX) {
        queue->replacement_count++;
    }
    mutex_unlock(queue->mutex);
}

/*
* Caskey, Damon V.
* 2026-08-15
*
* Insert compressed video with ordinary bounded blocking while audio has
* usable reserves. Return zero without consuming the packet when audio
* becomes critical, allowing the demuxer to perform keyframe recovery.
*/
static int video_packet_queue_insert(
    webm_context *ctx,
    nestegg_packet *packet
)
{
    FixedSizeQueue *queue;

    if(!ctx || !(queue = ctx->video_ctx.packet_queue)) {
        return -1;
    }

    while(!webm_stop_is_requested(&ctx->quit)) {
        int index;
        int queue_full;

        mutex_lock(queue->mutex);
        if(queue->size < queue->max_size) {
            index = (queue->start + queue->size) % queue->max_size;
            queue->data[index] = packet;
            queue->size++;
            cond_signal(queue->not_empty);
            mutex_unlock(queue->mutex);
            return 1;
        }

        cond_wait_timed(queue->not_full, queue->mutex, 10);
        queue_full = queue->size == queue->max_size;
        mutex_unlock(queue->mutex);
        if(queue_full && webm_audio_reserve_is_critical(ctx)) {
            return 0;
        }
    }
    return -1;
}

/*
* Caskey, Damon V.
* 2026-08-15
*
* Publish decoded frames in timestamp order during ordinary read-ahead.
* When active playback stalls long enough to fill most of the compressed
* video reserve, replace the oldest decoded frame instead of allowing video
* backpressure to stop the shared demuxer and starve audio. Paused playback
* retains every queued frame and continues using bounded blocking behavior.
*/
static int video_frame_queue_insert(
    video_context *video_ctx,
    void *data,
    void **replaced
)
{
    FixedSizeQueue *frame_queue;

    if(!video_ctx || !video_ctx->frame_queue ||
       !video_ctx->packet_queue || !replaced) {
        return -1;
    }
    frame_queue = video_ctx->frame_queue;
    *replaced = NULL;

    while(!webm_stop_is_requested(frame_queue->quit)) {
        int index;
        int video_packet_count = queue_get_status(
            video_ctx->packet_queue,
            NULL,
            NULL
        );

        mutex_lock(frame_queue->mutex);
        if(webm_stop_is_requested(frame_queue->quit)) {
            mutex_unlock(frame_queue->mutex);
            return -1;
        }
        if(frame_queue->size == frame_queue->max_size &&
           !video_ctx->playback_paused &&
           video_packet_count >=
               VIDEO_PACKET_QUEUE_BACKPRESSURE_THRESHOLD) {
            *replaced = frame_queue->data[frame_queue->start];
            frame_queue->start =
                (frame_queue->start + 1) % frame_queue->max_size;
            --frame_queue->size;
            if(frame_queue->replacement_count < UINT64_MAX) {
                frame_queue->replacement_count++;
            }
        }
        if(frame_queue->size < frame_queue->max_size) {
            index = (frame_queue->start + frame_queue->size) %
                frame_queue->max_size;
            frame_queue->data[index] = data;
            ++frame_queue->size;
            cond_signal(frame_queue->not_empty);
            mutex_unlock(frame_queue->mutex);
            return 0;
        }

        cond_wait_timed(
            frame_queue->not_full,
            frame_queue->mutex,
            10
        );
        mutex_unlock(frame_queue->mutex);
    }
    return -1;
}

void queue_destroy(FixedSizeQueue *queue)
{
    cond_destroy(queue->not_full);
    cond_destroy(queue->not_empty);
    mutex_destroy(queue->mutex);
    free(queue);
}

// used to keep playing current BGM in videos with no audio track
static int bgm_update_thread(void *data)
{
    webm_context *ctx = data;

    while(!webm_stop_is_requested(&ctx->quit))
    {
        sound_update_music();
        usleep(5000);
    }
    return 0;
}

/*
* Caskey, Damon V.
* 2026-08-12
*
* Convert a nanosecond interval to the number of complete PCM
* frames needed to reach or pass its end without overflowing.
*/
static uint64_t audio_nanoseconds_to_frames(
    uint64_t nanoseconds,
    int frequency
)
{
    const uint64_t nanoseconds_per_second = UINT64_C(1000000000);
    uint64_t whole_seconds;
    uint64_t remainder;
    uint64_t frames;
    uint64_t remainder_frames;

    if(frequency <= 0) {
        return UINT64_MAX;
    }

    whole_seconds = nanoseconds / nanoseconds_per_second;
    remainder = nanoseconds % nanoseconds_per_second;
    if(whole_seconds > UINT64_MAX / (uint64_t)frequency) {
        return UINT64_MAX;
    }
    frames = whole_seconds * (uint64_t)frequency;
    remainder_frames = remainder * (uint64_t)frequency;
    if(remainder_frames) {
        remainder_frames =
            ((remainder_frames - 1U) / nanoseconds_per_second) + 1U;
    }
    if(frames > UINT64_MAX - remainder_frames) {
        return UINT64_MAX;
    }
    return frames + remainder_frames;
}

/*
* Caskey, Damon V.
* 2026-08-17
*
* Convert a consumed PCM source-frame count to its media timeline duration
* without overflowing the 64-bit nanosecond clock. Whole seconds are split
* before multiplication so long-running streams retain bounded arithmetic.
*/
static uint64_t audio_frames_to_nanoseconds(
    uint64_t frame_count,
    int frequency
)
{
    const uint64_t nanoseconds_per_second = UINT64_C(1000000000);
    uint64_t whole_seconds;
    uint64_t remainder_frames;
    uint64_t remainder_nanoseconds;
    uint64_t nanoseconds;

    if(frequency <= 0) {
        return UINT64_MAX;
    }

    whole_seconds = frame_count / (uint64_t)frequency;
    remainder_frames = frame_count % (uint64_t)frequency;
    if(whole_seconds > UINT64_MAX / nanoseconds_per_second) {
        return UINT64_MAX;
    }
    nanoseconds = whole_seconds * nanoseconds_per_second;
    remainder_nanoseconds =
        remainder_frames * nanoseconds_per_second / (uint64_t)frequency;
    return nanoseconds <= UINT64_MAX - remainder_nanoseconds
        ? nanoseconds + remainder_nanoseconds
        : UINT64_MAX;
}

/*
* Caskey, Damon V.
* 2026-08-12
*
* Align decoded audio to the requested movie timestamp. Samples
* preceding the target are consumed silently. If Nestegg returns
* the first audio packet after the target, equivalent silence is
* inserted so audio retains its position on the movie timeline.
*/
static void audio_align_packet(
    audio_context *audio_ctx,
    uint64_t packet_timestamp
)
{
    uint64_t frame_count;
    int available_samples;
    int skipped_samples;

    if(!audio_ctx->aligning || audio_ctx->avail_samples <= 0) {
        return;
    }

    if(packet_timestamp < audio_ctx->output_timestamp) {
        available_samples = audio_ctx->avail_samples;
        frame_count = audio_nanoseconds_to_frames(
            audio_ctx->output_timestamp - packet_timestamp,
            audio_ctx->frequency
        );
        skipped_samples = frame_count < (uint64_t)available_samples
            ? (int)frame_count
            : available_samples;
        vorbis_skip_pcm(&audio_ctx->vorbis_ctx, (size_t)skipped_samples);
        audio_ctx->avail_samples -= skipped_samples;
        if(skipped_samples < available_samples) {
            audio_ctx->aligning = 0;
        }
        return;
    }

    audio_ctx->leading_silence_frames = audio_nanoseconds_to_frames(
        packet_timestamp - audio_ctx->output_timestamp,
        audio_ctx->frequency
    );
    audio_ctx->aligning = 0;
}

static int audio_decode_frame(
    audio_context *audio_ctx,
    uint8_t *audio_buf,
    int buf_size,
    int *terminal
)
{
    vorbis_context *vorbis_ctx = &audio_ctx->vorbis_ctx;
    //audio_clock += 1000000000LL * audio_ctx->last_samples / audio_ctx->frequency;
    int samples = buf_size / (vorbis_ctx->channels * 2);
    int samples_written = 0;

    *terminal = 0;

    while (samples)
    {
        if(audio_ctx->leading_silence_frames) {
            int silence_samples = audio_ctx->leading_silence_frames <
                (uint64_t)samples
                ? (int)audio_ctx->leading_silence_frames
                : samples;
            size_t silence_bytes =
                (size_t)silence_samples *
                (size_t)vorbis_ctx->channels *
                sizeof(int16_t);

            memset(audio_buf, 0, silence_bytes);
            audio_buf += silence_bytes;
            audio_ctx->leading_silence_frames -=
                (uint64_t)silence_samples;
            samples -= silence_samples;
            samples_written += silence_samples;
            continue;
        }

        if (audio_ctx->avail_samples == 0)
        {
            nestegg_packet *pkt;
            uint64_t timestamp;
            unsigned chunk, num_chunks;

            debug_printf("audio queue size=%i\n", audio_ctx->packet_queue->size);
            if ((pkt = queue_get(audio_ctx->packet_queue)) == NULL)
            {
                *terminal = 1;
                break;
            }
            nestegg_packet_tstamp(pkt, &timestamp);
            //audio_clock = timestamp;
            nestegg_packet_count(pkt, &num_chunks);
            for (chunk=0; chunk<num_chunks; chunk++)
            {
                unsigned char *data;
                size_t data_size;
                nestegg_packet_data(pkt, chunk, &data, &data_size);
                audio_ctx->avail_samples = vorbis_packet(vorbis_ctx, data, data_size);
            }
            nestegg_free_packet(pkt);
            audio_align_packet(audio_ctx, timestamp);
            continue;
        }

        int samples_read = MIN(audio_ctx->avail_samples, samples);
        vorbis_getpcm(vorbis_ctx, audio_buf, samples_read);
        audio_buf += 2 * vorbis_ctx->channels * samples_read;
        audio_ctx->avail_samples -= samples_read;
        samples -= samples_read;
        samples_written += samples_read;
    }

    audio_ctx->last_samples = samples_written;
    return samples_written * vorbis_ctx->channels * (int)sizeof(int16_t);
}

/*
* Caskey, Damon V.
* 2026-08-15
*
* Decode and publish WebM audio from an elevated-priority producer.
* High-resolution VP8 decoding and software composition must not prevent
* this thread from replenishing the real-time mixer's PCM reserve.
*/
static int audio_thread(void *data)
{
    audio_context *audio_ctx = (audio_context *)data;
    int decoded_bytes;
    int frame_count;
    int queue_result;
    int terminal;
    unsigned int retry_delay_microseconds;
    uint64_t underrun_count;

    /* Priority elevation may be denied on restricted platforms. */
    SDL_SetThreadPriority(SDL_THREAD_PRIORITY_HIGH);

    while(!webm_stop_is_requested(audio_ctx->quit))
    {
        decoded_bytes = audio_decode_frame(
            audio_ctx,
            audio_ctx->pcm_buffer,
            sizeof(audio_ctx->pcm_buffer),
            &terminal
        );
        frame_count = decoded_bytes /
            (audio_ctx->vorbis_ctx.channels * (int)sizeof(int16_t));

#ifdef BOR_BIG_ENDIAN
        /* Generic 16-bit channel stream buffers use little-endian PCM. */
        {
            int sample_index;
            uint16_t *pcm = (uint16_t *)audio_ctx->pcm_buffer;

            for(sample_index = 0;
                sample_index < decoded_bytes / (int)sizeof(*pcm);
                sample_index++) {
                pcm[sample_index] = SwapLSB16(pcm[sample_index]);
            }
        }
#endif

        queue_result = 1;
        if(audio_ctx->output_active) {
            do {
                queue_result = sound_queue_channel_pcm_stream(
                    audio_ctx->channel,
                    audio_ctx->stream_play_id,
                    frame_count > 0 ? audio_ctx->pcm_buffer : NULL,
                    (uint64_t)frame_count,
                    terminal,
                    &underrun_count,
                    &retry_delay_microseconds
                );
                if(queue_result == 0) {
                    usleep(retry_delay_microseconds);
                }
            } while(!webm_stop_is_requested(audio_ctx->quit) &&
                    queue_result == 0);

            if(queue_result > 0 &&
               underrun_count != audio_ctx->observed_underrun_count) {
                audio_ctx->observed_underrun_count = underrun_count;
                if(!audio_ctx->reported_underrun_count ||
                   (underrun_count > audio_ctx->reported_underrun_count &&
                    underrun_count - audio_ctx->reported_underrun_count >=
                        UINT64_C(16))) {
                    s_sound_pcm_stream_status status = { 0 };
                    uint64_t dropped_packets;
                    uint64_t dropped_frames;
                    uint64_t resync_count;
                    int audio_packets;
                    int decoded_frames;
                    int video_packets;

                    sound_get_channel_pcm_stream_status_owned(
                        audio_ctx->channel,
                        audio_ctx->stream_play_id,
                        &status
                    );
                    audio_packets = queue_get_status(
                        audio_ctx->packet_queue,
                        NULL,
                        NULL
                    );
                    video_packets = queue_get_status(
                        audio_ctx->video_packet_queue,
                        &dropped_packets,
                        &resync_count
                    );
                    decoded_frames = queue_get_status(
                        audio_ctx->video_frame_queue,
                        &dropped_frames,
                        NULL
                    );
                    audio_ctx->reported_underrun_count = underrun_count;
                    printf(
                        "Warning: WebM audio underrun on sound channel %d "
                        "(%" PRIu64 " total): PCM=%u/%u, "
                        "packets=%d/%d audio and %d/%d video, "
                        "frames=%d/%d (%" PRIu64 " stale dropped), "
                        "recovery=%" PRIu64 " incoming packets skipped in %" PRIu64
                        " keyframe resync event%s.\n",
                        audio_ctx->channel,
                        underrun_count,
                        status.ready_buffer_count,
                        SOUND_STREAM_BUFFER_COUNT,
                        audio_packets,
                        AUDIO_PACKET_QUEUE_SIZE,
                        video_packets,
                        VIDEO_PACKET_QUEUE_SIZE,
                        decoded_frames,
                        FRAME_QUEUE_SIZE,
                        dropped_frames,
                        dropped_packets,
                        resync_count,
                        resync_count == 1U ? "" : "s"
                    );
                }
            }

            if(queue_result < 0) {
                /*
                 * Keep draining this track when another producer replaces
                 * its sound channel. Otherwise the full audio packet queue
                 * would also stall video demuxing.
                 */
                audio_ctx->output_active = 0;
            }
        }

        if(terminal) {
            return 0;
        }
    }

    return 0;
}

/*
* Caskey, Damon V.
* 2026-08-15
*
* Let the decoder establish its PCM reserve before the owning movie clock
* starts. The bounded wait preserves failure tolerance for malformed or
* unusually slow inputs while ordinary playback begins with continuous
* audio already available to the mixer.
*/
static void audio_wait_for_preroll(webm_context *ctx)
{
    audio_context *audio_ctx;
    uint64_t start_time;

    if(!ctx || !ctx->audio_initialized) {
        return;
    }
    audio_ctx = &ctx->audio_ctx;
    start_time = timer_uticks();

    while(!webm_stop_is_requested(audio_ctx->quit) &&
          !webm_close_is_requested(ctx)) {
        s_sound_pcm_stream_status status;
        uint64_t now;

        if(!sound_get_channel_pcm_stream_status_owned(
            audio_ctx->channel,
            audio_ctx->stream_play_id,
            &status
        )) {
            return;
        }
        audio_ctx->observed_underrun_count = status.underrun_count;
        if(status.ready_buffer_count >= AUDIO_PREROLL_BUFFER_COUNT ||
           (status.producer_finished && status.ready_buffer_count > 0)) {
            return;
        }

        now = timer_uticks();
        if(now < start_time ||
           now - start_time >= AUDIO_PREROLL_TIMEOUT_MICROSECONDS) {
            return;
        }
        usleep(1000);
    }
}

/*
* Caskey, Damon V.
* 2026-08-12
*
* Initialize WebM audio on an explicit generic channel with
* play-ID ownership and optional legacy replace-all behavior.
*/
static int init_audio(
    nestegg *ctx,
    int track,
    audio_context *audio_ctx,
    int volume,
    int sound_channel,
    uint64_t seek_timestamp,
    int replace_all_audio,
    SDL_atomic_t *quit
)
{
    // read vorbis header and initialize vorbis decoding
    unsigned chunk, chunks;
    nestegg_audio_params audioParams;

    vorbis_init(&(audio_ctx->vorbis_ctx));
    if(nestegg_track_codec_data_count(ctx, track, &chunks) < 0 || chunks != 3) {
        vorbis_destroy(&(audio_ctx->vorbis_ctx));
        return -1;
    }
    for (chunk=0; chunk<chunks; chunk++)
    {
        unsigned char *data;
        size_t data_size;
        if(nestegg_track_codec_data(ctx, track, chunk, &data, &data_size) < 0) {
            vorbis_destroy(&(audio_ctx->vorbis_ctx));
            return -1;
        }
        vorbis_headerpacket(&(audio_ctx->vorbis_ctx), data, data_size, chunk);
    }

    // initialize audio decoding context
    vorbis_prepare(&(audio_ctx->vorbis_ctx));
    if(nestegg_track_audio_params(ctx, track, &audioParams) < 0 ||
       audioParams.rate < 1.0 ||
       audioParams.rate > (double)INT_MAX ||
       (audioParams.channels != CHANNEL_TYPE_MONO &&
        audioParams.channels != CHANNEL_TYPE_STEREO)) {
        vorbis_destroy(&(audio_ctx->vorbis_ctx));
        return -1;
    }
    audio_ctx->vorbis_ctx.channels = audioParams.channels;
    audio_ctx->frequency = (int)audioParams.rate;
    audio_ctx->avail_samples = audio_ctx->last_samples = 0;
    audio_ctx->aligning = 1;
    audio_ctx->playback_start_timestamp = seek_timestamp;
    audio_ctx->output_timestamp = seek_timestamp <=
        UINT64_MAX - audioParams.codec_delay
        ? seek_timestamp + audioParams.codec_delay
        : UINT64_MAX;
    audio_ctx->quit = quit;
    audio_ctx->packet_queue = queue_init(AUDIO_PACKET_QUEUE_SIZE, quit);
    if(!audio_ctx->packet_queue) {
        vorbis_destroy(&(audio_ctx->vorbis_ctx));
        return -1;
    }
    printf("Audio track: %f Hz, %d channels, %d bits/sample\n",
            audioParams.rate, audioParams.channels, audioParams.depth);
    if(audio_ctx->frequency < SOUND_MUSIC_FREQUENCY_MIN ||
       audio_ctx->frequency > SOUND_MUSIC_FREQUENCY_MAX)
    {
        printf("Warning: the audio frequency (%i Hz) is outside the supported %i-%i Hz range\n",
                audio_ctx->frequency,
                SOUND_MUSIC_FREQUENCY_MIN,
                SOUND_MUSIC_FREQUENCY_MAX);
    }

    if(replace_all_audio) {
        /* Legacy fullscreen playback retains its replace-all behavior. */
        sound_stopall_sample(true);
    }
    audio_ctx->channel = sound_channel;
    audio_ctx->stream_play_id = sound_open_channel_pcm_stream(
        audio_ctx->channel,
        audio_ctx->frequency,
        audioParams.channels,
        volume
    );
    if(audio_ctx->stream_play_id < 0) {
        queue_destroy(audio_ctx->packet_queue);
        audio_ctx->packet_queue = NULL;
        vorbis_destroy(&(audio_ctx->vorbis_ctx));
        return -1;
    }

    audio_ctx->output_active = 1;
    sound_set_channel_volume(
        audio_ctx->channel,
        SOUND_SPATIAL_CHANNEL_LEFT,
        volume
    );
    sound_set_channel_volume(
        audio_ctx->channel,
        SOUND_SPATIAL_CHANNEL_RIGHT,
        volume
    );
    sound_set_channel_volume_divisor(
        audio_ctx->channel,
        SOUND_VOLUME_DIVISOR_MUSIC
    );
    if(!sound_pause_channel_owned(
        audio_ctx->channel,
        audio_ctx->stream_play_id,
        1
    )) {
        sound_close_channel_pcm_stream(
            audio_ctx->channel,
            audio_ctx->stream_play_id
        );
        queue_destroy(audio_ctx->packet_queue);
        audio_ctx->packet_queue = NULL;
        vorbis_destroy(&(audio_ctx->vorbis_ctx));
        return -1;
    }
    audio_ctx->output_paused = 1;
    {
        s_sound_pcm_stream_status status;

        if(sound_get_channel_pcm_stream_status_owned(
            audio_ctx->channel,
            audio_ctx->stream_play_id,
            &status
        )) {
            audio_ctx->observed_underrun_count = status.underrun_count;
        }
    }
    return 0;
}

static void close_audio(audio_context *audio_ctx)
{
    // empty and free the packet queue
    while(audio_ctx->packet_queue && audio_ctx->packet_queue->size)
    {
        nestegg_packet *packet = queue_get(audio_ctx->packet_queue);
        if(packet) nestegg_free_packet(packet);
    }
    if(audio_ctx->packet_queue) {
        queue_destroy(audio_ctx->packet_queue);
    }

    // close the vorbis decoding context
    vorbis_destroy(&(audio_ctx->vorbis_ctx));

    sound_close_channel_pcm_stream(
        audio_ctx->channel,
        audio_ctx->stream_play_id
    );
}

/*
* Caskey, Damon V.
* 2026-08-13
*
* Resolve timestamps for laced video frames without requiring the
* optional WebM DefaultDuration element. Prefer the track default,
* then the packet duration, and finally preserve decode order with
* the smallest representable timestamp step.
*/
static uint64_t video_packet_frame_delay(
    video_context *video_ctx,
    nestegg_packet *packet,
    unsigned int chunks
)
{
    uint64_t packet_duration = 0;

    if(video_ctx->frame_delay || chunks < 2) {
        return video_ctx->frame_delay;
    }
    nestegg_packet_duration(packet, &packet_duration);
    if(packet_duration) {
        packet_duration /= chunks;
    }
    return packet_duration ? packet_duration : UINT64_C(1);
}

static int video_thread(void *data)
{
    video_context *ctx = (video_context*) data;
    uint64_t frame_delay;
    uint64_t timestamp;

    while(!webm_stop_is_requested(ctx->quit))
    {
        unsigned int chunk, chunks;
        nestegg_packet *pkt;

        debug_printf("video queue size=%i\n", ctx->packet_queue->size);
        pkt = queue_get(ctx->packet_queue);
        if(webm_stop_is_requested(ctx->quit) || pkt == NULL) break;
        nestegg_packet_count(pkt, &chunks);
        nestegg_packet_tstamp(pkt, &timestamp);
        frame_delay = video_packet_frame_delay(ctx, pkt, chunks);

        for (chunk = 0; chunk < chunks; ++chunk)
        {
            unsigned char *data;
            size_t data_size;
            nestegg_packet_data(pkt, chunk, &data, &data_size);

            vpx_image_t *img;
            vpx_codec_iter_t iter = NULL;
            if (vpx_codec_decode(&ctx->vpx_ctx, data, data_size, NULL, 0))
            {
                printf("Error: libvpx failed to decode chunk\n");
                webm_request_decoder_stop(ctx->quit);
                break;
            }
            while((img = vpx_codec_get_frame(&ctx->vpx_ctx, &iter)))
            {
                void *replaced_frame = NULL;

                if(img->d_w != (unsigned int)ctx->width ||
                   img->d_h != (unsigned int)ctx->height) {
                    printf("Error: WebM frame dimensions changed during playback\n");
                    webm_request_decoder_stop(ctx->quit);
                    break;
                }
                yuv_frame *frame = yuv_frame_create(img->d_w, img->d_h);
                if(!frame) {
                    printf("Error: Unable to allocate a decoded WebM frame\n");
                    webm_request_decoder_stop(ctx->quit);
                    break;
                }
                frame->timestamp = timestamp;

                int y;
                for(y = 0; y < img->d_h; y++)
                    memcpy(frame->lum+(y*img->d_w), img->planes[0]+(y*img->stride[0]), img->d_w);
                for(y = 0; y < img->d_h / 2; y++)
                {
                    memcpy(frame->cr+(y*img->d_w/2), img->planes[1]+(y*img->stride[1]), img->d_w / 2);
                    memcpy(frame->cb+(y*img->d_w/2), img->planes[2]+(y*img->stride[2]), img->d_w / 2);
                }

                if(video_frame_queue_insert(
                    ctx,
                    (void *)frame,
                    &replaced_frame
                ) < 0)
                {
                    debug_printf("destroying last frame\n");
                    yuv_frame_destroy(frame);
                    break;
                }
                yuv_frame_destroy((yuv_frame *)replaced_frame);
                timestamp = timestamp <= UINT64_MAX - frame_delay
                    ? timestamp + frame_delay
                    : UINT64_MAX;
            }
            if(webm_stop_is_requested(ctx->quit)) break;
        }
        nestegg_free_packet(pkt);
    }

    {
        void *replaced_frame = NULL;

        if(video_frame_queue_insert(
            ctx,
            NULL,
            &replaced_frame
        ) == 0) {
            yuv_frame_destroy((yuv_frame *)replaced_frame);
        }
    }
    return 0;
}

/*
* Caskey, Damon V.
* 2026-08-15
*
* Select a bounded VP8 worker count from source resolution and available
* logical processors. Multi-core systems retain one processor for the main
* engine and audio when possible, while one- and two-core devices remain
* able to use every processor required for practical decoding.
*/
static unsigned int video_decoder_thread_count(
    unsigned int width,
    unsigned int height
)
{
    uint64_t pixel_count = (uint64_t)width * (uint64_t)height;
    unsigned int resolution_limit;
    unsigned int threads;
    int cpu_count = SDL_GetCPUCount();

    if(pixel_count <= UINT64_C(640) * UINT64_C(480)) {
        resolution_limit = 1;
    } else if(pixel_count <= UINT64_C(1280) * UINT64_C(720)) {
        resolution_limit = 2;
    } else if(pixel_count <= UINT64_C(1920) * UINT64_C(1080)) {
        resolution_limit = 4;
    } else {
        resolution_limit = VIDEO_DECODER_THREAD_MAX;
    }

    threads = cpu_count > 0 ? (unsigned int)cpu_count : 1U;
    if(threads > 2U) {
        --threads;
    }
    if(threads > resolution_limit) {
        threads = resolution_limit;
    }
    if(threads > VIDEO_DECODER_THREAD_MAX) {
        threads = VIDEO_DECODER_THREAD_MAX;
    }
    return threads ? threads : 1U;
}

// returns 0 on success, -1 on error
/*
* Caskey, Damon V.
* 2026-08-15
*
* Initialize a resolution-aware multithreaded video decoder and queues
* against their owning stop state. DefaultDuration is optional - packet
* timestamps and packet duration retain playback timing when omitted.
*/
static int init_video(
    nestegg *nestegg_ctx,
    int track,
    video_context *video_ctx,
    SDL_atomic_t *quit
)
{
    nestegg_video_params video_params;
    vpx_codec_dec_cfg_t decoder_config = { 0 };
    int has_default_duration;

    if(nestegg_track_video_params(nestegg_ctx, track, &video_params) < 0) {
        printf("Error: Unable to read WebM video track parameters\n");
        return -1;
    }
    if(video_params.stereo_mode != NESTEGG_VIDEO_MONO ||
       video_params.width < 2 ||
       video_params.height < 2 ||
       (video_params.width & 1U) ||
       (video_params.height & 1U) ||
       video_params.width > (unsigned int)(INT_MAX >> 16) ||
       video_params.height > (unsigned int)(INT_MAX >> 16) ||
       video_params.width > (unsigned int)INT_MAX / video_params.height ||
       video_params.display_width > (unsigned int)INT_MAX ||
       video_params.display_height > (unsigned int)INT_MAX) {
        printf("Error: Unsupported WebM video dimensions or stereo mode\n");
        return -1;
    }

    decoder_config.threads = video_decoder_thread_count(
        video_params.width,
        video_params.height
    );
    decoder_config.w = video_params.width;
    decoder_config.h = video_params.height;
    if(vpx_codec_dec_init(
        &(video_ctx->vpx_ctx),
        vpx_codec_vp8_dx(),
        &decoder_config,
        0
    ))
    {
        printf("Error: failed to initialize libvpx\n");
        return -1;
    }
    video_ctx->width = video_params.width;
    video_ctx->height = video_params.height;
    video_ctx->display_width = video_params.display_width;
    video_ctx->display_height = video_params.display_height;
    video_ctx->playback_paused = 1;
    has_default_duration = nestegg_track_default_duration(
        nestegg_ctx,
        track,
        &(video_ctx->frame_delay)
    ) == 0 && video_ctx->frame_delay;
    if(has_default_duration) {
        printf("Video track: resolution=%i*%i, display resolution=%i*%i, %.2f frames/second\n",
            video_params.width, video_params.height,
            video_params.display_width, video_params.display_height,
            1000000000.0 / video_ctx->frame_delay);
    } else {
        video_ctx->frame_delay = 0;
        printf("Video track: resolution=%i*%i, display resolution=%i*%i, packet-timestamp timing\n",
            video_params.width, video_params.height,
            video_params.display_width, video_params.display_height);
    }
    printf(
        "Video decoder: %u thread%s.\n",
        decoder_config.threads,
        decoder_config.threads == 1U ? "" : "s"
    );
    video_ctx->quit = quit;
    video_ctx->packet_queue = queue_init(VIDEO_PACKET_QUEUE_SIZE, quit);
    video_ctx->frame_queue = queue_init(FRAME_QUEUE_SIZE, quit);
    if(!video_ctx->packet_queue || !video_ctx->frame_queue) {
        printf("Error: Unable to allocate WebM video queues\n");
        if(video_ctx->packet_queue) queue_destroy(video_ctx->packet_queue);
        if(video_ctx->frame_queue) queue_destroy(video_ctx->frame_queue);
        video_ctx->packet_queue = NULL;
        video_ctx->frame_queue = NULL;
        vpx_codec_destroy(&(video_ctx->vpx_ctx));
        return -1;
    }
    return 0;
}

static void close_video(video_context *video_ctx)
{
    if(vpx_codec_destroy(&(video_ctx->vpx_ctx)))
    {
        printf("Warning: failed to destroy libvpx context: %s\n", vpx_codec_error(&video_ctx->vpx_ctx));
    }
    if(video_ctx->packet_queue)
    {
        while(video_ctx->packet_queue && video_ctx->packet_queue->size)
        {
            nestegg_packet *packet = queue_get(video_ctx->packet_queue);
            if(packet) nestegg_free_packet(packet);
        }
        queue_destroy(video_ctx->packet_queue);
    }
    if(video_ctx->frame_queue)
    {
        while(video_ctx->frame_queue && video_ctx->frame_queue->size)
        {
            yuv_frame_destroy((yuv_frame *) queue_get(video_ctx->frame_queue));
        }
        queue_destroy(video_ctx->frame_queue);
    }
}

/*
* Caskey, Damon V.
* 2026-08-17
*
* Report the first completed compressed-video recovery and then every
* sixteenth occurrence. The discard total includes stale queued packets and
* newly demuxed packets skipped before the next verified keyframe.
*/
static void video_packet_queue_report_resync(FixedSizeQueue *queue)
{
    uint64_t dropped_packets;
    uint64_t resync_count;

    queue_get_status(queue, &dropped_packets, &resync_count);
    if(resync_count &&
       (resync_count == 1U || !(resync_count % UINT64_C(16)))) {
        printf(
            "Warning: WebM video backlog recovery discarded %" PRIu64
            " compressed packets in %" PRIu64
            " keyframe resync event%s.\n",
            dropped_packets,
            resync_count,
            resync_count == 1U ? "" : "s"
        );
    }
}

/*
* Caskey, Damon V.
* 2026-08-17
*
* Route container packets to their decoder queues. WebM playback with audio
* elevates this shared producer and protects a critical audio reserve from
* compressed-video head-of-line blocking. Recovery atomically discards the
* stale compressed queue and resumes admission at a verified keyframe so the
* decoder does not spend the recovery interval processing obsolete work.
*/
static int demux_thread(void *data)
{
    webm_context *ctx = (webm_context *)data;
    nestegg_packet *pkt;
    int discard_video_until_keyframe = 0;
    int r;

    if(ctx->audio_track >= 0) {
        /* Priority elevation may be denied on restricted platforms. */
        SDL_SetThreadPriority(SDL_THREAD_PRIORITY_HIGH);
    }

    while ((r = nestegg_read_packet(ctx->nestegg_ctx, &pkt)) > 0)
    {
        unsigned int track;
        nestegg_packet_track(pkt, &track);

        if (track == ctx->audio_track)
        {
            if (queue_insert(ctx->audio_ctx.packet_queue, pkt) < 0)
            {
                nestegg_free_packet(pkt);
                break;
            }
        }
        else if (track == ctx->video_track)
        {
            int insert_result;

            if(discard_video_until_keyframe) {
                if(video_packet_is_keyframe(pkt)) {
                    insert_result = video_packet_queue_try_insert(
                        ctx->video_ctx.packet_queue,
                        pkt
                    );
                    if(insert_result > 0) {
                        discard_video_until_keyframe = 0;
                        video_packet_queue_report_resync(
                            ctx->video_ctx.packet_queue
                        );
                        if(webm_stop_is_requested(&ctx->quit)) break;
                        continue;
                    }
                    if(insert_result < 0) {
                        nestegg_free_packet(pkt);
                        break;
                    }
                }
                video_packet_queue_record_drop(
                    ctx->video_ctx.packet_queue
                );
                nestegg_free_packet(pkt);
                if(webm_stop_is_requested(&ctx->quit)) break;
                continue;
            }
            insert_result = video_packet_queue_insert(ctx, pkt);
            if(insert_result < 0) {
                nestegg_free_packet(pkt);
                break;
            }
            if(insert_result == 0) {
                int packet_is_keyframe = video_packet_is_keyframe(pkt);

                if(video_packet_queue_begin_resync(
                    ctx->video_ctx.packet_queue,
                    packet_is_keyframe ? pkt : NULL
                ) < 0) {
                    nestegg_free_packet(pkt);
                    break;
                }
                if(packet_is_keyframe) {
                    video_packet_queue_report_resync(
                        ctx->video_ctx.packet_queue
                    );
                    if(webm_stop_is_requested(&ctx->quit)) break;
                    continue;
                }
                video_packet_queue_record_drop(
                    ctx->video_ctx.packet_queue
                );
                nestegg_free_packet(pkt);
                discard_video_until_keyframe = 1;
                if(webm_stop_is_requested(&ctx->quit)) break;
                continue;
            }
        }
        else
        {
            nestegg_free_packet(pkt);
        }

        if(webm_stop_is_requested(&ctx->quit)) break;
    }
    if(discard_video_until_keyframe) {
        video_packet_queue_report_resync(ctx->video_ctx.packet_queue);
    }
    if (ctx->audio_track >= 0) queue_insert(ctx->audio_ctx.packet_queue, NULL);
    queue_insert(ctx->video_ctx.packet_queue, NULL);
    return 0;
}

/*
* Caskey, Damon V.
* 2026-08-17
*
* Publish an asynchronous lifecycle stage unless teardown was requested.
* Cancellation also raises the decoder stop flag so in-progress preroll and
* decoder workers leave their bounded waits without engine-thread joins.
*/
static int webm_publish_decoder_state(
    webm_context *ctx,
    e_webm_decoder_state state
)
{
    int accepted;

    mutex_lock(ctx->lifecycle_mutex);
    accepted = !ctx->close_requested;
    if(accepted) {
        ctx->decoder_state = state;
    }
    else {
        ctx->decoder_state = WEBM_DECODER_STATE_CLOSING;
        webm_request_decoder_stop(&ctx->quit);
    }
    mutex_unlock(ctx->lifecycle_mutex);
    return accepted;
}

/*
* Caskey, Damon V.
* 2026-08-17
*
* Join decoder workers and release every resource opened by the lifecycle
* thread. Initialization flags make the same path safe for partial opens,
* failed seeks, ordinary stops, and engine shutdown.
*/
static void webm_close_resources(webm_context *ctx)
{
    webm_request_decoder_stop(&ctx->quit);
    if(ctx->the_demux_thread) {
        thread_join(ctx->the_demux_thread);
        ctx->the_demux_thread = NULL;
    }
    if(ctx->the_video_thread) {
        thread_join(ctx->the_video_thread);
        ctx->the_video_thread = NULL;
    }
    if(ctx->the_audio_thread) {
        thread_join(ctx->the_audio_thread);
        ctx->the_audio_thread = NULL;
    }
    if(ctx->video_initialized) {
        close_video(&ctx->video_ctx);
        ctx->video_initialized = 0;
    }
    if(ctx->audio_initialized) {
        close_audio(&ctx->audio_ctx);
        ctx->audio_initialized = 0;
    }
    if(ctx->nestegg_ctx) {
        nestegg_destroy(ctx->nestegg_ctx);
        ctx->nestegg_ctx = NULL;
    }
    if(ctx->io_ctx.packhandle >= 0) {
        closepackfile(ctx->io_ctx.packhandle);
        ctx->io_ctx.packhandle = -1;
    }
}

/*
* Caskey, Damon V.
* 2026-08-17
*
* Perform container open, initial seek, and decoder creation on the lifecycle
* worker, then publish PREROLLING when audio must establish its PCM reserve.
* Reverse playback and ordinary seeks use the same decoder path as initial
* playback.
*/
static int webm_open_resources(webm_context *ctx)
{
    nestegg_io io;
    int video_track = -1;
    int audio_track = -1;
    unsigned int num_tracks;
    unsigned int i;

    io.read = webm_io_read;
    io.seek = webm_io_seek;
    io.tell = webm_io_tell;
    io.userdata = &ctx->io_ctx;

    if(ctx->io_ctx.cache_buffer) {
        if(!ctx->io_ctx.cache_size) {
            printf("Error: Cached WebM source %s has no data\n", ctx->path);
            return -1;
        }
    }
    else {
        ctx->io_ctx.packhandle = openpackfile(ctx->path, packfile);
        if(ctx->io_ctx.packhandle < 0) {
            printf("Error: Unable to open file %s for playback\n", ctx->path);
            return -1;
        }
        ctx->io_ctx.stream_size = seekpackfile64(
            ctx->io_ctx.packhandle,
            0,
            SEEK_END
        );
        if(ctx->io_ctx.stream_size <= 0 ||
           seekpackfile64(ctx->io_ctx.packhandle, 0, SEEK_SET) != 0) {
            printf("Error: Unable to size WebM stream %s\n", ctx->path);
            return -1;
        }
    }

    if(!webm_publish_decoder_state(
        ctx,
        WEBM_DECODER_STATE_OPENING
    )) {
        return -1;
    }
    if(nestegg_init(&ctx->nestegg_ctx, io, NULL, -1) < 0) {
        printf("Error: Unable to initialize WebM container %s\n", ctx->path);
        return -1;
    }
    nestegg_duration(ctx->nestegg_ctx, &ctx->duration);
    if(nestegg_track_count(ctx->nestegg_ctx, &num_tracks) < 0) {
        printf("Error: Unable to read WebM tracks from %s\n", ctx->path);
        return -1;
    }

    for(i = 0; i < num_tracks; ++i) {
        int track_type = nestegg_track_type(ctx->nestegg_ctx, i);
        int codec = nestegg_track_codec_id(ctx->nestegg_ctx, i);

        if(track_type == NESTEGG_TRACK_VIDEO && video_track < 0) {
            if(codec != NESTEGG_CODEC_VP8) {
                printf("Error: unsupported video codec; only VP8 is supported\n");
                return -1;
            }
            video_track = (int)i;
        }
        else if(track_type == NESTEGG_TRACK_AUDIO && audio_track < 0) {
            if(codec != NESTEGG_CODEC_VORBIS) {
                printf("Error: unsupported audio codec; only Vorbis is supported\n");
                return -1;
            }
            audio_track = (int)i;
        }
    }
    if(video_track < 0) {
        printf("Error: WebM file %s does not contain a video track\n", ctx->path);
        return -1;
    }

    if(!webm_publish_decoder_state(
        ctx,
        WEBM_DECODER_STATE_SEEKING
    )) {
        return -1;
    }
    if(ctx->duration && ctx->seek_timestamp >= ctx->duration) {
        ctx->seek_timestamp = ctx->duration - 1U;
    }
    if(ctx->seek_timestamp &&
       nestegg_track_seek(
           ctx->nestegg_ctx,
           (unsigned int)video_track,
           ctx->seek_timestamp
       ) < 0) {
        printf("Error: Unable to seek WebM file %s\n", ctx->path);
        return -1;
    }
    if(!webm_publish_decoder_state(
        ctx,
        WEBM_DECODER_STATE_OPENING
    )) {
        return -1;
    }

    ctx->video_track = video_track;
    if(init_video(
        ctx->nestegg_ctx,
        ctx->video_track,
        &ctx->video_ctx,
        &ctx->quit
    ) < 0) {
        return -1;
    }
    ctx->video_initialized = 1;
    ctx->the_video_thread = thread_create(
        video_thread,
        "video",
        &ctx->video_ctx
    );
    if(!ctx->the_video_thread) {
        printf("Error: Unable to start WebM video decoder thread\n");
        return -1;
    }

    if(audio_track >= 0 && ctx->play_audio) {
        ctx->audio_track = audio_track;
        if(init_audio(
            ctx->nestegg_ctx,
            ctx->audio_track,
            &ctx->audio_ctx,
            ctx->volume,
            ctx->sound_channel,
            ctx->seek_timestamp,
            ctx->replace_all_audio,
            &ctx->quit
        ) == 0) {
            ctx->audio_initialized = 1;
            ctx->audio_ctx.video_packet_queue =
                ctx->video_ctx.packet_queue;
            ctx->audio_ctx.video_frame_queue =
                ctx->video_ctx.frame_queue;
            ctx->the_audio_thread = thread_create(
                audio_thread,
                "audio",
                &ctx->audio_ctx
            );
            if(!ctx->the_audio_thread) {
                printf("Error: Unable to start WebM audio decoder thread\n");
                return -1;
            }
        }
        else {
            printf(
                "Warning: Unable to open the WebM audio track on channel %d\n",
                ctx->sound_channel
            );
            ctx->audio_track = -1;
        }
    }
    else if(audio_track < 0 &&
            ctx->replace_all_audio &&
            sound_query_music(NULL, NULL)) {
        /* Blocking legacy playback retains its channel-zero service. */
        ctx->the_audio_thread = thread_create(bgm_update_thread, "bgm", ctx);
        if(!ctx->the_audio_thread) {
            printf("Error: Unable to start WebM legacy audio service thread\n");
            return -1;
        }
    }

    ctx->the_demux_thread = thread_create(demux_thread, "demux", ctx);
    if(!ctx->the_demux_thread) {
        printf("Error: Unable to start WebM demux thread\n");
        return -1;
    }
    if(ctx->audio_track >= 0) {
        if(!webm_publish_decoder_state(
            ctx,
            WEBM_DECODER_STATE_PREROLLING
        )) {
            return -1;
        }
    }
    return webm_stop_is_requested(&ctx->quit) ? -1 : 0;
}

/*
* Caskey, Damon V.
* 2026-08-17
*
* Own the decoder lifecycle after the constructor returns. Successful opens
* wait in READY until an asynchronous close request arrives; failed opens
* remain observable as FAILED until the movie layer retires the context.
*/
static int webm_lifecycle_thread(void *data)
{
    webm_context *ctx = data;
    int open_result;

    mutex_lock(webm_lifecycle_operation_mutex);
    open_result = webm_open_resources(ctx);
    mutex_unlock(webm_lifecycle_operation_mutex);
    if(open_result == 0 && ctx->audio_track >= 0) {
        audio_wait_for_preroll(ctx);
        if(webm_stop_is_requested(&ctx->quit) ||
           webm_close_is_requested(ctx)) {
            open_result = -1;
        }
    }
    if(open_result < 0) {
        mutex_lock(webm_lifecycle_operation_mutex);
        webm_close_resources(ctx);
        mutex_unlock(webm_lifecycle_operation_mutex);
    }

    mutex_lock(ctx->lifecycle_mutex);
    if(ctx->close_requested) {
        ctx->decoder_state = WEBM_DECODER_STATE_CLOSING;
    }
    else {
        ctx->decoder_state = open_result == 0
            ? WEBM_DECODER_STATE_READY
            : WEBM_DECODER_STATE_FAILED;
    }
    cond_broadcast(ctx->lifecycle_condition);
    while(!ctx->close_requested) {
        cond_wait(ctx->lifecycle_condition, ctx->lifecycle_mutex);
    }
    ctx->decoder_state = WEBM_DECODER_STATE_CLOSING;
    webm_request_decoder_stop(&ctx->quit);
    mutex_unlock(ctx->lifecycle_mutex);

    if(open_result == 0) {
        mutex_lock(webm_lifecycle_operation_mutex);
        webm_close_resources(ctx);
        mutex_unlock(webm_lifecycle_operation_mutex);
    }

    mutex_lock(ctx->lifecycle_mutex);
    ctx->decoder_state = WEBM_DECODER_STATE_CLOSED;
    cond_broadcast(ctx->lifecycle_condition);
    mutex_unlock(ctx->lifecycle_mutex);
    return 0;
}

/*
* Caskey, Damon V.
* 2026-08-17
*
* Allocate an independently owned decoder request and return as soon as its
* lifecycle worker starts. Container I/O, seeking, codec initialization, and
* audio preroll continue asynchronously behind the published state machine.
*/
webm_context *webm_start_playback_ex(
    const char *path,
    int volume,
    int sound_channel,
    const unsigned char *cache_buffer,
    size_t cache_size,
    uint64_t seek_timestamp,
    int play_audio,
    int replace_all_audio
)
{
    webm_context *ctx;

    if(!path || !path[0] ||
       sound_channel < 0 ||
       (unsigned int)sound_channel >= SOUND_CHANNEL_COUNT_MAX ||
       (cache_buffer && !cache_size)) {
        printf("Error: Invalid WebM playback request\n");
        return NULL;
    }
    if(!webm_lifecycle_init()) {
        printf("Error: Unable to initialize WebM lifecycle synchronization\n");
        return NULL;
    }

    ctx = calloc(1, sizeof(*ctx));
    if(!ctx) {
        printf("Error: Unable to allocate WebM playback context\n");
        return NULL;
    }
    ctx->path = webm_copy_path(path);
    ctx->lifecycle_mutex = mutex_create();
    ctx->lifecycle_condition = cond_create();
    if(!ctx->path || !ctx->lifecycle_mutex || !ctx->lifecycle_condition) {
        if(ctx->lifecycle_condition) {
            cond_destroy(ctx->lifecycle_condition);
        }
        if(ctx->lifecycle_mutex) {
            mutex_destroy(ctx->lifecycle_mutex);
        }
        free(ctx->path);
        free(ctx);
        return NULL;
    }

    ctx->io_ctx.packhandle = -1;
    ctx->io_ctx.cache_buffer = cache_buffer;
    ctx->io_ctx.cache_size = cache_size;
    ctx->audio_track = -1;
    ctx->video_track = -1;
    ctx->seek_timestamp = seek_timestamp;
    ctx->volume = volume;
    ctx->sound_channel = sound_channel;
    ctx->play_audio = play_audio != 0;
    ctx->replace_all_audio = replace_all_audio != 0;
    ctx->decoder_state = WEBM_DECODER_STATE_OPENING;
    SDL_AtomicSet(&ctx->quit, 0);
    ctx->the_lifecycle_thread = thread_create(
        webm_lifecycle_thread,
        "webm-lifecycle",
        ctx
    );
    if(!ctx->the_lifecycle_thread) {
        cond_destroy(ctx->lifecycle_condition);
        mutex_destroy(ctx->lifecycle_mutex);
        free(ctx->path);
        free(ctx);
        return NULL;
    }
    return ctx;
}

/*
* Caskey, Damon V.
* 2026-08-17
*
* Snapshot one decoder lifecycle state under its owner mutex so the movie
* layer can advance transitions without reading partially published state.
*/
e_webm_decoder_state webm_get_decoder_state(webm_context *ctx)
{
    e_webm_decoder_state state;

    if(!ctx) {
        return WEBM_DECODER_STATE_FAILED;
    }
    mutex_lock(ctx->lifecycle_mutex);
    state = ctx->decoder_state;
    mutex_unlock(ctx->lifecycle_mutex);
    return state;
}

/*
* Caskey, Damon V.
* 2026-08-17
*
* Request decoder cancellation without waiting for lifecycle, demux, audio,
* or video workers. The owning movie channel later reaps the context after
* CLOSED is observable.
*/
void webm_request_close(webm_context *ctx)
{
    if(!ctx) {
        return;
    }

    mutex_lock(ctx->lifecycle_mutex);
    if(ctx->decoder_state != WEBM_DECODER_STATE_CLOSED) {
        ctx->close_requested = 1;
        ctx->decoder_state = WEBM_DECODER_STATE_CLOSING;
        cond_broadcast(ctx->lifecycle_condition);
    }
    mutex_unlock(ctx->lifecycle_mutex);
}

/*
* Caskey, Damon V.
* 2026-08-17
*
* Reap one decoder only after asynchronous teardown is complete. CLOSED is
* published at the lifecycle worker's exit boundary, keeping this poll free
* of container I/O and decoder-thread waits during normal engine updates.
*/
int webm_poll_closed(webm_context *ctx)
{
    if(!ctx ||
       webm_get_decoder_state(ctx) != WEBM_DECODER_STATE_CLOSED) {
        return 0;
    }

    thread_join(ctx->the_lifecycle_thread);
    cond_destroy(ctx->lifecycle_condition);
    mutex_destroy(ctx->lifecycle_mutex);
    free(ctx->path);
    free(ctx);
    return 1;
}

/*
* Caskey, Damon V.
* 2026-08-17
*
* Provide a blocking finalizer only for engine shutdown and legacy ownership
* boundaries. Runtime movie stop and replacement use request-plus-poll so
* teardown never joins decoder workers on an engine update.
*/
void webm_close(webm_context *ctx)
{
    if(!ctx) {
        return;
    }

    webm_request_close(ctx);
    thread_join(ctx->the_lifecycle_thread);
    cond_destroy(ctx->lifecycle_condition);
    mutex_destroy(ctx->lifecycle_mutex);
    free(ctx->path);
    free(ctx);
}

void webm_get_video_info(webm_context *ctx, yuv_video_mode *dims)
{
    if(!ctx || !dims ||
       webm_get_decoder_state(ctx) != WEBM_DECODER_STATE_READY) {
        return;
    }
    dims->width = ctx->video_ctx.width;
    dims->height = ctx->video_ctx.height;
    dims->display_width = ctx->video_ctx.display_width;
    dims->display_height = ctx->video_ctx.display_height;
}

/*
* Caskey, Damon V.
* 2026-08-12
*
* Return the container duration in nanoseconds.
*/
uint64_t webm_get_duration(webm_context *ctx)
{
    return ctx &&
        webm_get_decoder_state(ctx) == WEBM_DECODER_STATE_READY
        ? ctx->duration
        : 0;
}

/*
* Caskey, Damon V.
* 2026-08-17
*
* Resolve the movie timeline from PCM frames consumed by this WebM object's
* owned sound channel. The clock stops with real audio during pauses and
* underruns, and becomes unavailable if another producer takes the channel.
*/
int webm_get_audio_playback_position(
    webm_context *ctx,
    uint64_t *position
)
{
    audio_context *audio_ctx;
    uint64_t playback_frame;
    uint64_t elapsed;

    if(!ctx || !position ||
       webm_get_decoder_state(ctx) != WEBM_DECODER_STATE_READY ||
       ctx->audio_track < 0) {
        return 0;
    }

    audio_ctx = &ctx->audio_ctx;
    if(!sound_get_channel_pcm_stream_playback_frame_owned(
        audio_ctx->channel,
        audio_ctx->stream_play_id,
        &playback_frame
    )) {
        return 0;
    }

    elapsed = audio_frames_to_nanoseconds(
        playback_frame,
        audio_ctx->frequency
    );
    *position = elapsed <= UINT64_MAX - audio_ctx->playback_start_timestamp
        ? audio_ctx->playback_start_timestamp + elapsed
        : UINT64_MAX;
    if(ctx->duration && *position > ctx->duration) {
        *position = ctx->duration;
    }
    return 1;
}

/*
* Caskey, Damon V.
* 2026-08-17
*
* Apply pause state to decoder backpressure and audio output only after the
* asynchronous lifecycle publishes READY. Pending requests retain their
* desired pause state in the owning movie record until that transition.
*/
void webm_set_audio_paused(webm_context *ctx, int paused)
{
    audio_context *audio_ctx;

    if(!ctx ||
       webm_get_decoder_state(ctx) != WEBM_DECODER_STATE_READY) {
        return;
    }
    paused = paused != 0;
    if(ctx->video_ctx.frame_queue) {
        mutex_lock(ctx->video_ctx.frame_queue->mutex);
        ctx->video_ctx.playback_paused = paused;
        mutex_unlock(ctx->video_ctx.frame_queue->mutex);
    }
    if(ctx->audio_track < 0) {
        return;
    }

    audio_ctx = &ctx->audio_ctx;
    if(audio_ctx->output_paused == paused) {
        return;
    }

    if(sound_pause_channel_owned(
        audio_ctx->channel,
        audio_ctx->stream_play_id,
        paused
    )) {
        audio_ctx->output_paused = paused;
    }
}

void webm_set_audio_speed(webm_context *ctx, double speed)
{
    if(ctx &&
       webm_get_decoder_state(ctx) == WEBM_DECODER_STATE_READY &&
       ctx->audio_track >= 0) {
        sound_set_channel_speed_owned(
            ctx->audio_ctx.channel,
            ctx->audio_ctx.stream_play_id,
            speed
        );
    }
}

/*
* Caskey, Damon V.
* 2026-08-15
*
* Snapshot decoded-frame occupancy for one bounded main-thread poll. Frames
* published after this count is captured remain queued for the next engine
* update instead of extending the current update indefinitely.
*/
int webm_get_pending_frame_count(webm_context *ctx)
{
    int count;

    if(!ctx ||
       webm_get_decoder_state(ctx) != WEBM_DECODER_STATE_READY) {
        return 0;
    }
    count = queue_get_status(
        ctx->video_ctx.frame_queue,
        NULL,
        NULL
    );
    return count > 0 ? count : 0;
}

/*
* Caskey, Damon V.
* 2026-08-12
*
* Poll decoded video without blocking the main engine update.
*/
int webm_try_get_next_frame(webm_context *ctx, yuv_frame **frame)
{
    e_webm_decoder_state state;
    void *queued_frame;

    if(!ctx || !frame) {
        return -1;
    }
    state = webm_get_decoder_state(ctx);
    if(state != WEBM_DECODER_STATE_READY) {
        return state == WEBM_DECODER_STATE_FAILED ||
            state == WEBM_DECODER_STATE_CLOSING ||
            state == WEBM_DECODER_STATE_CLOSED
            ? -1
            : 0;
    }
    if(!queue_try_get(ctx->video_ctx.frame_queue, &queued_frame)) {
        return webm_stop_is_requested(&ctx->quit) ? -1 : 0;
    }

    *frame = queued_frame;
    return queued_frame ? 1 : -1;
}
