/*
 * OpenBOR - http://www.chronocrash.com
 * -----------------------------------------------------------------------
 * All rights reserved, see LICENSE in OpenBOR root for details.
 *
 * Copyright (c)  OpenBOR Team
 */

/*
**	Sound mixer.
**	High quality, with support for WAV and Vorbis-compressed audio.
**
**	Also plays WAV files (8-bit, 16-bit, and 24-bit) and Ogg Vorbis.
**	Note: 8-bit WAVs are unsigned; all other supported PCM is signed.
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
#include "sblaster.h"
#include "borendian.h"
#include "packfile.h"
#include "rand32.h"

#if TREMOR
#define sound_vorbis_decode(vf, buffer, length, bitstream) ov_read(vf, buffer, length, bitstream)
#else
#define sound_vorbis_decode(vf, buffer, length, bitstream) ov_read(vf, buffer, length, 0, 2, 1, bitstream)
#endif

#define		MIXSHIFT		     3	    // 2 should be OK
#define     SOUND_PCM_24_MIN     (-8388607 - 1)
#define     SOUND_PCM_24_MAX     8388607
#define     SOUND_PCM_8_TO_24    65536
#define     SOUND_PCM_16_TO_24   256
#define     SOUND_PCM_24_TO_S32  256
#define     SOUND_OUTPUT_BITS_FALLBACK        16
#define     SOUND_OUTPUT_FREQUENCY_FALLBACK   44100

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
#define     MIXBUF_SAMPLE_COUNT  (MIXBUF_SIZE / sizeof(s32))
#define		PREMIX_SIZE		     1024
#define		MIX_BLOCK_SIZE		 32

#pragma pack(4)

s_sound_parameters sound_parameters = {
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
static s64 *mixbuf = NULL;
static int playbits;
int playfrequency;

// Indicates whether the hardware is playing, and if mixing is active
static int mixing_active = 0;

// Indicates whether the sound system is initialized
static int mixing_inited = 0;

// Counts the total number of samples played
static u32 samplesplayed;

//////////////////////////////// WAVE LOADER //////////////////////////////////

#pragma pack(push, 1)

#define		HEX_RIFF	0x46464952
#define		HEX_WAVE	0x45564157
#define		HEX_fmt		0x20746D66
#define		HEX_data	0x61746164
#define		FMT_PCM		0x0001
#define		FMT_EXTENSIBLE	0xFFFE

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
    sample->file_type = SOUND_SAMPLE_FILE_TYPE_WAVE;

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

/*
* Caskey, Damon V.
* 2026-08-01
*
* Adapt dynamically allocated packfile handles to
* the libvorbisfile callback contract. Decoder seeks
* retain 64-bit packfile positions.
*/
static size_t sound_vorbis_pack_read(void *destination, size_t item_size, size_t item_count, void *source) {
    int *handle = source;
    size_t requested_items;
    size_t requested_bytes;
    int bytes_read;

    if(!handle || *handle < 0 || item_size == 0 || item_count == 0) {
        return 0;
    }

    requested_items = item_count;
    if(requested_items > (size_t)INT_MAX / item_size) {
        requested_items = (size_t)INT_MAX / item_size;
    }
    if(requested_items == 0) {
        return 0;
    }

    requested_bytes = item_size * requested_items;
    bytes_read = readpackfile(*handle, destination, (int)requested_bytes);
    if(bytes_read <= 0) {
        return 0;
    }

    return (size_t)bytes_read / item_size;
}

static int sound_vorbis_pack_seek(void *source, ogg_int64_t offset, int origin) {
    int *handle = source;
    packfile_signed_offset_t position;

    if(!handle || *handle < 0 || offset < INT64_MIN || offset > INT64_MAX) {
        return -1;
    }

    position = seekpackfile64(*handle, (packfile_signed_offset_t)offset, origin);
    return position < 0 ? -1 : 0;
}

static int sound_vorbis_pack_close(void *source) {
    int *handle = source;
    int result;

    if(!handle || *handle < 0) {
        return 0;
    }

    result = closepackfile(*handle);
    *handle = -1;
    return result;
}

static long sound_vorbis_pack_tell(void *source) {
    int *handle = source;
    packfile_signed_offset_t position;

    if(!handle || *handle < 0) {
        return -1;
    }

    position = seekpackfile64(*handle, 0, SEEK_CUR);
    if(position < 0 || (uint64_t)position > (uint64_t)LONG_MAX) {
        return -1;
    }

    return (long)position;
}

static ov_callbacks sound_vorbis_callbacks = {
    sound_vorbis_pack_read,
    sound_vorbis_pack_seek,
    sound_vorbis_pack_close,
    sound_vorbis_pack_tell
};

/*
* Caskey, Damon V.
* 2026-08-01
*
* Open one independent Vorbis decoder. The decoder
* owns the dynamic packfile handle after a successful
* ov_open_callbacks() call.
*/
static bool sound_vorbis_open_decoder(
    const char *filename,
    const char *packname,
    int *handle,
    OggVorbis_File **decoder
) {
    OggVorbis_File *new_decoder;

    if(!filename || !packname || !handle || !decoder) {
        return false;
    }

    *handle = openpackfile(filename, packname);
    *decoder = NULL;
    if(*handle < 0) {
        return false;
    }

    new_decoder = malloc(sizeof(*new_decoder));
    if(!new_decoder) {
        closepackfile(*handle);
        *handle = -1;
        return false;
    }

    if(ov_open_callbacks(handle, new_decoder, NULL, 0, sound_vorbis_callbacks) != 0) {
        free(new_decoder);
        closepackfile(*handle);
        *handle = -1;
        return false;
    }

    *decoder = new_decoder;
    return true;
}

/*
* Caskey, Damon V.
* 2026-08-01
*
* Close a Vorbis decoder and its callback-owned
* packfile handle.
*/
static void sound_vorbis_close_decoder(int *handle, OggVorbis_File **decoder) {
    if(decoder && *decoder) {
        ov_clear(*decoder);
        free(*decoder);
        *decoder = NULL;
    } else if(handle && *handle >= 0) {
        closepackfile(*handle);
        *handle = -1;
    }
}

static void sound_vorbis_copy_comment(char destination[64], const char *source, size_t source_length) {
    size_t copy_length;

    if(!destination || !source) {
        return;
    }

    copy_length = source_length < 63U ? source_length : 63U;
    memcpy(destination, source, copy_length);
    destination[copy_length] = '\0';
}

/*
* Caskey, Damon V.
* 2026-08-01
*
* Retain common Vorbis comments with sample metadata
* so legacy music queries do not depend on global
* decoder state.
*/
static void sound_vorbis_read_comments(OggVorbis_File *decoder, samplestruct *sample) {
    vorbis_comment *comment;
    int comment_index;

    if(!decoder || !sample) {
        return;
    }

    comment = ov_comment(decoder, -1);
    if(!comment) {
        return;
    }

    for(comment_index = 0; comment_index < comment->comments; comment_index++) {
        const char *text = comment->user_comments[comment_index];
        size_t text_length;

        if(!text) {
            continue;
        }

        text_length = comment->comment_lengths
            ? (size_t)comment->comment_lengths[comment_index]
            : strlen(text);

        if(text_length >= 7U && memcmp(text, "ARTIST=", 7U) == 0) {
            sound_vorbis_copy_comment(sample->artist, text + 7U, text_length - 7U);
        } else if(text_length >= 6U && memcmp(text, "TITLE=", 6U) == 0) {
            sound_vorbis_copy_comment(sample->title, text + 6U, text_length - 6U);
        }
    }
}

/*
* Caskey, Damon V.
* 2026-08-01
*
* Validate Ogg Vorbis and retain either decoded
* 16-bit PCM or metadata for channel streaming.
*/
static bool loadvorbis(char *filename, char *packname, samplestruct *sample, uint64_t maximum_data_bytes, bool stream) {
    OggVorbis_File *decoder = NULL;
    vorbis_info *stream_info;
    vorbis_info *link_info;
    ogg_int64_t total_frames;
    long logical_stream_count;
    long logical_stream_index;
    uint64_t retained_frames;
    uint64_t retained_bytes;
    size_t allocation_size;
    size_t bytes_written;
    int current_section = 0;
    int handle = -1;

    if(!sample) {
        return false;
    }

    memset(sample, 0, sizeof(*sample));
    if(!sound_vorbis_open_decoder(filename, packname, &handle, &decoder)) {
        return false;
    }

    logical_stream_count = ov_streams(decoder);
    stream_info = ov_info(decoder, 0);
    total_frames = ov_pcm_total(decoder, -1);
    if(logical_stream_count < 1 ||
       logical_stream_count > INT_MAX ||
       !stream_info ||
       (stream_info->channels != CHANNEL_TYPE_MONO && stream_info->channels != CHANNEL_TYPE_STEREO) ||
       stream_info->rate < SOUND_MUSIC_FREQUENCY_MIN ||
       stream_info->rate > SOUND_MUSIC_FREQUENCY_MAX ||
       total_frames <= 0 ||
       (uint64_t)total_frames > SOUND_SAMPLE_FIXED_MAX_INTEGER) {
        sound_vorbis_close_decoder(&handle, &decoder);
        return false;
    }

    /* Chained streams must retain one mixer-compatible PCM format. */
    for(logical_stream_index = 1; logical_stream_index < logical_stream_count; logical_stream_index++) {
        link_info = ov_info(decoder, (int)logical_stream_index);
        if(!link_info ||
           link_info->channels != stream_info->channels ||
           link_info->rate != stream_info->rate) {
            sound_vorbis_close_decoder(&handle, &decoder);
            return false;
        }
    }

    retained_frames = (uint64_t)total_frames;
    if(retained_frames > UINT64_MAX / ((uint64_t)stream_info->channels * sizeof(int16_t))) {
        sound_vorbis_close_decoder(&handle, &decoder);
        return false;
    }

    retained_bytes = retained_frames * (uint64_t)stream_info->channels * sizeof(int16_t);
    if(!stream && retained_bytes > maximum_data_bytes) {
        retained_bytes = maximum_data_bytes;
        retained_bytes -= retained_bytes % ((uint64_t)stream_info->channels * sizeof(int16_t));
        retained_frames = retained_bytes / ((uint64_t)stream_info->channels * sizeof(int16_t));
    }
    if(retained_frames == 0) {
        sound_vorbis_close_decoder(&handle, &decoder);
        return false;
    }

    sample->soundbytes = retained_bytes;
    sample->soundlen = retained_frames * (uint64_t)stream_info->channels;
    sample->framecount = retained_frames;
    sample->bits = 16;
    sample->frequency = (int)stream_info->rate;
    sample->channels = stream_info->channels;
    sample->blockalign = stream_info->channels * (int)sizeof(int16_t);
    sample->file_type = SOUND_SAMPLE_FILE_TYPE_VORBIS;
    sound_vorbis_read_comments(decoder, sample);

    if(stream) {
        sound_vorbis_close_decoder(&handle, &decoder);
        return true;
    }

    if(retained_bytes > (uint64_t)(SIZE_MAX - 8U)) {
        sound_vorbis_close_decoder(&handle, &decoder);
        return false;
    }

    allocation_size = (size_t)retained_bytes + 8U;
    sample->sampleptr = malloc(allocation_size);
    if(!sample->sampleptr || ov_pcm_seek(decoder, 0) != 0) {
        free(sample->sampleptr);
        sample->sampleptr = NULL;
        sound_vorbis_close_decoder(&handle, &decoder);
        return false;
    }

    memset(sample->sampleptr, 0, allocation_size);
    bytes_written = 0;
    while(bytes_written < (size_t)retained_bytes) {
        size_t remaining_bytes = (size_t)retained_bytes - bytes_written;
        int requested_bytes = remaining_bytes > (size_t)INT_MAX ? INT_MAX : (int)remaining_bytes;
        long decoded_bytes = sound_vorbis_decode(
            decoder,
            (char *)sample->sampleptr + bytes_written,
            requested_bytes,
            &current_section
        );

        if(decoded_bytes <= 0) {
            free(sample->sampleptr);
            sample->sampleptr = NULL;
            sound_vorbis_close_decoder(&handle, &decoder);
            return false;
        }

        bytes_written += (size_t)decoded_bytes;
    }

    sound_vorbis_close_decoder(&handle, &decoder);
    return true;
}

/*
* Caskey, Damon V.
* 2026-08-01
*
* Identify WAV and Ogg Vorbis containers by signature
* so audio loading does not depend on filename extensions.
*/
static e_sound_sample_file_type sound_sample_file_type_detect(char *filename, char *packname) {
    unsigned char signature[16];
    int handle;
    int bytes_read;

    handle = openpackfile(filename, packname);
    if(handle < 0) {
        return SOUND_SAMPLE_FILE_TYPE_NONE;
    }

    bytes_read = readpackfile(handle, signature, sizeof(signature));
    closepackfile(handle);
    if(bytes_read != sizeof(signature)) {
        return SOUND_SAMPLE_FILE_TYPE_NONE;
    }

    if(memcmp(signature, "RIFF", 4U) == 0) {
        return SOUND_SAMPLE_FILE_TYPE_WAVE;
    }
    if(memcmp(signature, "OggS", 4U) == 0) {
        return SOUND_SAMPLE_FILE_TYPE_VORBIS;
    }
    return SOUND_SAMPLE_FILE_TYPE_NONE;
}

static bool sound_load_sample_source(char *filename, char *packname, samplestruct *sample, uint64_t maximum_data_bytes, bool stream) {
    switch(sound_sample_file_type_detect(filename, packname)) {
    case SOUND_SAMPLE_FILE_TYPE_WAVE:
        return loadwave(filename, packname, sample, maximum_data_bytes, stream);
    case SOUND_SAMPLE_FILE_TYPE_VORBIS:
        return loadvorbis(filename, packname, sample, maximum_data_bytes, stream);
    case SOUND_SAMPLE_FILE_TYPE_NONE:
    default:
        return false;
    }
}

bool sound_reload_sample(int index) {
    samplestruct loaded_sample;
    char *filename;
    char *packfilename;
    bool stream;
    bool load_success;

    if(!mixing_inited) {
        return false;
    }

    SB_lock_audio();
    if(index < 0 || index >= sound_cached) {
        SB_unlock_audio();
        return false;
    }

    stream = soundcache[index].stream;
    if((stream && soundcache[index].sample.framecount > 0) ||
       (!stream && soundcache[index].sample.sampleptr)) {
        SB_unlock_audio();
        return true;
    }

    filename = soundcache[index].filename;
    packfilename = soundcache[index].packfilename;
    SB_unlock_audio();

    memset(&loaded_sample, 0, sizeof(loaded_sample));
    load_success = sound_load_sample_source(
        filename,
        packfilename,
        &loaded_sample,
        sound_parameters.sound_length_max,
        stream
    );
    if(!load_success) {
        return false;
    }

    SB_lock_audio();
    if(index < 0 || index >= sound_cached) {
        SB_unlock_audio();
        free(loaded_sample.sampleptr);
        return false;
    }

    soundcache[index].sample = loaded_sample;
    SB_unlock_audio();
    return true;
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

    const bool load_success = sound_load_sample_source(
        filename,
        packfilename,
        &sample,
        sound_parameters.sound_length_max,
        stream
    );

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

    SB_lock_audio();
    expanded_cache = realloc(soundcache, sizeof(*soundcache) * ((size_t)sound_cached + 1U));
    if(!expanded_cache) {
        SB_unlock_audio();
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
    sound_cached++;
    SB_unlock_audio();

    free(cache_key);

    return sound_cached - 1;
}

// Changed to conserve memory: added this function
void sound_unload_sample(int index)
{
    if(!mixing_inited)
    {
        return;
    }

    SB_lock_audio();
    if(index < 0 || index >= sound_cached)
    {
        SB_unlock_audio();
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
    SB_unlock_audio();
}

void sound_unload_all_samples()
{
    int i;

    SB_lock_audio();
    if(!soundcache)
    {
        SB_unlock_audio();
        return;
    }
    for(i = 0; i < sound_cached; i++)
    {
        free(soundcache[i].sample.sampleptr);
        memset(&soundcache[i].sample, 0, sizeof(samplestruct));
        free(soundcache[i].filename);
        soundcache[i].filename = NULL;
        free(soundcache[i].packfilename);
        soundcache[i].packfilename = NULL;
    }
    List_Clear(&audio_global.samplelist);
    free(soundcache);
    soundcache = NULL;
    sound_cached = 0;
    SB_unlock_audio();
}

#pragma pack(pop)

/////////////////////////////// Mix to DMA //////////////////////////////////
// Mixbuffer / DMA buffer data handling
// Writes signed 24-bit values mixed in a 64-bit array
// to the active SDL transport format.

/*
* Caskey, Damon V.
* 2026-08-06
*
* Keep intermediate mixing in 64-bit storage. This
* accommodates every channel at the maximum writable
* volume without overflowing before final output clipping.
*/
static void clearmixbuffer(s64 *buf, int n) {
    while((--n) >= 0) {
        *buf = 0;
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
* Normalize supported PCM source widths to the
* mixer's signed 24-bit range. Multiplication is
* used for signed promotion so negative shifts do
* not rely on implementation-defined behavior.
*/
static int32_t sound_pcm_to_mix_value(const unsigned char *sample_data, int sample_bits, size_t sample_index) {
    if(sample_bits == 8){
        return ((int32_t)sample_data[sample_index] - 0x80) * SOUND_PCM_8_TO_24;
    }

    if(sample_bits == 16) {

        size_t byte_index = sample_index * 2U;
        uint16_t sample_value = (uint16_t)sample_data[byte_index] |
                                ((uint16_t)sample_data[byte_index + 1U] << 8U);

        return (int32_t)(int16_t)sample_value * SOUND_PCM_16_TO_24;
    }

    if(sample_bits == 24) {
        size_t byte_index = sample_index * 3U;
        uint32_t sample_value = (uint32_t)sample_data[byte_index] |
                                ((uint32_t)sample_data[byte_index + 1U] << 8U) |
                                ((uint32_t)sample_data[byte_index + 2U] << 16U);

        if(sample_value & UINT32_C(0x00800000)) {
            return (int32_t)((int64_t)sample_value - INT64_C(0x01000000));
        }

        return (int32_t)sample_value;
    }

    return 0;
}

static int32_t sound_sample_to_mix_value(const samplestruct *sample, size_t sample_index) {
    return sound_pcm_to_mix_value(sample->sampleptr, sample->bits, sample_index);
}

static int32_t sound_pcm_frame_to_mix_value(
    const unsigned char *sample_data,
    int sample_bits,
    int sample_channels,
    size_t frame_index,
    e_channel_index spatial_channel
) {
    size_t sample_index = frame_index * (size_t)sample_channels;

    if(sample_channels == CHANNEL_TYPE_STEREO &&
       spatial_channel == SOUND_SPATIAL_CHANNEL_RIGHT) {
        sample_index++;
    }

    return sound_pcm_to_mix_value(sample_data, sample_bits, sample_index);
}

/*
* Apply linear interpolation using the fractional portion
* of a fixed-point PCM frame position. Integer positions
* return the source value exactly.
*/
static int32_t sound_mix_value_interpolate(
    int32_t current_value,
    int32_t next_value,
    sound_sample_fixed_t sample_position_fixed
) {
    uint32_t fraction = (uint32_t)(sample_position_fixed & (SOUND_SAMPLE_FIXED_ONE - 1U));
    int64_t difference;

    if(fraction == 0 || current_value == next_value) {
        return current_value;
    }

    difference = (int64_t)next_value - current_value;

    return current_value + (int32_t)(
        difference * fraction / (int64_t)SOUND_SAMPLE_FIXED_ONE
    );
}

/*
* Clip the accumulated mixer value to signed 24-bit PCM and
* place its meaningful bits in SDL's signed 32-bit transport.
*/
static int32_t sound_mix_value_to_s32_transport(s64 mix_value) {
    s64 output_value = mix_value / (INT64_C(1) << MIXSHIFT);

    if(output_value < SOUND_PCM_24_MIN) {
        output_value = SOUND_PCM_24_MIN;
    } else if(output_value > SOUND_PCM_24_MAX) {
        output_value = SOUND_PCM_24_MAX;
    }

    return (int32_t)output_value * SOUND_PCM_24_TO_S32;
}

typedef struct s_sound_wave_stream_read_context {
    int handle;
    uint64_t data_offset;
    size_t block_align;
} s_sound_wave_stream_read_context;

/*
* Caskey, Damon V.
* 2026-08-01
*
* Seek and fill one PCM stream buffer from a
* validated source frame. This runs only in the
* producer path, never in the audio callback.
*/
static bool sound_wave_stream_read_frames(
    void *context,
    uint64_t source_start_frame,
    void *destination,
    size_t bytes_to_read
) {
    s_sound_wave_stream_read_context *read_context = context;
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
* Decode one exact PCM frame range from a channel-owned
* Vorbis decoder. Seeks occur for initial offsets and
* automatic loop restarts, never in the audio callback.
*/
static bool sound_vorbis_stream_read_frames(
    void *context,
    uint64_t source_start_frame,
    void *destination,
    size_t bytes_to_read
) {
    OggVorbis_File *decoder = context;
    ogg_int64_t current_frame;
    size_t bytes_written = 0;
    int current_section = 0;

    if(!decoder || !destination || source_start_frame > (uint64_t)INT64_MAX) {
        return false;
    }

    current_frame = ov_pcm_tell(decoder);
    if(current_frame < 0) {
        return false;
    }
    if((uint64_t)current_frame != source_start_frame &&
       ov_pcm_seek(decoder, (ogg_int64_t)source_start_frame) != 0) {
        return false;
    }

    while(bytes_written < bytes_to_read) {
        size_t remaining_bytes = bytes_to_read - bytes_written;
        int requested_bytes = remaining_bytes > (size_t)INT_MAX ? INT_MAX : (int)remaining_bytes;
        long decoded_bytes = sound_vorbis_decode(
            decoder,
            (char *)destination + bytes_written,
            requested_bytes,
            &current_section
        );

        if(decoded_bytes <= 0) {
            return false;
        }

        bytes_written += (size_t)decoded_bytes;
    }

    return true;
}

/*
* Return the queued PCM frame following the current stream
* frame without consuming either buffer. Looping pull streams
* already publish their loop frame at the start of the next
* ready buffer. Push streams publish the next contiguous frame.
*/
static int32_t sound_stream_next_mix_value(
    const s_sound_stream *stream,
    const channelstruct *channel_record,
    const s_sound_stream_buffer *stream_buffer,
    size_t frame_index,
    e_channel_index spatial_channel,
    int32_t current_value
) {
    const s_sound_stream_buffer *next_buffer;

    if((uint64_t)frame_index + 1U < stream_buffer->frame_count) {
        return sound_pcm_frame_to_mix_value(
            stream_buffer->data,
            channel_record->bits,
            channel_record->channels,
            frame_index + 1U,
            spatial_channel
        );
    }

    if(stream_buffer->terminal) {
        return current_value;
    }

    next_buffer = &stream->buffer[
        (stream->read_buffer + 1U) % SOUND_STREAM_BUFFER_COUNT
    ];

    if(!next_buffer->ready || next_buffer->frame_count == 0) {
        return current_value;
    }

    return sound_pcm_frame_to_mix_value(
        next_buffer->data,
        channel_record->bits,
        channel_record->channels,
        0,
        spatial_channel
    );
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
    unsigned int todo,
    int output_start
) {
    s_sound_stream *stream = &channel_record->stream;
    sound_sample_fixed_t buffer_position_fixed = stream->fp_buffer_position;
    int output_position;
    int left_volume = channel_record->volume[SOUND_SPATIAL_CHANNEL_LEFT];
    int right_volume = channel_record->volume[SOUND_SPATIAL_CHANNEL_RIGHT];
    int volume_divisor = channel_record->volume_divisor > 0
        ? channel_record->volume_divisor
        : MAX_SAMPLE_VOLUME;

    for(output_position = output_start; output_position + 1 < (int)todo; output_position += SOUND_SPATIAL_CHANNEL_MAX) {
        s_sound_stream_buffer *stream_buffer;
        sound_sample_fixed_t buffer_length_fixed;
        size_t frame_index;
        int32_t left_sample_value;
        int32_t right_sample_value;
        int terminal;

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
            terminal = stream_buffer->terminal;
            stream_buffer->ready = 0;
            stream->read_buffer = (stream->read_buffer + 1U) % SOUND_STREAM_BUFFER_COUNT;

            if(terminal) {
                channel_record->fp_samplepos = SOUND_SAMPLE_INT_TO_FIX(
                    stream_buffer->source_start_frame + stream_buffer->frame_count
                );
                stream->fp_buffer_position = 0;
                sound_channel_pool_deactivate(&sound_channel_pool, channel);
                return;
            }
        }

        frame_index = (size_t)SOUND_SAMPLE_FIX_TO_INT(buffer_position_fixed);
        left_sample_value = sound_pcm_frame_to_mix_value(
            stream_buffer->data,
            channel_record->bits,
            channel_record->channels,
            frame_index,
            SOUND_SPATIAL_CHANNEL_LEFT
        );
        right_sample_value = sound_pcm_frame_to_mix_value(
            stream_buffer->data,
            channel_record->bits,
            channel_record->channels,
            frame_index,
            SOUND_SPATIAL_CHANNEL_RIGHT
        );

        if(buffer_position_fixed & (SOUND_SAMPLE_FIXED_ONE - 1U)) {
            left_sample_value = sound_mix_value_interpolate(
                left_sample_value,
                sound_stream_next_mix_value(
                    stream,
                    channel_record,
                    stream_buffer,
                    frame_index,
                    SOUND_SPATIAL_CHANNEL_LEFT,
                    left_sample_value
                ),
                buffer_position_fixed
            );
            right_sample_value = sound_mix_value_interpolate(
                right_sample_value,
                sound_stream_next_mix_value(
                    stream,
                    channel_record,
                    stream_buffer,
                    frame_index,
                    SOUND_SPATIAL_CHANNEL_RIGHT,
                    right_sample_value
                ),
                buffer_position_fixed
            );
        }

        mixbuf[output_position] +=
            (s64)left_sample_value * left_volume / volume_divisor;
        mixbuf[output_position + 1] +=
            (s64)right_sample_value * right_volume / volume_divisor;

        channel_record->fp_samplepos =
            SOUND_SAMPLE_INT_TO_FIX(stream_buffer->source_start_frame) + buffer_position_fixed;
        buffer_position_fixed += channel_record->fp_period;
    }

    stream->fp_buffer_position = buffer_position_fixed;
}

/*
* Caskey, Damon V.
* 2026-08-07
*
* Prepare an active channel for the current output block.
* Delayed channels consume output frames without advancing
* sample data. Chance is evaluated once at the exact point
* playback becomes eligible. A failed roll releases the
* channel without emitting audio.
*/
static bool sound_channel_prepare_mix(
    int channel,
    channelstruct *record,
    unsigned int todo,
    int *output_start
) {
    uint64_t output_frames;
    uint64_t chance_threshold;
    unsigned int chance;

    if(!record || !output_start) {
        return false;
    }

    output_frames = todo / SOUND_SPATIAL_CHANNEL_MAX;
    if(!output_frames) {
        return false;
    }

    *output_start = 0;

    if(record->delay_frames) {
        if(record->delay_frames >= output_frames) {
            record->delay_frames -= output_frames;
            return false;
        }

        *output_start = (int)(record->delay_frames * SOUND_SPATIAL_CHANNEL_MAX);
        record->delay_frames = 0;
    }

    chance = record->chance;
    record->chance = SOUND_PLAY_CHANCE_MAX;

    if(chance >= SOUND_PLAY_CHANCE_MAX) {
        return true;
    }

    chance_threshold = ((uint64_t)chance * (UINT64_C(1) << 32))
        / SOUND_PLAY_CHANCE_MAX;

    if((uint64_t)record->chance_roll >= chance_threshold) {
        sound_channel_pool_deactivate(&sound_channel_pool, channel);
        return false;
    }

    return true;
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
    int32_t left_sample_value;
    int32_t right_sample_value;

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
                int volume_divisor;
                uint64_t channel_bit = UINT64_C(1) << channel_index;

                channel = (bank_index * (int)SOUND_CHANNEL_BANK_SIZE) + channel_index;

                if(!sound_channel_prepare_mix(
                    channel,
                    channel_record,
                    todo,
                    &output_position
                )) {
                    channel_mask &= ~channel_bit;
                    continue;
                }

                if(bank->streaming_mask & channel_bit) {
                    if(channel_record->stream_source == SOUND_CHANNEL_STREAM_SOURCE_NONE ||
                       channel_record->stream.block_align == 0 ||
                       (channel_record->bits != 8 &&
                        channel_record->bits != 16 &&
                        channel_record->bits != 24) ||
                       (channel_record->channels != CHANNEL_TYPE_MONO &&
                        channel_record->channels != CHANNEL_TYPE_STEREO)) {
                        sound_channel_pool_deactivate(&sound_channel_pool, channel);
                    } else {
                        sound_mix_stream_channel(
                            channel,
                            channel_record,
                            todo,
                            output_position
                        );
                    }
                    channel_mask &= ~channel_bit;
                    continue;
                }

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
                   !sample->sampleptr) {
                    sound_channel_pool_deactivate(&sound_channel_pool, channel);
                    channel_mask &= ~channel_bit;
                    continue;
                }

                sample_length_fixed = SOUND_SAMPLE_INT_TO_FIX(sample_frame_count);
                loop_start_fixed = channel_record->active == CHANNEL_LOOPING ? channel_record->fp_loop_start : 0;
                sample_position_fixed = channel_record->fp_samplepos;
                sample_period_fixed = channel_record->fp_period;
                left_volume = channel_record->volume[SOUND_SPATIAL_CHANNEL_LEFT];
                right_volume = channel_record->volume[SOUND_SPATIAL_CHANNEL_RIGHT];
                volume_divisor = channel_record->volume_divisor > 0
                    ? channel_record->volume_divisor
                    : MAX_SAMPLE_VOLUME;

                for(; output_position + 1 < (int)todo; output_position += SOUND_SPATIAL_CHANNEL_MAX) {
                    size_t left_sample_index;
                    size_t right_sample_index;
                    size_t sample_position_index;
                    size_t next_sample_position_index;
                    int sample_end_reached;

                    sample_position_index = (size_t)SOUND_SAMPLE_FIX_TO_INT(sample_position_fixed);
                    left_sample_index = sample_position_index * (size_t)sample->channels;
                    right_sample_index = sample->channels == CHANNEL_TYPE_STEREO ? left_sample_index + 1U : left_sample_index;

                    left_sample_value = sound_sample_to_mix_value(sample, left_sample_index);
                    right_sample_value = sound_sample_to_mix_value(sample, right_sample_index);

                    if(sample_position_fixed & (SOUND_SAMPLE_FIXED_ONE - 1U)) {
                        next_sample_position_index = sample_position_index;

                        if((uint64_t)sample_position_index + 1U < sample_frame_count) {
                            next_sample_position_index++;
                        } else if(channel_record->active == CHANNEL_LOOPING) {
                            next_sample_position_index = (size_t)SOUND_SAMPLE_FIX_TO_INT(
                                loop_start_fixed
                            );
                        }

                        left_sample_value = sound_mix_value_interpolate(
                            left_sample_value,
                            sound_pcm_frame_to_mix_value(
                                sample->sampleptr,
                                sample->bits,
                                sample->channels,
                                next_sample_position_index,
                                SOUND_SPATIAL_CHANNEL_LEFT
                            ),
                            sample_position_fixed
                        );
                        right_sample_value = sound_mix_value_interpolate(
                            right_sample_value,
                            sound_pcm_frame_to_mix_value(
                                sample->sampleptr,
                                sample->bits,
                                sample->channels,
                                next_sample_position_index,
                                SOUND_SPATIAL_CHANNEL_RIGHT
                            ),
                            sample_position_fixed
                        );
                    }

                    mixbuf[output_position] +=
                        (s64)left_sample_value * left_volume / volume_divisor;
                    mixbuf[output_position + 1] +=
                        (s64)right_sample_value * right_volume / volume_divisor;

                    sample_position_fixed = sound_sample_position_advance(sample_position_fixed, sample_period_fixed, sample_length_fixed, loop_start_fixed, &sample_end_reached);
                    if(sample_end_reached && channel_record->active != CHANNEL_LOOPING) {
                        sound_channel_pool_deactivate(&sound_channel_pool, channel);
                        break;
                    }
                }
                channel_record->fp_samplepos = sample_position_fixed;
                channel_mask &= ~channel_bit;
            }

            active_bank_mask &= ~(UINT64_C(1) << bank_index);
        }
    }
}


//////////////////////////////// ISR ///////////////////////////////////
// Called by Soundblaster ISR

void update_sample(unsigned char *buf, int size)
{
    int bytes_per_sample;
    int i;
    int todo;
    s64 u;

    if(!buf || size <= 0 || !mixbuf) {
        return;
    }

    switch(playbits) {
        case 8:
            bytes_per_sample = 1;
            memset(buf, 0x80, (size_t)size);
            break;

        case 16:
            bytes_per_sample = 2;
            memset(buf, 0, (size_t)size);
            break;

        case 24:
            /* SDL transports each signed 24-bit value in an S32 sample. */
            bytes_per_sample = 4;
            memset(buf, 0, (size_t)size);
            break;

        default:
            memset(buf, 0, (size_t)size);
            return;
    }

    todo = size / bytes_per_sample;
    todo -= todo % SOUND_SPATIAL_CHANNEL_MAX;
    if(todo <= 0 || (size_t)todo > MIXBUF_SAMPLE_COUNT) {
        return;
    }

    clearmixbuffer(mixbuf, todo);
    mixaudio(todo);
    samplesplayed += todo / SOUND_SPATIAL_CHANNEL_MAX;

    if(playbits == 8) {
        unsigned char *dst = buf;

        for(i = 0; i < todo; i++) {
            u = mixbuf[i] / (INT64_C(1) << (MIXSHIFT + 16));
            u += 0x80;

            if (u < 0)
            {
                u = 0;
            }
            else if (u > 0xff)
            {
                u = 0xff;
            }
            dst[i] = (unsigned char)u;
        }
    } else if(playbits == 16) {
        int16_t *dst = (int16_t *)buf;

        for(i = 0; i < todo; i++) {
            u = mixbuf[i] / (INT64_C(1) << (MIXSHIFT + 8));

            if(u < INT16_MIN)
            {
                u = INT16_MIN;
            }
            else if(u > INT16_MAX)
            {
                u = INT16_MAX;
            }

            dst[i] = (int16_t)u;
        }
    } else {
        int32_t *dst = (int32_t *)buf;

        for(i = 0; i < todo; i++) {
            dst[i] = sound_mix_value_to_s32_transport(mixbuf[i]);
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
    OggVorbis_File *decoder;

    if(!record) {
        return;
    }

    decoder = record->stream_decoder;
    if(record->stream_source == SOUND_CHANNEL_STREAM_SOURCE_VORBIS && decoder) {
        sound_vorbis_close_decoder(&record->stream.handle, &decoder);
    } else if(record->stream.handle >= 0) {
        closepackfile(record->stream.handle);
    }
    record->stream_decoder = NULL;
    record->stream_source = SOUND_CHANNEL_STREAM_SOURCE_NONE;

    SB_lock_audio();
    sound_stream_reset(&record->stream);
    sound_channel_pool_stream(&sound_channel_pool, channel, 0);
    SB_unlock_audio();
}

/*
* Caskey, Damon V.
* 2026-08-01
*
* Clear playback fields without releasing the
* stream buffers retained by the channel.
*/
static void sound_channel_record_clear(channelstruct *record) {
    unsigned char *buffer_data[SOUND_STREAM_BUFFER_COUNT];
    unsigned int buffer_index;
    e_object_type object_type;
    int index;

    if(!record) {
        return;
    }

    object_type = record->object_type;
    index = record->index;
    for(buffer_index = 0; buffer_index < SOUND_STREAM_BUFFER_COUNT; buffer_index++) {
        buffer_data[buffer_index] = record->stream.buffer[buffer_index].data;
    }

    memset(record, 0, sizeof(*record));
    record->object_type = object_type;
    record->index = index;
    record->samplenum = -1;
    record->playid = -1;
    record->chance = SOUND_PLAY_CHANCE_MAX;
    sound_stream_init(&record->stream);

    for(buffer_index = 0; buffer_index < SOUND_STREAM_BUFFER_COUNT; buffer_index++) {
        record->stream.buffer[buffer_index].data = buffer_data[buffer_index];
    }
}

/*
* Caskey, Damon V.
* 2026-08-01
*
* Fill one rotating buffer from the source producer
* selected by the cached sample container.
*/
static int sound_stream_fill_channel(channelstruct *record, const s_soundcache *cache, size_t *bytes_filled) {
    s_sound_wave_stream_read_context wave_context;

    if(!record || !cache) {
        return -1;
    }

    switch(cache->sample.file_type) {
    case SOUND_SAMPLE_FILE_TYPE_WAVE:
        wave_context.handle = record->stream.handle;
        wave_context.data_offset = cache->sample.data_offset;
        wave_context.block_align = (size_t)cache->sample.blockalign;
        return sound_stream_fill(
            &record->stream,
            sound_wave_stream_read_frames,
            &wave_context,
            bytes_filled
        );
    case SOUND_SAMPLE_FILE_TYPE_VORBIS:
        return sound_stream_fill(
            &record->stream,
            sound_vorbis_stream_read_frames,
            record->stream_decoder,
            bytes_filled
        );
    case SOUND_SAMPLE_FILE_TYPE_NONE:
    default:
        return -1;
    }
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
    OggVorbis_File *decoder = NULL;
    unsigned int buffer_index;
    int fill_result;
    int filled_buffers = 0;

    if(!record ||
       !cache ||
       !cache->stream) {
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

    switch(cache->sample.file_type) {
    case SOUND_SAMPLE_FILE_TYPE_WAVE:
        record->stream.handle = openpackfile(cache->filename, cache->packfilename);
        record->stream_source = SOUND_CHANNEL_STREAM_SOURCE_WAVE;
        break;
    case SOUND_SAMPLE_FILE_TYPE_VORBIS:
        if(sound_vorbis_open_decoder(
            cache->filename,
            cache->packfilename,
            &record->stream.handle,
            &decoder
        )) {
            record->stream_decoder = decoder;
            record->stream_source = SOUND_CHANNEL_STREAM_SOURCE_VORBIS;
        }
        break;
    case SOUND_SAMPLE_FILE_TYPE_NONE:
    default:
        break;
    }

    if(record->stream.handle < 0 ||
       (cache->sample.file_type == SOUND_SAMPLE_FILE_TYPE_VORBIS &&
        !record->stream_decoder)) {
        sound_stream_close_channel(channel, record);
        return false;
    }

    for(buffer_index = 0; buffer_index < SOUND_STREAM_BUFFER_COUNT; buffer_index++) {
        fill_result = sound_stream_fill_channel(record, cache, NULL);

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

    SB_lock_audio();
    sound_channel_pool_stream(&sound_channel_pool, channel, 1);
    SB_unlock_audio();
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
    channelstruct *record;
    s_soundcache *cache;
    size_t bytes_filled = 0;
    int close_stream = 0;
    int fill_result;

    SB_lock_audio();
    record = sound_channel_pool_get(&sound_channel_pool, channel);
    if(!record) {
        SB_unlock_audio();
        return 0;
    }

    if(!sound_channel_pool_is_active(&sound_channel_pool, channel)) {
        close_stream = 1;
    } else if(record->stream_source == SOUND_CHANNEL_STREAM_SOURCE_PUSH) {
        SB_unlock_audio();
        return 0;
    } else if(record->samplenum < 0 ||
              record->samplenum >= sound_cached ||
              !soundcache[record->samplenum].stream) {
        sound_channel_pool_deactivate(&sound_channel_pool, channel);
        close_stream = 1;
    }
    SB_unlock_audio();

    if(close_stream) {
        sound_stream_close_channel(channel, record);
        return 0;
    }

    cache = &soundcache[record->samplenum];
    fill_result = sound_stream_fill_channel(record, cache, &bytes_filled);

    if(fill_result < 0) {
        SB_lock_audio();
        sound_channel_pool_deactivate(&sound_channel_pool, channel);
        SB_unlock_audio();
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
    const uint64_t streaming_channel_mask[SOUND_CHANNEL_BANK_COUNT],
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
        uint64_t channel_mask;
        int channel_index;

        channel_mask = streaming_channel_mask[bank_index];
        if(!channel_mask) {
            continue;
        }

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
    uint64_t streaming_channel_mask[SOUND_CHANNEL_BANK_COUNT] = { 0 };
    uint64_t streaming_bank_mask;
    uint64_t streaming_bank_snapshot;
    size_t remaining_budget = SOUND_STREAM_UPDATE_BYTE_BUDGET;
    unsigned int start_channel;
    unsigned int last_channel;
    int bank_index;
    bool first_range_complete;

    SB_lock_audio();
    streaming_bank_snapshot = sound_channel_pool.streaming_bank_mask;
    streaming_bank_mask = streaming_bank_snapshot;
    while((bank_index = sound_channel_mask_first(streaming_bank_mask)) >= 0) {
        streaming_channel_mask[bank_index] = sound_channel_pool.bank[bank_index]->streaming_mask;
        streaming_bank_mask &= ~(UINT64_C(1) << bank_index);
    }
    SB_unlock_audio();

    if(!streaming_bank_snapshot) {
        return;
    }

    start_channel = sound_stream_update_cursor;
    last_channel = start_channel;
    first_range_complete = sound_stream_update_range(
        start_channel,
        SOUND_CHANNEL_COUNT_MAX,
        streaming_channel_mask,
        &remaining_budget,
        &last_channel
    );

    if(first_range_complete && remaining_budget > 0 && start_channel > 0) {
        sound_stream_update_range(
            0,
            start_channel,
            streaming_channel_mask,
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
        uint64_t channel_mask = bank->active_mask & ~bank->reserved_mask;
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
* 2026-08-07
*
* Convert caller clock units to output sample frames. Split
* whole and remainder units before multiplication so extreme
* delays saturate instead of overflowing. Partial output
* frames round up so playback never begins before the full
* requested duration has elapsed.
*/
static uint64_t sound_play_delay_frames_calculate(const s_sound_play_options *options) {
    uint64_t delay_frames;
    uint64_t delay_remainder;
    uint64_t delay_whole;
    uint64_t partial_frames;
    uint64_t partial_numerator;
    uint64_t output_frequency;
    uint64_t rate;

    if(!options || !options->delay) {
        return 0;
    }

    output_frequency = playfrequency > 0
        ? (uint64_t)playfrequency
        : SOUND_OUTPUT_FREQUENCY_DEFAULT;
    rate = options->delay_rate;

    if(!rate) {
        return UINT64_MAX;
    }

    delay_whole = options->delay / rate;
    delay_remainder = options->delay % rate;

    if(delay_whole > UINT64_MAX / output_frequency) {
        return UINT64_MAX;
    }

    delay_frames = delay_whole * output_frequency;
    partial_numerator = delay_remainder * output_frequency;
    partial_frames = (partial_numerator + rate - 1U) / rate;

    if(partial_frames > UINT64_MAX - delay_frames) {
        return UINT64_MAX;
    }

    return delay_frames + partial_frames;
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
    uint64_t loop_start_frame,
    int forced_channel,
    const s_sound_play_options *options
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
    if(options && options->delay && !options->delay_rate) {
        return -1;
    }
    if(((soundcache[samplenum].stream && soundcache[samplenum].sample.framecount == 0) ||
        (!soundcache[samplenum].stream && !soundcache[samplenum].sample.sampleptr)) &&
       !sound_reload_sample(samplenum)) {
        return -1;
    }

    sample = &soundcache[samplenum].sample;

    SB_lock_audio();
    if(forced_channel >= 0) {
        unsigned int bank_index;
        channelstruct *forced_record;

        if((unsigned int)forced_channel >= SOUND_CHANNEL_COUNT_MAX) {
            SB_unlock_audio();
            return -1;
        }

        bank_index = (unsigned int)forced_channel / SOUND_CHANNEL_BANK_SIZE;
        if(!sound_channel_pool_allocate_bank(&sound_channel_pool, bank_index)) {
            SB_unlock_audio();
            return -1;
        }

        forced_record = sound_channel_pool_get(
            &sound_channel_pool,
            forced_channel
        );

        if(sound_channel_pool_is_active(
               &sound_channel_pool,
               forced_channel
           ) && (!forced_record || forced_record->priority > priority)) {
            SB_unlock_audio();
            return -1;
        }

        channel = forced_channel;
    } else {
        channel = sound_channel_pool_acquire(&sound_channel_pool);
        if(channel < 0) {
            channel = sound_find_lowest_priority_channel(&priority_low);
            if(channel < 0 || priority_low > priority) {
                SB_unlock_audio();
                return -1;
            }
        }
    }
    SB_unlock_audio();

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

    SB_lock_audio();
    record = sound_channel_pool_get(&sound_channel_pool, channel);
    if(!record) {
        SB_unlock_audio();
        return -1;
    }

    sound_channel_pool_deactivate(&sound_channel_pool, channel);
    SB_unlock_audio();
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
    record->volume_divisor = MAX_SAMPLE_VOLUME;
    record->priority = priority;
    record->owner_id = options ? options->owner_id : UINT64_C(0);
    record->group = options ? options->group : SOUND_GROUP_NONE;
    record->delay_frames = sound_play_delay_frames_calculate(options);
    record->chance = options && options->chance < SOUND_PLAY_CHANCE_MAX
        ? options->chance
        : SOUND_PLAY_CHANCE_MAX;
    record->chance_roll = record->chance < SOUND_PLAY_CHANCE_MAX
        ? rand32()
        : 0;
    record->bits = sample->bits;
    record->frequency = sample->frequency;
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

    SB_lock_audio();
    sound_channel_pool_activate(&sound_channel_pool, channel, looping ? CHANNEL_LOOPING : CHANNEL_PLAYING);
    SB_unlock_audio();

    return channel;
}

int sound_play_sample(int samplenum, unsigned int priority, int lvolume, int rvolume, unsigned int speed) {
    return sound_play_sample_internal(samplenum, priority, lvolume, rvolume, speed, 0, 0, 0, 0, -1, NULL);
}

int sound_play_sample_with_options(int samplenum, unsigned int priority, int lvolume, int rvolume, unsigned int speed, const s_sound_play_options *options) {
    int forced_channel = -1;

    if(options && options->channel_supplied) {
        if(options->channel >= SOUND_CHANNEL_COUNT_MAX) {
            return -1;
        }
        forced_channel = (int)options->channel;
    }

    return sound_play_sample_internal(
        samplenum,
        priority,
        lvolume,
        rvolume,
        speed,
        options && options->loop,
        options && options->start_offset_supplied,
        options ? options->start_offset : 0,
        options ? options->loop_offset : 0,
        forced_channel,
        options
    );
}

int sound_play_sample_offset(int samplenum, unsigned int priority, int lvolume, int rvolume, unsigned int speed, uint64_t start_frame) {
    return sound_play_sample_internal(samplenum, priority, lvolume, rvolume, speed, 0, 1, start_frame, 0, -1, NULL);
}

int sound_loop_sample(int samplenum, unsigned int priority, int lvolume, int rvolume, unsigned int speed) {
    return sound_play_sample_internal(samplenum, priority, lvolume, rvolume, speed, 1, 0, 0, 0, -1, NULL);
}

int sound_loop_sample_offset(int samplenum, unsigned int priority, int lvolume, int rvolume, unsigned int speed, uint64_t start_frame, uint64_t loop_start_frame) {
    return sound_play_sample_internal(samplenum, priority, lvolume, rvolume, speed, 1, 1, start_frame, loop_start_frame, -1, NULL);
}

int sound_query_channel(int playid) {
    uint64_t active_bank_mask;
    int bank_index;
    int channel = -1;

    SB_lock_audio();
    active_bank_mask = sound_channel_pool.active_bank_mask;
    while((bank_index = sound_channel_mask_first(active_bank_mask)) >= 0) {
        s_sound_channel_bank *bank = sound_channel_pool.bank[bank_index];
        uint64_t channel_mask = bank->active_mask;
        int channel_index;

        while((channel_index = sound_channel_mask_first(channel_mask)) >= 0) {
            if(bank->channel[channel_index].playid == playid) {
                channel = (bank_index * (int)SOUND_CHANNEL_BANK_SIZE) + channel_index;
                break;
            }
            channel_mask &= ~(UINT64_C(1) << channel_index);
        }
        if(channel >= 0) {
            break;
        }
        active_bank_mask &= ~(UINT64_C(1) << bank_index);
    }
    SB_unlock_audio();

    return channel;
}

/*
* Caskey, Damon V.
* 2026-08-02
*
* Return the stable sound object assigned to a
* flattened channel index.
*/
channelstruct *sound_get_channel_object(int channel) {
    channelstruct *record;

    SB_lock_audio();
    record = sound_channel_pool_get(&sound_channel_pool, channel);
    SB_unlock_audio();
    return record;
}

/*
* Caskey, Damon V.
* 2026-08-02
*
* Resolve a sound object to its flattened channel
* index after verifying pool ownership.
*/
int sound_get_channel_index(const channelstruct *record) {
    int channel;

    SB_lock_audio();
    channel = sound_channel_pool_get_index(&sound_channel_pool, record);
    SB_unlock_audio();
    return channel;
}

/*
* Caskey, Damon V.
* 2026-08-04
*
* Copy the script-visible channel fields while the
* SDL callback is excluded from updating them.
*/
bool sound_get_channel_snapshot(const channelstruct *record, channelstruct *snapshot) {
    int channel;

    if(!record || !snapshot) {
        return false;
    }

    SB_lock_audio();
    channel = sound_channel_pool_get_index(&sound_channel_pool, record);
    if(channel >= 0) {
        *snapshot = *sound_channel_pool_get(&sound_channel_pool, channel);
    }
    SB_unlock_audio();
    return channel >= 0;
}

/*
* Caskey, Damon V.
* 2026-08-02
*
* Return a pool-level bank mask for script and
* engine channel traversal.
*/
uint64_t sound_get_channel_bank_mask(e_sound_channel_bank_mask mask) {
    uint64_t result;

    SB_lock_audio();
    result = sound_channel_pool_get_bank_mask(&sound_channel_pool, mask);
    SB_unlock_audio();
    return result;
}

/*
* Caskey, Damon V.
* 2026-08-02
*
* Return one state mask from an allocated channel
* bank. Unallocated banks return an empty mask.
*/
uint64_t sound_get_channel_mask(unsigned int bank_index, e_sound_channel_mask mask) {
    uint64_t result;

    SB_lock_audio();
    result = sound_channel_pool_get_mask(&sound_channel_pool, bank_index, mask);
    SB_unlock_audio();
    return result;
}

int sound_id(int channel) {
    channelstruct *record;
    int playid = -1;

    SB_lock_audio();
    if(!sound_channel_pool_is_active(&sound_channel_pool, channel)) {
        SB_unlock_audio();
        return -1;
    }

    record = sound_channel_pool_get(&sound_channel_pool, channel);
    if(record) {
        playid = record->playid;
    }
    SB_unlock_audio();
    return playid;
}

int sound_is_active(int channel) {
    int active;

    SB_lock_audio();
    active = sound_channel_pool_is_active(&sound_channel_pool, channel) ? 1 : 0;
    SB_unlock_audio();
    return active;
}

void sound_stop_sample(int channel) {
    channelstruct *record;

    SB_lock_audio();
    record = sound_channel_pool_get(&sound_channel_pool, channel);
    sound_channel_pool_deactivate(&sound_channel_pool, channel);
    SB_unlock_audio();
    if(record) {
        sound_stream_close_channel(channel, record);
    }
}

/*
* Caskey, Damon V.
* 2026-08-05
*
* Stop channel playback and release stream resources.
* Normal cleanup preserves soft-reserved channels.
* Forced teardown includes every reserved stream.
*/
void sound_stopall_sample(bool force) {
    uint64_t streaming_channel_mask[SOUND_CHANNEL_BANK_COUNT] = { 0 };
    uint64_t streaming_bank_mask;
    int bank_index;

    SB_lock_audio();
    streaming_bank_mask = sound_channel_pool.streaming_bank_mask;
    while((bank_index = sound_channel_mask_first(streaming_bank_mask)) >= 0) {
        s_sound_channel_bank *bank = sound_channel_pool.bank[bank_index];
        streaming_channel_mask[bank_index] = force
            ? bank->streaming_mask
            : bank->streaming_mask & ~bank->reserved_mask;
        streaming_bank_mask &= ~(UINT64_C(1) << bank_index);
    }
    sound_channel_pool_stop_all(&sound_channel_pool, force);
    SB_unlock_audio();

    for(bank_index = 0; bank_index < (int)SOUND_CHANNEL_BANK_COUNT; bank_index++) {
        s_sound_channel_bank *bank = sound_channel_pool.bank[bank_index];
        uint64_t channel_mask = streaming_channel_mask[bank_index];
        int channel_index;

        if(!bank) {
            continue;
        }

        while((channel_index = sound_channel_mask_first(channel_mask)) >= 0) {
            int channel = bank_index * (int)SOUND_CHANNEL_BANK_SIZE + channel_index;
            sound_stream_close_channel(channel, &bank->channel[channel_index]);
            channel_mask &= ~(UINT64_C(1) << channel_index);
        }
    }
}

void sound_pause_sample(int toggle) {
    SB_lock_audio();
    sound_channel_pool_pause_all(&sound_channel_pool, toggle);
    SB_unlock_audio();
}

void sound_pause_single_sample(int toggle, int channel) {
    SB_lock_audio();
    sound_channel_pool_pause(&sound_channel_pool, channel, toggle);
    SB_unlock_audio();
}

/*
* Caskey, Damon V.
* 2026-08-08
*
* Build the active channel mask matching a sound group and
* owner within one bank. UINT64_MAX selects every owner.
* Caller must hold the audio lock.
*/
static uint64_t sound_group_get_bank_match_mask(
    const s_sound_channel_bank *bank,
    const sound_group_mask_t group,
    const uint64_t owner_id
) {
    const channelstruct *record;
    uint64_t active_mask;
    uint64_t result = 0;
    int channel_index;

    if(!bank || !group) {
        return 0;
    }

    active_mask = bank->active_mask;

    while((channel_index = sound_channel_mask_first(active_mask)) >= 0) {
        record = &bank->channel[channel_index];

        if(sound_channel_matches_group(record, group, owner_id)) {
            result |= UINT64_C(1) << channel_index;
        }

        active_mask &= ~(UINT64_C(1) << channel_index);
    }

    return result;
}

/*
* Caskey, Damon V.
* 2026-08-08
*
* Stop every active channel sharing at least one requested
* group and matching the requested owner. Stream resources
* are closed after releasing the audio callback lock.
*/
size_t sound_group_stop(
    sound_group_mask_t group,
    const uint64_t owner_id
) {
    uint64_t match_mask[SOUND_CHANNEL_BANK_COUNT] = { 0 };
    uint64_t active_bank_mask;
    size_t match_count = 0;
    int bank_index;

    group &= SOUND_GROUP_ALL;
    if(!group) {
        return 0;
    }

    SB_lock_audio();
    active_bank_mask = sound_channel_pool.active_bank_mask;

    while((bank_index = sound_channel_mask_first(active_bank_mask)) >= 0) {
        s_sound_channel_bank *bank = sound_channel_pool.bank[bank_index];
        uint64_t channel_mask;
        int channel_index;

        channel_mask = sound_group_get_bank_match_mask(
            bank,
            group,
            owner_id
        );
        match_mask[bank_index] = channel_mask;

        while((channel_index = sound_channel_mask_first(channel_mask)) >= 0) {
            const int channel =
                bank_index * (int)SOUND_CHANNEL_BANK_SIZE + channel_index;

            sound_channel_pool_deactivate(&sound_channel_pool, channel);
            match_count++;
            channel_mask &= ~(UINT64_C(1) << channel_index);
        }

        active_bank_mask &= ~(UINT64_C(1) << bank_index);
    }
    SB_unlock_audio();

    for(bank_index = 0;
        bank_index < (int)SOUND_CHANNEL_BANK_COUNT;
        bank_index++) {
        s_sound_channel_bank *bank = sound_channel_pool.bank[bank_index];
        uint64_t channel_mask = match_mask[bank_index];
        int channel_index;

        if(!bank) {
            continue;
        }

        while((channel_index = sound_channel_mask_first(channel_mask)) >= 0) {
            const int channel =
                bank_index * (int)SOUND_CHANNEL_BANK_SIZE + channel_index;

            sound_stream_close_channel(
                channel,
                &bank->channel[channel_index]
            );
            channel_mask &= ~(UINT64_C(1) << channel_index);
        }
    }

    return match_count;
}

/*
* Caskey, Damon V.
* 2026-08-08
*
* Pause or resume active channels by sound group and owner.
* Return the number of matching channels.
*/
size_t sound_group_pause(
    const int toggle,
    sound_group_mask_t group,
    const uint64_t owner_id
) {
    uint64_t active_bank_mask;
    size_t match_count = 0;
    int bank_index;

    group &= SOUND_GROUP_ALL;
    if(!group) {
        return 0;
    }

    SB_lock_audio();
    active_bank_mask = sound_channel_pool.active_bank_mask;

    while((bank_index = sound_channel_mask_first(active_bank_mask)) >= 0) {
        s_sound_channel_bank *bank = sound_channel_pool.bank[bank_index];
        uint64_t channel_mask = sound_group_get_bank_match_mask(
            bank,
            group,
            owner_id
        );
        int channel_index;

        while((channel_index = sound_channel_mask_first(channel_mask)) >= 0) {
            const int channel =
                bank_index * (int)SOUND_CHANNEL_BANK_SIZE + channel_index;

            sound_channel_pool_pause(
                &sound_channel_pool,
                channel,
                toggle
            );
            match_count++;
            channel_mask &= ~(UINT64_C(1) << channel_index);
        }

        active_bank_mask &= ~(UINT64_C(1) << bank_index);
    }
    SB_unlock_audio();

    return match_count;
}

/*
* Caskey, Damon V.
* 2026-08-08
*
* Seek matching active channels to one PCM frame. Candidate
* masks are captured under lock; stream reconfiguration uses
* the normal per-channel synchronization path afterward.
*/
size_t sound_group_set_position(
    sound_group_mask_t group,
    const uint64_t owner_id,
    const uint64_t sample_position
) {
    uint64_t match_mask[SOUND_CHANNEL_BANK_COUNT] = { 0 };
    uint64_t active_bank_mask;
    size_t success_count = 0;
    int bank_index;

    group &= SOUND_GROUP_ALL;
    if(!group) {
        return 0;
    }

    SB_lock_audio();
    active_bank_mask = sound_channel_pool.active_bank_mask;

    while((bank_index = sound_channel_mask_first(active_bank_mask)) >= 0) {
        match_mask[bank_index] = sound_group_get_bank_match_mask(
            sound_channel_pool.bank[bank_index],
            group,
            owner_id
        );
        active_bank_mask &= ~(UINT64_C(1) << bank_index);
    }
    SB_unlock_audio();

    for(bank_index = 0;
        bank_index < (int)SOUND_CHANNEL_BANK_COUNT;
        bank_index++) {
        uint64_t channel_mask = match_mask[bank_index];
        int channel_index;

        while((channel_index = sound_channel_mask_first(channel_mask)) >= 0) {
            const int channel =
                bank_index * (int)SOUND_CHANNEL_BANK_SIZE + channel_index;

            if(sound_set_channel_position(channel, sample_position)) {
                success_count++;
            }

            channel_mask &= ~(UINT64_C(1) << channel_index);
        }
    }

    return success_count;
}

void sound_volume_sample(int channel, int lvolume, int rvolume) {
    channelstruct *record;

    SB_lock_audio();
    record = sound_channel_pool_get(&sound_channel_pool, channel);
    if(!record) {
        SB_unlock_audio();
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
    SB_unlock_audio();
}

/*
* Caskey, Damon V.
* 2026-08-02
*
* Restart a streamed channel from a requested frame
* while preserving its loop and pause state.
*/
static bool sound_reconfigure_streamed_channel(
    int channel,
    channelstruct *record,
    uint64_t start_frame,
    uint64_t loop_start_frame
) {
    s_soundcache *cache;
    int active_state;
    int paused;

    SB_lock_audio();
    if(!record ||
       record->samplenum < 0 ||
       record->samplenum >= sound_cached ||
       !soundcache[record->samplenum].stream ||
       !sound_channel_pool_is_active(&sound_channel_pool, channel)) {
        SB_unlock_audio();
        return false;
    }

    cache = &soundcache[record->samplenum];
    if(start_frame >= cache->sample.framecount ||
       loop_start_frame >= cache->sample.framecount ||
       start_frame > SOUND_SAMPLE_FIXED_MAX_INTEGER ||
       loop_start_frame > SOUND_SAMPLE_FIXED_MAX_INTEGER) {
        SB_unlock_audio();
        return false;
    }

    active_state = record->active;
    paused = record->paused;

    sound_channel_pool_deactivate(&sound_channel_pool, channel);
    SB_unlock_audio();
    sound_stream_close_channel(channel, record);

    if(!sound_stream_open_channel(
        channel,
        record,
        cache,
        start_frame,
        active_state == CHANNEL_LOOPING,
        loop_start_frame
    )) {
        return false;
    }

    record->fp_samplepos = SOUND_SAMPLE_INT_TO_FIX(start_frame);
    record->fp_loop_start = SOUND_SAMPLE_INT_TO_FIX(loop_start_frame);
    SB_lock_audio();
    sound_channel_pool_activate(&sound_channel_pool, channel, active_state);

    if(paused) {
        sound_channel_pool_pause(&sound_channel_pool, channel, 1);
    }
    SB_unlock_audio();

    return true;
}

/*
* Caskey, Damon V.
* 2026-08-02
*
* Set the automatic-loop frame. Streamed playback
* is requeued so prefetched data uses the new loop.
*/
bool sound_set_channel_loop_offset(int channel, uint64_t loop_start_frame) {
    channelstruct *record;
    samplestruct *sample;
    uint64_t sample_position;
    bool reconfigure_stream;

    SB_lock_audio();
    record = sound_channel_pool_get(&sound_channel_pool, channel);
    if(!record || record->samplenum < 0 || record->samplenum >= sound_cached) {
        SB_unlock_audio();
        return false;
    }

    sample = &soundcache[record->samplenum].sample;
    if(loop_start_frame >= sample->framecount ||
       loop_start_frame > SOUND_SAMPLE_FIXED_MAX_INTEGER) {
        SB_unlock_audio();
        return false;
    }

    reconfigure_stream = soundcache[record->samplenum].stream &&
                         sound_channel_pool_is_active(&sound_channel_pool, channel);
    if(reconfigure_stream) {
        sample_position = SOUND_SAMPLE_FIX_TO_INT(record->fp_samplepos);
    } else {
        record->fp_loop_start = SOUND_SAMPLE_INT_TO_FIX(loop_start_frame);
    }
    SB_unlock_audio();

    if(reconfigure_stream) {
        return sound_reconfigure_streamed_channel(
            channel,
            record,
            sample_position,
            loop_start_frame
        );
    }
    return true;
}

/*
* Caskey, Damon V.
* 2026-08-02
*
* Set the raw fixed-point playback period. Zero is
* rejected because it would stall the channel.
*/
bool sound_set_channel_period(int channel, uint64_t period) {
    channelstruct *record;

    SB_lock_audio();
    record = sound_channel_pool_get(&sound_channel_pool, channel);
    if(!record || period == 0) {
        SB_unlock_audio();
        return false;
    }

    record->fp_period = period;
    SB_unlock_audio();
    return true;
}

/*
* Caskey, Damon V.
* 2026-08-04
*
* Update replacement priority without racing the
* callback's active channel traversal.
*/
bool sound_set_channel_priority(int channel, unsigned int priority) {
    channelstruct *record;

    SB_lock_audio();
    record = sound_channel_pool_get(&sound_channel_pool, channel);
    if(!record) {
        SB_unlock_audio();
        return false;
    }

    record->priority = priority;
    SB_unlock_audio();
    return true;
}

/*
* Caskey, Damon V.
* 2026-08-04
*
* Set one spatial volume without requiring script code
* to read the opposite callback-owned channel first.
*/
bool sound_set_channel_volume(int channel, unsigned int spatial_channel, int volume) {
    channelstruct *record;

    if(spatial_channel >= SOUND_SPATIAL_CHANNEL_MAX) {
        return false;
    }
    if(volume < 0) {
        volume = 0;
    }
    if(volume > MAX_SAMPLE_VOLUME * 8) {
        volume = MAX_SAMPLE_VOLUME * 8;
    }

    SB_lock_audio();
    record = sound_channel_pool_get(&sound_channel_pool, channel);
    if(!record) {
        SB_unlock_audio();
        return false;
    }

    record->volume[spatial_channel] = volume;
    SB_unlock_audio();
    return true;
}

/*
* Caskey, Damon V.
* 2026-08-02
*
* Seek a resident or streamed sound object to a PCM
* frame without changing automatic-loop behavior.
*/
bool sound_set_channel_position(int channel, uint64_t sample_position) {
    channelstruct *record;
    samplestruct *sample;
    uint64_t loop_start_frame;
    bool reconfigure_stream;

    SB_lock_audio();
    record = sound_channel_pool_get(&sound_channel_pool, channel);
    if(!record || record->samplenum < 0 || record->samplenum >= sound_cached) {
        SB_unlock_audio();
        return false;
    }

    sample = &soundcache[record->samplenum].sample;
    if(sample_position >= sample->framecount ||
       sample_position > SOUND_SAMPLE_FIXED_MAX_INTEGER) {
        SB_unlock_audio();
        return false;
    }

    reconfigure_stream = soundcache[record->samplenum].stream &&
                         sound_channel_pool_is_active(&sound_channel_pool, channel);
    if(reconfigure_stream) {
        loop_start_frame = SOUND_SAMPLE_FIX_TO_INT(record->fp_loop_start);
    } else {
        record->fp_samplepos = SOUND_SAMPLE_INT_TO_FIX(sample_position);
    }
    SB_unlock_audio();

    if(reconfigure_stream) {
        return sound_reconfigure_streamed_channel(
            channel,
            record,
            sample_position,
            loop_start_frame
        );
    }
    return true;
}

/*
* Caskey, Damon V.
* 2026-08-02
*
* Set the channel volume divisor while preventing
* division by zero in the audio callback.
*/
bool sound_set_channel_volume_divisor(int channel, int volume_divisor) {
    channelstruct *record;

    SB_lock_audio();
    record = sound_channel_pool_get(&sound_channel_pool, channel);
    if(!record || volume_divisor < 1) {
        SB_unlock_audio();
        return false;
    }

    record->volume_divisor = volume_divisor;
    SB_unlock_audio();
    return true;
}

int sound_getpos_sample(int channel) {
    channelstruct *record;
    uint64_t sample_position;

    SB_lock_audio();
    record = sound_channel_pool_get(&sound_channel_pool, channel);
    if(!record) {
        SB_unlock_audio();
        return 0;
    }
    sample_position = SOUND_SAMPLE_FIX_TO_INT(record->fp_samplepos);
    SB_unlock_audio();
    return sample_position > (uint64_t)INT_MAX ? INT_MAX : (int)sample_position;
}

/////////////////////// Unified channel music ////////////////////////////////

/*
* Caskey, Damon V.
* 2026-08-05
*
* Return an active channel record while the audio
* callback is excluded by the caller.
*/
static channelstruct *sound_active_channel_record_locked(int channel) {
    channelstruct *record;

    record = sound_channel_pool_get(&sound_channel_pool, channel);
    if(!record || !sound_channel_pool_is_active(&sound_channel_pool, channel)) {
        return NULL;
    }
    return record;
}

/*
* Caskey, Damon V.
* 2026-08-05
*
* Route WAV or Ogg Vorbis music through the generic
* streamed sample path on soft-reserved channel zero.
*/
static int sound_open_sample_music(
    char *filename,
    char *packname,
    int volume,
    int loop,
    u32 music_offset
) {
    channelstruct *record;
    samplestruct *sample;
    uint64_t loop_start_frame;
    int sample_index;
    int channel;

    if(!mixing_inited || !mixing_active) {
        return 0;
    }

    sound_close_music();
    sample_index = sound_load_sample(filename, packname, false, true);
    if(sample_index < 0) {
        return 0;
    }

    sample = &soundcache[sample_index].sample;
    if(sample->file_type != SOUND_SAMPLE_FILE_TYPE_WAVE &&
       sample->file_type != SOUND_SAMPLE_FILE_TYPE_VORBIS) {
        return 0;
    }

    loop_start_frame = music_offset;

    channel = sound_play_sample_internal(
        sample_index,
        UINT_MAX,
        volume,
        volume,
        100,
        loop,
        1,
        0,
        loop_start_frame,
        SOUND_CHANNEL_MUSIC_DEFAULT,
        NULL
    );
    if(channel != SOUND_CHANNEL_MUSIC_DEFAULT) {
        return 0;
    }

    SB_lock_audio();
    record = sound_active_channel_record_locked(SOUND_CHANNEL_MUSIC_DEFAULT);
    if(!record) {
        SB_unlock_audio();
        sound_stop_sample(SOUND_CHANNEL_MUSIC_DEFAULT);
        return 0;
    }

    if(volume < 0) {
        volume = 0;
    }
    if(volume > MAX_SAMPLE_VOLUME * 8) {
        volume = MAX_SAMPLE_VOLUME * 8;
    }

    record->volume[SOUND_SPATIAL_CHANNEL_LEFT] = volume;
    record->volume[SOUND_SPATIAL_CHANNEL_RIGHT] = volume;
    record->volume_divisor = MAX_MUSIC_VOLUME;
    SB_unlock_audio();
    return 1;
}

/*
* Caskey, Damon V.
* 2026-08-05
*
* Open a live 16-bit PCM producer on an explicit
* generic sound channel. The returned play id guards
* producer writes against later channel replacement.
*/
int sound_open_channel_pcm_stream(
    int channel,
    int frequency,
    int channels,
    int volume
) {
    channelstruct *record;
    unsigned int bank_index;
    int play_id;

    if(!mixing_inited ||
       !mixing_active ||
       channel < 0 ||
       (unsigned int)channel >= SOUND_CHANNEL_COUNT_MAX ||
       frequency <= 0 ||
       (channels != CHANNEL_TYPE_MONO && channels != CHANNEL_TYPE_STEREO)) {
        return -1;
    }

    bank_index = (unsigned int)channel / SOUND_CHANNEL_BANK_SIZE;
    SB_lock_audio();
    if(!sound_channel_pool_allocate_bank(&sound_channel_pool, bank_index)) {
        SB_unlock_audio();
        return -1;
    }

    record = sound_channel_pool_get(&sound_channel_pool, channel);
    if(!record) {
        SB_unlock_audio();
        return -1;
    }
    sound_channel_pool_deactivate(&sound_channel_pool, channel);
    SB_unlock_audio();

    sound_stream_close_channel(channel, record);
    sound_channel_record_clear(record);
    if(!sound_stream_configure_push(
        &record->stream,
        (size_t)channels * sizeof(int16_t)
    )) {
        sound_channel_record_clear(record);
        return -1;
    }

    if(volume < 0) {
        volume = 0;
    }
    if(volume > MAX_SAMPLE_VOLUME) {
        volume = MAX_SAMPLE_VOLUME;
    }

    record->samplenum = -1;
    record->priority = UINT_MAX;
    record->bits = 16;
    record->frequency = frequency;
    record->channels = channels;
    record->stream_source = SOUND_CHANNEL_STREAM_SOURCE_PUSH;
    record->fp_samplepos = 0;
    record->fp_period = sound_sample_period_calculate(100, frequency);
    record->volume[SOUND_SPATIAL_CHANNEL_LEFT] = volume;
    record->volume[SOUND_SPATIAL_CHANNEL_RIGHT] = volume;
    record->volume_divisor = MAX_SAMPLE_VOLUME;
    record->playid = ++audio_global.sample_play_id;
    play_id = record->playid;

    SB_lock_audio();
    sound_channel_pool_stream(&sound_channel_pool, channel, 1);
    sound_channel_pool_activate(&sound_channel_pool, channel, CHANNEL_PLAYING);
    SB_unlock_audio();
    return play_id;
}

/*
* Caskey, Damon V.
* 2026-08-05
*
* Copy one live PCM block into a generic channel's
* rotating queue. Direct SDL locking is used because
* producers such as WebM publish from decoder threads.
*
* Returns 1 when queued, 0 while all buffers are ready,
* and -1 after replacement or malformed input.
*/
int sound_queue_channel_pcm_stream(
    int channel,
    int play_id,
    const void *pcm,
    uint64_t frame_count,
    int terminal
) {
    channelstruct *record;
    int result;

    SB_lock_audio_direct();
    record = sound_channel_pool_get(&sound_channel_pool, channel);
    if(play_id < 0 ||
       !sound_channel_pool_is_active(&sound_channel_pool, channel) ||
       !record ||
       record->playid != play_id ||
       record->stream_source != SOUND_CHANNEL_STREAM_SOURCE_PUSH) {
        SB_unlock_audio_direct();
        return -1;
    }

    result = sound_stream_push_locked(
        &record->stream,
        pcm,
        frame_count,
        terminal
    );
    SB_unlock_audio_direct();
    return result;
}

/*
* Caskey, Damon V.
* 2026-08-05
*
* Close a live PCM producer only if its play id still
* owns the requested channel. Later replacement
* playback remains untouched.
*/
void sound_close_channel_pcm_stream(int channel, int play_id) {
    channelstruct *record;

    SB_lock_audio();
    record = sound_channel_pool_get(&sound_channel_pool, channel);
    if(play_id >= 0 &&
       record &&
       record->playid == play_id &&
       record->stream_source == SOUND_CHANNEL_STREAM_SOURCE_PUSH) {
        sound_channel_pool_deactivate(&sound_channel_pool, channel);
        sound_stream_close_channel(channel, record);
    }
    SB_unlock_audio();
}

/*
* Caskey, Damon V.
* 2026-08-06
*
* Try the supplied music path exactly before appending
* each supported extension. Fallback storage is
* sized from the complete source path so long filenames
* cannot overflow a fixed buffer.
*/
int sound_open_music(char *filename, char *packname, int volume, int loop, u32 music_offset) {
    static const char fallback_extensions[][5] = {
        ".ogg",
        ".oga",
        ".wav"
    };
    char *fallback_filename;
    size_t extension_index;
    size_t filename_length;

    if(!filename || !packname) {
        return 0;
    }
#ifdef VERBOSE
    printf("trying to open music file %s from %s, vol %d, loop %d, ofs %u\n", filename, packname, volume, loop, music_offset);
#endif

    if(sound_open_sample_music(filename, packname, volume, loop, music_offset)) {
        return 1;
    }

    filename_length = strlen(filename);
    if(filename_length > SIZE_MAX - sizeof(fallback_extensions[0])) {
        return 0;
    }

    fallback_filename = malloc(filename_length + sizeof(fallback_extensions[0]));
    if(!fallback_filename) {
        return 0;
    }

    memcpy(fallback_filename, filename, filename_length);
    for(extension_index = 0;
        extension_index < sizeof(fallback_extensions) / sizeof(fallback_extensions[0]);
        extension_index++) {
        memcpy(
            fallback_filename + filename_length,
            fallback_extensions[extension_index],
            sizeof(fallback_extensions[extension_index])
        );

        if(sound_open_sample_music(
            fallback_filename,
            packname,
            volume,
            loop,
            music_offset
        )) {
            free(fallback_filename);
            return 1;
        }
    }

    free(fallback_filename);
    return 0;
}

void sound_close_music()
{
    sound_stop_sample(SOUND_CHANNEL_MUSIC_DEFAULT);
}

void sound_update_music()
{
    sound_update_streams();
}

int sound_query_music(char *artist, char *title)
{
    channelstruct *record;
    samplestruct *sample = NULL;

    SB_lock_audio();
    record = sound_active_channel_record_locked(SOUND_CHANNEL_MUSIC_DEFAULT);
    if(!record) {
        SB_unlock_audio();
        return 0;
    }

    if(record->samplenum >= 0 && record->samplenum < sound_cached) {
        sample = &soundcache[record->samplenum].sample;
    }
    if(artist) {
        strcpy(artist, sample ? sample->artist : "");
    }
    if(title) {
        strcpy(title, sample ? sample->title : "");
    }
    SB_unlock_audio();
    return 1;
}

void sound_music_tempo(int music_tempo)
{
    channelstruct *record;

    SB_lock_audio();
    record = sound_active_channel_record_locked(SOUND_CHANNEL_MUSIC_DEFAULT);
    if(record) {
        record->fp_period = music_tempo > 0
            ? sound_sample_period_calculate(
                (unsigned int)music_tempo,
                record->frequency
            )
            : 0;
    }
    SB_unlock_audio();
}

void sound_volume_music(int left, int right)
{
    channelstruct *record;

    if(left < 0) {
        left = 0;
    }
    if(right < 0) {
        right = 0;
    }
    if(left > MAX_SAMPLE_VOLUME * 8) {
        left = MAX_SAMPLE_VOLUME * 8;
    }
    if(right > MAX_SAMPLE_VOLUME * 8) {
        right = MAX_SAMPLE_VOLUME * 8;
    }

    SB_lock_audio();
    record = sound_active_channel_record_locked(SOUND_CHANNEL_MUSIC_DEFAULT);
    if(record) {
        record->volume[SOUND_SPATIAL_CHANNEL_LEFT] = left;
        record->volume[SOUND_SPATIAL_CHANNEL_RIGHT] = right;
        record->volume_divisor = MAX_MUSIC_VOLUME;
    }
    SB_unlock_audio();
}

void sound_pause_music(int toggle)
{
    SB_lock_audio();
    sound_channel_pool_pause(
        &sound_channel_pool,
        SOUND_CHANNEL_MUSIC_DEFAULT,
        toggle
    );
    SB_unlock_audio();
}

void sound_stop_playback() {
    if(!mixing_inited) {
        return;
    }
    if(!mixing_active) {
        return;
    }
    sound_stopall_sample(true);
    SB_playstop();
    mixing_active = 0;
}

int sound_start_playback() {
    /* Prefer 24-bit/48 kHz unconditionally. Older backends that cannot
     * open that logical SDL format fall back automatically, without a
     * creator-facing quality switch. */
    static const struct {
        int bits;
        int frequency;
    } output_candidates[] = {
        { SOUND_OUTPUT_BITS_DEFAULT,  SOUND_OUTPUT_FREQUENCY_DEFAULT },
        { SOUND_OUTPUT_BITS_FALLBACK, SOUND_OUTPUT_FREQUENCY_DEFAULT },
        { SOUND_OUTPUT_BITS_DEFAULT,  SOUND_OUTPUT_FREQUENCY_FALLBACK },
        { SOUND_OUTPUT_BITS_FALLBACK, SOUND_OUTPUT_FREQUENCY_FALLBACK }
    };
    size_t candidate_index;

    if(!mixing_inited) {
        return 0;
    }

    sound_stop_playback();

    samplesplayed = 0;

    sound_stopall_sample(true);
    SB_playstop();

    for(candidate_index = 0;
        candidate_index < sizeof(output_candidates) / sizeof(output_candidates[0]);
        candidate_index++) {
        playbits = output_candidates[candidate_index].bits;
        playfrequency = output_candidates[candidate_index].frequency;

        if(SB_playstart(playbits, playfrequency)) {
            mixing_active = 1;
            return 1;
        }
    }

    return 0;
}

// Stop everything and free used memory
void sound_exit() {
    sound_stop_playback();
    sound_stopall_sample(true);
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

    if(!sound_channel_pool_init(&sound_channel_pool) ||
       !sound_channel_pool_reserve_mask(&sound_channel_pool, 0, UINT64_C(1))) {
        sound_channel_pool_destroy(&sound_channel_pool);
        return 0;
    }

    /* Channel zero is ignored by automatic allocation but remains forceable. */

    /* Allocate the maximum amount ever needed for one mixing pass. */
    if((mixbuf = malloc(MIXBUF_SAMPLE_COUNT * sizeof(*mixbuf))) == NULL) {
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

    SB_lock_audio();
    msecs = 1000 * samplesplayed / playfrequency;
    samplesplayed -= msecs * playfrequency / 1000;
    SB_unlock_audio();

    return msecs;
}

int maxchannels() {
    return (int)SOUND_CHANNEL_COUNT_MAX;
}
