/*
 * OpenBOR - http://www.LavaLit.com
 * -----------------------------------------------------------------------
 * All rights reserved, see LICENSE in OpenBOR root for details.
 *
 * Copyright (c) 2004 - 2011 OpenBOR Team
 */

#include <SDL.h>
#include "timer.h"

#define GETTIME_FREQ (1000)

static unsigned lastinterval = 0;


void borTimerInit(){}
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
