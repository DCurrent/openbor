/////////////////////////////////////////////////////////////////////////////
//
// ps2sdr - raw 48khz output using sdr
//
/////////////////////////////////////////////////////////////////////////////

#include <stdio.h>
#include <kernel.h>
#include <tamtypes.h>
#include <loadfile.h>
#include <sjpcm.h>

#include "ps2port.h"

#include "ps2sdr.h"

ps2sdr_fillstereosamples_t thefillfunction;

ee_thread_t sound_thread;
s32 sound_thread_pid;

extern u32 _gp;

#define SAMPLES 2 * 960

#define HLINES_WAIT 80

short left[SAMPLES];
short right[SAMPLES];
u32 sound_stack[0x800];

void ps2sdr_play(void);
void ps2sdr_alarm(void);

void ps2sdr_init(void) {
    debug_printf("Starting playing thread...\n");
    
    SjPCM_InitEx(0, 12);
    SjPCM_Clearbuff();
    SjPCM_Setvol(0x3fff);
    SjPCM_SetCallback(2, ps2sdr_alarm);
    
    sound_thread.func = ps2sdr_play;
    sound_thread.gp_reg = &_gp;
    sound_thread.initial_priority = 4;
    sound_thread.stack = sound_stack;
    sound_thread.stack_size = sizeof(sound_stack);
    sound_thread_pid = CreateThread(&sound_thread);
    if (sound_thread_pid < 0) {
	debug_printf("Error: sound thread PID = %i\n", sound_thread_pid);
	SleepThread();
    }
}

void ps2sdr_setfillfunction(ps2sdr_fillstereosamples_t fillfunction) {
    thefillfunction = fillfunction;
    if (thefillfunction) {
	debug_printf("Waking up playing thread.\n");
	StartThread(sound_thread_pid, 0);
    }
}

void ps2sdr_alarm(void) {
    iWakeupThread(sound_thread_pid);
}

void ps2sdr_play() {
//    int loops = 0, todo;
    int todo;
    for (;;) {
	SleepThread();
	if (!thefillfunction) {
	    ExitDeleteThread();
	    return;
	}
//	if (!(loops % 32)) {
//	    todo = SjPCM_Available() / 960;
//	    loops = 0;
//	} else {
	    todo = 2;
//	}
//	loops++;
        thefillfunction(left, right, todo * 960);
        SjPCM_Enqueue(left, right, todo * 960, 0);
    }
}

void ps2sdr_setvolume(unsigned v) {
    SjPCM_Setvol(v);
}
