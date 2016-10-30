/*
 * OpenBOR - http://www.chronocrash.com
 * -----------------------------------------------------------------------
 * All rights reserved, see LICENSE in OpenBOR root for details.
 *
 * Copyright (c) 2004 - 2014 OpenBOR Team
 */

#ifndef TIMER_H
#define TIMER_H

#include "types.h"

void borTimerInit();
void borTimerExit();

/*
;----------------------------------------------------------------
; Proc:		timer_getinterval
; In:		ECX = frequency (1 to 1193181 Hz)
; Returns:	EAX = units passed since last call
; Destroys:	EBX ECX EDX
; Description:	Returns the time that passed since the last call,
;		measured in the specified frequency.
;		This function is extremely accurate, since all
;		rounding errors are compensatred for.
;		Only use for very short intervals!
;----------------------------------------------------------------
*/
unsigned timer_getinterval(unsigned freq);

// returns time passed since startup in milliseconds
unsigned timer_gettick();

// returns time passed since startup in microseconds
u64 timer_uticks();

unsigned get_last_interval();
void set_last_interval(unsigned);
void set_ticks(unsigned);

#endif
