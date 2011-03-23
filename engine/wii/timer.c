/*
 * OpenBOR - http://www.LavaLit.com
 * -----------------------------------------------------------------------
 * Licensed under the BSD license, see LICENSE in OpenBOR root for details.
 *
 * Copyright (c) 2004 - 2011 OpenBOR Team
 */


#include <ogc/lwp_watchdog.h>
#include "timer.h"

#define GETTIME_FREQ 1000

static unsigned lastinterval = 0;

void borTimerInit(){}
void borTimerExit(){}

unsigned timer_getinterval(unsigned freq)
{
	unsigned tickspassed,ebx,blocksize,now;
	now=timer_gettick();
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
	return gettime() / TB_TIMER_CLOCK;
}
