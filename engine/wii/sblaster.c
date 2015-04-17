/*
 * OpenBOR - http://www.chronocrash.com
 * -----------------------------------------------------------------------
 * All rights reserved, see LICENSE in OpenBOR root for details.
 *
 * Copyright (c) 2004 - 2015 OpenBOR Team
 */

/* This code was derived from code carrying the following copyright notice:

 Copyright (c) 2008 Francisco Muñoz 'Hermes' <www.elotrolado.net>
 All rights reserved.
 
 Proper (standard) vorbis usage by Tantric, 2009
 Threading modifications/corrections by Tantric, 2009

 Redistribution and use in source and binary forms, with or without 
 modification, are permitted provided that the following conditions are met:

 - Redistributions of source code must retain the above copyright notice, this
 list of conditions and the following disclaimer.
 - Redistributions in binary form must reproduce the above copyright notice, 
 this list of conditions and the following disclaimer in the documentation 
 and/or other materials provided with the distribution.
 - The names of the contributors may not be used to endorse or promote products 
 derived from this software without specific prior written permission.

 THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" 
 AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE 
 IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE 
 FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL 
 DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR 
 SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER 
 CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, 
 OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <asndlib.h>
#include <gccore.h>
#include <unistd.h>
#include <string.h>

#include "globals.h"
#include "sblaster.h"
#include "soundmix.h"

#define READ_SAMPLES 4096 // samples that must be read before sending to hardware
#define MAX_PCMOUT 4096 // minimum samples to mix
typedef struct
{
	int flag;
	int volume;
	int samplerate;
	short pcmout[2][READ_SAMPLES + MAX_PCMOUT * 2]; /* take 4k out of the data segment, not the stack */
	int pcmout_pos;
	int pcm_indx;
} private_data;

static private_data sb_private;

#define STACKSIZE		8192

static u8 sb_stack[STACKSIZE];
static lwpq_t sb_queue = LWP_TQUEUE_NULL;
static lwp_t sb_thread = LWP_THREAD_NULL;
static int sb_thread_running = 0;

static int sb_inited = 0;

static void SB_init()
{
	if (sb_inited) return;
	AUDIO_Init(NULL);
	ASND_Init();
	ASND_Pause(0);
	sb_inited = 1;
}

void SB_exit()
{
	ASND_Pause(1);
	ASND_End();
	sb_inited = 0;
}

static void SB_Callback(int voice)
{
	if (!sb_thread_running)
	{
		ASND_StopVoice(0);
		return;
	}

	if (sb_private.pcm_indx >= READ_SAMPLES)
	{
		if (ASND_AddVoice(0,
				(void *) sb_private.pcmout[sb_private.pcmout_pos],
				sb_private.pcm_indx << 1) == 0)
		{
			sb_private.pcmout_pos ^= 1;
			sb_private.pcm_indx = 0;
			sb_private.flag = 0;
			LWP_ThreadSignal(sb_queue);
		}
	}
	else
	{
		if (sb_private.flag & 64)
		{
			sb_private.flag &= ~64;
			LWP_ThreadSignal(sb_queue);
		}
	}
}

static void *SB_Thread(private_data * priv)
{
	int first_time = 1;

	LWP_InitQueue(&sb_queue);
	ASND_Pause(0);

	priv[0].pcm_indx = 0;
	priv[0].pcmout_pos = 0;
	priv[0].flag = 0;

	sb_thread_running = 1;

	while (sb_thread_running)
	{
		if (priv[0].flag)
			LWP_ThreadSleep(sb_queue);

		if (priv[0].flag == 0) // wait until all samples are sent
		{
			if (ASND_TestPointer(0, priv[0].pcmout[priv[0].pcmout_pos])
					&& ASND_StatusVoice(0) != SND_UNUSED)
			{
				priv[0].flag |= 64;
				continue;
			}
			if (priv[0].pcm_indx < READ_SAMPLES)
			{
				priv[0].flag = 3;

				update_sample((u8 *) &priv[0].pcmout[priv[0].pcmout_pos][priv[0].pcm_indx], MAX_PCMOUT);
				priv[0].flag &= 192;
				priv[0].pcm_indx += MAX_PCMOUT >> 1; //16 bit samples
			}
			else
				priv[0].flag = 1;
		}

		if (priv[0].flag == 1)
		{
			if (ASND_StatusVoice(0) == SND_UNUSED || first_time)
			{
				first_time = 0;
				ASND_SetVoice(0, VOICE_STEREO_16BIT, priv[0].samplerate, 0,
						(void *) priv[0].pcmout[priv[0].pcmout_pos],
						priv[0].pcm_indx << 1, priv[0].volume,
						priv[0].volume, SB_Callback);
				priv[0].pcmout_pos ^= 1;
				priv[0].pcm_indx = 0;
				priv[0].flag = 0;
			}
		}
		usleep(100);
	}
	priv[0].pcm_indx = 0;

	return 0;
}

void SB_playstop()
{
	ASND_StopVoice(0);
	sb_thread_running = 0;

	if(sb_thread != LWP_THREAD_NULL)
	{
		if(sb_queue != LWP_TQUEUE_NULL)
			LWP_ThreadSignal(sb_queue);
		LWP_JoinThread(sb_thread, NULL);
		sb_thread = LWP_THREAD_NULL;
	}
	if(sb_queue != LWP_TQUEUE_NULL)
	{
		LWP_CloseQueue(sb_queue);
		sb_queue = LWP_TQUEUE_NULL;
	}
}

int SB_playstart(int bits, int samplerate)
{
	SB_init();
	SB_playstop();

	sb_private.volume = 127;
	sb_private.flag = 0;
	sb_private.samplerate = samplerate;

	if (LWP_CreateThread(&sb_thread, (void *) SB_Thread,
			&sb_private, sb_stack, STACKSIZE, 80) == -1)
	{
		sb_thread_running = 0;
		return 0;
	}
	return 1;
}

void SB_setvolume(char dev, char volume)
{
}

void SB_updatevolume(int volume)
{
}
