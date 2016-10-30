/*
 * OpenBOR - http://www.LavaLit.com
 * -----------------------------------------------------------------------
 * Licensed under the BSD license, see LICENSE in OpenBOR root for details.
 *
 * Copyright (c) 2004 - 2011 OpenBOR Team
 */

#include <psputils.h>
#include <psprtc.h>
#include "timer.h"

#define GETTIME_FREQ (sceRtcGetTickResolution())

static unsigned lastinterval;

void borTimerInit(void){}
void borTimerExit(void){}

unsigned timer_getinterval(unsigned freq)
{
	unsigned tickspassed,ebx,blocksize;
	u64 now;
	sceRtcGetCurrentTick(&now);
	ebx=now-lastinterval;
	blocksize=GETTIME_FREQ/freq;
	ebx+=GETTIME_FREQ%freq;
	tickspassed=ebx/blocksize;
	ebx-=ebx%blocksize;
	lastinterval+=ebx;
	return tickspassed;
}

unsigned timer_gettick()
{
	u64 now;
	sceRtcGetCurrentTick(&now);
	return now;
}

u64 timer_uticks()
{
	return timer_gettick() * 1000LL;
}

unsigned get_last_interval()
{
	return lastinterval;
}

void set_last_interval(unsigned value)
{
	lastinterval = value;
}


