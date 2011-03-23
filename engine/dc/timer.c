/*
 * OpenBOR - http://www.LavaLit.com
 * -----------------------------------------------------------------------
 * Licensed under the BSD license, see LICENSE in OpenBOR root for details.
 *
 * Copyright (c) 2004 - 2011 OpenBOR Team
 */

#include "kos.h"
#include "timer.h"

static unsigned lastinterval = 0;
static unsigned tickinit = 0;

unsigned getTicks()
{
	uint32 milis=0, secs=0;
	timer_ms_gettime(&secs, &milis);
	return (((secs*1000)+milis)-tickinit);
}

void borTimerInit()
{
	uint32 milis=0, secs=0;
	timer_ms_gettime(&secs, &milis);
	tickinit = (secs*1000)+milis;
}

void borTimerExit() { }

unsigned timer_getinterval(unsigned freq)
{
	unsigned tickspassed,ebx,blocksize,now;
	now=timer_gettick();
	ebx=now-lastinterval;
	blocksize=1000/freq;
	ebx+=1000%freq;
	tickspassed=ebx/blocksize;
	ebx-=ebx%blocksize;
	lastinterval+=ebx;
	return tickspassed;
}

unsigned timer_gettick()
{
  	return getTicks();
}
