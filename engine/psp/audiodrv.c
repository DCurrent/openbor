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

#define LCH				0
#define RCH				1
#define CHANNELS		2
#define SAMPLES			512
#define MAXVOLUME		0x8000

static   int audio_ready = 0;
static   int audio_handle = -1;
volatile int audio_terminate = 1;
static   int audio_volumes[CHANNELS] = {MAXVOLUME, MAXVOLUME};
static short audio_buffers[SAMPLES*CHANNELS] = {0};
static   int audio_thread_handle = -1;

static void (*audio_ChannelCallback)(void *buf, unsigned len);

void audio_UpdateCallback(void *buf, unsigned len)
{
	update_sample(buf,len*2);
}

int audio_OutputPannedBlocking(unsigned int vol1, unsigned int vol2, void *buf)
{
	if(!audio_ready) return -1;
	if(vol1 > MAXVOLUME) vol1=MAXVOLUME;
	if(vol2 > MAXVOLUME) vol2=MAXVOLUME;
	return sceAudioOutputPannedBlocking(audio_handle, vol1, vol2, buf);
}

static int audio_ChannelThread(SceSize argc, void* argv)
{
	int i = 0;
	void *bufptr = NULL;
	unsigned int *ptr = NULL;
	void (*audio_UpdateCallback)(void *buf, unsigned len);
	while(audio_terminate == 0)
	{
		bufptr = &audio_buffers;
		audio_UpdateCallback = audio_ChannelCallback;
		if(audio_UpdateCallback) audio_UpdateCallback(bufptr, SAMPLES*CHANNELS);
		else
		{
			ptr = bufptr;
			for(i=0; i<SAMPLES; i++) *(ptr++)=0;
		}
		audio_OutputPannedBlocking(audio_volumes[LCH], audio_volumes[RCH], bufptr);
	}
	sceKernelExitThread(0);
	return 0;
}

void audio_SetChannelCallback(int channel, void (*callback)(void*,unsigned))
{
	audio_ChannelCallback = callback;
}

void audio_SetVolume(int channel, int l, int r)
{
	audio_volumes[LCH] = l;
	audio_volumes[RCH] = r;
}

int audio_Init(void)
{
	audio_ready = 0;
	audio_handle = -1;
	audio_terminate = 0;
	audio_thread_handle = -1;
	audio_ChannelCallback = 0;
	audio_volumes[LCH] = MAXVOLUME;
	audio_volumes[RCH] = MAXVOLUME;
	
	if ((audio_handle = sceAudioChReserve(-1, SAMPLES, 0)) < 0)
	{
		return -1;
	}

	if ((audio_thread_handle = sceKernelCreateThread("MainAudioThread", audio_ChannelThread, 0x8, 0x10000, PSP_THREAD_ATTR_USER, NULL)) >= 0)
	{
		audio_ready = 1;
		sceKernelDelayThread(500*1000);
		sceKernelStartThread(audio_thread_handle, 0, 0);
		audio_SetChannelCallback(0, audio_UpdateCallback);
		return 0;
	}
	
	audio_terminate = 1;
	sceAudioChRelease(audio_handle);
	sceKernelWaitThreadEnd(audio_thread_handle, NULL);
	sceKernelDeleteThread(audio_thread_handle);
	
	return -1;
}

void audio_TermPre(void)
{
	audio_ready = 0;
	audio_terminate = 1;
}

void audio_Term(void)
{
	audio_ready = 0;
	audio_terminate = 1;

	if (audio_thread_handle != -1) 
	{
		sceKernelWaitThreadEnd(audio_thread_handle, NULL);
		sceKernelDeleteThread(audio_thread_handle);
	}

	if (audio_handle != -1) 
	{
		sceAudioChRelease(audio_handle);
	}
}

