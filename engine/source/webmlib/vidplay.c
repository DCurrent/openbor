/*
 * OpenBOR - http://www.chronocrash.com
 * -----------------------------------------------------------------------
 * All rights reserved, see LICENSE in OpenBOR root for details.
 *
 * Copyright (c) 2004 - 2014 OpenBOR Team
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdint.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>
#include <time.h>
#include "nestegg/nestegg.h"

// libvpx
#define VPX_CODEC_DISABLE_COMPAT 1
#include "vpx/vpx_decoder.h"
#include "vpx/vp8dx.h"

// our headers
#include "vorbis.h"
#include "yuv.h"
#include "threads.h"
#include "types.h"
#include "globals.h"
#include "timer.h"
#include "soundmix.h"
#include "video.h"

#include "openbor.h"

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
    uint64_t timestamp;
    void *lum;
    void *cb;
    void *cr;
} yuv_frame;

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
    uint64_t frame_delay;
} video_context;

typedef struct {
    nestegg *demux_ctx;
    audio_context *audio_ctx;
    video_context *video_ctx;
    FixedSizeQueue *audio_queue;
    int audio_stream;
    int video_stream;
} decoder_context;


static int quit_video;

int webm_read(void *buffer, size_t length, void *userdata)
{
    int webmfile = (int)userdata;
    int bytesRead = readpackfile(webmfile, buffer, length);
    if (bytesRead < 0) return -1;
    else if (bytesRead == 0) return 0;
    else return 1;
}

int webm_seek(int64_t offset, int whence, void *userdata)
{
    return seekpackfile((int)userdata, (int)offset, whence);
}

int64_t webm_tell(void *userdata)
{
    return seekpackfile((int)userdata, 0, SEEK_CUR);
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

#define SPIT(fmt, ...) debug_printf("%s:%i: " fmt, __func__, __LINE__, __VA_ARGS__)

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

yuv_frame *yuv_frame_create(int width, int height)
{
    yuv_frame *frame = malloc(sizeof(yuv_frame));
    frame->lum = malloc(width * height);
    frame->cr = malloc(width * height / 2);
    frame->cb = malloc(width * height / 2);
    return frame;
}

void yuv_frame_destroy(yuv_frame *frame)
{
    if(frame == NULL) return;
    free(frame->lum);
    free(frame->cr);
    free(frame->cb);
    free(frame);
}

#if LINUX // usleep has been deprecated in POSIX for a while
// sleeps for the given number of microseconds
void _usleep(uint64_t time)
{
    struct timespec sleeptime;
    sleeptime.tv_sec = time / 1000000LL;
    sleeptime.tv_nsec = (time % 1000000LL) * 1000;
    nanosleep(&sleeptime, NULL);
}
#define usleep _usleep
#endif

int audio_decode_frame(audio_context *audio_ctx, uint8_t *audio_buf, int buf_size)
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

            //fprintf(stderr, "audio queue size=%i\n", audio_ctx->packet_queue->size);
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

int audio_thread(void *data)
{
    decoder_context *ctx = (decoder_context *)data;
    audio_context *audio_ctx = ctx->audio_ctx;
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

void init_audio(nestegg *ctx, int track, audio_context *audio_ctx)
{
    nestegg_audio_params audioParams;
    nestegg_track_audio_params(ctx, track, &audioParams);

    // initialize audio decoding context
    audio_ctx->vorbis_ctx.channels = audioParams.channels;
    audio_ctx->frequency = (int)audioParams.rate;
    audio_ctx->avail_samples = audio_ctx->last_samples = 0;

    // start playback
    printf("Audio track: %f Hz, %d channels, %d bits/sample\n", audioParams.rate, audioParams.channels, audioParams.depth);
    sound_start_playback(16, audio_ctx->frequency);

    // initialize soundmix music channel
    memset(&musicchannel, 0, sizeof(musicchannel));
    musicchannel.fp_period = INT_TO_FIX(audio_ctx->frequency) / playfrequency;
    musicchannel.volume[0] = 256;
    musicchannel.volume[1] = 256;
    musicchannel.channels = audioParams.channels;
    musicchannel.active = 1;

    int i;
    for(i = 0; i < MUSIC_NUM_BUFFERS; i++)
    {
        musicchannel.buf[i] = malloc(MUSIC_BUF_SIZE * sizeof(short));
        memset(musicchannel.buf[i], 0, MUSIC_BUF_SIZE * sizeof(short));
    }
}

int video_thread(void *data)
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
                shutdown(1, "Failed to decode frame\n");
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

int demux_thread(void *data)
{
    decoder_context *ctx = (decoder_context *)data;
    nestegg_packet *pkt;
    int r;
    while ((r = nestegg_read_packet(ctx->demux_ctx, &pkt)) > 0)
    {
        unsigned int track;
        nestegg_packet_track(pkt, &track);

        if (track == ctx->audio_stream)
            queue_insert(ctx->audio_ctx->packet_queue, pkt);
        else if (track == ctx->video_stream)
            queue_insert(ctx->video_ctx->packet_queue, pkt);

        if (quit_video) break;
    }
    queue_insert(ctx->audio_ctx->packet_queue, NULL);
    queue_insert(ctx->video_ctx->packet_queue, NULL);
    return 0;
}

int play_webm(const char *path)
{
    int webmfile;
    nestegg_io io;
    nestegg *demux_ctx;
    video_context video_ctx;
    audio_context audio_ctx;
    int video_stream = -1, audio_stream = -1;

    quit_video = 0;

    // set up I/O callbacks
    io.read = webm_read;
    io.seek = webm_seek;
    io.tell = webm_tell;

    // open video file
    webmfile = openpackfile(path, "dummy.pak");
    assert(webmfile >= 0);
    io.userdata = (void*)webmfile;
    nestegg_init(&demux_ctx, io, NULL, -1);

    // get number of tracks
    unsigned num_tracks, i;
    if (nestegg_track_count(demux_ctx, &num_tracks) != 0) exit(2);

    // find the first video and audio tracks
    // TODO: handle files with no audio track
    for (i = 0; i < num_tracks; i++)
    {
        int track_type = nestegg_track_type(demux_ctx, i);
        int codec = nestegg_track_codec_id(demux_ctx, i);
        if (track_type == NESTEGG_TRACK_VIDEO)
        {
            assert(codec == NESTEGG_CODEC_VP8);
            video_stream = i;
        }
        else if (track_type == NESTEGG_TRACK_AUDIO)
        {
            assert(codec == NESTEGG_CODEC_VORBIS);
            audio_stream = i;
        }
    }

    // VP8 params
    nestegg_video_params video_params;
    nestegg_track_video_params(demux_ctx, video_stream, &video_params);
    printf("Video track: resolution=%i*%i, display resolution=%i*%i\n",
            video_params.width, video_params.height,
            video_params.display_width, video_params.display_height);
    assert(video_params.stereo_mode == NESTEGG_VIDEO_MONO);

    // initialize queues
    audio_ctx.packet_queue = queue_init(PACKET_QUEUE_SIZE);
    video_ctx.packet_queue = queue_init(PACKET_QUEUE_SIZE);
    video_ctx.frame_queue = queue_init(FRAME_QUEUE_SIZE);

    // set up demux
    decoder_context decoder_ctx;
    decoder_ctx.demux_ctx = demux_ctx;
    decoder_ctx.audio_ctx = &audio_ctx;
    decoder_ctx.video_ctx = &video_ctx;
    decoder_ctx.audio_stream = audio_stream;
    decoder_ctx.video_stream = video_stream;
    bor_thread *the_demux_thread = thread_create(demux_thread, "demux", &decoder_ctx);

    // set up audio
    unsigned chunk, chunks;
    vorbis_init(&audio_ctx.vorbis_ctx);
    nestegg_track_codec_data_count(demux_ctx, audio_stream, &chunks);
    assert(chunks == 3);
    for (chunk=0; chunk<chunks; chunk++)
    {
        unsigned char *data;
        size_t data_size;
        nestegg_track_codec_data(demux_ctx, audio_stream, chunk, &data, &data_size);
        vorbis_headerpacket(&audio_ctx.vorbis_ctx, data, data_size, chunk);
    }
    vorbis_prepare(&audio_ctx.vorbis_ctx);
    init_audio(demux_ctx, audio_stream, &audio_ctx);
    bor_thread *the_audio_thread = thread_create(audio_thread, "audio", &decoder_ctx);

    // set up video
    if (vpx_codec_dec_init(&video_ctx.vpx_ctx, vpx_codec_vp8_dx(), NULL, 0))
        shutdown(1, "Failed to initialize libvpx");
    video_ctx.width = video_params.width;
    video_ctx.height = video_params.height;
    yuv_init(pixelbytes[screenformat]);
    nestegg_track_default_duration(demux_ctx, video_stream, &(video_ctx.frame_delay));
    bor_thread *the_video_thread = thread_create(video_thread, "video", &video_ctx);

    uint64_t next_frame_time = 0;
    uint64_t perfFreq = 1000; //SDL_GetPerformanceFrequency();
    uint64_t myclock;
    uint64_t starttime = timer_gettick(); //SDL_GetPerformanceCounter();

    s_screen *surface = allocscreen(video_ctx.width, video_ctx.height, screenformat);

    while(!quit_video)
    {
        inputrefresh();

        myclock = timer_gettick(); //SDL_GetPerformanceCounter();
        uint64_t system_clock = (myclock - starttime) * (1000000000.0 / perfFreq);

        if (next_frame_time <= system_clock)
        {
            // display the new frame
            video_copy_screen(surface);

            // prepare the next frame for display
            debug_printf("size=%i\n", video_ctx.frame_queue->size);
            debug_printf("fc %lli, ac %lli, ", next_frame_time, audio_clock);
            debug_printf("uc %lli, ", system_clock);
            yuv_frame *frame = (yuv_frame *)queue_get(video_ctx.frame_queue);
            if (frame == NULL) break;
            // note: to swap red and blue components of output, just swap the cb and cr buffers
            yuv_to_rgb(frame->lum, frame->cr, frame->cb, surface->data, surface->height, surface->width, 0);
            next_frame_time = frame->timestamp;
            yuv_frame_destroy(frame);
        }
        else
        {
            uint64_t sleeptime_ns = next_frame_time - system_clock;
            usleep(sleeptime_ns / 1000);
        }
    }

    thread_join(the_demux_thread);
    thread_join(the_video_thread);
    thread_join(the_audio_thread);
    yuv_clear();
    freescreen(&surface);

    // clean up anything left in the queues
    nestegg_packet *packet;
    int size;
    while(audio_ctx.packet_queue->size)
    {
        packet = (nestegg_packet *) queue_get(audio_ctx.packet_queue);
        if(packet) nestegg_free_packet(packet);
    }
    while(video_ctx.packet_queue->size)
    {
        packet = (nestegg_packet *) queue_get(video_ctx.packet_queue);
        if(packet) nestegg_free_packet(packet);
    }
    for(i=0, size=video_ctx.frame_queue->size; i<size; i++)
        yuv_frame_destroy((yuv_frame *) queue_get(video_ctx.frame_queue));

    // free the queues
    queue_destroy(audio_ctx.packet_queue);
    queue_destroy(video_ctx.packet_queue);
    queue_destroy(video_ctx.frame_queue);

    // clean up audio context
    sound_stop_playback();
    vorbis_destroy(&audio_ctx.vorbis_ctx);

    // free up any memory used by libvpx and nestegg
    if(vpx_codec_destroy(&video_ctx.vpx_ctx))
        printf("Warning: failed to destroy libvpx context: %s\n", vpx_codec_error(&video_ctx.vpx_ctx));
    nestegg_destroy(demux_ctx);

    return 0;
}

