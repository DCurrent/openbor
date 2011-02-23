/*
 * OpenBOR - http://www.LavaLit.com
 * -----------------------------------------------------------------------
 * Licensed under the BSD license, see LICENSE in OpenBOR root for details.
 *
 * Copyright (c) 2004 - 2011 OpenBOR Team
 */

#include <ogc/lwp_watchdog.h>
#include <wiiuse/wpad.h>
#include <ogc/pad.h>
#include <ogc/system.h>
#include <stdio.h>
#include "wiiport.h"
#include "version.h"
#include "control.h"
#undef MIN
#undef MAX
#include "openbor.h"

#define	PAD_START			1
#define	MAX_PADS			4
#define	PAD_END				(MAX_BUTTONS*MAX_PADS)

static int usejoy;
static int initialized;
static int hwbutton = 0;
static int using_gc[MAX_PADS];
static int lastkey[MAX_PADS];
static int rumbling[MAX_PADS];
static long long unsigned rumble_msec[MAX_PADS];
static long long unsigned time2rumble[MAX_PADS];

static const char *padnames[PAD_END+1+1] = {
	"...",
#define CONTROLNAMES(x) \
    x" Up",             \
    x" Right",          \
    x" Down",           \
    x" Left",           \
    x" 1/A",            \
    x" 2/B",            \
	x" A/Y/X",          \
	x" B/X/Y",          \
	x" -/Menu",         \
	x" +/Start",        \
	x" Home/Z",         \
    x" R-Trigger",      \
    x" L-Trigger",      \
	x" ZR",             \
    x" ZL",             \
    x" Z/L",            \
    x" C/R",            \
    x" Substick Up",    \
    x" Substick Right", \
    x" Substick Down",  \
    x" Substick Left",
	CONTROLNAMES("P1")
	CONTROLNAMES("P2")
	CONTROLNAMES("P3")
	CONTROLNAMES("P4")
	"undefined"
};

void poweroff()
{
	hwbutton = WII_SHUTDOWN;
}

void reset()
{
	hwbutton = WII_RESET;
}

void wiimote_poweroff(int playernum)
{
	hwbutton = WII_SHUTDOWN;
}

// Resets or powers off the Wii if the corresponding buttons are pressed. 
// Resetting returns to the Homebrew Channel.
void respondToPowerReset()
{
	shutdown(hwbutton,
		"OpenBoR %s, Compile Date: " __DATE__ "\n"
		"Presented by Team Senile.\n"
		"This Version is unofficial and based on the Senile Source Code.\n"
		"\n"
		"Special thanks to SEGA and SNK.\n\n",
		VERSION
		);
}

static int flag_to_index(unsigned long flag)
{
	int index = 0;
	unsigned long bit = 1;
	while(!((bit<<index)&flag) && index<31) ++index;
	return index;
}

void control_exit()
{
	int i;
	for(i=0; i<MAX_PADS; i++)
	{
		rumbling[i] = 0;
		rumble_msec[i] = 0;
	}
	usejoy = 0;
}

void control_init(int joy_enable)
{
	int i;
	if(initialized) return;
	for(i=0; i<MAX_PADS; i++)
	{
		rumbling[i] = 0;
		rumble_msec[i] = 0;
	}
	usejoy = joy_enable;
	hwbutton = 0;
	PAD_Init();
	WPAD_Init();
	
	// set callbacks for power/reset buttons
	SYS_SetResetCallback(reset);
	SYS_SetPowerCallback(poweroff);
	WPAD_SetPowerButtonCallback(wiimote_poweroff);
	
	initialized = 1;
}

int control_usejoy(int enable)
{
	usejoy = enable;
	return 0;
}

int control_getjoyenabled()
{
	return usejoy;
}

int keyboard_getlastkey(void)
{
    int i, ret=0;
	for(i=0; i<MAX_PADS; i++)
	{
		ret |= lastkey[i];
		lastkey[i] = 0;
	}
	return ret;
}

void control_setkey(s_playercontrols * pcontrols, unsigned int flag, int key)
{
	if(!pcontrols) return;
	pcontrols->settings[flag_to_index(flag)] = key;
	pcontrols->keyflags = pcontrols->newkeyflags = 0;
}

// Scan input for newly-pressed keys.
// Return value:
// 0  = no key was pressed
// >0 = key code for pressed key
// <0 = error
int control_scankey()
{
	static unsigned ready = 0;
	unsigned i, k=0;

    for(i=0; i<MAX_PADS; i++)
    {
        if(lastkey[i])
        {
            k = 1 + i*MAX_BUTTONS + flag_to_index(lastkey[i]);
            break;
        }
    }

	if(ready && k)
	{
		ready = 0;
		return k;
	}
	ready = (!k);
	return 0;
}

char * control_getkeyname(unsigned keycode)
{
	if(keycode >= PAD_START && keycode <= PAD_END) return (char*)padnames[keycode];
	return "...";
}

void control_update(s_playercontrols ** playercontrols, int numplayers)
{
	unsigned long k;
	unsigned long i;
	int player;
	int t;
	s_playercontrols * pcontrols;
	unsigned port[MAX_PADS];
	long long unsigned msec = 0;
	for(i=0; i<MAX_PADS; i++)
	{
		port[i] = getPad(i);
		if(rumbling[i])
		{
			if(!msec) msec = ticks_to_millisecs(gettime());
			if(msec > time2rumble[i]+rumble_msec[i])
			{
				rumbling[i] = 0;
				WPAD_Rumble(i, 0);
				PAD_ControlMotor(i, 0);
			}
		}
	}
	for(player=0; player<numplayers; player++)
	{
		pcontrols = playercontrols[player];
		k = 0;
		for(i=0; i<32; i++)
		{
			t = pcontrols->settings[i];
			if(t >= PAD_START && t <= PAD_END)
			{
				int portnum = (t-1) / MAX_BUTTONS;
				int shiftby = (t-1) % MAX_BUTTONS;
				if(portnum >= 0 && portnum <= 3)
				{
					if((port[portnum] >> shiftby) & 1) k |= (1<<i);
				}
			}
		}
		pcontrols->kb_break = 0;
		pcontrols->newkeyflags = k & (~pcontrols->keyflags);
		pcontrols->keyflags = k;
	}
	
	if (hwbutton) respondToPowerReset();
}

void control_rumble(int port, int msec)
{
	WPADData *wpad;
	wpad = WPAD_Data(port);

	rumbling[port] = 1;
	rumble_msec[port] = msec * 3;
	time2rumble[port] = ticks_to_millisecs(gettime());

	if (using_gc[port])                             PAD_ControlMotor(port, 1);
	else if (wpad->exp.type != WPAD_EXP_CLASSIC)    WPAD_Rumble(port, 1);
}

unsigned long getPad(int port)
{
	unsigned long btns = 0;
	unsigned short gcbtns;
	WPADData *wpad;

	// necessary to detect GC controllers plugged in while OpenBOR is running
	PAD_Init();

	PAD_ScanPads();
	gcbtns = PAD_ButtonsDown(port) | PAD_ButtonsHeld(port);
	WPAD_ScanPads();
	wpad = WPAD_Data(port);

	if (gcbtns)       using_gc[port] = 1;
	else if (wpad->btns_h) using_gc[port] = 0;


	if(wpad->exp.type == WPAD_EXP_CLASSIC)
	{
		// Left thumb stick
		if(wpad->exp.classic.ljs.mag >= 0.3)
		{
			if (wpad->exp.classic.ljs.ang >= 310 ||
				wpad->exp.classic.ljs.ang <= 50)          btns |= WII_UP;
			if (wpad->exp.classic.ljs.ang >= 130 &&
				wpad->exp.classic.ljs.ang <= 230)         btns |= WII_DOWN;
			if (wpad->exp.classic.ljs.ang >= 220 &&
				wpad->exp.classic.ljs.ang <= 320)         btns |= WII_LEFT;
			if (wpad->exp.classic.ljs.ang >= 40 &&
				wpad->exp.classic.ljs.ang <= 140)         btns |= WII_RIGHT;
		}

		// Right thumb stick
		if(wpad->exp.classic.rjs.mag >= 0.3)
		{
			if (wpad->exp.classic.rjs.ang >= 310 ||
				wpad->exp.classic.rjs.ang <= 50)          btns |= WII_SUB_UP;
			if (wpad->exp.classic.rjs.ang >= 130 &&
				wpad->exp.classic.rjs.ang <= 230)         btns |= WII_SUB_DOWN;
			if (wpad->exp.classic.rjs.ang >= 220 &&
				wpad->exp.classic.rjs.ang <= 320)         btns |= WII_SUB_LEFT;
			if (wpad->exp.classic.rjs.ang >= 40 &&
				wpad->exp.classic.rjs.ang <= 140)         btns |= WII_SUB_RIGHT;
		}

		// D-pad
		if(wpad->btns_h & WPAD_CLASSIC_BUTTON_UP)         btns |= WII_UP;
		if(wpad->btns_h & WPAD_CLASSIC_BUTTON_DOWN)       btns |= WII_DOWN;
		if(wpad->btns_h & WPAD_CLASSIC_BUTTON_LEFT)       btns |= WII_LEFT;
		if(wpad->btns_h & WPAD_CLASSIC_BUTTON_RIGHT)      btns |= WII_RIGHT;
	}
	else if((wpad->exp.type == WPAD_EXP_NUNCHUK) && usejoy) // Nunchuck
	{
		if(wpad->exp.nunchuk.js.pos.y >= 0xB0)            btns |= WII_UP;
		if(wpad->exp.nunchuk.js.pos.y <= 0x40)            btns |= WII_DOWN;
		if(wpad->exp.nunchuk.js.pos.x <= 0x40)            btns |= WII_LEFT;
		if(wpad->exp.nunchuk.js.pos.x >= 0xB0)            btns |= WII_RIGHT;
	}
	else // Wiimote
	{
		if(wpad->btns_h & WPAD_BUTTON_UP)                 btns |= WII_LEFT;
		if(wpad->btns_h & WPAD_BUTTON_DOWN)               btns |= WII_RIGHT;
		if(wpad->btns_h & WPAD_BUTTON_LEFT)               btns |= WII_DOWN;
		if(wpad->btns_h & WPAD_BUTTON_RIGHT)              btns |= WII_UP;
	}

	// GameCube analog stick
	if(PAD_StickY(port) > 18)                             btns |= WII_UP;
	if(PAD_StickY(port) < -18)                            btns |= WII_DOWN;
	if(PAD_StickX(port) < -18)                            btns |= WII_LEFT;
	if(PAD_StickX(port) > 18)                             btns |= WII_RIGHT;

	// GameCube C-stick
	if(PAD_SubStickY(port) > 18)                          btns |= WII_SUB_UP;
	if(PAD_SubStickY(port) < -18)                         btns |= WII_SUB_DOWN;
	if(PAD_SubStickX(port) < -18)                         btns |= WII_SUB_LEFT;
	if(PAD_SubStickX(port) > 18)                          btns |= WII_SUB_RIGHT;

	// GameCube D-pad
	if(gcbtns & PAD_BUTTON_UP)                            btns |= WII_UP;
	if(gcbtns & PAD_BUTTON_DOWN)                          btns |= WII_DOWN;
	if(gcbtns & PAD_BUTTON_LEFT)                          btns |= WII_LEFT;
	if(gcbtns & PAD_BUTTON_RIGHT)                         btns |= WII_RIGHT;

	if(wpad->exp.type == WPAD_EXP_CLASSIC) // Classic Controller
	{
		if(wpad->btns_h & WPAD_CLASSIC_BUTTON_A)          btns |= WII_1_A;
		if(wpad->btns_h & WPAD_CLASSIC_BUTTON_B)          btns |= WII_2_B;
		if(wpad->btns_h & WPAD_CLASSIC_BUTTON_Y)          btns |= WII_A_Y_X;
		if(wpad->btns_h & WPAD_CLASSIC_BUTTON_X)          btns |= WII_B_X_Y;
		if(wpad->btns_h & WPAD_CLASSIC_BUTTON_MINUS)      btns |= WII_MINUS;
		if(wpad->btns_h & WPAD_CLASSIC_BUTTON_PLUS)       btns |= WII_PLUS_START;
		if(wpad->btns_h & WPAD_CLASSIC_BUTTON_HOME)       btns |= WII_HOME;
		if(wpad->btns_h & WPAD_CLASSIC_BUTTON_FULL_R)     btns |= WII_C_R;
		if(wpad->btns_h & WPAD_CLASSIC_BUTTON_FULL_L)     btns |= WII_Z_L;
		if(wpad->btns_h & WPAD_CLASSIC_BUTTON_ZL)         btns |= WII_ZL;
		if(wpad->btns_h & WPAD_CLASSIC_BUTTON_ZR)         btns |= WII_ZR;
	}
	else // Wiimote or Wiimote + Nunchuk
	{
		if(wpad->btns_h & WPAD_BUTTON_1)                  btns |= WII_1_A;
		if(wpad->btns_h & WPAD_BUTTON_2)                  btns |= WII_2_B;
		if(wpad->btns_h & WPAD_BUTTON_A)                  btns |= WII_A_Y_X;
		if(wpad->btns_h & WPAD_BUTTON_B)                  btns |= WII_B_X_Y;
		if(wpad->btns_h & WPAD_BUTTON_MINUS)              btns |= WII_MINUS;
		if(wpad->btns_h & WPAD_BUTTON_PLUS)               btns |= WII_PLUS_START;
		if(wpad->btns_h & WPAD_BUTTON_HOME)               btns |= WII_HOME;
		if(wpad->btns_h & WPAD_NUNCHUK_BUTTON_Z)          btns |= WII_Z_L;
		if(wpad->btns_h & WPAD_NUNCHUK_BUTTON_C)          btns |= WII_C_R;
	}

	if(gcbtns & PAD_BUTTON_X)                             btns |= WII_A_Y_X;
	if(gcbtns & PAD_BUTTON_Y)                             btns |= WII_B_X_Y;
	if(gcbtns & PAD_BUTTON_A)                             btns |= WII_1_A;
	if(gcbtns & PAD_BUTTON_B)                             btns |= WII_2_B;
	if(gcbtns & PAD_BUTTON_START)                         btns |= WII_PLUS_START;
	if(gcbtns & PAD_TRIGGER_R)                            btns |= WII_C_R;
	if(gcbtns & PAD_TRIGGER_L)                            btns |= WII_Z_L;
	if(gcbtns & PAD_TRIGGER_Z)                            btns |= WII_HOME;

	return lastkey[port] = btns;
}
