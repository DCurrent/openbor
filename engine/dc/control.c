/*
 * OpenBOR - http://www.LavaLit.com
 * -----------------------------------------------------------------------
 * Licensed under the BSD license, see LICENSE in OpenBOR root for details.
 *
 * Copyright (c) 2004 - 2011 OpenBOR Team
 */

#include "kos.h"
#include "control.h"

int dcpad_rumblepower[4];

static int usejoy;
static int lastkey[4];

#define	PAD_START 1
#define	PAD_END   (18*4)

static const char *padnames[PAD_END+1+1] = {
	"...",
#define CONTROLNAMES(x) \
	x" Up",         \
	x" Right",      \
	x" Down",       \
	x" Left",       \
	x" A",          \
	x" B",          \
	x" X",          \
	x" Y",          \
	x" L-Trigger",  \
	x" R-Trigger",  \
	x" Start",      \
	x" C",          \
	x" D",          \
	x" Z",          \
	x" Up 2",       \
	x" Down 2",     \
	x" Left 2",     \
	x" Right 2",
	CONTROLNAMES("P1")
	CONTROLNAMES("P2")
	CONTROLNAMES("P3")
	CONTROLNAMES("P4")
	"Undefined"
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

int keyboard_getlastkey()
{
	int i, ret[4];
	for(i=0; i<4; i++)
	{
		ret[i] = lastkey[i];
		lastkey[i] = 0;
	}
	return (ret[0] | ret[1] | ret[2] | ret[3]);
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
	unsigned k=0;
	unsigned port0=lastkey[0];
	unsigned port1=lastkey[1];
	unsigned port2=lastkey[2];
	unsigned port3=lastkey[3];

	     if(port0) k = 1 + 0*18 + flag_to_index(port0);
	else if(port1) k = 1 + 1*18 + flag_to_index(port1);
	else if(port2) k = 1 + 2*18 + flag_to_index(port2);
	else if(port3) k = 1 + 3*18 + flag_to_index(port3);

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
	unsigned port[4];
	for(i=0; i<4; i++) port[i] = getPad(i);
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
	dcpad_rumblepower[port] = msec;
}

unsigned long getPad(int port)
{
	unsigned long btns=0;
	maple_device_t *caddr;
	cont_state_t *cont;
	if((caddr = maple_enum_type(port, MAPLE_FUNC_CONTROLLER)) == NULL) goto DONE;
	if((cont = maple_dev_status(caddr)) == NULL) goto DONE;
	if(cont->buttons & CONT_DPAD_UP)	 btns |= DC_DPAD_UP;
	if(cont->buttons & CONT_DPAD_RIGHT)  btns |= DC_DPAD_RIGHT;
	if(cont->buttons & CONT_DPAD_DOWN)   btns |= DC_DPAD_DOWN;
	if(cont->buttons & CONT_DPAD_LEFT)   btns |= DC_DPAD_LEFT;
	if(cont->buttons & CONT_DPAD2_UP)	 btns |= DC_DPAD2_UP;
	if(cont->buttons & CONT_DPAD2_RIGHT) btns |= DC_DPAD2_RIGHT;
	if(cont->buttons & CONT_DPAD2_DOWN)  btns |= DC_DPAD2_DOWN;
	if(cont->buttons & CONT_DPAD2_LEFT)  btns |= DC_DPAD2_LEFT;
	if(cont->buttons & CONT_START)       btns |= DC_START;
	if(cont->buttons & CONT_A)           btns |= DC_A;
	if(cont->buttons & CONT_B)           btns |= DC_B;
	if(cont->buttons & CONT_C)           btns |= DC_C;
	if(cont->buttons & CONT_D)           btns |= DC_D;
	if(cont->buttons & CONT_X)           btns |= DC_X;
	if(cont->buttons & CONT_Y)           btns |= DC_Y;
	if(cont->buttons & CONT_Z)           btns |= DC_Z;
	if(cont->joyy < -40)                 btns |= DC_DPAD_UP;
	if(cont->joyx > 40)                  btns |= DC_DPAD_RIGHT;
	if(cont->joyy > 40)	                 btns |= DC_DPAD_DOWN;
	if(cont->joyx < -40)                 btns |= DC_DPAD_LEFT;
	if(cont->joy2y < -40)                btns |= DC_DPAD2_UP;
	if(cont->joy2x > 40)                 btns |= DC_DPAD2_RIGHT;
	if(cont->joy2y > 40)	             btns |= DC_DPAD2_DOWN;
	if(cont->joy2x < -40)                btns |= DC_DPAD2_LEFT;
	if(cont->ltrig > 20)                 btns |= DC_LEFT_TRIGGER;
	if(cont->rtrig > 20)                 btns |= DC_RIGHT_TRIGGER;

DONE:
  	return lastkey[port] = btns;
}
