/*
 * OpenBOR - http://www.LavaLit.com
 * -----------------------------------------------------------------------
 * Licensed under the BSD license, see LICENSE in OpenBOR root for details.
 *
 * Copyright (c) 2004 - 2011 OpenBOR Team
 */

#include <ogcsys.h>
#include <string.h>
#include "soundmix.h"
#include "sblaster.h"

#define SB_SAMPLE_SIZE 512
#define SB_STACK 32768

static u32 sb_buffers[2][SB_SAMPLE_SIZE*8] __attribute__((aligned(32)));
static lwpq_t sb_queue;
static lwp_t sb_thread = LWP_THREAD_NULL;
static u8 sb_inited = 0;
static u8 sb_stack[SB_STACK];
static u8 sb_stop = 0;
static u8 sb_which = 0;

static void SB_init()
{
	if(sb_inited) return;
	memset(sb_buffers[0], 0, SB_SAMPLE_SIZE*8);
	memset(sb_buffers[1], 0, SB_SAMPLE_SIZE*8);
	AUDIO_Init(0);
	AUDIO_SetDSPSampleRate(AI_SAMPLERATE_48KHZ);
	sb_inited = 1;
}

static void *SB_Thread (void *arg)
{
	while (1)
	{
		if(sb_stop) break;
		sb_which ^= 1;
		memset(sb_buffers[sb_which], 0, SB_SAMPLE_SIZE*8);
		update_sample((u8 *)sb_buffers[sb_which], SB_SAMPLE_SIZE*4);
		LWP_ThreadSleep(sb_queue);
	}
	return NULL;
}

static void SB_Callback()
{
	DCFlushRange(sb_buffers[sb_which], SB_SAMPLE_SIZE*8);
	AUDIO_InitDMA((u32)sb_buffers[sb_which], SB_SAMPLE_SIZE*4);
	LWP_ThreadSignal(sb_queue);
}

int SB_playstart(int bits, int samplerate)
{
	sb_stop = 0;
	SB_init();
	LWP_InitQueue(&sb_queue);
	LWP_CreateThread (&sb_thread, SB_Thread, NULL, sb_stack, SB_STACK, 70);
	AUDIO_RegisterDMACallback(SB_Callback);
	SB_Callback();
	AUDIO_StartDMA();
	return 1;
}

void SB_playstop()
{
	sb_stop = 1;
}

void SB_exit()
{
	AUDIO_StopDMA();
	AUDIO_RegisterDMACallback(0);
	LWP_ThreadSignal(sb_queue);
	LWP_JoinThread(sb_thread, NULL);
	LWP_CloseQueue(sb_queue);
	sb_thread = LWP_THREAD_NULL;
}

void SB_setvolume(char dev, char volume)
{
}

void SB_updatevolume(int volume)
{
}
