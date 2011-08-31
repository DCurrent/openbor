/*
 * OpenBOR - http://www.LavaLit.com
 * -----------------------------------------------------------------------
 * Licensed under the BSD license, see LICENSE in OpenBOR root for details.
 *
 * Copyright (c) 2004 - 2011 OpenBOR Team
 */

// A soundblaster interface. No bugs?

#include "sblaster.h"
#include "soundmix.h"
#include "audiodrv.h"

// The interface
static int inited;

int SB_playstart(int bits, int samplerate)
{
	if (inited) return 0;
	audio_Init();
	inited = 1;
	return 1;
}

int SB_getpos()
{
	return 0;
}

void SB_playstop()
{
	if (!inited) return;
	audio_Term(1);
	inited = 0;
}

void SB_exit()
{
}

void SB_setvolume(char dev, char volume)
{
	switch(dev) {
		case SB_VOICEVOL:
			/* 0-15 to 0-0x8000 */
			audio_SetVolume(volume<<11, volume<<11);
			break;
	}
}


