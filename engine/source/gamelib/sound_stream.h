/*
 * OpenBOR - http://www.chronocrash.com
 * -----------------------------------------------------------------------
 * All rights reserved, see LICENSE in OpenBOR root for details.
 *
 * Copyright (c) OpenBOR Team
 */

#ifndef SOUND_STREAM_H
#define SOUND_STREAM_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/*
* Caskey, Damon V.
* 2026-08-01
*
* Streamed channels use SOUND_STREAM_BUFFER_COUNT
 * retained PCM buffers. At the largest supported input
 * format, each SOUND_STREAM_BUFFER_SIZE buffer supplies
 * approximately 57 milliseconds of data. Total reserve
 * is approximately 57 milliseconds multiplied by
 * SOUND_STREAM_BUFFER_COUNT.
 */
#define SOUND_STREAM_BUFFER_COUNT 4U
#define SOUND_STREAM_BUFFER_SIZE  (16U * 1024U)
#define SOUND_STREAM_HANDLE_CLOSED (-1)

typedef struct s_sound_stream_buffer {
    unsigned char *data;
    uint64_t source_start_frame;
    uint64_t frame_count;
    int terminal;
    int ready;
} s_sound_stream_buffer;

typedef struct s_sound_stream {
    s_sound_stream_buffer buffer[SOUND_STREAM_BUFFER_COUNT];
    unsigned int read_buffer;
    unsigned int write_buffer;
    uint64_t next_source_frame;
    uint64_t source_frame_count;
    uint64_t loop_start_frame;
    uint64_t fp_buffer_position;
    size_t block_align;
    int handle;
    int looping;
    int producer_finished;
} s_sound_stream;

typedef bool (*sound_stream_read_callback)(
    void *context,
    uint64_t source_start_frame,
    void *destination,
    size_t bytes_to_read
);

void sound_stream_init(s_sound_stream *stream);
void sound_stream_reset(s_sound_stream *stream);
void sound_stream_destroy(s_sound_stream *stream);
bool sound_stream_configure(
    s_sound_stream *stream,
    size_t block_align,
    uint64_t source_frame_count,
    uint64_t start_frame,
    int looping,
    uint64_t loop_start_frame
);
bool sound_stream_configure_push(
    s_sound_stream *stream,
    size_t block_align
);
int sound_stream_fill(
    s_sound_stream *stream,
    sound_stream_read_callback read_callback,
    void *read_context,
    size_t *bytes_filled
);
int sound_stream_push_locked(
    s_sound_stream *stream,
    const void *source,
    uint64_t frame_count,
    int terminal
);

#endif
