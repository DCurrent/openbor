/*
 * OpenBOR - http://www.chronocrash.com
 * -----------------------------------------------------------------------
 * Licensed under a BSD-style license, see LICENSE in OpenBOR root for details.
 *
 * Copyright (c) 2004 - 2017 OpenBOR Team
 */

#include <stdint.h>
#include <stdbool.h>
#include <psp2/audioout.h>
#include <psp2/kernel/threadmgr.h>
#include "sblaster.h"
#include "soundmix.h"
#include "vitaport.h"
#include "globals.h"

#define BUF_SIZE 4096

static SceUID audioThreadHandle = -1;
static volatile bool audioStopPlayback;
static int16_t audioBuffers[2][BUF_SIZE];

static int SB_thread(int args, void *argp)
{
	static int whichBuffer = 0;

    int port = sceAudioOutOpenPort(SCE_AUDIO_OUT_PORT_TYPE_BGM, BUF_SIZE / 4, 44100, SCE_AUDIO_OUT_MODE_STEREO);
    sceAudioOutSetVolume(port, SCE_AUDIO_VOLUME_FLAG_L_CH | SCE_AUDIO_VOLUME_FLAG_R_CH,
                         (int[]){SCE_AUDIO_VOLUME_0DB, SCE_AUDIO_VOLUME_0DB});

	while (!audioStopPlayback)
	{
	    whichBuffer = !whichBuffer;
	    update_sample((unsigned char*)audioBuffers[whichBuffer], BUF_SIZE);
		sceAudioOutOutput(port, audioBuffers[whichBuffer]);
	}

	sceAudioOutReleasePort(port);
	return 0;
}

int SB_playstart(int bits, int samplerate)
{
    if (audioThreadHandle >= 0) return 1;

    audioStopPlayback = false;
    audioThreadHandle = sceKernelCreateThread("audio_thread", (void*)&SB_thread,
                                              0x10000100, 0x10000, 0, 0, NULL);

    if (audioThreadHandle < 0)
    {
        printf("sceKernelCreateThread() failed with code %08x\n", audioThreadHandle);
    }

    int dummy;
    sceKernelStartThread(audioThreadHandle, sizeof(dummy), &dummy);
	return 1;
}

void SB_playstop(void)
{
    if (audioThreadHandle < 0) return;
    audioStopPlayback = true;
    sceKernelWaitThreadEnd(audioThreadHandle, NULL, NULL);
    sceKernelDeleteThread(audioThreadHandle);
    audioThreadHandle = -1;
}

void SB_setvolume(char dev, char volume)
{
}

void SB_updatevolume(int volume)
{
}


