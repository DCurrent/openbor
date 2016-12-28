/*
 * OpenBOR - http://www.chronocrash.com
 * -----------------------------------------------------------------------
 * All rights reserved, see LICENSE in OpenBOR root for details.
 *
 * Copyright (c) 2004 - 2014 OpenBOR Team
 */

#ifndef SOUNDMIX_H
#define SOUNDMIX_H

#include "types.h"

/*
**	Sound mixer.
**	Now supports ADPCM instead of MP3 (costs less CPU time).
**
**	Also plays WAV files (unsigned, mono, both 8-bit and 16-bit).
*/

// 20:12 fixed-point conversion macros.
// The maximum size of a sound is linked directly
// to the range of the fixed-point variables!
#define		INT_TO_FIX(i)		((unsigned int)i<<12)
#define		FIX_TO_INT(f)		((unsigned int)f>>12)
#define		MAX_SOUND_LEN		0xFFFFF
#define		CHANNEL_PLAYING		1
#define		CHANNEL_LOOPING		2
#define		MUSIC_NUM_BUFFERS	4
#define		MUSIC_BUF_SIZE		(16*1024)	// In samples
#define		SOUND_MONO			1
#define		SOUND_STEREO		2

typedef struct
{
    int            active;
    int            paused;
    short 		   *buf[MUSIC_NUM_BUFFERS];
    unsigned int   fp_playto[MUSIC_NUM_BUFFERS];
    unsigned int   fp_samplepos;  // Position (fixed-point)
    unsigned int   fp_period;	  // Period (fixed-point)
    int			   playing_buffer;
    int            volume[2];
    int            channels;
} musicchannelstruct;

extern musicchannelstruct musicchannel;
extern int playfrequency;

void sound_stop_playback();
int sound_start_playback(int bits, int frequency);
void sound_exit();
int sound_init(int channels);

extern int sample_play_id;

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

#ifdef DC
int sound_was_music_opened();
#endif

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
