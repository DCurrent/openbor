/*
 * OpenBOR - http://www.chronocrash.com
 * -----------------------------------------------------------------------
 * All rights reserved, see LICENSE in OpenBOR root for details.
 *
 * Copyright (c) OpenBOR Team
 */

#include <stdlib.h>
#include <string.h>

#include "sound_channel.h"

/*
* Caskey, Damon V.
* 2026-07-31
*
* Synchronize global active and availability
* masks after a bank changes state.
*/
static void sound_channel_pool_update_bank_masks(s_sound_channel_pool *pool, unsigned int bank_index) {
    s_sound_channel_bank *bank;
    uint64_t bank_bit;

    bank = pool->bank[bank_index];
    bank_bit = UINT64_C(1) << bank_index;

    if(bank->active_mask) {
        pool->active_bank_mask |= bank_bit;
    } else {
        pool->active_bank_mask &= ~bank_bit;
    }

    if((bank->active_mask | bank->reserved_mask) == SOUND_CHANNEL_BANK_FULL_MASK) {
        pool->available_bank_mask &= ~bank_bit;
    } else {
        pool->available_bank_mask |= bank_bit;
    }

    if(bank->streaming_mask) {
        pool->streaming_bank_mask |= bank_bit;
    } else {
        pool->streaming_bank_mask &= ~bank_bit;
    }
}

/*
* Caskey, Damon V.
* 2026-07-31
*
* Return the lowest set bit in a 64-bit channel
* mask, or -1 when the mask is empty.
*/
int sound_channel_mask_first(uint64_t mask) {
    int bit_index = 0;

    if(!mask) {
        return -1;
    }

    /* Locate the bit in six bounded comparisons. */
    if(!(mask & UINT64_C(0xFFFFFFFF))) {
        mask >>= 32;
        bit_index += 32;
    }
    if(!(mask & UINT64_C(0xFFFF))) {
        mask >>= 16;
        bit_index += 16;
    }
    if(!(mask & UINT64_C(0xFF))) {
        mask >>= 8;
        bit_index += 8;
    }
    if(!(mask & UINT64_C(0xF))) {
        mask >>= 4;
        bit_index += 4;
    }
    if(!(mask & UINT64_C(0x3))) {
        mask >>= 2;
        bit_index += 2;
    }
    if(!(mask & UINT64_C(0x1))) {
        bit_index++;
    }

    return bit_index;
}

/*
* Caskey, Damon V.
* 2026-07-31
*
* Allocate a requested channel bank. Existing
* banks are retained and treated as success.
*/
bool sound_channel_pool_allocate_bank(s_sound_channel_pool *pool, unsigned int bank_index) {
    unsigned int channel_index;
    uint64_t bank_bit;

    if(!pool || bank_index >= SOUND_CHANNEL_BANK_COUNT) {
        return false;
    }

    bank_bit = UINT64_C(1) << bank_index;
    if(pool->bank[bank_index]) {
        return true;
    }

    pool->bank[bank_index] = calloc(1, sizeof(*pool->bank[bank_index]));
    if(!pool->bank[bank_index]) {
        return false;
    }

    for(channel_index = 0; channel_index < SOUND_CHANNEL_BANK_SIZE; channel_index++) {
        pool->bank[bank_index]->channel[channel_index].object_type = OBJECT_TYPE_SOUND;
        pool->bank[bank_index]->channel[channel_index].index =
            ((int)bank_index * (int)SOUND_CHANNEL_BANK_SIZE) + (int)channel_index;
        pool->bank[bank_index]->channel[channel_index].samplenum = -1;
        pool->bank[bank_index]->channel[channel_index].playid = -1;
        sound_stream_init(&pool->bank[bank_index]->channel[channel_index].stream);
    }

    pool->allocated_bank_mask |= bank_bit;
    pool->available_bank_mask |= bank_bit;
    return true;
}

/*
* Caskey, Damon V.
* 2026-07-31
*
* Initialize the channel pool with the default
* bank of 64 channels.
*/
bool sound_channel_pool_init(s_sound_channel_pool *pool) {
    if(!pool) {
        return false;
    }

    memset(pool, 0, sizeof(*pool));
    return sound_channel_pool_allocate_bank(pool, 0);
}

/*
* Caskey, Damon V.
* 2026-07-31
*
* Release every allocated channel bank during
* sound shutdown.
*/
void sound_channel_pool_destroy(s_sound_channel_pool *pool) {
    unsigned int bank_index;
    unsigned int channel_index;

    if(!pool) {
        return;
    }

    for(bank_index = 0; bank_index < SOUND_CHANNEL_BANK_COUNT; bank_index++) {
        if(pool->bank[bank_index]) {
            for(channel_index = 0; channel_index < SOUND_CHANNEL_BANK_SIZE; channel_index++) {
                sound_stream_destroy(&pool->bank[bank_index]->channel[channel_index].stream);
            }
        }
        free(pool->bank[bank_index]);
    }

    memset(pool, 0, sizeof(*pool));
}

/*
* Caskey, Damon V.
* 2026-07-31
*
* Get a channel record from its flattened public
* channel number.
*/
channelstruct *sound_channel_pool_get(s_sound_channel_pool *pool, int channel) {
    unsigned int bank_index;
    unsigned int channel_index;

    if(!pool || channel < 0 || (unsigned int)channel >= SOUND_CHANNEL_COUNT_MAX) {
        return NULL;
    }

    bank_index = (unsigned int)channel / SOUND_CHANNEL_BANK_SIZE;
    channel_index = (unsigned int)channel % SOUND_CHANNEL_BANK_SIZE;
    if(!pool->bank[bank_index]) {
        return NULL;
    }

    return &pool->bank[bank_index]->channel[channel_index];
}

/*
* Caskey, Damon V.
* 2026-08-02
*
* Resolve a stable channel object back to its
* flattened public index. Scan allocated banks to
* confirm pool membership before dereferencing a
* caller-supplied pointer.
*/
int sound_channel_pool_get_index(const s_sound_channel_pool *pool, const channelstruct *record) {
    const s_sound_channel_bank *bank;
    const channelstruct *indexed_record;
    uintptr_t bank_address;
    uintptr_t record_address;
    uintptr_t record_offset;
    uint64_t allocated_bank_mask;
    int bank_index;
    unsigned int channel_index;
    int channel;

    if(!pool || !record) {
        return -1;
    }

    record_address = (uintptr_t)(const void*)record;
    allocated_bank_mask = pool->allocated_bank_mask;
    while((bank_index = sound_channel_mask_first(allocated_bank_mask)) >= 0) {
        bank = pool->bank[bank_index];
        allocated_bank_mask &= ~(UINT64_C(1) << bank_index);
        if(!bank) {
            continue;
        }

        bank_address = (uintptr_t)(const void*)&bank->channel[0];
        if(record_address < bank_address) {
            continue;
        }

        record_offset = record_address - bank_address;
        if(record_offset >= sizeof(bank->channel) ||
           record_offset % sizeof(bank->channel[0])) {
            continue;
        }

        channel_index = (unsigned int)(record_offset / sizeof(bank->channel[0]));
        indexed_record = &bank->channel[channel_index];
        channel = ((int)bank_index * (int)SOUND_CHANNEL_BANK_SIZE) + (int)channel_index;

        if((uintptr_t)(const void*)indexed_record != record_address ||
           indexed_record->object_type != OBJECT_TYPE_SOUND ||
           indexed_record->index != channel) {
            return -1;
        }

        return channel;
    }

    return -1;
}

/*
* Caskey, Damon V.
* 2026-08-02
*
* Return a pool-level mask describing allocated,
* active, available, or streaming banks.
*/
uint64_t sound_channel_pool_get_bank_mask(const s_sound_channel_pool *pool, e_sound_channel_bank_mask mask) {
    if(!pool) {
        return 0;
    }

    switch(mask) {
        case SOUND_CHANNEL_BANK_MASK_ALLOCATED:
            return pool->allocated_bank_mask;
        case SOUND_CHANNEL_BANK_MASK_ACTIVE:
            return pool->active_bank_mask;
        case SOUND_CHANNEL_BANK_MASK_AVAILABLE:
            return pool->available_bank_mask;
        case SOUND_CHANNEL_BANK_MASK_STREAMING:
            return pool->streaming_bank_mask;
        case SOUND_CHANNEL_BANK_MASK_END:
        default:
            return 0;
    }
}

/*
* Caskey, Damon V.
* 2026-08-02
*
* Return a channel-state mask from one allocated
* bank. Unallocated banks contain no set channels.
*/
uint64_t sound_channel_pool_get_mask(const s_sound_channel_pool *pool, unsigned int bank_index, e_sound_channel_mask mask) {
    const s_sound_channel_bank *bank;

    if(!pool || bank_index >= SOUND_CHANNEL_BANK_COUNT) {
        return 0;
    }

    bank = pool->bank[bank_index];
    if(!bank) {
        return 0;
    }

    switch(mask) {
        case SOUND_CHANNEL_MASK_ACTIVE:
            return bank->active_mask;
        case SOUND_CHANNEL_MASK_PAUSED:
            return bank->paused_mask;
        case SOUND_CHANNEL_MASK_RESERVED:
            return bank->reserved_mask;
        case SOUND_CHANNEL_MASK_STREAMING:
            return bank->streaming_mask;
        case SOUND_CHANNEL_MASK_END:
        default:
            return 0;
    }
}

/*
* Caskey, Damon V.
* 2026-07-31
*
* Report channel activity from the bank masks.
*/
bool sound_channel_pool_is_active(const s_sound_channel_pool *pool, int channel) {
    unsigned int bank_index;
    unsigned int channel_index;

    if(!pool || channel < 0 || (unsigned int)channel >= SOUND_CHANNEL_COUNT_MAX) {
        return false;
    }

    bank_index = (unsigned int)channel / SOUND_CHANNEL_BANK_SIZE;
    channel_index = (unsigned int)channel % SOUND_CHANNEL_BANK_SIZE;
    if(!pool->bank[bank_index]) {
        return false;
    }

    return (pool->bank[bank_index]->active_mask & (UINT64_C(1) << channel_index)) != 0;
}

/*
* Caskey, Damon V.
* 2026-07-31
*
* Acquire the lowest available channel. Allocate
* another bank when every current bank is full.
*/
int sound_channel_pool_acquire(s_sound_channel_pool *pool) {
    s_sound_channel_bank *bank;
    uint64_t bank_mask;
    uint64_t channel_mask;
    uint64_t unallocated_bank_mask;
    int bank_index;
    int channel_index;

    if(!pool) {
        return -1;
    }

    for(;;) {
        bank_mask = pool->available_bank_mask;
        if(!bank_mask) {
            unallocated_bank_mask = ~pool->allocated_bank_mask;
            bank_index = sound_channel_mask_first(unallocated_bank_mask);
            if(bank_index < 0 || !sound_channel_pool_allocate_bank(pool, (unsigned int)bank_index)) {
                return -1;
            }
            bank_mask = pool->available_bank_mask;
        }

        bank_index = sound_channel_mask_first(bank_mask);
        if(bank_index < 0) {
            return -1;
        }

        bank = pool->bank[bank_index];
        channel_mask = ~(bank->active_mask | bank->reserved_mask);
        channel_index = sound_channel_mask_first(channel_mask);
        if(channel_index >= 0) {
            return (bank_index * (int)SOUND_CHANNEL_BANK_SIZE) + channel_index;
        }

        /* Repair a stale availability bit and continue. */
        pool->available_bank_mask &= ~(UINT64_C(1) << bank_index);
    }
}

/*
* Caskey, Damon V.
* 2026-07-31
*
* Mark a channel active and synchronize all pool
* masks with the channel record.
*/
void sound_channel_pool_activate(s_sound_channel_pool *pool, int channel, int active_state) {
    s_sound_channel_bank *bank;
    channelstruct *record;
    uint64_t channel_bit;
    unsigned int bank_index;
    unsigned int channel_index;

    if(active_state == 0) {
        sound_channel_pool_deactivate(pool, channel);
        return;
    }

    record = sound_channel_pool_get(pool, channel);
    if(!record) {
        return;
    }

    bank_index = (unsigned int)channel / SOUND_CHANNEL_BANK_SIZE;
    channel_index = (unsigned int)channel % SOUND_CHANNEL_BANK_SIZE;
    bank = pool->bank[bank_index];
    channel_bit = UINT64_C(1) << channel_index;

    record->active = active_state;
    record->paused = 0;
    bank->active_mask |= channel_bit;
    bank->paused_mask &= ~channel_bit;
    sound_channel_pool_update_bank_masks(pool, bank_index);
}

/*
* Caskey, Damon V.
* 2026-07-31
*
* Stop one channel and make its slot available
* without releasing the containing bank.
*/
void sound_channel_pool_deactivate(s_sound_channel_pool *pool, int channel) {
    s_sound_channel_bank *bank;
    channelstruct *record;
    uint64_t channel_bit;
    unsigned int bank_index;
    unsigned int channel_index;

    record = sound_channel_pool_get(pool, channel);
    if(!record) {
        return;
    }

    bank_index = (unsigned int)channel / SOUND_CHANNEL_BANK_SIZE;
    channel_index = (unsigned int)channel % SOUND_CHANNEL_BANK_SIZE;
    bank = pool->bank[bank_index];
    channel_bit = UINT64_C(1) << channel_index;

    record->active = 0;
    record->paused = 0;
    bank->active_mask &= ~channel_bit;
    bank->paused_mask &= ~channel_bit;
    sound_channel_pool_update_bank_masks(pool, bank_index);
}

/*
* Caskey, Damon V.
* 2026-07-31
*
* Pause or resume one active channel.
*/
void sound_channel_pool_pause(s_sound_channel_pool *pool, int channel, int toggle) {
    s_sound_channel_bank *bank;
    channelstruct *record;
    uint64_t channel_bit;
    unsigned int bank_index;
    unsigned int channel_index;

    if(!sound_channel_pool_is_active(pool, channel)) {
        return;
    }

    record = sound_channel_pool_get(pool, channel);
    bank_index = (unsigned int)channel / SOUND_CHANNEL_BANK_SIZE;
    channel_index = (unsigned int)channel % SOUND_CHANNEL_BANK_SIZE;
    bank = pool->bank[bank_index];
    channel_bit = UINT64_C(1) << channel_index;
    record->paused = toggle ? 1 : 0;

    if(toggle) {
        bank->paused_mask |= channel_bit;
    } else {
        bank->paused_mask &= ~channel_bit;
    }
}

/*
* Caskey, Damon V.
* 2026-08-01
*
* Mark a channel as owning live streaming state.
* Streaming masks drive producer updates and remain
* independent from active playback masks so finished
* channels can close files outside the audio callback.
*/
void sound_channel_pool_stream(s_sound_channel_pool *pool, int channel, int toggle) {
    s_sound_channel_bank *bank;
    uint64_t channel_bit;
    unsigned int bank_index;
    unsigned int channel_index;

    if(!sound_channel_pool_get(pool, channel)) {
        return;
    }

    bank_index = (unsigned int)channel / SOUND_CHANNEL_BANK_SIZE;
    channel_index = (unsigned int)channel % SOUND_CHANNEL_BANK_SIZE;
    bank = pool->bank[bank_index];
    channel_bit = UINT64_C(1) << channel_index;

    if(toggle) {
        bank->streaming_mask |= channel_bit;
    } else {
        bank->streaming_mask &= ~channel_bit;
    }

    sound_channel_pool_update_bank_masks(pool, bank_index);
}

/*
* Caskey, Damon V.
* 2026-07-31
*
* Reserve a bank-sized channel mask from ordinary
* automatic allocation. Active channels continue.
*/
bool sound_channel_pool_reserve_mask(s_sound_channel_pool *pool, unsigned int bank_index, uint64_t reserved_mask) {
    if(!sound_channel_pool_allocate_bank(pool, bank_index)) {
        return false;
    }

    pool->bank[bank_index]->reserved_mask = reserved_mask;
    sound_channel_pool_update_bank_masks(pool, bank_index);
    return true;
}

/*
* Caskey, Damon V.
* 2026-07-31
*
* Stop active channels through the bank masks. Reserved
* channels retain playback and pause state unless force
* is enabled for complete playback teardown.
*/
void sound_channel_pool_stop_all(s_sound_channel_pool *pool, bool force) {
    s_sound_channel_bank *bank;
    uint64_t active_bank_mask;
    uint64_t allocated_bank_mask;
    uint64_t channel_mask;
    uint64_t retained_mask;
    int bank_index;
    int channel_index;

    if(!pool) {
        return;
    }

    active_bank_mask = pool->active_bank_mask;
    while((bank_index = sound_channel_mask_first(active_bank_mask)) >= 0) {
        bank = pool->bank[bank_index];
        retained_mask = force ? 0 : bank->reserved_mask;
        channel_mask = bank->active_mask & ~retained_mask;

        while((channel_index = sound_channel_mask_first(channel_mask)) >= 0) {
            bank->channel[channel_index].active = 0;
            bank->channel[channel_index].paused = 0;
            channel_mask &= ~(UINT64_C(1) << channel_index);
        }

        bank->active_mask &= retained_mask;
        bank->paused_mask &= retained_mask;
        active_bank_mask &= ~(UINT64_C(1) << bank_index);
    }

    pool->active_bank_mask = 0;
    pool->available_bank_mask = 0;
    allocated_bank_mask = pool->allocated_bank_mask;
    while((bank_index = sound_channel_mask_first(allocated_bank_mask)) >= 0) {
        sound_channel_pool_update_bank_masks(pool, (unsigned int)bank_index);
        allocated_bank_mask &= ~(UINT64_C(1) << bank_index);
    }
}

/*
* Caskey, Damon V.
* 2026-07-31
*
* Pause or resume every non-reserved active channel
* through the bank masks. Reserved channel state is
* controlled explicitly by its owner.
*/
void sound_channel_pool_pause_all(s_sound_channel_pool *pool, int toggle) {
    s_sound_channel_bank *bank;
    uint64_t active_bank_mask;
    uint64_t channel_mask;
    int bank_index;
    int channel_index;

    if(!pool) {
        return;
    }

    active_bank_mask = pool->active_bank_mask;
    while((bank_index = sound_channel_mask_first(active_bank_mask)) >= 0) {
        bank = pool->bank[bank_index];
        channel_mask = bank->active_mask & ~bank->reserved_mask;

        while((channel_index = sound_channel_mask_first(channel_mask)) >= 0) {
            bank->channel[channel_index].paused = toggle ? 1 : 0;
            channel_mask &= ~(UINT64_C(1) << channel_index);
        }

        if(toggle) {
            bank->paused_mask |= bank->active_mask & ~bank->reserved_mask;
        } else {
            bank->paused_mask &= bank->reserved_mask;
        }
        active_bank_mask &= ~(UINT64_C(1) << bank_index);
    }
}
