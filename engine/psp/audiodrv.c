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
#define SAMPLE_SIZE     512
#define STREAM_SIZE     SAMPLE_SIZE * 4
#define THREAD_SIZE     32768
#define THREAD_PRIORITY 8

static   int audio_ready;
static   int audio_which;
static   int audio_handle;
volatile int audio_terminate;
static   int audio_thread_handle;
static   int audio_volumes[CHANNELS];
static short audio_buffers[CHANNELS][STREAM_SIZE];

static int audio_ChannelThread(SceSize argc, void* argv)
{
    while(!audio_terminate)
	{
        audio_which ^= 1;
        update_sample((void *)&audio_buffers[audio_which], STREAM_SIZE);

        if (audio_ready)
        {
            sceAudioOutputPannedBlocking(
                audio_handle,
                audio_volumes[LCH] > PSP_AUDIO_VOLUME_MAX ? PSP_AUDIO_VOLUME_MAX : audio_volumes[LCH],
                audio_volumes[RCH] > PSP_AUDIO_VOLUME_MAX ? PSP_AUDIO_VOLUME_MAX : audio_volumes[RCH],
                &audio_buffers[audio_which]);
        }
	}
	sceKernelExitThread(0);
	return 0;
}

void audio_SetVolume(int l, int r)
{
	audio_volumes[LCH] = l;
	audio_volumes[RCH] = r;
}

int audio_Init(void)
{
	audio_ready = 0;
    audio_which = LCH;
	audio_terminate = 0;
	audio_thread_handle = -1;
    audio_volumes[LCH] = PSP_AUDIO_VOLUME_MAX;
    audio_volumes[RCH] = PSP_AUDIO_VOLUME_MAX;
    memset(audio_buffers[LCH], 0, STREAM_SIZE);
    memset(audio_buffers[RCH], 0, STREAM_SIZE);

    audio_handle =
        sceAudioChReserve(
            PSP_AUDIO_NEXT_CHANNEL,
            PSP_AUDIO_SAMPLE_ALIGN(SAMPLE_SIZE),
            PSP_AUDIO_FORMAT_STEREO);

	if (audio_handle < 0)
	{
		return -1;
	}

    audio_thread_handle =
        sceKernelCreateThread(
            "MainAudioThread",
            audio_ChannelThread,
            THREAD_PRIORITY,
            THREAD_SIZE,
            PSP_THREAD_ATTR_USER,
            NULL);

    if (audio_thread_handle >= 0)
	{
		audio_ready = 1;
		sceKernelDelayThread(500*1000);
		sceKernelStartThread(audio_thread_handle, 0, 0);
		return 0;
	}

	audio_terminate = 1;
	sceAudioChRelease(audio_handle);
	sceKernelWaitThreadEnd(audio_thread_handle, NULL);
	sceKernelDeleteThread(audio_thread_handle);

	return -1;
}

void audio_Term(int kill_thread)
{
	audio_ready = 0;
	audio_terminate = 1;

    if (kill_thread)
    {
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
}

