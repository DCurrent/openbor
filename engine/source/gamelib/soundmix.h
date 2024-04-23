/*
 * OpenBOR - http://www.chronocrash.com
 * -----------------------------------------------------------------------
 * All rights reserved, see LICENSE in OpenBOR root for details.
 *
 * Copyright (c)  OpenBOR Team
 */

#ifndef SOUNDMIX_H
#define SOUNDMIX_H

#include "types.h"
#include "List.h"

/*
**	Sound mixer.
**	Now supports ADPCM instead of MP3 (costs less CPU time).
**
**	Also plays WAV files (unsigned, mono, both 8-bit and 16-bit).
*/

// 20:12 fixed-point conversion macros.
// The maximum size of a sound is linked directly
// to the range of the fixed-point variables!
// Kratus (01-2024) Increased the length limit for samples, from 8 seconds to 1 minute
#define		INT_TO_FIX(i)		((unsigned int)i<<4)
#define		FIX_TO_INT(f)		((unsigned int)f>>4)
//#define		MAX_SOUND_LEN		0x4ffffb
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
    unsigned int sound_length_max; // MAX_SOUND_LEN; Maximum sound length in samples
    const unsigned int music_buffers_count; // MUSIC_NUM_BUFFERS
    const unsigned int music_buffer_size;    // MUSIC_BUF_SIZE - In samples
} s_sound_parameters;

typedef struct
{
    int            active;		 // 1 = play, 2 = loop
    int				paused;
    int            samplenum;	 // Index of sound playing
    unsigned int   priority;	 // Used for SFX
    int				playid;
    int            volume[SOUND_SPATIAL_CHANNEL_MAX];	 // Stereo :)
    int            channels;
    unsigned int   fp_samplepos; // Position (fixed-point)
    unsigned int   fp_period;	 // Period (fixed-point)
} channelstruct;

typedef struct
{
    void* sampleptr;
    int			   soundlen;	 // Length in samples
    int            bits;		 // 8/16 bit
    int            frequency;    // 11025 * 1,2,4
    int            channels;
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
    unsigned int   fp_playto[MUSIC_NUM_BUFFERS];
    unsigned int   fp_samplepos;  // Position (fixed-point)
    unsigned int   fp_period;	  // Period (fixed-point)
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
int sound_init(int channels);



// Returns interval in milliseconds
u32 sound_getinterval();
int sound_load_sample(char *filename, char *packfilename, int iLog);
int sound_reload_sample(int index);
void sound_unload_sample(int index);
void sound_unload_all_samples();
int sound_query_channel(int playid);
int sound_id(int channel);
int sound_is_active(int channel);
int sound_play_sample(int samplenum, unsigned int priority, int lvolume, int rvolume, unsigned int speed);
int sound_loop_sample(int samplenum, unsigned int priority, int lvolume, int rvolume, unsigned int speed);
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
void update_sample(unsigned char *buf, int size);
int maxchannels(void);

#endif
