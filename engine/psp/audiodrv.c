/*
 * OpenBOR - http://www.LavaLit.com
 * -----------------------------------------------------------------------
 * Licensed under the BSD license, see LICENSE in OpenBOR root for details.
 *
 * Copyright (c) 2004 - 2011 OpenBOR Team
 */

#include <stdio.h>
#include <string.h>
#include <pspsdk.h>
#include <pspaudio.h>
#include "audiodrv.h"
#include "soundmix.h"

#define CHANNELS		1
#define SAMPLES			512
#define MAXVOLUME		0x8000

static   int audio_ready;
volatile int audio_terminate;
static   int audio_handles[CHANNELS];
static   int audio_volumes[CHANNELS][2];
static short audio_buffers[CHANNELS][2][SAMPLES*2];
static   int audio_thread_handle[CHANNELS];


static void (*audio_ChannelCallback[CHANNELS])(void *buf, unsigned len);


void audio_UpdateCallback(void *buf, unsigned len)
{
	update_sample(buf,len*2*2);
}

int audio_OutputPannedBlocking(unsigned int channel, unsigned int vol1, unsigned int vol2, void *buf)
{
	if(!audio_ready) return -1;
	if(channel >= CHANNELS) return -1;
	if(vol1 > MAXVOLUME) vol1=MAXVOLUME;
	if(vol2 > MAXVOLUME) vol2=MAXVOLUME;
	return sceAudioOutputPannedBlocking(audio_handles[channel], vol1, vol2, buf);
}

static int audio_ChannelThread(SceSize argc, void* argv)
{
	int i = 0;
	void *bufptr = NULL;
	unsigned int *ptr = NULL;
	volatile int bufid = 0;
	int channel = *(int *)argv;
	void (*audio_UpdateCallback)(void *buf, unsigned len);
	while(audio_terminate == 0)
	{
		bufptr = &audio_buffers[channel][bufid];
		audio_UpdateCallback = audio_ChannelCallback[channel];
		if(audio_UpdateCallback) audio_UpdateCallback(bufptr, SAMPLES);
		else
		{
			ptr = bufptr;
			for(i=0; i<SAMPLES; i++) *(ptr++)=0;
		}
		audio_OutputPannedBlocking(channel, audio_volumes[channel][0], audio_volumes[channel][1], bufptr);
		bufid = (bufid ? 0:1);
	}
	sceKernelExitThread(0);
	return 0;
}

void audio_SetChannelCallback(int channel, void (*callback)(void*,unsigned))
{
	audio_ChannelCallback[channel] = callback;
}

void audio_SetVolume(int channel, int left, int right)
{
	audio_volumes[channel][0] = left;
	audio_volumes[channel][1] = right;
}

int audio_Init(void)
{
	int i, ret;
	int failed = 0;
	char str[32];
	audio_terminate=0;
	audio_ready=0;
	for(i=0; i<CHANNELS; i++)
	{
		audio_handles[i] = -1;
		audio_thread_handle[i] = -1;
		audio_ChannelCallback[i] = 0;
		audio_volumes[i][0] = MAXVOLUME;
		audio_volumes[i][1] = MAXVOLUME;
	}
	for(i=0; i<CHANNELS; i++) if((audio_handles[i] = sceAudioChReserve(-1, SAMPLES, 0)) < 0) failed = 1;
	if(failed)
	{
		for(i=0; i<CHANNELS; i++)
		{
			if(audio_handles[i] != -1) sceAudioChRelease(audio_handles[i]);
			audio_handles[i] = -1;
		}
		return -1;
	}
	audio_ready = 1;
	strcpy(str,"Sound Thread");
	for(i=0; i<CHANNELS; i++)
	{
		str[6] = '0' + i;
		audio_thread_handle[i] = sceKernelCreateThread(str, audio_ChannelThread, 0x8, 0x10000, PSP_THREAD_ATTR_USER, NULL);
		if(audio_thread_handle[i] < 0)
		{
			audio_thread_handle[i] = -1;
			failed = 1;
			break;
		}
		ret = sceKernelStartThread(audio_thread_handle[i], sizeof(i), &i);
		if(ret != 0)
		{
			failed = 1;
			break;
		}
	}
	if(failed)
	{
		audio_terminate = 1;
		for(i=0; i<CHANNELS; i++)
		{
			if(audio_thread_handle[i] != -1)
			{
				sceKernelWaitThreadEnd(audio_thread_handle[i],NULL);
				sceKernelDeleteThread(audio_thread_handle[i]);
			}
			audio_thread_handle[i] = -1;
		}
		audio_ready = 0;
		return -1;
	}
	audio_SetChannelCallback(0, audio_UpdateCallback);
	return 0;
}

void audio_TermPre(void)
{
	audio_ready = 0;
	audio_terminate = 1;
}

void audio_Term(void)
{
	int i;
	audio_ready = 0;
	audio_terminate = 1;
	for (i=0; i<CHANNELS; i++) {
		if (audio_thread_handle[i] != -1) {
			sceKernelWaitThreadEnd(audio_thread_handle[i], NULL);
			sceKernelDeleteThread(audio_thread_handle[i]);
		}
		audio_thread_handle[i] = -1;
	}
	for (i=0; i<CHANNELS; i++) {
		if (audio_handles[i] != -1) {
			sceAudioChRelease(audio_handles[i]);
			audio_handles[i] = -1;
		}
	}
}

