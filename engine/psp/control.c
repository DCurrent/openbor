/*
 * OpenBOR - http://www.LavaLit.com
 * -----------------------------------------------------------------------
 * Licensed under the BSD license, see LICENSE in OpenBOR root for details.
 *
 * Copyright (c) 2004 - 2011 OpenBOR Team
 */

#include <pspkernel.h>
#include <pspctrl.h>
#include "pspport.h"
#include "control.h"
#include "control/control.h"

#define	PAD_START			1
#define	MAX_PADS			4
#define	PAD_END				(18*MAX_PADS)

static int usejoy;
static int lastkey[MAX_PADS];

static const char *padnames[PAD_END+1+1] = {
	"...",
#define CONTROLNAMES(x) \
	x" Up",             \
	x" Right",          \
	x" Down",           \
	x" Left",           \
	x" X",              \
	x" O",              \
	x" []",             \
	x" /\\",            \
	x" L-Trigger",      \
	x" R-Trigger",      \
	x" Start",          \
	x" Select",         \
	x" Note",           \
	x" Home",           \
	x" Hold",           \
	x" Screen",         \
	x" Volume Up",      \
	x" Volume Down",
	CONTROLNAMES("PSP 1")
	CONTROLNAMES("PSP 2")
	CONTROLNAMES("PSP 3")
	CONTROLNAMES("PSP 4")
	"undefined"
};

static int flag_to_index(unsigned long flag)
{
	int index = 0;
	unsigned long bit = 1;
	while(!((bit<<index)&flag) && index<31) ++index;
	return index;
}

void control_exit()
{
	usejoy = 0;
}

void control_init(int joy_enable)
{
	usejoy = joy_enable;
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
			k = 1 + i*18 + flag_to_index(lastkey[i]);
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
	for(i=0; i<MAX_PADS; i++) port[i] = getPad(i);
	for(player=0; player<numplayers; player++)
	{
		pcontrols = playercontrols[player];
		k = 0;
		for(i=0; i<32; i++){
			t = pcontrols->settings[i];
			if(t >= PAD_START && t <= PAD_END)
			{
				int portnum = (t-1) / 18;
				int shiftby = (t-1) % 18;
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
}

void control_rumble(int port, int msec)
{
}

unsigned long getPad(int port)
{
	unsigned long btns = 0;
	SceCtrlData data;
	getCtrlData(&data);

	if(port != 0) return lastkey[port] = 0;

	if(control_getjoyenabled())
	{
		if(data.Ly >= 0xC0)              btns |= PSP_DPAD_DOWN;
		if(data.Ly <= 0x30)              btns |= PSP_DPAD_UP;
		if(data.Lx <= 0x30)              btns |= PSP_DPAD_LEFT;
		if(data.Lx >= 0xC0)              btns |= PSP_DPAD_RIGHT;
	}

	if(data.Buttons & PSP_CTRL_SELECT)   btns |= PSP_SELECT;
	if(data.Buttons & PSP_CTRL_START)    btns |= PSP_START;
	if(data.Buttons & PSP_CTRL_UP)	     btns |= PSP_DPAD_UP;
	if(data.Buttons & PSP_CTRL_RIGHT)    btns |= PSP_DPAD_RIGHT;
	if(data.Buttons & PSP_CTRL_DOWN)     btns |= PSP_DPAD_DOWN;
	if(data.Buttons & PSP_CTRL_LEFT)     btns |= PSP_DPAD_LEFT;
	if(data.Buttons & PSP_CTRL_LTRIGGER) btns |= PSP_LEFT_TRIGGER;
	if(data.Buttons & PSP_CTRL_RTRIGGER) btns |= PSP_RIGHT_TRIGGER;
	if(data.Buttons & PSP_CTRL_TRIANGLE) btns |= PSP_TRIANGLE;
	if(data.Buttons & PSP_CTRL_CIRCLE)   btns |= PSP_CIRCLE;
	if(data.Buttons & PSP_CTRL_CROSS)	 btns |= PSP_CROSS;
	if(data.Buttons & PSP_CTRL_SQUARE)   btns |= PSP_SQUARE;
	if(data.Buttons & PSP_CTRL_NOTE)     btns |= PSP_NOTE;
	if(data.Buttons & PSP_CTRL_HOME)     btns |= PSP_HOME;
	if(data.Buttons & PSP_CTRL_HOLD)     btns |= PSP_HOLD;
	if(data.Buttons & PSP_CTRL_SCREEN)   btns |= PSP_SCREEN;
	if(data.Buttons & PSP_CTRL_VOLUP)    btns |= PSP_VOLUP;
	if(data.Buttons & PSP_CTRL_VOLDOWN)  btns |= PSP_VOLDOWN;

	if(btns & PSP_HOME && btns & PSP_START) borExit(-1);

	return lastkey[port] = btns;
}
