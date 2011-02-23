#ifndef SOUNDMIX_H
#define SOUNDMIX_H

/*
**	Sound mixer.
**	Now supports ADPCM instead of MP3 (costs less CPU time).
**
**	Also plays WAV files (unsigned, mono, both 8-bit and 16-bit).
*/

void sound_stop_playback();
int sound_start_playback(int stereo, int bits, int frequency);
void sound_exit();
int sound_init(int channels);


// Returns interval in milliseconds
unsigned gamelib_long sound_getinterval();


void sound_unload_sample(int which);
int sound_load_sample(char *filename, char *packfilename);
int sound_play_sample(int samplenum, unsigned int priority, int lvolume, int rvolume, unsigned int speed);
int sound_loop_sample(int samplenum, unsigned int priority, int lvolume, int rvolume, unsigned int speed);
void sound_stop_sample(int channel);
void sound_stopall_sample();
void sound_volume_sample(int channel, int lvolume, int rvolume);
int sound_getpos_sample(int channel);




int sound_async_open_music(char * filename, char * packfile, int defaultvolume, int loop);
int sound_was_music_opened(void);
void sound_close_music();
void sound_update_music();
void sound_volume_music(int left, int right);
int sound_query_music(char *artist, char *title);
void sound_pause_music(int toggle);

#endif
