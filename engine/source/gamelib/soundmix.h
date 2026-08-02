/*
 * OpenBOR - http://www.chronocrash.com
 * -----------------------------------------------------------------------
 * All rights reserved, see LICENSE in OpenBOR root for details.
 *
 * Copyright (c)  OpenBOR Team
 */

#ifndef SOUNDMIX_H
#define SOUNDMIX_H

#include <stddef.h>
#include <stdint.h>

#include "types.h"
#include "List.h"
#include "sound_channel.h"

/*
**	Sound mixer.
**	Now supports ADPCM instead of MP3 (costs less CPU time).
**
**	Also plays WAV and Ogg Vorbis samples.
*/

/*
* Music buffers use a 32-bit frame cursor. In-memory effects use a separate
* 64-bit frame cursor so long samples do not wrap during playback.
*/
#define		MUSIC_SAMPLE_FIXED_SHIFT		16U
#define		MUSIC_SAMPLE_FIXED_ONE			((unsigned int)1U << MUSIC_SAMPLE_FIXED_SHIFT)
#define		INT_TO_FIX(integer_value)		((unsigned int)(integer_value) << MUSIC_SAMPLE_FIXED_SHIFT)
#define		FIX_TO_INT(fixed_value)		((unsigned int)(fixed_value) >> MUSIC_SAMPLE_FIXED_SHIFT)

#define		SOUND_MUSIC_FREQUENCY_MIN		11025
#define		SOUND_MUSIC_FREQUENCY_MAX		48000
#define		SOUND_OUTPUT_BITS_DEFAULT		16
#define		SOUND_OUTPUT_FREQUENCY_DEFAULT	44100
#define		CHANNEL_PLAYING		1
#define		CHANNEL_LOOPING		2
#define		MUSIC_NUM_BUFFERS	4
#define		MUSIC_BUF_SIZE		(16*1024)	// In samples

typedef enum e_sound_file_type
{
    SOUND_FILE_TYPE_NONE = -1,
    SOUND_FILE_TYPE_ADPCM = 0,
    SOUND_FILE_TYPE_SAMPLE = 1,
    SOUND_FILE_TYPE_VORBIS = SOUND_FILE_TYPE_SAMPLE /* Legacy script alias. */
} e_sound_file_type;

typedef enum e_sound_sample_file_type
{
    SOUND_SAMPLE_FILE_TYPE_NONE = 0,
    SOUND_SAMPLE_FILE_TYPE_WAVE,
    SOUND_SAMPLE_FILE_TYPE_VORBIS
} e_sound_sample_file_type;

typedef enum e_channel_type
{
    CHANNEL_TYPE_MONO = 1,
    CHANNEL_TYPE_STEREO = 2
} e_channel_type;

typedef enum e_sound_spatial_channel
{
    SOUND_SPATIAL_CHANNEL_LEFT  = 0,
    SOUND_SPATIAL_CHANNEL_RIGHT = 1,
    SOUND_SPATIAL_CHANNEL_MAX   = 2
} e_channel_index;

typedef struct s_sound_parameters {
    /*
    * Maximum sample data bytes. UINT64_MAX means no engine cap.
    */
    uint64_t sound_length_max;
    const unsigned int music_buffers_count; /* MUSIC_NUM_BUFFERS */
    const unsigned int music_buffer_size;   /* MUSIC_BUF_SIZE - In samples */
} s_sound_parameters;

typedef struct
{
    void* sampleptr;
    
    uint64_t       soundbytes;  // Raw PCM byte count.
    uint64_t       soundlen;    // Scalar PCM sample units retained as cache metadata.
    uint64_t       framecount;  // Complete PCM frames in the playback range.
    uint64_t       data_offset; // PCM byte offset for directly streamed containers.
    int            bits;
    int            frequency;
    int            channels;
    int            blockalign;  // Bytes in one complete PCM frame.
    e_sound_sample_file_type file_type;
    char           artist[64];
    char           title[64];
} samplestruct;

typedef struct
{
    samplestruct  sample;
    int index;
    char* filename;
    char* packfilename;
    bool stream;
} s_soundcache;

typedef struct
{
    int            active;
    int            paused;
    short 		   *buf[MUSIC_NUM_BUFFERS];
    unsigned int   fp_playto[MUSIC_NUM_BUFFERS]; // PCM frame count (fixed-point).
    unsigned int   fp_samplepos;  // PCM frame position (fixed-point).
    unsigned int   fp_period;	  // PCM frames advanced per output frame (fixed-point).
    int			   playing_buffer;
    int            volume[SOUND_SPATIAL_CHANNEL_MAX];
    e_channel_type channels;
    e_object_type  object_type;
} musicchannelstruct;

typedef struct s_audio_global
{
    List samplelist;
    s_soundcache* soundcache;
    int sound_cached;
    unsigned int sample_play_id;
} s_audio_global;

extern musicchannelstruct musicchannel;
extern s_audio_global audio_global;
extern int playfrequency;

void sound_stop_playback();
int sound_start_playback();
void sound_exit();
int sound_init(void);



// Returns interval in milliseconds
u32 sound_getinterval();
int sound_load_sample(char *filename, char *packfilename, bool log_errors, bool stream);
bool sound_reload_sample(int index);
void sound_unload_sample(int index);
void sound_unload_all_samples();
int sound_query_channel(int playid);
channelstruct *sound_get_channel_object(int channel);
int sound_get_channel_index(const channelstruct *record);
uint64_t sound_get_channel_bank_mask(e_sound_channel_bank_mask mask);
uint64_t sound_get_channel_mask(unsigned int bank_index, e_sound_channel_mask mask);
int sound_id(int channel);
int sound_is_active(int channel);
int sound_play_sample(int samplenum, unsigned int priority, int lvolume, int rvolume, unsigned int speed);
int sound_play_sample_offset(int samplenum, unsigned int priority, int lvolume, int rvolume, unsigned int speed, uint64_t start_frame);
int sound_loop_sample(int samplenum, unsigned int priority, int lvolume, int rvolume, unsigned int speed);
// start_frame applies once. loop_start_frame applies after each pass reaches the end.
int sound_loop_sample_offset(int samplenum, unsigned int priority, int lvolume, int rvolume, unsigned int speed, uint64_t start_frame, uint64_t loop_start_frame);
void sound_stop_sample(int channel);
void sound_stopall_sample();
void sound_pause_sample(int toggle);
void sound_pause_single_sample(int toggle, int channel);
void sound_volume_sample(int channel, int lvolume, int rvolume);
bool sound_set_channel_loop_offset(int channel, uint64_t loop_start_frame);
bool sound_set_channel_period(int channel, uint64_t period);
bool sound_set_channel_position(int channel, uint64_t sample_position);
bool sound_set_channel_volume_divisor(int channel, int volume_divisor);
int sound_getpos_sample(int channel);
void sound_music_channel_clear(musicchannelstruct* const music_channel);
int sound_open_music(char *filename, char *packname, int volume, int loop, u32 music_offset);
void sound_close_music();
void sound_update_music();
void sound_volume_music(int left, int right);
void sound_music_tempo(int music_tempo);
int sound_query_music(char *artist, char *title);
void sound_pause_music(int toggle);
unsigned int sound_music_period_calculate(int source_frequency, int tempo);
void update_sample(unsigned char *buf, int size);
int maxchannels(void);

#endif // SOUNDMIX_H
