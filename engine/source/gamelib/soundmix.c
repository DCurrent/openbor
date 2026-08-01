/*
 * OpenBOR - http://www.chronocrash.com
 * -----------------------------------------------------------------------
 * All rights reserved, see LICENSE in OpenBOR root for details.
 *
 * Copyright (c)  OpenBOR Team
 */

/*
**	Sound mixer.
**	High quality, with support for ADPCM and Vorbis-compressed music.
**
**	Also plays WAV files (8-bit, 16-bit, and 24-bit).
**	Note: 8-bit WAVs are unsigned; 16-bit and 24-bit WAVs are signed.
**
**
**	Function naming convention:
**	- Public functions start with "sound_"
**	- Music-related functions end with "_music"
**	- Soundeffect-related functions end with "_sample"
**
**
**	To do:
**	- I think it's stable now, but stay alert!
**	- test 16-bit soundfx
**
**
**	Note:
**  If any of the #defines are increased in size
**  pay close attention to the the size of variables
**  which are used in conjunction with it.  You could
**  be going beyond the variable's current size which
**  will cause errors!!!
*/

#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
/*
Caution: move vorbis headers here otherwise the structs will
 get poisoned by #pragma in other header files, i.e. list.h
*/
#if TREMOR
#include <tremor/ivorbisfile.h>
#else
#include <vorbis/vorbisfile.h>
#endif
#include "soundmix.h"
#include "globals.h"
#include "adpcm.h"
#include "sblaster.h"
#include "borendian.h"
#include "packfile.h"



#define		MIXSHIFT		     3	    // 2 should be OK

/*
    Kratus (01-2024) Reverted all volume values but separated both music/sample volumes in different constants 
    Fixed the "nullified" samples when many of them are played at the same time using 8 bits
    This was made to equalize both in the volume of 100, and at the same time to make them louder than before
    This way we don't need to increase the volume too much in the audio files, preventing distortions and quality loss
*/ 
#define		MAX_SAMPLE_VOLUME   100 // 64 for backw. compat
#define		MAX_MUSIC_VOLUME    60 // 64 for backw. compat
// Hardware settings for SoundBlaster (change only if latency is too big)
#define		SB_BUFFER_SIZE		 0x8000
#define		SB_BUFFER_SIZE_MASK	 0x7FFF
#define		SB_WBUFFER_SIZE		 0x4000
#define		SB_WBUFFER_SIZE_MASK 0x3FFF
#define		MIXBUF_SIZE		     SB_BUFFER_SIZE*8
#define		PREMIX_SIZE		     1024
#define		MIX_BLOCK_SIZE		 32

#pragma pack(4)

s_sound_parameters sound_parameters = {
    .music_buffers_count = 4,
    .music_buffer_size = (16 * 1024),
    .sound_length_max = UINT64_MAX
};

s_audio_global audio_global =
{
    .sample_play_id = 0,
    .soundcache = NULL,
    .sound_cached = 0,
};

//static List samplelist;
static s_soundcache *soundcache = NULL;
static int sound_cached = 0;
int sample_play_id = 0;

static s_sound_channel_pool sound_channel_pool;
musicchannelstruct musicchannel = { .object_type = OBJECT_TYPE_MUSIC_CHANNEL };
static s32 *mixbuf = NULL;
static int playbits;
int playfrequency;

// Indicates whether the hardware is playing, and if mixing is active
static int mixing_active = 0;

// Indicates whether the sound system is initialized
static int mixing_inited = 0;

// Counts the total number of samples played
static u32 samplesplayed;

// Records type of currently playing music.
static e_sound_file_type music_type = SOUND_FILE_TYPE_ADPCM;

//////////////////////////////// WAVE LOADER //////////////////////////////////

#pragma pack(push, 1)

#define		HEX_RIFF	0x46464952
#define		HEX_WAVE	0x45564157
#define		HEX_fmt		0x20746D66
#define		HEX_data	0x61746164
#define		FMT_PCM		0x0001
#define		FMT_EXTENSIBLE	0xFFFE

void  sound_music_channel_clear(musicchannelstruct* const music_channel)
{
    memset(music_channel, 0, sizeof(*music_channel));
    music_channel->object_type = OBJECT_TYPE_MUSIC_CHANNEL;
}

typedef struct s_wave_riff_header
{
    u32 riff;
    u32 size;
    u32 type;
} s_wave_riff_header;

typedef struct s_wave_chunk_header
{
    u32 tag;
    u32 size;
} s_wave_chunk_header;

typedef struct s_wave_format_header
{
    u16 format;       /* PCM or WAVE_FORMAT_EXTENSIBLE. */
    u16 channels;     /* Mono, stereo */
    u32 samplerate;   /* Source sample rate. */
    u32 bps;          /* Bytes/second */
    u16 blockalign;   /* Bytes in one complete PCM frame. */
    u16 samplebits;   /* 8, 16, or packed 24. */
} s_wave_format_header;

typedef struct s_wave_format_extensible
{
    u16 extension_size;
    u16 valid_bits;
    u32 channel_mask;
    u8 subformat[16];
} s_wave_format_extensible;

static const u8 wave_subformat_pcm[16] = {
    0x01, 0x00, 0x00, 0x00,
    0x00, 0x00,
    0x10, 0x00,
    0x80, 0x00,
    0x00, 0xAA, 0x00, 0x38, 0x9B, 0x71
};

/*
* readpackfile() intentionally uses int-sized 
* requests. Keep the WAV loader future-proof by 
* reading large in-memory samples through repeated 
* reads.
*/
static int sound_read_packfile_exact(int packfile_handle, void *destination_buffer, uint64_t bytes_to_read) {
    unsigned char *write_position = (unsigned char *)destination_buffer;

    while(bytes_to_read > 0) {
        int requested_read_size;
        int actual_read_size;

        requested_read_size = bytes_to_read > (uint64_t)INT_MAX ? INT_MAX : (int)bytes_to_read;
        actual_read_size = readpackfile(packfile_handle, write_position, requested_read_size);

        if(actual_read_size <= 0)
        {
            return 0;
        }

        write_position += actual_read_size;
        bytes_to_read -= (uint64_t)actual_read_size;
    }

    return 1;
}

/*
* WAV chunks are word aligned. The size stored in the chunk header does not
* include the optional pad byte, so callers need to include it when skipping.
*/
static int sound_wave_chunk_skip_size(uint64_t chunk_data_size, uint64_t *chunk_skip_size) {

    if(chunk_data_size == UINT64_MAX) {
        return 0;
    }

    *chunk_skip_size = chunk_data_size + (chunk_data_size & 1U);
    return 1;
}

/*
* The pack layer exposes 64-bit seeks, but the offset is signed. Keep the
* conversion guarded so malformed or future oversized chunks fail cleanly.
*/
static int sound_seek_packfile_forward(int packfile_handle, uint64_t bytes_to_skip) {
    uint64_t chunk_skip_size;

    if(!sound_wave_chunk_skip_size(bytes_to_skip, &chunk_skip_size)) {
        return 0;
    
    } 
    
    if(chunk_skip_size > (uint64_t)INT64_MAX) {
        return 0;
    }

    return seekpackfile64(packfile_handle, (packfile_signed_offset_t)chunk_skip_size, SEEK_CUR) >= 0;
}

static int sound_read_wave_chunk_header(int packfile_handle, s_wave_chunk_header *wave_chunk_header) {
    
    if(readpackfile(packfile_handle, wave_chunk_header, sizeof(*wave_chunk_header)) != sizeof(*wave_chunk_header)) {
        return 0;
    }

    wave_chunk_header->tag = SwapLSB32(wave_chunk_header->tag);
    wave_chunk_header->size = SwapLSB32(wave_chunk_header->size);

    return 1;
}

/*
* Caskey, Damon V.
* 2026-08-01
*
* Validate RIFF/WAVE PCM and retain either the
* complete sample or streaming metadata only. File
* and chunk sizes remain 64-bit so larger container
* formats can use the same sample data model later.
*/
static bool loadwave(char *filename, char *packname, samplestruct *sample, uint64_t maximum_data_bytes, bool stream) {

    s_wave_riff_header wave_riff_header;
    s_wave_chunk_header wave_chunk_header;
    s_wave_format_header wave_format_header;
    uint64_t data_chunk_size = 0;
    uint64_t sample_data_bytes = 0;
    uint64_t sample_unit_count = 0;
    uint64_t bytes_per_sample_unit = 0;
    uint64_t complete_frame_count = 0;
    packfile_signed_offset_t data_chunk_offset;
    packfile_signed_offset_t source_file_size;
    size_t allocation_size = 0;
    int packfile_handle;
    int format_chunk_found = 0;
    bool wave_format_is_pcm = false;

    if(sample == NULL) {
        return false;
    }

    memset(sample, 0, sizeof(*sample));

    packfile_handle = openpackfile(filename, packname);

    if(packfile_handle == -1) {
        return false;
    }

    if(readpackfile(packfile_handle, &wave_riff_header, sizeof(wave_riff_header)) != sizeof(wave_riff_header)) {
        closepackfile(packfile_handle);
        return false;
    }

    wave_riff_header.riff = SwapLSB32(wave_riff_header.riff);
    wave_riff_header.size = SwapLSB32(wave_riff_header.size);
    wave_riff_header.type = SwapLSB32(wave_riff_header.type);

    if(wave_riff_header.riff != HEX_RIFF || wave_riff_header.type != HEX_WAVE) {
        closepackfile(packfile_handle);
        return false;
    }

    /*
    * Find and read the PCM format chunk before loading sample data. Unknown
    * chunks are skipped with 64-bit seek support and RIFF pad-byte handling.
    */
    while(!format_chunk_found) {
        uint64_t format_chunk_size;
        uint64_t remaining_format_bytes;

        if(!sound_read_wave_chunk_header(packfile_handle, &wave_chunk_header)) {
            closepackfile(packfile_handle);
            return false;
        }

        if(wave_chunk_header.tag != HEX_fmt) {
            if(!sound_seek_packfile_forward(packfile_handle, wave_chunk_header.size)) {
                closepackfile(packfile_handle);
                return false;
            }
            continue;
        }

        format_chunk_size = wave_chunk_header.size;
        if(format_chunk_size < sizeof(wave_format_header)) {
            closepackfile(packfile_handle);
            return false;
        }

        if(readpackfile(packfile_handle, &wave_format_header, sizeof(wave_format_header)) != sizeof(wave_format_header)) {
            closepackfile(packfile_handle);
            return false;
        }

        wave_format_header.format = SwapLSB16(wave_format_header.format);
        wave_format_header.channels = SwapLSB16(wave_format_header.channels);
        wave_format_header.blockalign = SwapLSB16(wave_format_header.blockalign);
        wave_format_header.samplebits = SwapLSB16(wave_format_header.samplebits);
        wave_format_header.samplerate = SwapLSB32(wave_format_header.samplerate);
        wave_format_header.bps = SwapLSB32(wave_format_header.bps);

        remaining_format_bytes = format_chunk_size - sizeof(wave_format_header);

        if(wave_format_header.format == FMT_PCM) {
            wave_format_is_pcm = true;
        } else if(wave_format_header.format == FMT_EXTENSIBLE) {
            s_wave_format_extensible wave_format_extensible;

            if(remaining_format_bytes < sizeof(wave_format_extensible) ||
               readpackfile(packfile_handle, &wave_format_extensible, sizeof(wave_format_extensible)) != sizeof(wave_format_extensible)) {
                closepackfile(packfile_handle);
                return false;
            }

            wave_format_extensible.extension_size = SwapLSB16(wave_format_extensible.extension_size);
            wave_format_extensible.valid_bits = SwapLSB16(wave_format_extensible.valid_bits);
            wave_format_extensible.channel_mask = SwapLSB32(wave_format_extensible.channel_mask);
            remaining_format_bytes -= sizeof(wave_format_extensible);

            wave_format_is_pcm = wave_format_extensible.extension_size >= 22U &&
                                 wave_format_extensible.valid_bits == wave_format_header.samplebits &&
                                 memcmp(wave_format_extensible.subformat, wave_subformat_pcm, sizeof(wave_subformat_pcm)) == 0;
        }

        if(remaining_format_bytes > 0 && !sound_seek_packfile_forward(packfile_handle, remaining_format_bytes)) {
            closepackfile(packfile_handle);
            return false;
        }

        format_chunk_found = 1;
    }

    if(!wave_format_is_pcm ||
       (wave_format_header.channels != 1 && wave_format_header.channels != 2) ||
       (wave_format_header.samplebits != 8 && wave_format_header.samplebits != 16 && wave_format_header.samplebits != 24)) {
        closepackfile(packfile_handle);
        return false;
    }

    bytes_per_sample_unit = (uint64_t)(wave_format_header.samplebits / 8U);
    if(bytes_per_sample_unit == 0 || wave_format_header.blockalign == 0) {
        closepackfile(packfile_handle);
        return false;
    }

    /*
    * Retain soundlen in scalar sample units as cache metadata. Playback uses
    * framecount so stereo channel pairs always advance together.
    */
    if(wave_format_header.blockalign != wave_format_header.channels * bytes_per_sample_unit) {
        closepackfile(packfile_handle);
        return false;
    }

    /*
    * Find the sample data chunk after the format has been validated.
    */
    for(;;) {

        if(!sound_read_wave_chunk_header(packfile_handle, &wave_chunk_header)) {
            closepackfile(packfile_handle);
            return false;
        }

        if(wave_chunk_header.tag == HEX_data) {
            data_chunk_size = wave_chunk_header.size;
            break;
        }

        if(!sound_seek_packfile_forward(packfile_handle, wave_chunk_header.size)) {
            closepackfile(packfile_handle);
            return false;
        }
    }

    sample_data_bytes = data_chunk_size;
    if(!stream && sample_data_bytes > maximum_data_bytes) {
        sample_data_bytes = maximum_data_bytes;
    }

    /*
    * Only complete PCM frames are exposed to 
    * the mixer. This prevents partial trailing 
    * bytes from being addressable if a malformed 
    * file or cap appears.
    */
    sample_data_bytes -= sample_data_bytes % wave_format_header.blockalign;
    if(sample_data_bytes == 0) {
        closepackfile(packfile_handle);
        return false;
    }

    sample_unit_count = sample_data_bytes / bytes_per_sample_unit;
    complete_frame_count = sample_data_bytes / wave_format_header.blockalign;
    if(complete_frame_count > SOUND_SAMPLE_FIXED_MAX_INTEGER) {
        closepackfile(packfile_handle);
        return false;
    }

    data_chunk_offset = seekpackfile64(packfile_handle, 0, SEEK_CUR);
    source_file_size = seekpackfile64(packfile_handle, 0, SEEK_END);
    if(data_chunk_offset < 0 ||
       source_file_size < data_chunk_offset ||
       sample_data_bytes > (uint64_t)(source_file_size - data_chunk_offset) ||
       seekpackfile64(packfile_handle, data_chunk_offset, SEEK_SET) != data_chunk_offset) {
        closepackfile(packfile_handle);
        return false;
    }

    sample->soundbytes = sample_data_bytes;
    sample->soundlen = sample_unit_count;
    sample->framecount = complete_frame_count;
    sample->data_offset = (uint64_t)data_chunk_offset;
    sample->bits = wave_format_header.samplebits;
    sample->frequency = wave_format_header.samplerate;
    sample->channels = wave_format_header.channels;
    sample->blockalign = wave_format_header.blockalign;

    /* Streamed samples retain metadata only. */
    if(stream) {
        closepackfile(packfile_handle);
        return true;
    }

    if(sample_data_bytes > (uint64_t)(SIZE_MAX - 8U)) {
        closepackfile(packfile_handle);
        return false;
    }

    allocation_size = (size_t)sample_data_bytes + 8U;
    sample->sampleptr = malloc(allocation_size);
    if(sample->sampleptr == NULL) {
        closepackfile(packfile_handle);
        return false;
    }

    memset(sample->sampleptr, wave_format_header.samplebits == 8 ? 0x80 : 0x00, allocation_size);

    if(!sound_read_packfile_exact(packfile_handle, sample->sampleptr, sample_data_bytes)) {
        free(sample->sampleptr);
        sample->sampleptr = NULL;
        closepackfile(packfile_handle);
        return false;
    }

    closepackfile(packfile_handle);

    return true;
}

bool sound_reload_sample(int index) {
    
    if(!mixing_inited) {
        return false;
    }

    if(index < 0 || index >= sound_cached) {
        return false;
    }

    if(soundcache[index].stream) {
        if(soundcache[index].sample.framecount > 0) {
            return true;
        }

        return loadwave(
            soundcache[index].filename,
            soundcache[index].packfilename,
            &soundcache[index].sample,
            sound_parameters.sound_length_max,
            true
        );
    }

    if(!soundcache[index].sample.sampleptr) {

        //printf("packfile: '%s'\n", packfile);
        return loadwave(
            soundcache[index].filename,
            soundcache[index].packfilename,
            &soundcache[index].sample,
            sound_parameters.sound_length_max,
            false
        );
    
    } else {
        return true;
    }
}


/*
* Caskey, Damon V.
* 2026-08-01
*
* Load or find a resident or streamed sample. Cache
* identities include storage mode so the same source
* may be loaded independently in both forms.
*/
int sound_load_sample(char *filename, char *packfilename, bool log_errors, bool stream) {

    s_soundcache *cache;
    s_soundcache *expanded_cache;
    samplestruct sample;
    char *cache_key;
    char *source_filename;
    char *source_packfilename;
    const char *cache_prefix;
    size_t cache_key_size;
    
    if(!mixing_inited) {
        return -1;
    }

    if(!filename || !packfilename) {
        return -1;
    }

    cache_prefix = stream ? "stream:" : "memory:";
    cache_key_size = strlen(cache_prefix) + strlen(filename) + 1U;
    cache_key = malloc(cache_key_size);
    if(!cache_key) {
        return -1;
    }

    snprintf(cache_key, cache_key_size, "%s%s", cache_prefix, filename);
    lc(cache_key, strlen(cache_key));

    /*
    * First look for existing sample in the cache. If
    * it is found, then attempt to restore the sample 
    * pointer if it is NULL and return its index. If 
    * the sample  cannot be restored, log an error.
    */
    {
        const bool sample_found = List_FindByName(&audio_global.samplelist, cache_key);

        if(sample_found) {
            
            cache = &soundcache[(size_t)List_Retrieve(&audio_global.samplelist)];
            free(cache_key);
            
            if((cache->stream && cache->sample.framecount == 0) ||
               (!cache->stream && !cache->sample.sampleptr)) {

                const bool reload_success = sound_reload_sample(cache->index);

                if(!reload_success && log_errors) {
                    printf("sound_load_sample can't restore sample from file '%s'!\n", filename);
                }
            }
            return cache->index;
        }
    }

    /*
    * No existing sample was found in the cache, so 
    * attempt to load it from the packfile.
    */
    
    memset(&sample, 0, sizeof(sample));

    const bool load_success = loadwave(filename, packfilename, &sample, sound_parameters.sound_length_max, stream);

    if(!load_success) {
        free(cache_key);
        if(log_errors) {
            printf("sound_load_sample can't load sample from file '%s'!\n", filename);
        }
        return -1;
    }

    source_filename = malloc(strlen(filename) + 1U);
    source_packfilename = malloc(strlen(packfilename) + 1U);
    if(!source_filename || !source_packfilename) {
        free(source_filename);
        free(source_packfilename);
        free(sample.sampleptr);
        free(cache_key);
        return -1;
    }

    strcpy(source_filename, filename);
    strcpy(source_packfilename, packfilename);

    if((size_t)sound_cached >= (SIZE_MAX / sizeof(*soundcache)) - 1U) {
        free(source_filename);
        free(source_packfilename);
        free(sample.sampleptr);
        free(cache_key);
        return -1;
    }

    expanded_cache = realloc(soundcache, sizeof(*soundcache) * ((size_t)sound_cached + 1U));
    if(!expanded_cache) {
        free(source_filename);
        free(source_packfilename);
        free(sample.sampleptr);
        free(cache_key);
        return -1;
    }

    soundcache = expanded_cache;
    memset(&soundcache[sound_cached], 0, sizeof(soundcache[sound_cached]));
    soundcache[sound_cached].sample = sample;

    List_GotoLast(&audio_global.samplelist);
    List_InsertAfter(&audio_global.samplelist, (void *)(size_t)sound_cached, cache_key);
    soundcache[sound_cached].index = sound_cached;
    soundcache[sound_cached].filename = source_filename;
    soundcache[sound_cached].packfilename = source_packfilename;
    soundcache[sound_cached].stream = stream;

    free(cache_key);

    sound_cached++;
    return sound_cached - 1;
}

// Changed to conserve memory: added this function
void sound_unload_sample(int index)
{
    if(!mixing_inited)
    {
        return;
    }
    if(index < 0 || index >= sound_cached)
    {
        return;
    }
    if(soundcache[index].sample.sampleptr != NULL)
    {
        free(soundcache[index].sample.sampleptr);
        memset(&soundcache[index].sample, 0, sizeof(samplestruct));
    }
    else if(soundcache[index].stream)
    {
        memset(&soundcache[index].sample, 0, sizeof(samplestruct));
    }
}

void sound_unload_all_samples()
{
    int i;
    if(!soundcache)
    {
        return;
    }
    for(i = 0; i < sound_cached; i++)
    {
        sound_unload_sample(i);
        free(soundcache[i].filename);
        soundcache[i].filename = NULL;
        free(soundcache[i].packfilename);
        soundcache[i].packfilename = NULL;
    }
    List_Clear(&audio_global.samplelist);
    free(soundcache);
    soundcache = NULL;
    sound_cached = 0;
}

#pragma pack(pop)

/////////////////////////////// Mix to DMA //////////////////////////////////
// Mixbuffer / DMA buffer data handling
// Writes mixbuffer data (16-bit mixed in 32-bit array)
// to 8-bit or 16-bit DMA buffer.

// Fill the mixbuffer with silence
static void clearmixbuffer(unsigned int *buf, int n)
{
    while((--n) >= 0)
    {
        *buf = 0x8000 << MIXSHIFT;
        ++buf;
    }
}


/////////////////////////////////// Mixers ///////////////////////////////////
// Mixers: mix (16-bit) in the mixbuffer, then write to DMA memory (see above).
// The mixing code handles fixed-point conversion and looping.


/*
* Advance a PCM frame cursor without allowing 
* 64-bit addition to wrap. Once the end is reached, 
* preserve any overshoot and resume from loop_start_fixed.
* The caller uses sample_end_reached to stop one-shot 
* playback.
*/
static sound_sample_fixed_t sound_sample_position_advance(sound_sample_fixed_t sample_position_fixed, sound_sample_fixed_t sample_period_fixed, sound_sample_fixed_t sample_length_fixed, sound_sample_fixed_t loop_start_fixed, int *sample_end_reached)
{
    sound_sample_fixed_t distance_to_end;
    sound_sample_fixed_t loop_length_fixed;

    if(sample_position_fixed >= sample_length_fixed || loop_start_fixed >= sample_length_fixed)
    {
        *sample_end_reached = 1;
        return 0;
    }

    distance_to_end = sample_length_fixed - sample_position_fixed;
    if(sample_period_fixed < distance_to_end)
    {
        *sample_end_reached = 0;
        return sample_position_fixed + sample_period_fixed;
    }

    *sample_end_reached = 1;
    loop_length_fixed = sample_length_fixed - loop_start_fixed;

    return loop_start_fixed + ((sample_period_fixed - distance_to_end) % loop_length_fixed);
}

/*
* Caskey, Damon V.
* 2026-07-31
* 
* Calculate the PCM frame period for 
* a given source frequency and tempo.
*/
unsigned int sound_music_period_calculate(int source_frequency, int tempo) {
    uint64_t period_numerator;
    uint64_t period_denominator;
    uint64_t period;

    if(source_frequency <= 0 
        || tempo <= 0 
        || playfrequency <= 0) {
        return 0;
    }

    period_numerator = (uint64_t)(unsigned int)source_frequency * (uint64_t)(unsigned int)tempo;
    if(period_numerator > UINT64_MAX / MUSIC_SAMPLE_FIXED_ONE) {
        return UINT_MAX;
    }

    period_numerator *= MUSIC_SAMPLE_FIXED_ONE;
    period_denominator = (uint64_t)(unsigned int)playfrequency * 100U;
    period = period_numerator / period_denominator;

    if(period > UINT_MAX) {
        return UINT_MAX;
    }

    return period == 0 ? 1U : (unsigned int)period;
}

/*
* Caskey, Damon V.
* 2026-07-31
*
* Normalize supported PCM source widths 
* to the mixer's signed 16-bit range.
* 
* Packed 24-bit input drops its least-significant 
* byte until we can widen the output path in a 
* future update.
*/
static int sound_pcm_to_mix_value(const unsigned char *sample_data, int sample_bits, size_t sample_index) {
    if(sample_bits == 8){
        return ((int)sample_data[sample_index] - 0x80) * 0x100;
    }

    if(sample_bits == 16) {

        size_t byte_index = sample_index * 2U;
        uint16_t sample_value = (uint16_t)sample_data[byte_index] |
                                ((uint16_t)sample_data[byte_index + 1U] << 8U);

        return (int)(int16_t)sample_value;
    }

    if(sample_bits == 24) {
        size_t byte_index = sample_index * 3U;
        uint16_t sample_value = (uint16_t)sample_data[byte_index + 1U] |
                                ((uint16_t)sample_data[byte_index + 2U] << 8U);

        return (int)(int16_t)sample_value;
    }

    return 0;
}

static int sound_sample_to_mix_value(const samplestruct *sample, size_t sample_index) {
    return sound_pcm_to_mix_value(sample->sampleptr, sample->bits, sample_index);
}

typedef struct s_sound_stream_read_context {
    int handle;
    uint64_t data_offset;
    size_t block_align;
} s_sound_stream_read_context;

/*
* Caskey, Damon V.
* 2026-08-01
*
* Seek and fill one PCM stream buffer from a
* validated source frame. This runs only in the
* producer path, never in the audio callback.
*/
static bool sound_stream_read_frames(
    void *context,
    uint64_t source_start_frame,
    void *destination,
    size_t bytes_to_read
) {
    s_sound_stream_read_context *read_context = context;
    uint64_t byte_offset;

    if(!read_context ||
       read_context->handle < 0 ||
       read_context->block_align == 0 ||
       source_start_frame > (UINT64_MAX - read_context->data_offset) / read_context->block_align) {
        return false;
    }

    byte_offset = read_context->data_offset + source_start_frame * read_context->block_align;
    if(byte_offset > (uint64_t)INT64_MAX || bytes_to_read > (size_t)INT_MAX) {
        return false;
    }

    if(seekpackfile64(read_context->handle, (packfile_signed_offset_t)byte_offset, SEEK_SET) !=
       (packfile_signed_offset_t)byte_offset) {
        return false;
    }

    return sound_read_packfile_exact(read_context->handle, destination, bytes_to_read) != 0;
}

/*
* Caskey, Damon V.
* 2026-08-01
*
* Mix one streamed effect from ready PCM buffers.
* Empty buffers cause silence without advancing the
* cursor, so an underrun does not discard unheard
* audio. Consumed buffers are returned to the main
* thread producer for refill.
*/
static void sound_mix_stream_channel(
    int channel,
    channelstruct *channel_record,
    const samplestruct *sample,
    unsigned int todo
) {
    s_sound_stream *stream = &channel_record->stream;
    sound_sample_fixed_t buffer_position_fixed = stream->fp_buffer_position;
    int output_position;
    int left_volume = channel_record->volume[SOUND_SPATIAL_CHANNEL_LEFT];
    int right_volume = channel_record->volume[SOUND_SPATIAL_CHANNEL_RIGHT];

    for(output_position = 0; output_position + 1 < (int)todo; output_position += SOUND_SPATIAL_CHANNEL_MAX) {
        s_sound_stream_buffer *stream_buffer;
        sound_sample_fixed_t buffer_length_fixed;
        size_t frame_index;
        size_t left_sample_index;
        size_t right_sample_index;
        int left_sample_value;
        int right_sample_value;

        for(;;) {
            stream_buffer = &stream->buffer[stream->read_buffer];
            if(!stream_buffer->ready) {
                stream->fp_buffer_position = buffer_position_fixed;
                return;
            }

            buffer_length_fixed = SOUND_SAMPLE_INT_TO_FIX(stream_buffer->frame_count);
            if(buffer_position_fixed < buffer_length_fixed) {
                break;
            }

            buffer_position_fixed -= buffer_length_fixed;
            stream_buffer->ready = 0;
            stream->read_buffer = (stream->read_buffer + 1U) % SOUND_STREAM_BUFFER_COUNT;

            if(stream_buffer->terminal) {
                channel_record->fp_samplepos = SOUND_SAMPLE_INT_TO_FIX(sample->framecount);
                stream->fp_buffer_position = 0;
                sound_channel_pool_deactivate(&sound_channel_pool, channel);
                return;
            }
        }

        frame_index = (size_t)SOUND_SAMPLE_FIX_TO_INT(buffer_position_fixed);
        left_sample_index = frame_index * (size_t)sample->channels;
        right_sample_index = sample->channels == CHANNEL_TYPE_STEREO ? left_sample_index + 1U : left_sample_index;
        left_sample_value = sound_pcm_to_mix_value(stream_buffer->data, sample->bits, left_sample_index);
        right_sample_value = sound_pcm_to_mix_value(stream_buffer->data, sample->bits, right_sample_index);
        mixbuf[output_position] += left_sample_value * left_volume / MAX_SAMPLE_VOLUME;
        mixbuf[output_position + 1] += right_sample_value * right_volume / MAX_SAMPLE_VOLUME;

        channel_record->fp_samplepos =
            SOUND_SAMPLE_INT_TO_FIX(stream_buffer->source_start_frame) + buffer_position_fixed;
        buffer_position_fixed += channel_record->fp_period;
    }

    stream->fp_buffer_position = buffer_position_fixed;
}


// Input: number of input samples to mix
static void mixaudio(unsigned int todo)
{
    int bank_index;
    int channel;
    int output_position;
    int channel_index;
    int left_volume;
    int right_volume;
    int left_sample_value;
    int right_sample_value;

    if(musicchannel.active &&
       (musicchannel.channels != CHANNEL_TYPE_MONO && musicchannel.channels != CHANNEL_TYPE_STEREO))
    {
        musicchannel.active = 0;
    }

    if(musicchannel.active && !musicchannel.paused)
    {
        unsigned int music_position_fixed;
        unsigned int music_period_fixed;
        unsigned int music_playto_fixed;
        short *music_sample_data;

        music_sample_data = musicchannel.buf[musicchannel.playing_buffer];
        music_playto_fixed = musicchannel.fp_playto[musicchannel.playing_buffer];
        music_position_fixed = musicchannel.fp_samplepos;
        music_period_fixed = musicchannel.fp_period;
        left_volume = musicchannel.volume[SOUND_SPATIAL_CHANNEL_LEFT];
        right_volume = musicchannel.volume[SOUND_SPATIAL_CHANNEL_RIGHT];

        for(output_position = 0; output_position + 1 < (int)todo; output_position += SOUND_SPATIAL_CHANNEL_MAX)
        {
            size_t frame_index;
            size_t left_sample_index;
            size_t right_sample_index;

            if(music_position_fixed >= music_playto_fixed)
            {
                musicchannel.fp_playto[musicchannel.playing_buffer] = 0;
                musicchannel.playing_buffer++;
                musicchannel.playing_buffer %= MUSIC_NUM_BUFFERS;
                music_position_fixed = music_position_fixed - music_playto_fixed;

                if(music_position_fixed < musicchannel.fp_playto[musicchannel.playing_buffer])
                {
                    music_sample_data = musicchannel.buf[musicchannel.playing_buffer];
                    music_playto_fixed = musicchannel.fp_playto[musicchannel.playing_buffer];
                }
                else
                {
                    musicchannel.fp_playto[musicchannel.playing_buffer] = 0;
                    music_position_fixed = 0;
                    musicchannel.active = 0;
                    break;
                }
            }

            frame_index = (size_t)FIX_TO_INT(music_position_fixed);
            left_sample_index = frame_index * (size_t)musicchannel.channels;
            right_sample_index = musicchannel.channels == CHANNEL_TYPE_STEREO ? left_sample_index + 1U : left_sample_index;
            left_sample_value = music_sample_data[left_sample_index];
            right_sample_value = music_sample_data[right_sample_index];
            left_sample_value = (left_sample_value * left_volume / MAX_MUSIC_VOLUME);
            right_sample_value = (right_sample_value * right_volume / MAX_MUSIC_VOLUME);
            mixbuf[output_position] += left_sample_value;
            mixbuf[output_position + 1] += right_sample_value;
            music_position_fixed += music_period_fixed;
        }
        musicchannel.fp_samplepos = music_position_fixed;
    }

    /*
    * Caskey, Damon V.
    * 2026-07-31
    *
    * Walk only active, unpaused sound effect
    * channels instead of scanning pool capacity.
    */
    {
        uint64_t active_bank_mask = sound_channel_pool.active_bank_mask;

        while((bank_index = sound_channel_mask_first(active_bank_mask)) >= 0) {
            s_sound_channel_bank *bank = sound_channel_pool.bank[bank_index];
            uint64_t channel_mask = bank->active_mask & ~bank->paused_mask;

            while((channel_index = sound_channel_mask_first(channel_mask)) >= 0) {
                channelstruct *channel_record = &bank->channel[channel_index];
                s_soundcache *cache;
                samplestruct *sample;
                uint64_t sample_frame_count;
                sound_sample_fixed_t sample_length_fixed;
                sound_sample_fixed_t loop_start_fixed;
                sound_sample_fixed_t sample_position_fixed;
                sound_sample_fixed_t sample_period_fixed;
                int sample_index;

                channel = (bank_index * (int)SOUND_CHANNEL_BANK_SIZE) + channel_index;
                sample_index = channel_record->samplenum;
                if(sample_index < 0 || sample_index >= sound_cached) {
                    sound_channel_pool_deactivate(&sound_channel_pool, channel);
                    channel_mask &= ~(UINT64_C(1) << channel_index);
                    continue;
                }

                cache = &soundcache[sample_index];
                sample = &cache->sample;
                sample_frame_count = sample->framecount;
                if(sample_frame_count < 1 ||
                   sample_frame_count > SOUND_SAMPLE_FIXED_MAX_INTEGER ||
                   (sample->bits != 8 && sample->bits != 16 && sample->bits != 24) ||
                   (sample->channels != CHANNEL_TYPE_MONO && sample->channels != CHANNEL_TYPE_STEREO) ||
                   (!cache->stream && !sample->sampleptr) ||
                   (cache->stream && channel_record->stream.handle < 0)) {
                    sound_channel_pool_deactivate(&sound_channel_pool, channel);
                    channel_mask &= ~(UINT64_C(1) << channel_index);
                    continue;
                }

                if(cache->stream) {
                    sound_mix_stream_channel(channel, channel_record, sample, todo);
                    channel_mask &= ~(UINT64_C(1) << channel_index);
                    continue;
                }

                sample_length_fixed = SOUND_SAMPLE_INT_TO_FIX(sample_frame_count);
                loop_start_fixed = channel_record->active == CHANNEL_LOOPING ? channel_record->fp_loop_start : 0;
                sample_position_fixed = channel_record->fp_samplepos;
                sample_period_fixed = channel_record->fp_period;
                left_volume = channel_record->volume[SOUND_SPATIAL_CHANNEL_LEFT];
                right_volume = channel_record->volume[SOUND_SPATIAL_CHANNEL_RIGHT];

                for(output_position = 0; output_position + 1 < (int)todo; output_position += SOUND_SPATIAL_CHANNEL_MAX) {
                    size_t left_sample_index;
                    size_t right_sample_index;
                    size_t sample_position_index;
                    int sample_end_reached;

                    sample_position_index = (size_t)SOUND_SAMPLE_FIX_TO_INT(sample_position_fixed);
                    left_sample_index = sample_position_index * (size_t)sample->channels;
                    right_sample_index = sample->channels == CHANNEL_TYPE_STEREO ? left_sample_index + 1U : left_sample_index;

                    left_sample_value = sound_sample_to_mix_value(sample, left_sample_index);
                    right_sample_value = sound_sample_to_mix_value(sample, right_sample_index);
                    mixbuf[output_position] += left_sample_value * left_volume / MAX_SAMPLE_VOLUME;
                    mixbuf[output_position + 1] += right_sample_value * right_volume / MAX_SAMPLE_VOLUME;

                    sample_position_fixed = sound_sample_position_advance(sample_position_fixed, sample_period_fixed, sample_length_fixed, loop_start_fixed, &sample_end_reached);
                    if(sample_end_reached && channel_record->active != CHANNEL_LOOPING) {
                        sound_channel_pool_deactivate(&sound_channel_pool, channel);
                        break;
                    }
                }
                channel_record->fp_samplepos = sample_position_fixed;
                channel_mask &= ~(UINT64_C(1) << channel_index);
            }

            active_bank_mask &= ~(UINT64_C(1) << bank_index);
        }
    }
}


//////////////////////////////// ISR ///////////////////////////////////
// Called by Soundblaster ISR

void update_sample(unsigned char *buf, int size)
{
    int i, u, todo = size;
    if (playbits == 16)
    {
        todo >>= 1;
    }

    clearmixbuffer((unsigned int *)mixbuf, todo);
    mixaudio(todo);
    samplesplayed += (todo >> 1);

    if (playbits == 8)
    {
        unsigned char *dst = buf;
        for(i = 0; i < todo; i++)
        {
            u = mixbuf[i] >> (MIXSHIFT + 8);
            if (u < 0)
            {
                u = 0;
            }
            else if (u > 0xff)
            {
                u = 0xff;
            }
            dst[i] = u;
        }
    }
    else
    {
        unsigned short *dst = (unsigned short *)buf;
        for(i = 0; i < todo; i++)
        {
            u = mixbuf[i] >> MIXSHIFT;
            if (u < 0)
            {
                u = 0;
            }
            else if (u > 0xffff)
            {
                u = 0xffff;
            }
            u ^= 0x8000;
            dst[i] = u;
        }
    }
}

////////////////////////// Sound effects control /////////////////////////////
// Functions to start, stop, loop, etc.

/*
* Calculate sample playback period with the 
* same formula as the old mixer, but keep the 
* math in 64-bit fixed-point for long sample 
* support.
*/
static sound_sample_fixed_t sound_sample_period_calculate(unsigned int speed, int sample_frequency) {
    uint64_t sample_period;

    if(sample_frequency <= 0 || playfrequency <= 0) {
        return SOUND_SAMPLE_FIXED_ONE;
    }

    sample_period = SOUND_SAMPLE_FIXED_ONE;
    sample_period = sample_period * (uint64_t)speed / 100U;

    if(sample_period > UINT64_MAX / (uint64_t)sample_frequency) {
        return UINT64_MAX;
    }

    sample_period = sample_period * (uint64_t)sample_frequency / (uint64_t)playfrequency;
    if(sample_period == 0) {
        sample_period = 1;
    }

    return (sound_sample_fixed_t)sample_period;
}

#define SOUND_STREAM_UPDATE_BYTE_BUDGET (256U * 1024U)

static unsigned int sound_stream_update_cursor;

/*
* Caskey, Damon V.
* 2026-08-01
*
* Close a channel's stream handle on the main
* thread and retain its PCM buffers for reuse.
*/
static void sound_stream_close_channel(int channel, channelstruct *record) {
    if(!record) {
        return;
    }

    if(record->stream.handle >= 0) {
        closepackfile(record->stream.handle);
    }

    sound_stream_reset(&record->stream);
    sound_channel_pool_stream(&sound_channel_pool, channel, 0);
}

/*
* Caskey, Damon V.
* 2026-08-01
*
* Clear playback fields without releasing the
* stream buffers retained by the channel.
*/
static void sound_channel_record_clear(channelstruct *record) {
    s_sound_stream retained_stream;

    if(!record) {
        return;
    }

    retained_stream = record->stream;
    memset(record, 0, sizeof(*record));
    record->stream = retained_stream;
}

/*
* Caskey, Damon V.
* 2026-08-01
*
* Open and prefill a streamed sample before its
* channel becomes visible to the audio callback.
*/
static bool sound_stream_open_channel(
    int channel,
    channelstruct *record,
    const s_soundcache *cache,
    uint64_t start_frame,
    int looping,
    uint64_t loop_start_frame
) {
    s_sound_stream_read_context read_context;
    unsigned int buffer_index;
    int fill_result;
    int filled_buffers = 0;

    if(!record || !cache || !cache->stream) {
        return false;
    }

    if(!sound_stream_configure(
        &record->stream,
        (size_t)cache->sample.blockalign,
        cache->sample.framecount,
        start_frame,
        looping,
        loop_start_frame
    )) {
        return false;
    }

    record->stream.handle = openpackfile(cache->filename, cache->packfilename);
    if(record->stream.handle < 0) {
        sound_stream_reset(&record->stream);
        return false;
    }

    read_context.handle = record->stream.handle;
    read_context.data_offset = cache->sample.data_offset;
    read_context.block_align = (size_t)cache->sample.blockalign;

    for(buffer_index = 0; buffer_index < SOUND_STREAM_BUFFER_COUNT; buffer_index++) {
        fill_result = sound_stream_fill(
            &record->stream,
            sound_stream_read_frames,
            &read_context,
            NULL
        );

        if(fill_result < 0) {
            sound_stream_close_channel(channel, record);
            return false;
        }
        if(fill_result == 0) {
            break;
        }
        filled_buffers++;
    }

    if(filled_buffers == 0) {
        sound_stream_close_channel(channel, record);
        return false;
    }

    sound_channel_pool_stream(&sound_channel_pool, channel, 1);
    return true;
}

/*
* Caskey, Damon V.
* 2026-08-01
*
* Refill one buffer for a live streamed channel.
* Finished channels close here rather than inside
* the real-time audio callback.
*/
static size_t sound_stream_update_channel(int channel) {
    s_sound_stream_read_context read_context;
    channelstruct *record;
    s_soundcache *cache;
    size_t bytes_filled = 0;
    int fill_result;

    record = sound_channel_pool_get(&sound_channel_pool, channel);
    if(!record) {
        return 0;
    }

    if(!sound_channel_pool_is_active(&sound_channel_pool, channel)) {
        sound_stream_close_channel(channel, record);
        return 0;
    }

    if(record->samplenum < 0 ||
       record->samplenum >= sound_cached ||
       !soundcache[record->samplenum].stream) {
        sound_channel_pool_deactivate(&sound_channel_pool, channel);
        sound_stream_close_channel(channel, record);
        return 0;
    }

    cache = &soundcache[record->samplenum];
    read_context.handle = record->stream.handle;
    read_context.data_offset = cache->sample.data_offset;
    read_context.block_align = (size_t)cache->sample.blockalign;
    fill_result = sound_stream_fill(
        &record->stream,
        sound_stream_read_frames,
        &read_context,
        &bytes_filled
    );

    if(fill_result < 0) {
        sound_channel_pool_deactivate(&sound_channel_pool, channel);
        sound_stream_close_channel(channel, record);
        return 0;
    }

    return bytes_filled;
}

/*
* Caskey, Damon V.
* 2026-08-01
*
* Update a flattened range of streaming channels
* through bank masks. Each channel receives at most
* one buffer per pass for fair producer scheduling.
*/
static bool sound_stream_update_range(
    unsigned int first_channel,
    unsigned int end_channel,
    size_t *remaining_budget,
    unsigned int *last_channel
) {
    unsigned int first_bank;
    unsigned int end_bank;
    unsigned int bank_index;

    if(first_channel >= end_channel || end_channel > SOUND_CHANNEL_COUNT_MAX) {
        return true;
    }

    first_bank = first_channel / SOUND_CHANNEL_BANK_SIZE;
    end_bank = (end_channel - 1U) / SOUND_CHANNEL_BANK_SIZE;

    for(bank_index = first_bank; bank_index <= end_bank; bank_index++) {
        s_sound_channel_bank *bank;
        uint64_t channel_mask;
        int channel_index;

        if(!(sound_channel_pool.streaming_bank_mask & (UINT64_C(1) << bank_index))) {
            continue;
        }

        bank = sound_channel_pool.bank[bank_index];
        channel_mask = bank->streaming_mask;

        while((channel_index = sound_channel_mask_first(channel_mask)) >= 0) {
            unsigned int channel = bank_index * SOUND_CHANNEL_BANK_SIZE + (unsigned int)channel_index;
            size_t bytes_filled;

            channel_mask &= ~(UINT64_C(1) << channel_index);
            if(channel < first_channel || channel >= end_channel) {
                continue;
            }

            bytes_filled = sound_stream_update_channel((int)channel);
            *last_channel = channel;

            if(bytes_filled >= *remaining_budget) {
                *remaining_budget = 0;
                return false;
            }
            *remaining_budget -= bytes_filled;
        }
    }

    return true;
}

/*
* Caskey, Damon V.
* 2026-08-01
*
* Refill streamed effects round-robin under a
* bounded per-update byte budget.
*/
static void sound_update_streams(void) {
    size_t remaining_budget = SOUND_STREAM_UPDATE_BYTE_BUDGET;
    unsigned int start_channel;
    unsigned int last_channel;
    bool first_range_complete;

    if(!sound_channel_pool.streaming_bank_mask) {
        return;
    }

    start_channel = sound_stream_update_cursor;
    last_channel = start_channel;
    first_range_complete = sound_stream_update_range(
        start_channel,
        SOUND_CHANNEL_COUNT_MAX,
        &remaining_budget,
        &last_channel
    );

    if(first_range_complete && remaining_budget > 0 && start_channel > 0) {
        sound_stream_update_range(
            0,
            start_channel,
            &remaining_budget,
            &last_channel
        );
    }

    sound_stream_update_cursor = (last_channel + 1U) % SOUND_CHANNEL_COUNT_MAX;
}

/*
* Caskey, Damon V.
* 2026-07-31
*
* Find the active channel with the lowest current
* priority. Masks avoid scanning inactive slots.
*/
static int sound_find_lowest_priority_channel(unsigned int *lowest_priority) {
    uint64_t active_bank_mask;
    int bank_index;
    int channel = -1;

    if(!lowest_priority) {
        return -1;
    }

    *lowest_priority = UINT_MAX;
    active_bank_mask = sound_channel_pool.active_bank_mask;
    while((bank_index = sound_channel_mask_first(active_bank_mask)) >= 0) {
        s_sound_channel_bank *bank = sound_channel_pool.bank[bank_index];
        uint64_t channel_mask = bank->active_mask;
        int channel_index;

        while((channel_index = sound_channel_mask_first(channel_mask)) >= 0) {
            channelstruct *record = &bank->channel[channel_index];

            if(channel < 0 || record->priority < *lowest_priority) {
                channel = (bank_index * (int)SOUND_CHANNEL_BANK_SIZE) + channel_index;
                *lowest_priority = record->priority;
            }
            channel_mask &= ~(UINT64_C(1) << channel_index);
        }
        active_bank_mask &= ~(UINT64_C(1) << bank_index);
    }

    return channel;
}

/*
* Caskey, Damon V.
* 2026-08-01
*
* Play a resident or streamed sample on an available
* channel. Explicit start frames apply only to the
* initial pass; loop frames retain automatic restart
* behavior after the source end.
*/
static int sound_play_sample_internal(
    int samplenum,
    unsigned int priority,
    int lvolume,
    int rvolume,
    unsigned int speed,
    int looping,
    int start_frame_supplied,
    uint64_t start_frame,
    uint64_t loop_start_frame
) {
    channelstruct *record;
    samplestruct *sample;
    uint64_t initial_frame;
    unsigned int priority_low;
    int channel;

    if(!mixing_inited) {
        return -1;
    }
    if(samplenum < 0 || samplenum >= sound_cached) {
        return -1;
    }
    if(speed < 1) {
        speed = 100;
    }
    if(((soundcache[samplenum].stream && soundcache[samplenum].sample.framecount == 0) ||
        (!soundcache[samplenum].stream && !soundcache[samplenum].sample.sampleptr)) &&
       !sound_reload_sample(samplenum)) {
        return -1;
    }

    sample = &soundcache[samplenum].sample;

    channel = sound_channel_pool_acquire(&sound_channel_pool);
    if(channel < 0) {
        channel = sound_find_lowest_priority_channel(&priority_low);
        if(channel < 0 || priority_low > priority) {
            return -1;
        }
    }

    if(lvolume < 0) {
        lvolume = 0;
    }
    if(rvolume < 0) {
        rvolume = 0;
    }
    if(lvolume > MAX_SAMPLE_VOLUME) {
        lvolume = MAX_SAMPLE_VOLUME;
    }
    if(rvolume > MAX_SAMPLE_VOLUME) {
        rvolume = MAX_SAMPLE_VOLUME;
    }

    if(sample->framecount < 1 ||
       sample->framecount > SOUND_SAMPLE_FIXED_MAX_INTEGER ||
       (sample->bits != 8 && sample->bits != 16 && sample->bits != 24) ||
       (sample->channels != CHANNEL_TYPE_MONO &&
        sample->channels != CHANNEL_TYPE_STEREO) ||
       (start_frame_supplied && start_frame >= sample->framecount) ||
       (looping && loop_start_frame >= sample->framecount)) {
        return -1;
    }

    record = sound_channel_pool_get(&sound_channel_pool, channel);
    if(!record) {
        return -1;
    }

    sound_channel_pool_deactivate(&sound_channel_pool, channel);
    sound_stream_close_channel(channel, record);
    sound_channel_record_clear(record);
    record->samplenum = samplenum;

    /*
    * Preserve legacy phase staggering when no start
    * frame is supplied. An explicit start frame is
    * used exactly once when playback begins.
    */
    initial_frame = start_frame_supplied
        ? start_frame
        : (((uint64_t)channel * 4U) / (uint64_t)sample->channels) % sample->framecount;

    record->fp_samplepos = SOUND_SAMPLE_INT_TO_FIX(initial_frame);
    record->fp_period = sound_sample_period_calculate(speed, sample->frequency);
    record->fp_loop_start = SOUND_SAMPLE_INT_TO_FIX(loop_start_frame);
    record->volume[SOUND_SPATIAL_CHANNEL_LEFT] = lvolume;
    record->volume[SOUND_SPATIAL_CHANNEL_RIGHT] = rvolume;
    record->priority = priority;
    record->channels = sample->channels;
    record->playid = ++audio_global.sample_play_id;

    if(soundcache[samplenum].stream &&
       !sound_stream_open_channel(
           channel,
           record,
           &soundcache[samplenum],
           initial_frame,
           looping,
           loop_start_frame
       )) {
        sound_channel_record_clear(record);
        return -1;
    }

    sound_channel_pool_activate(&sound_channel_pool, channel, looping ? CHANNEL_LOOPING : CHANNEL_PLAYING);

    return channel;
}

int sound_play_sample(int samplenum, unsigned int priority, int lvolume, int rvolume, unsigned int speed) {
    return sound_play_sample_internal(samplenum, priority, lvolume, rvolume, speed, 0, 0, 0, 0);
}

int sound_play_sample_offset(int samplenum, unsigned int priority, int lvolume, int rvolume, unsigned int speed, uint64_t start_frame) {
    return sound_play_sample_internal(samplenum, priority, lvolume, rvolume, speed, 0, 1, start_frame, 0);
}

int sound_loop_sample(int samplenum, unsigned int priority, int lvolume, int rvolume, unsigned int speed) {
    return sound_play_sample_internal(samplenum, priority, lvolume, rvolume, speed, 1, 0, 0, 0);
}

int sound_loop_sample_offset(int samplenum, unsigned int priority, int lvolume, int rvolume, unsigned int speed, uint64_t start_frame, uint64_t loop_start_frame) {
    return sound_play_sample_internal(samplenum, priority, lvolume, rvolume, speed, 1, 1, start_frame, loop_start_frame);
}

int sound_query_channel(int playid) {
    uint64_t active_bank_mask = sound_channel_pool.active_bank_mask;
    int bank_index;

    while((bank_index = sound_channel_mask_first(active_bank_mask)) >= 0) {
        s_sound_channel_bank *bank = sound_channel_pool.bank[bank_index];
        uint64_t channel_mask = bank->active_mask;
        int channel_index;

        while((channel_index = sound_channel_mask_first(channel_mask)) >= 0) {
            if(bank->channel[channel_index].playid == playid) {
                return (bank_index * (int)SOUND_CHANNEL_BANK_SIZE) + channel_index;
            }
            channel_mask &= ~(UINT64_C(1) << channel_index);
        }
        active_bank_mask &= ~(UINT64_C(1) << bank_index);
    }

    return -1;
}

int sound_id(int channel) {
    channelstruct *record;

    if(!sound_channel_pool_is_active(&sound_channel_pool, channel)) {
        return -1;
    }

    record = sound_channel_pool_get(&sound_channel_pool, channel);
    return record ? record->playid : -1;
}

int sound_is_active(int channel) {
    return sound_channel_pool_is_active(&sound_channel_pool, channel) ? 1 : 0;
}

void sound_stop_sample(int channel) {
    channelstruct *record = sound_channel_pool_get(&sound_channel_pool, channel);

    sound_channel_pool_deactivate(&sound_channel_pool, channel);
    if(record) {
        sound_stream_close_channel(channel, record);
    }
}

void sound_stopall_sample() {
    uint64_t streaming_bank_mask = sound_channel_pool.streaming_bank_mask;
    int bank_index;

    while((bank_index = sound_channel_mask_first(streaming_bank_mask)) >= 0) {
        s_sound_channel_bank *bank = sound_channel_pool.bank[bank_index];
        uint64_t channel_mask = bank->streaming_mask;
        int channel_index;

        while((channel_index = sound_channel_mask_first(channel_mask)) >= 0) {
            int channel = bank_index * (int)SOUND_CHANNEL_BANK_SIZE + channel_index;
            sound_stream_close_channel(channel, &bank->channel[channel_index]);
            channel_mask &= ~(UINT64_C(1) << channel_index);
        }

        streaming_bank_mask &= ~(UINT64_C(1) << bank_index);
    }

    sound_channel_pool_stop_all(&sound_channel_pool);
}

void sound_pause_sample(int toggle) {
    sound_channel_pool_pause_all(&sound_channel_pool, toggle);
}

void sound_pause_single_sample(int toggle, int channel) {
    sound_channel_pool_pause(&sound_channel_pool, channel, toggle);
}

void sound_volume_sample(int channel, int lvolume, int rvolume) {
    channelstruct *record = sound_channel_pool_get(&sound_channel_pool, channel);

    if(!record) {
        return;
    }
    if(lvolume < 0) {
        lvolume = 0;
    }
    if(rvolume < 0) {
        rvolume = 0;
    }
    if(lvolume > MAX_SAMPLE_VOLUME) {
        lvolume = MAX_SAMPLE_VOLUME;
    }
    if(rvolume > MAX_SAMPLE_VOLUME) {
        rvolume = MAX_SAMPLE_VOLUME;
    }
    record->volume[SOUND_SPATIAL_CHANNEL_LEFT] = lvolume;
    record->volume[SOUND_SPATIAL_CHANNEL_RIGHT] = rvolume;
}

int sound_getpos_sample(int channel) {
    channelstruct *record = sound_channel_pool_get(&sound_channel_pool, channel);

    if(!record) {
        return 0;
    }
    {
        uint64_t sample_position = SOUND_SAMPLE_FIX_TO_INT(record->fp_samplepos);
        return sample_position > (uint64_t)INT_MAX ? INT_MAX : (int)sample_position;
    }
}

//////////////////////////////// ADPCM music ////////////////////////////////

static int adpcm_handle = -1;
static unsigned char *adpcm_inbuf;
static int music_looping = 0;
static int music_atend = 0;
#define	BOR_MUSIC_VERSION_MONO   0x00010000
#define	BOR_MUSIC_VERSION_STEREO 0x00010001
#define	BOR_IDENTIFIER "BOR music"

#pragma pack (1)

typedef struct
{
    char	identifier[16];
    char	artist[64];
    char	title[64];
    unsigned int	version;
    int		frequency;
    int		channels;
    int		datastart;
} bor_header;

static bor_header borhead;
static short loop_valprev[SOUND_SPATIAL_CHANNEL_MAX];
static char loop_index[SOUND_SPATIAL_CHANNEL_MAX];
static int loop_state_set;
static u32 loop_offset;

void sound_close_adpcm()
{

    int i;

    // Prevent any further access by the ISR
    musicchannel.active = 0;
    for(i = 0; i < MUSIC_NUM_BUFFERS; i++)
    {
        musicchannel.fp_playto[i] = 0;
    }

    // Close file...
    if(adpcm_handle >= 0)
    {
        closepackfile(adpcm_handle);
    }
    adpcm_handle = -1;

    if(adpcm_inbuf != NULL)
    {
        free(adpcm_inbuf);
        adpcm_inbuf = NULL;
    }

    for(i = 0; i < MUSIC_NUM_BUFFERS; i++)
    {
        if(musicchannel.buf[i] != NULL)
        {
            free(musicchannel.buf[i]);
            musicchannel.buf[i] = NULL;
        }
    }

    

    sound_music_channel_clear(&musicchannel);

    memset(&borhead, 0, sizeof(bor_header));

    adpcm_reset();
    loop_valprev[SOUND_SPATIAL_CHANNEL_LEFT] = loop_valprev[SOUND_SPATIAL_CHANNEL_RIGHT] = 0;
    loop_index[SOUND_SPATIAL_CHANNEL_LEFT] = loop_index[SOUND_SPATIAL_CHANNEL_RIGHT] = 0;
    loop_state_set = 0;
}

int sound_open_adpcm(char *filename, char *packname, int volume, int loop, u32 music_offset)
{

    int i;

    if(!mixing_inited)
    {
        return 0;
    }
    if(!mixing_active)
    {
        return 0;
    }

    sound_close_music();

    // Open file, etcetera
    adpcm_handle = openpackfile(filename, packname);
    if(adpcm_handle < 0)
    {
        return 0;
    }

    // Read header
    if(readpackfile(adpcm_handle, &borhead, sizeof(bor_header)) != sizeof(bor_header))
    {
        goto error_exit;
    }

    borhead.version = SwapLSB32(borhead.version);
    borhead.frequency = SwapLSB32(borhead.frequency);
    borhead.channels = SwapLSB32(borhead.channels);
    borhead.datastart = SwapLSB32(borhead.datastart);

    // Is it really a BOR music file?
    if(strncmp(borhead.identifier, BOR_IDENTIFIER, 16) != 0)
    {
        goto error_exit;
    }

    // Can I play it?
    if((borhead.version != BOR_MUSIC_VERSION_MONO && borhead.version != BOR_MUSIC_VERSION_STEREO) ||
            (borhead.channels != 1 && borhead.channels != 2) ||
            borhead.frequency < SOUND_MUSIC_FREQUENCY_MIN ||
            borhead.frequency > SOUND_MUSIC_FREQUENCY_MAX)
    {
        goto error_exit;
    }
    // Seek to beginning of data
    if(seekpackfile(adpcm_handle, borhead.datastart, SEEK_SET) != borhead.datastart)
    {
        goto error_exit;
    }

    sound_music_channel_clear(&musicchannel);

    musicchannel.fp_period = sound_music_period_calculate((int)borhead.frequency, 100);
    musicchannel.volume[SOUND_SPATIAL_CHANNEL_LEFT] = volume;
    musicchannel.volume[SOUND_SPATIAL_CHANNEL_RIGHT] = volume;
    musicchannel.channels = borhead.channels;
    music_looping = loop;
    music_atend = 0;

    adpcm_inbuf = malloc(MUSIC_BUF_SIZE / 2);
    if(adpcm_inbuf == NULL)
    {
        goto error_exit;
    }

    for(i = 0; i < MUSIC_NUM_BUFFERS; i++)
    {
        musicchannel.buf[i] = malloc(MUSIC_BUF_SIZE * sizeof(short));
        if(musicchannel.buf[i] == NULL)
        {
            goto error_exit;
        }
        memset(musicchannel.buf[i], 0, MUSIC_BUF_SIZE * sizeof(short));
    }

    loop_offset = music_offset;
    music_type = SOUND_FILE_TYPE_ADPCM;

    return 1;
error_exit:
    sound_close_music();
    closepackfile(adpcm_handle);
    return 0;
}

void sound_update_adpcm()
{

    int samples, readsamples, samples_to_read;
    short *outptr;
    int i, j;

    if((adpcm_handle < 0) || (music_type != SOUND_FILE_TYPE_ADPCM))
    {
        return;
    }
    if(!mixing_inited || !mixing_active)
    {
        sound_close_music();
        return;
    }
    if(musicchannel.paused)
    {
        return;
    }


    // Just to be sure: check if all goes well...
    for(i = 0; i < MUSIC_NUM_BUFFERS; i++)
    {
        if(musicchannel.fp_playto[i] > INT_TO_FIX(MUSIC_BUF_SIZE / musicchannel.channels))
        {
            musicchannel.fp_playto[i] = 0;
            return;
        }
    }


    // Need to update?
    for(j = 0, i = musicchannel.playing_buffer + 1; j < MUSIC_NUM_BUFFERS; j++, i++)
    {
        i %= MUSIC_NUM_BUFFERS;

        if(musicchannel.fp_playto[i] == 0)
        {
            // Buffer needs to be filled

            samples = 0;
            outptr = musicchannel.buf[i];

            if(!music_looping)
            {
                if(music_atend)
                {
                    // Close file when done playing all buffers
                    if(!musicchannel.active)
                    {
                        sound_close_music();
                        return;
                    }
                }
                else
                {
                    readsamples = readpackfile(adpcm_handle, adpcm_inbuf, MUSIC_BUF_SIZE / 2) * 2;
                    if(readsamples <= 0)
                    {
                        // EOF
                        music_atend = 1;
                        return;
                    }
                    // Play this bit
                    adpcm_decode(adpcm_inbuf, outptr, readsamples / 2, musicchannel.channels);
                    samples = readsamples;
                }
            }
            else while(samples < MUSIC_BUF_SIZE)
                {
                    samples_to_read = MUSIC_BUF_SIZE - samples;
                    if(!loop_state_set && seekpackfile(adpcm_handle, 0, SEEK_CUR) <= (borhead.datastart + loop_offset) && seekpackfile(adpcm_handle, 0, SEEK_CUR) > (borhead.datastart + loop_offset - samples_to_read / 2))
                    {
                        readsamples = readpackfile(adpcm_handle, adpcm_inbuf, borhead.datastart + loop_offset - seekpackfile(adpcm_handle, 0, SEEK_CUR)) * 2;
                        adpcm_decode(adpcm_inbuf, outptr, readsamples / 2, musicchannel.channels);
                        loop_valprev[SOUND_SPATIAL_CHANNEL_LEFT] = adpcm_valprev(0);
                        loop_index[SOUND_SPATIAL_CHANNEL_LEFT] = adpcm_index(0);
                        if(musicchannel.channels == CHANNEL_TYPE_STEREO)
                        {
                            loop_valprev[SOUND_SPATIAL_CHANNEL_RIGHT] = adpcm_valprev(1);
                            loop_index[SOUND_SPATIAL_CHANNEL_RIGHT] = adpcm_index(1);
                        }
                        loop_state_set = 1;
                        outptr += readsamples;
                        samples += readsamples;
                    }
                    else
                    {
                        readsamples = readpackfile(adpcm_handle, adpcm_inbuf, samples_to_read / 2) * 2;
                        if(readsamples < 0)
                        {
                            // Error
                            sound_close_music();
                            return;
                        }
                        if(readsamples)
                        {
                            adpcm_decode(adpcm_inbuf, outptr, readsamples / 2, musicchannel.channels);
                            outptr += readsamples;
                            samples += readsamples;
                        }
                        if(readsamples < samples_to_read)
                        {
                            // At start of data already?
                            if(seekpackfile(adpcm_handle, 0, SEEK_CUR) == borhead.datastart)
                            {
                                // Must be some error
                                sound_close_music();
                                return;
                            }
                            // Seek to beginning of data
                            if(seekpackfile(adpcm_handle, borhead.datastart + loop_offset, SEEK_SET) != borhead.datastart + loop_offset)
                            {
                                sound_close_music();
                                return;
                            }
                            // Reset decoder
                            adpcm_loop_reset(SOUND_SPATIAL_CHANNEL_LEFT, loop_valprev[SOUND_SPATIAL_CHANNEL_LEFT], loop_index[SOUND_SPATIAL_CHANNEL_LEFT]);
                            if(musicchannel.channels == CHANNEL_TYPE_STEREO)
                            {
                                adpcm_loop_reset(SOUND_SPATIAL_CHANNEL_RIGHT, loop_valprev[SOUND_SPATIAL_CHANNEL_RIGHT], loop_index[SOUND_SPATIAL_CHANNEL_RIGHT]);
                            }
                        }
                    }
                }
            // Activate
            musicchannel.fp_playto[i] = INT_TO_FIX(samples / musicchannel.channels);
            if(!musicchannel.active)
            {
                musicchannel.playing_buffer = i;
                musicchannel.active = 1;
            }
        }
    }
}

void sound_adpcm_tempo(int music_tempo)
{
    musicchannel.fp_period = sound_music_period_calculate((int)borhead.frequency, music_tempo);
}

int sound_query_adpcm(char *artist, char *title)
{
    if(adpcm_handle < 0)
    {
        return 0;
    }
    if(artist)
    {
        strcpy(artist, borhead.artist);
    }
    if(title)
    {
        strcpy(title, borhead.title);
    }
    return 1;
}

/////////////////////////// Ogg Vorbis decoding ///////////////////////////////
// Plombo's Ogg Vorbis decoder for OpenBOR. Uses libvorbisfile or libvorbisidec.

#if TREMOR
#define ov_decode(vf,buffer,length,bitstream) ov_read(vf,buffer,length,bitstream)
#else
#define ov_decode(vf,buffer,length,bitstream) ov_read(vf,buffer,length,0,2,1,bitstream)
#endif

OggVorbis_File *oggfile;
vorbis_info *stream_info;
int current_section, ogg_handle;

// I/O functions used by libvorbisfile
size_t readpackfile_callback(void *buf, size_t len, size_t nmembers, int *handle)
{
    return readpackfile(*handle, buf, (int)(len * nmembers));
}
int closepackfile_callback(void *ptr)
{
#ifdef VERBOSE
    printf ("closepack cb %d\n", *(int *)ptr);
#endif

    return closepackfile(*(int *)ptr);
}
int seekpackfile_callback(int *handle, ogg_int64_t offset, int whence)
{
    return seekpackfile(*handle, (int)offset, whence);
}
int tellpackfile_callback(int *handle)
{
    return seekpackfile(*handle, 0, SEEK_CUR);
}

void sound_close_ogg()
{
    int i;

    ov_clear(oggfile);
    free(oggfile);
    oggfile = NULL;
    music_type = SOUND_FILE_TYPE_NONE;

    for(i = 0; i < MUSIC_NUM_BUFFERS; i++)
    {
        if(musicchannel.buf[i] != NULL)
        {
            free(musicchannel.buf[i]);
            musicchannel.buf[i] = NULL;
        }
    }

    sound_music_channel_clear(&musicchannel);
}

int sound_open_ogg(char *filename, char *packname, int volume, int loop, u32 music_offset)
{

    int i;

    static ov_callbacks ogg_callbacks =
    {
        (size_t ( *)(void *, size_t, size_t, void *))  readpackfile_callback,
        (int ( *)(void *, ogg_int64_t, int))           seekpackfile_callback,
        (int ( *)(void *))                             closepackfile_callback,
        (long ( *)(void *))                            tellpackfile_callback
    };

    if(!mixing_inited)
    {
        return 0;
    }
    if(!mixing_active)
    {
        return 0;
    }

    sound_close_music();
#ifdef VERBOSE
    printf("trying to open OGG file %s from %s, vol %d, loop %d, ofs %u\n", filename, packname, volume, loop, music_offset);
#endif
    // Open file, etcetera
    ogg_handle = openpackfile(filename, packname);
#ifdef VERBOSE
    printf ("ogg handle %d\n", ogg_handle);
#endif
    if(ogg_handle < 0)
    {
#ifdef VERBOSE
        printf("couldn't get handle\n");
#endif
        return 0;
    }
    oggfile = malloc(sizeof(OggVorbis_File));
    if (ov_open_callbacks(&ogg_handle, oggfile, NULL, 0, ogg_callbacks) != 0)
    {
#ifdef VERBOSE
        printf("ov_open_callbacks failed\n");
#endif
        goto error_exit;
    }
    // Can I play it?
    stream_info = ov_info(oggfile, -1);
    if((stream_info->channels != 1 && stream_info->channels != 2) ||
            stream_info->rate < SOUND_MUSIC_FREQUENCY_MIN ||
            stream_info->rate > SOUND_MUSIC_FREQUENCY_MAX)
    {
        sound_close_ogg();
#ifdef VERBOSE
        printf("NOT can i play it\n");
#endif

        goto error_exit;
    }

    sound_music_channel_clear(&musicchannel);

    musicchannel.fp_period = sound_music_period_calculate((int)stream_info->rate, 100);
    musicchannel.volume[SOUND_SPATIAL_CHANNEL_LEFT] = volume;
    musicchannel.volume[SOUND_SPATIAL_CHANNEL_RIGHT] = volume;
    musicchannel.channels = stream_info->channels;
    music_looping = loop;
    music_atend = 0;

    for(i = 0; i < MUSIC_NUM_BUFFERS; i++)
    {
        musicchannel.buf[i] = malloc(MUSIC_BUF_SIZE * sizeof(short));
        if(musicchannel.buf[i] == NULL)
        {
            sound_close_ogg();
#ifdef VERBOSE
            printf("buf is null\n");
#endif
            goto error_exit;
        }
        memset(musicchannel.buf[i], 0, MUSIC_BUF_SIZE * sizeof(short));
        musicchannel.object_type = OBJECT_TYPE_MUSIC_CHANNEL;
    }

    loop_offset = music_offset;
    music_type = SOUND_FILE_TYPE_VORBIS;

#ifdef VERBOSE
    printf("ogg is opened\n");
#endif
    return 1;

error_exit:
    closepackfile(ogg_handle);
    return 0;

}

void sound_update_ogg()
{

    int samples, readsamples, samples_to_read;
    short *outptr;
    int i, j;

    if(!mixing_inited || !mixing_active)
    {
        sound_close_music();
        return;
    }
    if(musicchannel.paused)
    {
        return;
    }

    // Just to be sure: check if all goes well...
    for(i = 0; i < MUSIC_NUM_BUFFERS; i++)
    {
        if(musicchannel.fp_playto[i] > INT_TO_FIX(MUSIC_BUF_SIZE / musicchannel.channels))
        {
            musicchannel.fp_playto[i] = 0;
            return;
        }
    }


    // Need to update?
    for(j = 0, i = musicchannel.playing_buffer + 1; j < MUSIC_NUM_BUFFERS; j++, i++)
    {
        i %= MUSIC_NUM_BUFFERS;

        if(musicchannel.fp_playto[i] == 0)
        {
            // Buffer needs to be filled

            samples = 0;
            outptr = musicchannel.buf[i];

            if(!music_looping)
            {
                if(music_atend)
                {
                    // Close file when done playing all buffers
                    if(!musicchannel.active)
                    {
                        sound_close_music();
                        return;
                    }
                }
                else while(samples < MUSIC_BUF_SIZE)
                    {
                        readsamples = ov_decode(oggfile, (char *)outptr, 2 * (MUSIC_BUF_SIZE - samples), &current_section) / 2;
                        if (readsamples == 0)
                        {
                            music_atend = 1;
                            return;
                        }
                        else if (readsamples < 0)
                        {
                            sound_close_music();
                            return;
                        }
                        outptr += readsamples;
                        samples += readsamples;
                    }
            }
            else while(samples < MUSIC_BUF_SIZE)
                {
                    samples_to_read = MUSIC_BUF_SIZE - samples;
                    readsamples = ov_decode(oggfile, (char *)outptr, 2 * samples_to_read, &current_section) / 2;
                    if(readsamples < 0)
                    {
                        // Error
                        sound_close_music();
                        return;
                    }
                    else if(readsamples > 0)
                    {
                        outptr += readsamples;
                        samples += readsamples;
                    }
                    else if(readsamples < samples_to_read)
                    {
                        // At start of data already?
                        if(ov_pcm_tell(oggfile) == 0)
                        {
                            // Must be some error
                            sound_close_music();
                            return;
                        }
                        // Seek to beginning of data
                        if(ov_pcm_seek(oggfile, loop_offset) != 0)
                        {
                            sound_close_music();
                            return;
                        }
                    }
                }
            // Activate
            musicchannel.fp_playto[i] = INT_TO_FIX(samples / musicchannel.channels);
            if(!musicchannel.active)
            {
                musicchannel.playing_buffer = i;
                musicchannel.active = 1;
            }
        }
    }
}

void sound_ogg_tempo(int music_tempo)
{
    musicchannel.fp_period = sound_music_period_calculate((int)stream_info->rate, music_tempo);
}

int sound_query_ogg(char *artist, char *title)
{
    int i;
    char *current;
    vorbis_comment *comment = ov_comment(oggfile, -1);

    if (!artist || !title)
    {
        return 1;
    }

    for(i = 0; i < comment->comments; i++)
    {
        current = comment->user_comments[i];
        if (strncmp("ARTIST=", current, 7) == 0)
        {
            strcpy(artist, current + 7);
        }
        else if (strncmp("TITLE=", current, 6) == 0)
        {
            strcpy(title, current + 6);
        }
    }

    return 1;
}

/////////////////////////////// INIT / EXIT //////////////////////////////////

int sound_open_music(char *filename, char *packname, int volume, int loop, u32 music_offset)
{
    static char fnam[128];
#ifdef VERBOSE
    printf("trying to open music file %s from %s, vol %d, loop %d, ofs %u\n", filename, packname, volume, loop, music_offset);
#endif
    // try opening filename exactly as specified
    if(sound_open_adpcm(filename, packname, volume, loop, music_offset))
    {
        return 1;
    }
    if(sound_open_ogg(filename, packname, volume, loop, music_offset))
    {
        return 1;
    }

    // handle adding an extension to the filename
    sprintf(fnam, "%s.bor", filename);
    if(sound_open_adpcm(fnam, packname, volume, loop, music_offset))
    {
        return 1;
    }
    sprintf(fnam, "%s.ogg", filename);
    if(sound_open_ogg(fnam, packname, volume, loop, music_offset))
    {
        return 1;
    }
    sprintf(fnam, "%s.oga", filename);
    if(sound_open_ogg(fnam, packname, volume, loop, music_offset))
    {
        return 1;
    }

    return 0;
}

void sound_close_music()
{
    switch(music_type)
    {
    case SOUND_FILE_TYPE_ADPCM:
        sound_close_adpcm();
        break;
    case SOUND_FILE_TYPE_VORBIS:
        sound_close_ogg();
        break;
    case SOUND_FILE_TYPE_NONE:
        return;
    }
    music_type = SOUND_FILE_TYPE_NONE;
}

void sound_update_music()
{
    sound_update_streams();

    switch(music_type)
    {
    case SOUND_FILE_TYPE_ADPCM:
        sound_update_adpcm();
        break;
    case SOUND_FILE_TYPE_VORBIS:
        sound_update_ogg();
        break;
    case SOUND_FILE_TYPE_NONE:
        return;
    }
}

int sound_query_music(char *artist, char *title)
{
    switch(music_type)
    {
    case SOUND_FILE_TYPE_ADPCM:
        return sound_query_adpcm(artist, title);
    case SOUND_FILE_TYPE_VORBIS:
        return sound_query_ogg(artist, title);
    case SOUND_FILE_TYPE_NONE:
        return 0;
    }

    return 0;
}

void sound_music_tempo(int music_tempo)
{

    switch(music_type)
    {
    case SOUND_FILE_TYPE_ADPCM:
        sound_adpcm_tempo(music_tempo);
        break;
    case SOUND_FILE_TYPE_VORBIS:        
        sound_ogg_tempo(music_tempo);
        break;
    case SOUND_FILE_TYPE_NONE:
        return;
    }
}

void sound_volume_music(int left, int right)
{
    if(left < 0)
    {
        left = 0;
    }
    if(right < 0)
    {
        right = 0;
    }
    if(left > MAX_SAMPLE_VOLUME * 8)
    {
        left = MAX_SAMPLE_VOLUME * 8;
    }
    if(right > MAX_SAMPLE_VOLUME * 8)
    {
        right = MAX_SAMPLE_VOLUME * 8;
    }
    musicchannel.volume[SOUND_SPATIAL_CHANNEL_LEFT] = left;
    musicchannel.volume[SOUND_SPATIAL_CHANNEL_RIGHT] = right;
}

void sound_pause_music(int toggle)
{
    musicchannel.paused = toggle;
}

void sound_stop_playback() {
    if(!mixing_inited) {
        return;
    }
    if(!mixing_active) {
        return;
    }
    sound_close_music();
    sound_stopall_sample();
    SB_playstop();
    mixing_active = 0;
}

int sound_start_playback() {
    if(!mixing_inited) {
        return 0;
    }

    sound_stop_playback();

    playbits = SOUND_OUTPUT_BITS_DEFAULT;
    playfrequency = SOUND_OUTPUT_FREQUENCY_DEFAULT;

    sound_stopall_sample();
    SB_playstop();
    if(!SB_playstart(playbits, playfrequency)) {
        return 0;
    }

    mixing_active = 1;
    samplesplayed = 0;
    return 1;
}

// Stop everything and free used memory
void sound_exit() {
    sound_stop_playback();
    sound_stopall_sample();
    sound_unload_all_samples();

    if(mixbuf != NULL) {
        free(mixbuf);
        mixbuf = NULL;
    }

    sound_channel_pool_destroy(&sound_channel_pool);
    sound_stream_update_cursor = 0;
    mixing_inited = 0;
}

/*
* Caskey, Damon V.
* 2026-07-31
*
* Initialize sound with one 64-channel bank.
* Additional banks are allocated on demand.
*/
int sound_init(void) {
    sound_exit();

    if(!sound_channel_pool_init(&sound_channel_pool)) {
        return 0;
    }

    /* Allocate the maximum amount ever needed for one mixing pass. */
    if((mixbuf = malloc(MIXBUF_SIZE)) == NULL) {
        sound_channel_pool_destroy(&sound_channel_pool);
        return 0;
    }

    mixing_active = 0;
    mixing_inited = 1;
    List_Init(&audio_global.samplelist);

    return 1;
}

// Returns time passed in milliseconds (since last call or start of playback),
// or 0xFFFFFFFF if not available. This function is useful when synchronizing
// stuff to sound.
u32 sound_getinterval()
{
    u32 msecs;

    if(!mixing_active)
    {
        return 0xFFFFFFFF;
    }

    msecs = 1000 * samplesplayed / playfrequency;
    samplesplayed -= msecs * playfrequency / 1000;

    return msecs;
}

int maxchannels() {
    return (int)SOUND_CHANNEL_COUNT_MAX;
}
