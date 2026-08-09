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
	int bytes_per_frame;

	spec.channels = 2;

	switch(bits)
	{
		case 8:
			spec.format = AUDIO_U8;
			break;

		case 16:
			spec.format = AUDIO_S16SYS;
			break;

		/* SDL has no packed 24-bit format. Retain 24 meaningful bits
		 * in a signed 32-bit transport sample instead. */
		case 24:
			spec.format = AUDIO_S32SYS;
			break;

		default:
			return 0;
	}

	spec.freq = samplerate;
	bytes_per_frame = (SDL_AUDIO_BITSIZE(spec.format) / 8) * spec.channels;
	if(bytes_per_frame <= 0 || buffsize < bytes_per_frame)
	{
		return 0;
	}

	spec.samples = (Uint16)(buffsize / bytes_per_frame);
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
		SB_lock_audio_direct();
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
		SB_unlock_audio_direct();
	}
}

/*
* Caskey, Damon V.
* 2026-08-05
*
* Lock the SDL device without participating in the
* main-thread nesting counter. Live PCM producer
* threads use this pair exactly once per publication.
*/
void SB_lock_audio_direct()
{
	if(started)
	{
		SDL_LockAudioDevice(audio_dev);
	}
}

void SB_unlock_audio_direct()
{
	if(started)
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
