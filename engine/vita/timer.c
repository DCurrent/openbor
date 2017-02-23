/*
 * OpenBOR - http://www.chronocrash.com
 * -----------------------------------------------------------------------
 * Licensed under a BSD-style license, see LICENSE in OpenBOR root for details.
 *
 * Copyright (c) 2004 - 2017 OpenBOR Team
 */

#include <psp2/kernel/processmgr.h>
#include <stdint.h>
#include "timer.h"
#include "types.h"

#define GETTIME_FREQ (1000)

static uint64_t start;
static unsigned lastinterval = 0;
static unsigned newticks = 0;

void borTimerInit()
{
	start = sceKernelGetProcessTimeWide();
}

void borTimerExit(){}

unsigned timer_getinterval(unsigned freq)
{
	unsigned tickspassed,ebx,blocksize,now;
	now=timer_gettick()-newticks;
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
	return (sceKernelGetProcessTimeWide() - start) / 1000;
}

u64 timer_uticks()
{
	return sceKernelGetProcessTimeWide() - start;
}

unsigned get_last_interval()
{
	return lastinterval;
}

void set_last_interval(unsigned value)
{
	lastinterval = value;
}

void set_ticks(unsigned value)
{
    newticks = value;
}


