/*
 * OpenBOR - http://www.chronocrash.com
 * -----------------------------------------------------------------------
 * All rights reserved, see LICENSE in OpenBOR root for details.
 *
 * Copyright (c) OpenBOR Team
 */

#include <stdlib.h>
#include <string.h>

#include "sound_stream.h"
#include "sblaster.h"

/*
* Caskey, Damon V.
* 2026-08-01
*
* Initialize an empty streamed sound queue.
*/
void sound_stream_init(s_sound_stream *stream) {
    if(!stream) {
        return;
    }

    memset(stream, 0, sizeof(*stream));
    stream->handle = SOUND_STREAM_HANDLE_CLOSED;
}

/*
* Caskey, Damon V.
* 2026-08-01
*
* Reset playback state while retaining allocated
* PCM buffers for reuse by the channel.
*/
void sound_stream_reset(s_sound_stream *stream) {
    unsigned char *buffer_data[SOUND_STREAM_BUFFER_COUNT];
    unsigned int buffer_index;

    if(!stream) {
        return;
    }

    for(buffer_index = 0; buffer_index < SOUND_STREAM_BUFFER_COUNT; buffer_index++) {
        buffer_data[buffer_index] = stream->buffer[buffer_index].data;
    }

    sound_stream_init(stream);

    for(buffer_index = 0; buffer_index < SOUND_STREAM_BUFFER_COUNT; buffer_index++) {
        stream->buffer[buffer_index].data = buffer_data[buffer_index];
    }
}

/*
* Caskey, Damon V.
* 2026-08-01
*
* Release retained PCM buffers during sound
* channel bank shutdown.
*/
void sound_stream_destroy(s_sound_stream *stream) {
    unsigned int buffer_index;

    if(!stream) {
        return;
    }

    for(buffer_index = 0; buffer_index < SOUND_STREAM_BUFFER_COUNT; buffer_index++) {
        free(stream->buffer[buffer_index].data);
    }

    sound_stream_init(stream);
}

/*
* Caskey, Damon V.
* 2026-08-01
*
* Configure the source frame range for a new
* streamed playback. Looping begins only after
* the initial pass reaches the source end.
*/
bool sound_stream_configure(
    s_sound_stream *stream,
    size_t block_align,
    uint64_t source_frame_count,
    uint64_t start_frame,
    int looping,
    uint64_t loop_start_frame
) {
    if(!stream ||
       block_align == 0 ||
       block_align > SOUND_STREAM_BUFFER_SIZE ||
       source_frame_count == 0 ||
       start_frame >= source_frame_count ||
       (looping && loop_start_frame >= source_frame_count)) {
        return false;
    }

    sound_stream_reset(stream);
    stream->block_align = block_align;
    stream->source_frame_count = source_frame_count;
    stream->next_source_frame = start_frame;
    stream->looping = looping ? 1 : 0;
    stream->loop_start_frame = loop_start_frame;
    return true;
}

/*
* Caskey, Damon V.
* 2026-08-01
*
* Fill the next empty PCM buffer from the source.
* Buffers never cross the source end, allowing the
* consumer to report exact source positions and to
* distinguish the first pass from automatic loops.
*
* Returns 1 when a buffer is filled, 0 when no fill
* is needed, and -1 on allocation or read failure.
*/
int sound_stream_fill(
    s_sound_stream *stream,
    sound_stream_read_callback read_callback,
    void *read_context,
    size_t *bytes_filled
) {
    s_sound_stream_buffer *buffer;
    uint64_t available_frames;
    uint64_t capacity_frames;
    uint64_t frames_to_read;
    size_t requested_bytes;

    if(bytes_filled) {
        *bytes_filled = 0;
    }

    if(!stream || !read_callback || stream->block_align == 0) {
        return -1;
    }

    buffer = &stream->buffer[stream->write_buffer];
    SB_lock_audio();
    if(buffer->ready) {
        SB_unlock_audio();
        return 0;
    }
    SB_unlock_audio();

    if(stream->next_source_frame >= stream->source_frame_count) {
        if(!stream->looping) {
            return 0;
        }
        stream->next_source_frame = stream->loop_start_frame;
    }

    if(!buffer->data) {
        buffer->data = malloc(SOUND_STREAM_BUFFER_SIZE);
        if(!buffer->data) {
            return -1;
        }
    }

    capacity_frames = SOUND_STREAM_BUFFER_SIZE / stream->block_align;
    available_frames = stream->source_frame_count - stream->next_source_frame;
    frames_to_read = available_frames < capacity_frames ? available_frames : capacity_frames;
    requested_bytes = (size_t)frames_to_read * stream->block_align;

    if(frames_to_read == 0 ||
       !read_callback(read_context, stream->next_source_frame, buffer->data, requested_bytes)) {
        return -1;
    }

    /* Publish PCM ownership only after all buffer metadata is complete. */
    SB_lock_audio();
    buffer->source_start_frame = stream->next_source_frame;
    buffer->frame_count = frames_to_read;
    buffer->terminal = !stream->looping && frames_to_read == available_frames;
    buffer->ready = 1;
    stream->next_source_frame += frames_to_read;
    stream->write_buffer = (stream->write_buffer + 1U) % SOUND_STREAM_BUFFER_COUNT;
    SB_unlock_audio();

    if(bytes_filled) {
        *bytes_filled = requested_bytes;
    }
    return 1;
}
