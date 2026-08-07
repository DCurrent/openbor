/*
 * OpenBOR - http://www.chronocrash.com
 * -----------------------------------------------------------------------
 * All rights reserved, see LICENSE in OpenBOR root for details.
 *
 * Copyright (c) OpenBOR Team
 */

#ifndef SOUND_CHANNEL_H
#define SOUND_CHANNEL_H

#include <stdbool.h>
#include <stdint.h>

#include "sound_stream.h"
#include "types.h"

/*
* Caskey, Damon V.
* 2026-07-31
*
* Sound effect channels use 64 banks of 64.
* Bank zero is allocated during sound startup.
* Remaining banks are allocated only as needed
* and retained until sound shutdown.
*/
#define SOUND_CHANNEL_BANK_COUNT        64U
#define SOUND_CHANNEL_BANK_SIZE         64U
#define SOUND_CHANNEL_COUNT_MAX         (SOUND_CHANNEL_BANK_COUNT * SOUND_CHANNEL_BANK_SIZE)
#define SOUND_CHANNEL_BANK_FULL_MASK    UINT64_MAX

typedef uint64_t sound_sample_fixed_t;
typedef uint64_t sound_sample_position_t;

typedef enum e_sound_channel_stream_source {
    SOUND_CHANNEL_STREAM_SOURCE_NONE,
    SOUND_CHANNEL_STREAM_SOURCE_WAVE,
    SOUND_CHANNEL_STREAM_SOURCE_VORBIS,
    SOUND_CHANNEL_STREAM_SOURCE_ADPCM,
    SOUND_CHANNEL_STREAM_SOURCE_PUSH
} e_sound_channel_stream_source;

/*
* Caskey, Damon V.
* 2026-08-02
*
* Pool masks identify banks with a shared state.
* Channel masks identify slots within one bank.
*/
typedef enum e_sound_channel_bank_mask {
    SOUND_CHANNEL_BANK_MASK_ALLOCATED,
    SOUND_CHANNEL_BANK_MASK_ACTIVE,
    SOUND_CHANNEL_BANK_MASK_AVAILABLE,
    SOUND_CHANNEL_BANK_MASK_STREAMING,
    SOUND_CHANNEL_BANK_MASK_END
} e_sound_channel_bank_mask;

typedef enum e_sound_channel_mask {
    SOUND_CHANNEL_MASK_ACTIVE,
    SOUND_CHANNEL_MASK_PAUSED,
    SOUND_CHANNEL_MASK_RESERVED,
    SOUND_CHANNEL_MASK_STREAMING,
    SOUND_CHANNEL_MASK_END
} e_sound_channel_mask;

#define SOUND_SAMPLE_FIXED_SHIFT        16U
#define SOUND_SAMPLE_FIXED_ONE          ((sound_sample_fixed_t)1U << SOUND_SAMPLE_FIXED_SHIFT)
#define SOUND_SAMPLE_FIXED_MAX_INTEGER  (UINT64_MAX >> SOUND_SAMPLE_FIXED_SHIFT)
#define SOUND_SAMPLE_INT_TO_FIX(integer_value) ((sound_sample_fixed_t)(integer_value) << SOUND_SAMPLE_FIXED_SHIFT)
#define SOUND_SAMPLE_FIX_TO_INT(fixed_value) ((sound_sample_position_t)((fixed_value) >> SOUND_SAMPLE_FIXED_SHIFT))

typedef struct s_sound_channel {
    e_object_type object_type;
    int index;        /* Flattened public channel index. */
    int active;       /* 1 = play, 2 = loop. */
    int paused;
    int samplenum;    /* Index of sound playing. */
    unsigned int priority;
    unsigned int chance;       /* Playback chance from 0 through 100. */
    uint32_t chance_roll;       /* Main-thread roll evaluated when delay expires. */
    int playid;
    int volume[2];
    int volume_divisor;
    int bits;
    int frequency;
    int channels;
    e_sound_channel_stream_source stream_source;

    uint64_t delay_frames;              /* Output frames remaining before playback. */
    sound_sample_fixed_t fp_samplepos;  /* Fixed point PCM frame position. */
    sound_sample_fixed_t fp_period;     /* Advance per output frame. */
    sound_sample_fixed_t fp_loop_start; /* PCM frame used after looping. */
    s_sound_stream stream;
    void *stream_decoder;
} channelstruct;

typedef struct s_sound_channel_bank {
    channelstruct channel[SOUND_CHANNEL_BANK_SIZE];
    uint64_t active_mask;
    uint64_t paused_mask;
    uint64_t reserved_mask;
    uint64_t streaming_mask;
} s_sound_channel_bank;

typedef struct s_sound_channel_pool {
    s_sound_channel_bank *bank[SOUND_CHANNEL_BANK_COUNT];
    uint64_t allocated_bank_mask;
    uint64_t active_bank_mask;
    uint64_t available_bank_mask;
    uint64_t streaming_bank_mask;
} s_sound_channel_pool;

bool sound_channel_pool_init(s_sound_channel_pool *pool);
void sound_channel_pool_destroy(s_sound_channel_pool *pool);
bool sound_channel_pool_allocate_bank(s_sound_channel_pool *pool, unsigned int bank_index);
int sound_channel_pool_acquire(s_sound_channel_pool *pool);
channelstruct *sound_channel_pool_get(s_sound_channel_pool *pool, int channel);
int sound_channel_pool_get_index(const s_sound_channel_pool *pool, const channelstruct *record);
uint64_t sound_channel_pool_get_bank_mask(const s_sound_channel_pool *pool, e_sound_channel_bank_mask mask);
uint64_t sound_channel_pool_get_mask(const s_sound_channel_pool *pool, unsigned int bank_index, e_sound_channel_mask mask);
bool sound_channel_pool_is_active(const s_sound_channel_pool *pool, int channel);
void sound_channel_pool_activate(s_sound_channel_pool *pool, int channel, int active_state);
void sound_channel_pool_deactivate(s_sound_channel_pool *pool, int channel);
void sound_channel_pool_pause(s_sound_channel_pool *pool, int channel, int toggle);
void sound_channel_pool_stream(s_sound_channel_pool *pool, int channel, int toggle);
bool sound_channel_pool_reserve_mask(s_sound_channel_pool *pool, unsigned int bank_index, uint64_t reserved_mask);
void sound_channel_pool_stop_all(s_sound_channel_pool *pool, bool force);
void sound_channel_pool_pause_all(s_sound_channel_pool *pool, int toggle);
int sound_channel_mask_first(uint64_t mask);

#endif
