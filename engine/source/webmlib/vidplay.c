/*
 * OpenBOR - http://www.chronocrash.com
 * -----------------------------------------------------------------------
 * All rights reserved, see LICENSE in OpenBOR root for details.
 *
 * Copyright (c) 2004 - 2015 OpenBOR Team
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdint.h>
#include <string.h>
#include <assert.h>
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
#include "soundmix.h"

// lowering these might save a bit of memory but could also cause lag
#define PACKET_QUEUE_SIZE 20
#define FRAME_QUEUE_SIZE 10

#define debug_printf(...) //printf(__VA_ARGS__)

typedef struct {
    int start;
    int size;
    int max_size;
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
} video_context;

// see header for typedef
struct webm_context {
    int packhandle;
    nestegg *nestegg_ctx;
    audio_context audio_ctx;
    video_context video_ctx;
    FixedSizeQueue *audio_queue;
    int audio_track;
    int video_track;
    bor_thread *the_demux_thread;
    bor_thread *the_video_thread;
    bor_thread *the_audio_thread;
};


static int quit_video;

int webm_read(void *buffer, size_t length, void *userdata)
{
    int bytesRead = readpackfile((int)(size_t)userdata, buffer, length);
    if (bytesRead < 0) return -1;
    else if (bytesRead == 0) return 0;
    else return 1;
}

int webm_seek(int64_t offset, int whence, void *userdata)
{
    return seekpackfile((int)(size_t)userdata, (int)offset, whence);
}

int64_t webm_tell(void *userdata)
{
    return seekpackfile((int)(size_t)userdata, 0, SEEK_CUR);
}

FixedSizeQueue *queue_init(int max_size)
{
    FixedSizeQueue *queue;
    queue = malloc(sizeof(*queue) - sizeof(queue->data) + (max_size * sizeof(void *)));
    queue->start = 0;
    queue->size = 0;
    queue->max_size = max_size;
    queue->mutex = mutex_create();
    queue->not_full = cond_create();
    queue->not_empty = cond_create();
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
            if (quit_video)
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
            if (quit_video)
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
    while (!quit_video)
    {
        sound_update_music();
        usleep(5000);
    }
    return 0;
}

static int audio_decode_frame(audio_context *audio_ctx, uint8_t *audio_buf, int buf_size)
{
    vorbis_context *vorbis_ctx = &audio_ctx->vorbis_ctx;
    //audio_clock += 1000000000LL * audio_ctx->last_samples / audio_ctx->frequency;
    int samples = buf_size / (vorbis_ctx->channels * 2);
    audio_ctx->last_samples = samples;

    while (samples)
    {
        if (audio_ctx->avail_samples == 0)
        {
            nestegg_packet *pkt;
            uint64_t timestamp;
            unsigned chunk, num_chunks;

            debug_printf("audio queue size=%i\n", audio_ctx->packet_queue->size);
            if ((pkt = queue_get(audio_ctx->packet_queue)) == NULL)
                return -1;
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
    }
    return buf_size;
}

static int audio_thread(void *data)
{
    audio_context *audio_ctx = (audio_context *)data;
    int i, j;

    while(!quit_video)
    {
        if(musicchannel.paused)
        {
            continue;
        }

        // Just to be sure: check if all goes well...
        for(i = 0; i < MUSIC_NUM_BUFFERS; i++)
        {
            if(musicchannel.fp_playto[i] > INT_TO_FIX(MUSIC_BUF_SIZE))
            {
                musicchannel.fp_playto[i] = 0;
            }
        }

        // Need to update?
        for(j = 0, i = musicchannel.playing_buffer + 1; j < MUSIC_NUM_BUFFERS; j++, i++)
        {
            i %= MUSIC_NUM_BUFFERS;
            if(musicchannel.fp_playto[i] == 0)
            {
                // Buffer needs to be filled
                if (audio_decode_frame(audio_ctx, (uint8_t*)musicchannel.buf[i], MUSIC_BUF_SIZE * sizeof(short)) < 0)
                    return 0;
                musicchannel.fp_playto[i] = INT_TO_FIX(MUSIC_BUF_SIZE);
                if(!musicchannel.active)
                {
                    musicchannel.playing_buffer = i;
                    musicchannel.active = 1;
                }
            }
        }

        // Sleep for 1 ms so that this thread doesn't waste CPU cycles busywaiting
        usleep(1000);
    }

    return 0;
}

static void init_audio(nestegg *ctx, int track, audio_context *audio_ctx, int volume)
{
    // read vorbis header and initialize vorbis decoding
    unsigned chunk, chunks;
    vorbis_init(&(audio_ctx->vorbis_ctx));
    nestegg_track_codec_data_count(ctx, track, &chunks);
    assert(chunks == 3);
    for (chunk=0; chunk<chunks; chunk++)
    {
        unsigned char *data;
        size_t data_size;
        nestegg_track_codec_data(ctx, track, chunk, &data, &data_size);
        vorbis_headerpacket(&(audio_ctx->vorbis_ctx), data, data_size, chunk);
    }

    // initialize audio decoding context
    vorbis_prepare(&(audio_ctx->vorbis_ctx));
    nestegg_audio_params audioParams;
    nestegg_track_audio_params(ctx, track, &audioParams);
    audio_ctx->vorbis_ctx.channels = audioParams.channels;
    audio_ctx->frequency = (int)audioParams.rate;
    audio_ctx->avail_samples = audio_ctx->last_samples = 0;
    audio_ctx->packet_queue = queue_init(PACKET_QUEUE_SIZE);
    printf("Audio track: %f Hz, %d channels, %d bits/sample\n",
            audioParams.rate, audioParams.channels, audioParams.depth / audioParams.channels);
    if(audio_ctx->frequency % 11025)
    {
        printf("Warning: the audio frequency (%i Hz) is suboptimal; resample to 44100 Hz for best quality\n",
                audio_ctx->frequency);
    }

    // initialize soundmix music channel
    sound_close_music();
    memset(&musicchannel, 0, sizeof(musicchannel));
    musicchannel.fp_period = INT_TO_FIX(audio_ctx->frequency) / playfrequency;
    musicchannel.volume[0] = volume;
    musicchannel.volume[1] = volume;
    musicchannel.channels = audioParams.channels;
    musicchannel.active = 1;

    int i;
    for(i = 0; i < MUSIC_NUM_BUFFERS; i++)
    {
        musicchannel.buf[i] = malloc(MUSIC_BUF_SIZE * sizeof(short));
        memset(musicchannel.buf[i], 0, MUSIC_BUF_SIZE * sizeof(short));
    }
}

static void close_audio(audio_context *audio_ctx)
{
    // empty and free the packet queue
    while(audio_ctx->packet_queue->size)
    {
        nestegg_packet *packet = queue_get(audio_ctx->packet_queue);
        if(packet) nestegg_free_packet(packet);
    }
    queue_destroy(audio_ctx->packet_queue);

    // close the vorbis decoding context
    vorbis_destroy(&(audio_ctx->vorbis_ctx));

    // set the state of soundmix to how it was before
    int i;
    musicchannel.active = 0;
    for(i = 0; i < MUSIC_NUM_BUFFERS; i++)
    {
        free(musicchannel.buf[i]);
        musicchannel.buf[i] = NULL;
    }
}

static int video_thread(void *data)
{
    video_context *ctx = (video_context*) data;
    uint64_t timestamp;

    while(!quit_video)
    {
        unsigned int chunk, chunks;
        nestegg_packet *pkt;

        debug_printf("video queue size=%i\n", ctx->packet_queue->size);
        pkt = queue_get(ctx->packet_queue);
        if (quit_video || pkt == NULL) break;
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
                quit_video = 1;
                break;
            }
            while((img = vpx_codec_get_frame(&ctx->vpx_ctx, &iter)))
            {
                assert(img->d_w == ctx->width);
                assert(img->d_h == ctx->height);
                yuv_frame *frame = yuv_frame_create(img->d_w, img->d_h);
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
        }
        nestegg_free_packet(pkt);
    }

    queue_insert(ctx->frame_queue, NULL);
    return 0;
}

// returns 0 on success, -1 on error
static int init_video(nestegg *nestegg_ctx, int track, video_context *video_ctx)
{
    nestegg_video_params video_params;
    nestegg_track_video_params(nestegg_ctx, track, &video_params);
    assert(video_params.stereo_mode == NESTEGG_VIDEO_MONO);

    if (vpx_codec_dec_init(&(video_ctx->vpx_ctx), vpx_codec_vp8_dx(), NULL, 0))
    {
        printf("Error: failed to initialize libvpx\n");
        return -1;
    }
    video_ctx->width = video_params.width;
    video_ctx->height = video_params.height;
    video_ctx->display_width = video_params.display_width;
    video_ctx->display_height = video_params.display_height;
    nestegg_track_default_duration(nestegg_ctx, track, &(video_ctx->frame_delay));
    printf("Video track: resolution=%i*%i, display resolution=%i*%i, %.2f frames/second\n",
            video_params.width, video_params.height,
            video_params.display_width, video_params.display_height,
            1000000000.0 / video_ctx->frame_delay);
    video_ctx->packet_queue = queue_init(PACKET_QUEUE_SIZE);
    video_ctx->frame_queue = queue_init(FRAME_QUEUE_SIZE);
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

        if (quit_video) break;
    }
    queue_insert(ctx->video_ctx.packet_queue, NULL);
    if (ctx->audio_track >= 0) queue_insert(ctx->audio_ctx.packet_queue, NULL);
    return 0;
}

webm_context *webm_start_playback(const char *path, int volume)
{
    webm_context *ctx;
    nestegg_io io;
    int video_track = -1, audio_track = -1;

    quit_video = 0;
    ctx = malloc(sizeof(*ctx));
    if(!ctx) return NULL;
    memset(ctx, 0, sizeof(*ctx));

    // set up I/O callbacks
    io.read = webm_read;
    io.seek = webm_seek;
    io.tell = webm_tell;

    // open video file
    ctx->packhandle = openpackfile(path, packfile);
    if(ctx->packhandle < 0)
    {
        printf("Error: Unable to open file %s for playback\n", path);
        goto error1;
    }
    io.userdata = (void*)(size_t)ctx->packhandle;
    if(nestegg_init(&(ctx->nestegg_ctx), io, NULL, -1) < 0) goto error2;

    // get number of tracks
    unsigned int num_tracks, i;
    if(nestegg_track_count(ctx->nestegg_ctx, &num_tracks) < 0) goto error3;

    // find the first video and audio tracks
    for (i = 0; i < num_tracks; i++)
    {
        int track_type = nestegg_track_type(ctx->nestegg_ctx, i);
        int codec = nestegg_track_codec_id(ctx->nestegg_ctx, i);
        if (track_type == NESTEGG_TRACK_VIDEO)
        {
            if(codec != NESTEGG_CODEC_VP8)
            {
                printf("Error: unsupported video codec; only VP8 is supported\n");
                goto error3;
            }
            video_track = i;
        }
        else if (track_type == NESTEGG_TRACK_AUDIO)
        {
            if(codec != NESTEGG_CODEC_VORBIS)
            {
                printf("Error: unsupported audio codec; only Vorbis is supported\n");
                goto error3;
            }
            audio_track = i;
        }
    }

    // set up video
    ctx->video_track = video_track;
    init_video(ctx->nestegg_ctx, ctx->video_track, &(ctx->video_ctx));
    ctx->the_video_thread = thread_create(video_thread, "video", &(ctx->video_ctx));
    assert(ctx->the_video_thread);

    // set up audio, if applicable
    ctx->audio_track = audio_track;
    if (audio_track >= 0)
    {
        // use the audio track of this file
        init_audio(ctx->nestegg_ctx, ctx->audio_track, &(ctx->audio_ctx), volume);
        ctx->the_audio_thread = thread_create(audio_thread, "audio", &(ctx->audio_ctx));
        assert(ctx->the_audio_thread);
    }
    else if (sound_query_music(NULL, NULL))
    {
        // continue to play the BGM that's already playing
        ctx->the_audio_thread = thread_create(bgm_update_thread, "bgm", NULL);
        assert(ctx->the_audio_thread);
    }

    // finally, start the demuxing thread
    ctx->the_demux_thread = thread_create(demux_thread, "demux", ctx);
    assert(ctx->the_demux_thread);
    return ctx;

error3:
    nestegg_destroy(ctx->nestegg_ctx);
error2:
    closepackfile(ctx->packhandle);
error1:
    free(ctx);
    return NULL;
}

void webm_close(webm_context *ctx)
{
    quit_video = 1;
    thread_join(ctx->the_demux_thread);
    thread_join(ctx->the_video_thread);
    close_video(&(ctx->video_ctx));
    if (ctx->the_audio_thread) thread_join(ctx->the_audio_thread);
    if (ctx->audio_track >= 0) close_audio(&(ctx->audio_ctx));
    nestegg_destroy(ctx->nestegg_ctx);
    closepackfile(ctx->packhandle);
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

yuv_frame *webm_get_next_frame(webm_context *ctx)
{
    debug_printf("frame queue size=%i\n", ctx->video_ctx.frame_queue->size);
    return (yuv_frame *)queue_get(ctx->video_ctx.frame_queue);
}


