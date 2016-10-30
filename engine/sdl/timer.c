/*
 * OpenBOR - http://www.chronocrash.com
 * -----------------------------------------------------------------------
 * All rights reserved, see LICENSE in OpenBOR root for details.
 *
 * Copyright (c) 2004 - 2014 OpenBOR Team
 */

#include <SDL.h>
#include "timer.h"
#include "types.h"

#define GETTIME_FREQ (1000)

static unsigned lastinterval = 0;
static u64 startcounter;

void borTimerInit()
{
	startcounter = SDL_GetPerformanceCounter();
}

void borTimerExit(){}

unsigned timer_getinterval(unsigned freq)
{
	unsigned tickspassed,ebx,blocksize,now;
	now=SDL_GetTicks();
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
	return SDL_GetTicks();
}

u64 timer_uticks()
{
	u64 freq = SDL_GetPerformanceFrequency();
	u64 counter = SDL_GetPerformanceCounter();
	return counter * (1000000.0 / freq);
}

unsigned get_last_interval()
{
	return lastinterval;
}

void set_last_interval(unsigned value)
{
	lastinterval = value;
}


