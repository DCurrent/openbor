/*
 * OpenBOR - http://www.chronocrash.com
 * -----------------------------------------------------------------------
 * All rights reserved, see LICENSE in OpenBOR root for details.
 *
 * Copyright (c)  OpenBOR Team
 */

// A soundblaster interface. No bugs?

#include "sblaster.h"
#include "soundmix.h"
#include "sdlport.h"

static SDL_AudioSpec cspec;
static SDL_AudioDeviceID audio_dev;
static int sample_per_byte;
static int voicevol = 15;
static int buffsize = 4096;
static unsigned int audio_lock_depth;

static void callback(void *userdata, Uint8 *stream, int len)
{
	update_sample(stream,len);
	//writeToLogFile("sb call back\n");
}

static int started;

int SB_playstart(int bits, int samplerate)
{
	SDL_AudioSpec spec = { 0 };
	spec.channels = 2;
	spec.format = bits==16?AUDIO_S16SYS:AUDIO_U8;
	spec.freq = samplerate;
	sample_per_byte = 16/bits*spec.channels;
	spec.samples = buffsize/sample_per_byte/2;
	spec.userdata = NULL;
	spec.callback = callback;

	//if (SDL_OpenAudio(&spec,&cspec)<0) return 0;
	if (!(audio_dev = SDL_OpenAudioDevice(NULL, 0, &spec, &cspec, 0))) return 0; //SDL_AUDIO_ALLOW_FREQUENCY_CHANGE | SDL_AUDIO_ALLOW_CHANNELS_CHANGE
	started = 1;
	//SDL_PauseAudio(0);
	SDL_PauseAudioDevice(audio_dev, 0);

	return 1;
}

void SB_playstop()
{
	if(!started)
	{
		return;
	}

	started = 0;
	//SDL_CloseAudio();
    SDL_CloseAudioDevice(audio_dev);
	audio_dev = 0;
	audio_lock_depth = 0;
}

/*
* Caskey, Damon V.
* 2026-08-04
*
* Exclude the SDL callback while the main thread
* publishes or tears down shared mixer state. Nested
* engine helpers share one device lock.
*/
void SB_lock_audio()
{
	if(!started)
	{
		return;
	}

	if(audio_lock_depth++ == 0)
	{
		SDL_LockAudioDevice(audio_dev);
	}
}

void SB_unlock_audio()
{
	if(!started || audio_lock_depth == 0)
	{
		return;
	}

	if(--audio_lock_depth == 0)
	{
		SDL_UnlockAudioDevice(audio_dev);
	}
}

void SB_setvolume(char dev, char volume)
{
	if(dev == SB_VOICEVOL)
	{
		voicevol = volume;
	}
}


void SB_updatevolume(int volume)
{
}
