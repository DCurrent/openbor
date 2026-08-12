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

// lowering these might save a bit of memory but could also cause lag
#define PACKET_QUEUE_SIZE 20
#define FRAME_QUEUE_SIZE 10

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
    volatile int *quit;
    bor_mutex *mutex;
    bor_cond *not_full;
    bor_cond *not_empty;
    void *data[ANYNUMBER];
} FixedSizeQueue;

typedef struct {
    FixedSizeQueue *packet_queue;
    vorbis_context vorbis_ctx;
    int frequency;
    int avail_samples;
    int last_samples;
    int channel;
    int stream_play_id;
    int output_active;
    volatile int *quit;
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
    uint64_t frame_delay;
    volatile int *quit;
} video_context;

typedef struct {
    int packhandle;
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
    uint64_t duration;
    volatile int quit;
};

/*
* Caskey, Damon V.
* 2026-08-12
*
* Read or seek either a creator-requested memory cache or
* the ordinary packfile stream through the same Nestegg I/O.
*/
static int webm_io_read(void *buffer, size_t length, void *userdata)
{
    webm_io_context *io_ctx = userdata;
    int bytesRead;

    if(io_ctx->cache_buffer) {
        size_t remaining = io_ctx->cache_size - io_ctx->cache_position;

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

    if(length > (size_t)INT_MAX) {
        return -1;
    }
    bytesRead = readpackfile(io_ctx->packhandle, buffer, (int)length);
    if (bytesRead < 0) return -1;
    else if ((size_t)bytesRead != length) return 0;
    else return 1;
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

    if(offset < INT_MIN || offset > INT_MAX) {
        return -1;
    }
    return seekpackfile(io_ctx->packhandle, (int)offset, whence);
}

static int64_t webm_io_tell(void *userdata)
{
    webm_io_context *io_ctx = userdata;

    if(io_ctx->cache_buffer) {
        return (int64_t)io_ctx->cache_position;
    }
    return seekpackfile(io_ctx->packhandle, 0, SEEK_CUR);
}

static FixedSizeQueue *queue_init(int max_size, volatile int *quit)
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
            if (*queue->quit)
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
            if (*queue->quit)
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

    while (!ctx->quit)
    {
        sound_update_music();
        usleep(5000);
    }
    return 0;
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

static int audio_thread(void *data)
{
    audio_context *audio_ctx = (audio_context *)data;
    int decoded_bytes;
    int frame_count;
    int queue_result;
    int terminal;

    while(!*audio_ctx->quit)
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
                    terminal
                );
                if(queue_result == 0) {
                    usleep(1000);
                }
            } while(!*audio_ctx->quit && queue_result == 0);

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
    int replace_all_audio,
    volatile int *quit
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
    audio_ctx->quit = quit;
    audio_ctx->packet_queue = queue_init(PACKET_QUEUE_SIZE, quit);
    if(!audio_ctx->packet_queue) {
        vorbis_destroy(&(audio_ctx->vorbis_ctx));
        return -1;
    }
    printf("Audio track: %f Hz, %d channels, %d bits/sample\n",
            audioParams.rate, audioParams.channels, audioParams.depth / audioParams.channels);
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

static int video_thread(void *data)
{
    video_context *ctx = (video_context*) data;
    uint64_t timestamp;

    while(!*ctx->quit)
    {
        unsigned int chunk, chunks;
        nestegg_packet *pkt;

        debug_printf("video queue size=%i\n", ctx->packet_queue->size);
        pkt = queue_get(ctx->packet_queue);
        if (*ctx->quit || pkt == NULL) break;
        nestegg_packet_count(pkt, &chunks);
        nestegg_packet_tstamp(pkt, &timestamp);

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
                *ctx->quit = 1;
                break;
            }
            while((img = vpx_codec_get_frame(&ctx->vpx_ctx, &iter)))
            {
                if(img->d_w != (unsigned int)ctx->width ||
                   img->d_h != (unsigned int)ctx->height) {
                    printf("Error: WebM frame dimensions changed during playback\n");
                    *ctx->quit = 1;
                    break;
                }
                yuv_frame *frame = yuv_frame_create(img->d_w, img->d_h);
                if(!frame) {
                    printf("Error: Unable to allocate a decoded WebM frame\n");
                    *ctx->quit = 1;
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

                if (queue_insert(ctx->frame_queue, (void *)frame) < 0)
                {
                    debug_printf("destroying last frame\n");
                    yuv_frame_destroy(frame);
                    break;
                }
                timestamp += ctx->frame_delay;
            }
            if(*ctx->quit) break;
        }
        nestegg_free_packet(pkt);
    }

    queue_insert(ctx->frame_queue, NULL);
    return 0;
}

// returns 0 on success, -1 on error
/*
* Caskey, Damon V.
* 2026-08-12
*
* Initialize video queues against their owning decoder's
* stop state so multiple contexts can run independently.
*/
static int init_video(
    nestegg *nestegg_ctx,
    int track,
    video_context *video_ctx,
    volatile int *quit
)
{
    nestegg_video_params video_params;
    if(nestegg_track_video_params(nestegg_ctx, track, &video_params) < 0 ||
       video_params.stereo_mode != NESTEGG_VIDEO_MONO ||
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

    if (vpx_codec_dec_init(&(video_ctx->vpx_ctx), vpx_codec_vp8_dx(), NULL, 0))
    {
        printf("Error: failed to initialize libvpx\n");
        return -1;
    }
    video_ctx->width = video_params.width;
    video_ctx->height = video_params.height;
    video_ctx->display_width = video_params.display_width;
    video_ctx->display_height = video_params.display_height;
    if(nestegg_track_default_duration(
        nestegg_ctx,
        track,
        &(video_ctx->frame_delay)
    ) < 0 || !video_ctx->frame_delay) {
        vpx_codec_destroy(&(video_ctx->vpx_ctx));
        return -1;
    }
    printf("Video track: resolution=%i*%i, display resolution=%i*%i, %.2f frames/second\n",
            video_params.width, video_params.height,
            video_params.display_width, video_params.display_height,
            1000000000.0 / video_ctx->frame_delay);
    video_ctx->quit = quit;
    video_ctx->packet_queue = queue_init(PACKET_QUEUE_SIZE, quit);
    video_ctx->frame_queue = queue_init(FRAME_QUEUE_SIZE, quit);
    if(!video_ctx->packet_queue || !video_ctx->frame_queue) {
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

static int demux_thread(void *data)
{
    webm_context *ctx = (webm_context *)data;
    nestegg_packet *pkt;
    int r;
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
            if (queue_insert(ctx->video_ctx.packet_queue, pkt) < 0)
            {
                nestegg_free_packet(pkt);
                break;
            }
        }
        else
        {
            nestegg_free_packet(pkt);
        }

        if (ctx->quit) break;
    }
    queue_insert(ctx->video_ctx.packet_queue, NULL);
    if (ctx->audio_track >= 0) queue_insert(ctx->audio_ctx.packet_queue, NULL);
    return 0;
}

/*
* Caskey, Damon V.
* 2026-08-12
*
* Open one independently owned decoder. Optional shared cache,
* initial seek, and sound routing are established
* before its worker threads become visible.
*/
webm_context *webm_start_playback_ex(
    const char *path,
    int volume,
    int sound_channel,
    const unsigned char *cache_buffer,
    size_t cache_size,
    uint64_t seek_timestamp,
    int replace_all_audio
)
{
    webm_context *ctx;
    nestegg_io io;
    int video_track = -1, audio_track = -1;
    unsigned int num_tracks, i;

    if(!path || !path[0] ||
       sound_channel < 0 ||
       (unsigned int)sound_channel >= SOUND_CHANNEL_COUNT_MAX) {
        return NULL;
    }

    ctx = calloc(1, sizeof(*ctx));
    if(!ctx) return NULL;
    ctx->io_ctx.packhandle = -1;
    ctx->audio_track = -1;
    ctx->video_track = -1;

    io.read = webm_io_read;
    io.seek = webm_io_seek;
    io.tell = webm_io_tell;
    io.userdata = &ctx->io_ctx;

    if(cache_buffer) {
        if(!cache_size) {
            goto error_io;
        }
        ctx->io_ctx.cache_buffer = cache_buffer;
        ctx->io_ctx.cache_size = cache_size;
    } else {
        ctx->io_ctx.packhandle = openpackfile(path, packfile);
        if(ctx->io_ctx.packhandle < 0) {
            printf("Error: Unable to open file %s for playback\n", path);
            goto error_io;
        }
    }

    if(nestegg_init(&(ctx->nestegg_ctx), io, NULL, -1) < 0) goto error_io;
    nestegg_duration(ctx->nestegg_ctx, &ctx->duration);
    if(nestegg_track_count(ctx->nestegg_ctx, &num_tracks) < 0) goto error_nestegg;

    for(i = 0; i < num_tracks; i++) {
        int track_type = nestegg_track_type(ctx->nestegg_ctx, i);
        int codec = nestegg_track_codec_id(ctx->nestegg_ctx, i);

        if(track_type == NESTEGG_TRACK_VIDEO && video_track < 0) {
            if(codec != NESTEGG_CODEC_VP8) {
                printf("Error: unsupported video codec; only VP8 is supported\n");
                goto error_nestegg;
            }
            video_track = (int)i;
        } else if(track_type == NESTEGG_TRACK_AUDIO && audio_track < 0) {
            if(codec != NESTEGG_CODEC_VORBIS) {
                printf("Error: unsupported audio codec; only Vorbis is supported\n");
                goto error_nestegg;
            }
            audio_track = (int)i;
        }
    }

    if(video_track < 0) {
        printf("Error: WebM file %s does not contain a video track\n", path);
        goto error_nestegg;
    }

    if(ctx->duration && seek_timestamp >= ctx->duration) {
        seek_timestamp = ctx->duration - 1;
    }
    if(seek_timestamp &&
       nestegg_track_seek(ctx->nestegg_ctx, (unsigned int)video_track, seek_timestamp) < 0) {
        printf("Error: Unable to seek WebM file %s\n", path);
        goto error_nestegg;
    }

    ctx->video_track = video_track;
    if(init_video(
        ctx->nestegg_ctx,
        ctx->video_track,
        &(ctx->video_ctx),
        &ctx->quit
    ) < 0) {
        goto error_nestegg;
    }
    ctx->the_video_thread = thread_create(video_thread, "video", &(ctx->video_ctx));
    if(!ctx->the_video_thread) {
        webm_close(ctx);
        return NULL;
    }

    ctx->audio_track = audio_track;
    if(audio_track >= 0) {
        if(init_audio(
            ctx->nestegg_ctx,
            ctx->audio_track,
            &(ctx->audio_ctx),
            volume,
            sound_channel,
            replace_all_audio,
            &ctx->quit
        ) == 0) {
            ctx->the_audio_thread = thread_create(audio_thread, "audio", &(ctx->audio_ctx));
            if(!ctx->the_audio_thread) {
                webm_close(ctx);
                return NULL;
            }
        } else {
            printf("Warning: Unable to open the WebM audio track on channel %d\n", sound_channel);
            ctx->audio_track = -1;
        }
    } else if(replace_all_audio && sound_query_music(NULL, NULL)) {
        /* Blocking legacy playback must service existing channel zero audio. */
        ctx->the_audio_thread = thread_create(bgm_update_thread, "bgm", ctx);
        if(!ctx->the_audio_thread) {
            webm_close(ctx);
            return NULL;
        }
    }

    ctx->the_demux_thread = thread_create(demux_thread, "demux", ctx);
    if(!ctx->the_demux_thread) {
        webm_close(ctx);
        return NULL;
    }
    return ctx;

error_nestegg:
    nestegg_destroy(ctx->nestegg_ctx);
error_io:
    if(ctx->io_ctx.packhandle >= 0) {
        closepackfile(ctx->io_ctx.packhandle);
    }
    free(ctx);
    return NULL;
}

/*
* Caskey, Damon V.
* 2026-08-12
*
* Preserve the original fullscreen decoder entrypoint with
* channel zero and replace-all audio defaults.
*/
webm_context *webm_start_playback(const char *path, int volume)
{
    return webm_start_playback_ex(
        path,
        volume,
        SOUND_CHANNEL_MUSIC_DEFAULT,
        NULL,
        0,
        0,
        1
    );
}

/*
* Caskey, Damon V.
* 2026-08-12
*
* Stop one decoder without signaling any other WebM
* object, then release its queues, codec, and I/O source.
*/
void webm_close(webm_context *ctx)
{
    if(!ctx) {
        return;
    }

    ctx->quit = 1;
    if(ctx->the_demux_thread) thread_join(ctx->the_demux_thread);
    if(ctx->the_video_thread) thread_join(ctx->the_video_thread);
    close_video(&(ctx->video_ctx));
    if(ctx->the_audio_thread) thread_join(ctx->the_audio_thread);
    if(ctx->audio_track >= 0) close_audio(&(ctx->audio_ctx));
    if(ctx->nestegg_ctx) nestegg_destroy(ctx->nestegg_ctx);
    if(ctx->io_ctx.packhandle >= 0) closepackfile(ctx->io_ctx.packhandle);
    free(ctx);
}

void webm_get_video_info(webm_context *ctx, yuv_video_mode *dims)
{
    assert(ctx);
    assert(dims);
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
    return ctx ? ctx->duration : 0;
}

/*
* Caskey, Damon V.
* 2026-08-12
*
* Apply pause and speed changes only while this decoder's
* play ID still owns its selected generic sound channel.
*/
void webm_set_audio_paused(webm_context *ctx, int paused)
{
    if(ctx && ctx->audio_track >= 0) {
        sound_pause_channel_owned(
            ctx->audio_ctx.channel,
            ctx->audio_ctx.stream_play_id,
            paused
        );
    }
}

void webm_set_audio_speed(webm_context *ctx, unsigned int speed)
{
    if(ctx && ctx->audio_track >= 0) {
        sound_set_channel_speed_owned(
            ctx->audio_ctx.channel,
            ctx->audio_ctx.stream_play_id,
            speed
        );
    }
}

yuv_frame *webm_get_next_frame(webm_context *ctx)
{
    debug_printf("frame queue size=%i\n", ctx->video_ctx.frame_queue->size);
    return (yuv_frame *)queue_get(ctx->video_ctx.frame_queue);
}

/*
* Caskey, Damon V.
* 2026-08-12
*
* Poll decoded video without blocking the main engine update.
*/
int webm_try_get_next_frame(webm_context *ctx, yuv_frame **frame)
{
    void *queued_frame;

    if(!ctx || !frame) {
        return -1;
    }
    if(!queue_try_get(ctx->video_ctx.frame_queue, &queued_frame)) {
        return ctx->quit ? -1 : 0;
    }

    *frame = queued_frame;
    return queued_frame ? 1 : -1;
}
