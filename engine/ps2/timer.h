#ifndef __TIMER_H
#define __TIMER_H

/*
	Very high-resolution timer stuff.
	Last update: 09-29-2000

	Last addition is timer_getinterval, a function to measure a short
	interval in a specified frequency. This function compensates for
	any and all rounding errors, making it extremely precise!

	Valid frequencies are 1 through 1193181.

*/

//#define		PIT_FREQ	1193181

void timer_init(void);

void timer_exit(void);

unsigned timer_gettime(void);

unsigned timer_getinterval(unsigned freq);

#endif

