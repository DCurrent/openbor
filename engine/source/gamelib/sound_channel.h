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
typedef uint64_t sound_group_mask_t;

/*
* Caskey, Damon V.
* 2026-08-08
*
* Independent sound group masks. Bits 0 through 51 provide
* two author-facing banks named a-z and a1-z1. Remaining
* bits are reserved for future sound-specific behavior.
*/
#define SOUND_GROUP_NONE               UINT64_C(0)

#define SOUND_GROUP_A                  (UINT64_C(1) << 0)
#define SOUND_GROUP_B                  (UINT64_C(1) << 1)
#define SOUND_GROUP_C                  (UINT64_C(1) << 2)
#define SOUND_GROUP_D                  (UINT64_C(1) << 3)
#define SOUND_GROUP_E                  (UINT64_C(1) << 4)
#define SOUND_GROUP_F                  (UINT64_C(1) << 5)
#define SOUND_GROUP_G                  (UINT64_C(1) << 6)
#define SOUND_GROUP_H                  (UINT64_C(1) << 7)
#define SOUND_GROUP_I                  (UINT64_C(1) << 8)
#define SOUND_GROUP_J                  (UINT64_C(1) << 9)
#define SOUND_GROUP_K                  (UINT64_C(1) << 10)
#define SOUND_GROUP_L                  (UINT64_C(1) << 11)
#define SOUND_GROUP_M                  (UINT64_C(1) << 12)
#define SOUND_GROUP_N                  (UINT64_C(1) << 13)
#define SOUND_GROUP_O                  (UINT64_C(1) << 14)
#define SOUND_GROUP_P                  (UINT64_C(1) << 15)
#define SOUND_GROUP_Q                  (UINT64_C(1) << 16)
#define SOUND_GROUP_R                  (UINT64_C(1) << 17)
#define SOUND_GROUP_S                  (UINT64_C(1) << 18)
#define SOUND_GROUP_T                  (UINT64_C(1) << 19)
#define SOUND_GROUP_U                  (UINT64_C(1) << 20)
#define SOUND_GROUP_V                  (UINT64_C(1) << 21)
#define SOUND_GROUP_W                  (UINT64_C(1) << 22)
#define SOUND_GROUP_X                  (UINT64_C(1) << 23)
#define SOUND_GROUP_Y                  (UINT64_C(1) << 24)
#define SOUND_GROUP_Z                  (UINT64_C(1) << 25)

#define SOUND_GROUP_A1                 (UINT64_C(1) << 26)
#define SOUND_GROUP_B1                 (UINT64_C(1) << 27)
#define SOUND_GROUP_C1                 (UINT64_C(1) << 28)
#define SOUND_GROUP_D1                 (UINT64_C(1) << 29)
#define SOUND_GROUP_E1                 (UINT64_C(1) << 30)
#define SOUND_GROUP_F1                 (UINT64_C(1) << 31)
#define SOUND_GROUP_G1                 (UINT64_C(1) << 32)
#define SOUND_GROUP_H1                 (UINT64_C(1) << 33)
#define SOUND_GROUP_I1                 (UINT64_C(1) << 34)
#define SOUND_GROUP_J1                 (UINT64_C(1) << 35)
#define SOUND_GROUP_K1                 (UINT64_C(1) << 36)
#define SOUND_GROUP_L1                 (UINT64_C(1) << 37)
#define SOUND_GROUP_M1                 (UINT64_C(1) << 38)
#define SOUND_GROUP_N1                 (UINT64_C(1) << 39)
#define SOUND_GROUP_O1                 (UINT64_C(1) << 40)
#define SOUND_GROUP_P1                 (UINT64_C(1) << 41)
#define SOUND_GROUP_Q1                 (UINT64_C(1) << 42)
#define SOUND_GROUP_R1                 (UINT64_C(1) << 43)
#define SOUND_GROUP_S1                 (UINT64_C(1) << 44)
#define SOUND_GROUP_T1                 (UINT64_C(1) << 45)
#define SOUND_GROUP_U1                 (UINT64_C(1) << 46)
#define SOUND_GROUP_V1                 (UINT64_C(1) << 47)
#define SOUND_GROUP_W1                 (UINT64_C(1) << 48)
#define SOUND_GROUP_X1                 (UINT64_C(1) << 49)
#define SOUND_GROUP_Y1                 (UINT64_C(1) << 50)
#define SOUND_GROUP_Z1                 (UINT64_C(1) << 51)

#define SOUND_GROUP_ALL_0                                             \
    (SOUND_GROUP_A  | SOUND_GROUP_B  | SOUND_GROUP_C  |              \
     SOUND_GROUP_D  | SOUND_GROUP_E  | SOUND_GROUP_F  |              \
     SOUND_GROUP_G  | SOUND_GROUP_H  | SOUND_GROUP_I  |              \
     SOUND_GROUP_J  | SOUND_GROUP_K  | SOUND_GROUP_L  |              \
     SOUND_GROUP_M  | SOUND_GROUP_N  | SOUND_GROUP_O  |              \
     SOUND_GROUP_P  | SOUND_GROUP_Q  | SOUND_GROUP_R  |              \
     SOUND_GROUP_S  | SOUND_GROUP_T  | SOUND_GROUP_U  |              \
     SOUND_GROUP_V  | SOUND_GROUP_W  | SOUND_GROUP_X  |              \
     SOUND_GROUP_Y  | SOUND_GROUP_Z)

#define SOUND_GROUP_ALL_1                                             \
    (SOUND_GROUP_A1 | SOUND_GROUP_B1 | SOUND_GROUP_C1 |              \
     SOUND_GROUP_D1 | SOUND_GROUP_E1 | SOUND_GROUP_F1 |              \
     SOUND_GROUP_G1 | SOUND_GROUP_H1 | SOUND_GROUP_I1 |              \
     SOUND_GROUP_J1 | SOUND_GROUP_K1 | SOUND_GROUP_L1 |              \
     SOUND_GROUP_M1 | SOUND_GROUP_N1 | SOUND_GROUP_O1 |              \
     SOUND_GROUP_P1 | SOUND_GROUP_Q1 | SOUND_GROUP_R1 |              \
     SOUND_GROUP_S1 | SOUND_GROUP_T1 | SOUND_GROUP_U1 |              \
     SOUND_GROUP_V1 | SOUND_GROUP_W1 | SOUND_GROUP_X1 |              \
     SOUND_GROUP_Y1 | SOUND_GROUP_Z1)

#define SOUND_GROUP_ALL             (SOUND_GROUP_ALL_0 | SOUND_GROUP_ALL_1)
#define SOUND_GROUP_DEFAULT         SOUND_GROUP_A

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

    uint64_t owner_id;                  /* Unique ID of the owning entity, or zero. */
    sound_group_mask_t group;           /* Sound groups assigned at submission. */
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
bool sound_channel_matches_group(
    const channelstruct *record,
    sound_group_mask_t group,
    uint64_t owner_id
);

#endif
