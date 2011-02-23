/*
 * OpenBOR - http://www.LavaLit.com
 * -----------------------------------------------------------------------
 * Licensed under the BSD license, see LICENSE in OpenBOR root for details.
 *
 * Copyright (c) 2004 - 2011 OpenBOR Team
 */

#include "kos.h"
#include "soundmix.h"

static unsigned short buffer[8192];
static snd_stream_hnd_t handle = -1;

int SB_thread(void *arg)
{
	while(1)
	{
		snd_stream_poll(handle);
		thd_pass();
	}
	return 0;
}

void *SB_callback(snd_stream_hnd_t hnd, int smp_req, int *smp_recv)
{
	update_sample((unsigned char*)buffer, smp_req);
	*smp_recv = smp_req;
	return buffer;
}

int SB_playstart(int bits, int samplerate)
{
	if(handle == -1)
	{
		snd_stream_init();
		handle = snd_stream_alloc(*SB_callback, 8192);
		thd_create(1, (void*)SB_thread, NULL);
	}
	if(handle == -1) return 0;
	snd_stream_start(handle, samplerate, 1);
	return 1;
}

void SB_playstop()
{
	if(handle == -1) return;
	snd_stream_stop(handle);
}

void SB_setvolume(char dev, char volume)
{
}

void SB_updatevolume(int volume)
{
}
