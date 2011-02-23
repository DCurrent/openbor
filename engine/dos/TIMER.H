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



#define		PIT_FREQ	1193181


extern void __near borTimerInit();
#pragma aux timer_init modify [eax ebx edx];

extern void __near borTimerExit();
#pragma aux timer_exit modify [eax edx];

extern unsigned long __near timer_gettime();
#pragma aux timer_gettime modify [ebx] value [eax];

extern unsigned long __near timer_getinterval(int);
#pragma aux timer_getinterval parm [ecx] modify [ebx ecx edx] value [eax];


#endif

