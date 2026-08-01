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
**	Also plays WAV files (mono or stereo, 8-bit, 16-bit, or 24-bit).
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
    SOUND_FILE_TYPE_VORBIS = 1
} e_sound_file_type;

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
    
    uint64_t       soundbytes;  // Raw bytes loaded from the WAV data chunk.    
    uint64_t       soundlen;    // Scalar PCM sample units retained as cache metadata.
    uint64_t       framecount;  // Complete PCM frames loaded from the WAV data chunk.
    int            bits;
    int            frequency;
    int            channels;
    int            blockalign;  // Bytes in one complete PCM frame.
} samplestruct;

typedef struct
{
    samplestruct  sample;
    int index;
    char* filename;
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
int sound_load_sample(char *filename, char *packfilename, bool iLog);
bool sound_reload_sample(int index);
void sound_unload_sample(int index);
void sound_unload_all_samples();
int sound_query_channel(int playid);
int sound_id(int channel);
int sound_is_active(int channel);
int sound_play_sample(int samplenum, unsigned int priority, int lvolume, int rvolume, unsigned int speed);
int sound_loop_sample(int samplenum, unsigned int priority, int lvolume, int rvolume, unsigned int speed);
// loop_start_frame is the PCM frame used after the first pass reaches the end.
int sound_loop_sample_offset(int samplenum, unsigned int priority, int lvolume, int rvolume, unsigned int speed, uint64_t loop_start_frame);
void sound_stop_sample(int channel);
void sound_stopall_sample();
void sound_pause_sample(int toggle);
void sound_pause_single_sample(int toggle, int channel);
void sound_volume_sample(int channel, int lvolume, int rvolume);
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
