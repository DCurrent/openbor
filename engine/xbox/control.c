/*
 * OpenBOR - http://www.LavaLit.com
 * -----------------------------------------------------------------------
 * Licensed under the BSD license, see LICENSE in OpenBOR root for details.
 *
 * Copyright (c) 2004 - 2011 OpenBOR Team
 */

#include <stdio.h>		// kbhit, getch
#include "control.h"
#include "xboxport.h"

static int usejoy;

#define	PAD_START 1
#define	PAD_END   (16*4)

static const char *padnames[PAD_END+1+1] = {
	"...",
#define CONTROLNAMES(x) \
	x" Up",             \
	x" Right",          \
	x" Down",           \
	x" Left",           \
	x" A",              \
	x" B",              \
	x" X",              \
	x" Y",              \
	x" Black",          \
	x" White",          \
	x" Start",          \
	x" Back",           \
	x" L-Trigger",      \
	x" R-Trigger",      \
	x" L-Thumb",        \
	x" R-Thumb",
	CONTROLNAMES("Joy 1")
	CONTROLNAMES("Joy 2")
	CONTROLNAMES("Joy 3")
	CONTROLNAMES("Joy 4")
	"undefined"
};

static int flag_to_index(unsigned long flag){
	int index = 0;
	unsigned long bit = 1;

	while(!((bit<<index)&flag) && index<31) ++index;
	return index;
}

void control_exit(){}

void control_init(int joy_enable){
	usejoy = 1 ;
}

int control_usejoy(int enable){
	return 1 ;
}

int control_getjoyenabled(){
	return usejoy;
}

void control_setkey(s_playercontrols * pcontrols, unsigned int flag, int key){
	if(!pcontrols) return;
	pcontrols->settings[flag_to_index(flag)] = key;
	pcontrols->keyflags = pcontrols->newkeyflags = 0;
}

// Scan input for newly-pressed keys.
// Return value:
// 0  = no key was pressed
// >0 = key code for pressed key
// <0 = error
int control_scankey(){
	static unsigned ready = 0;
	unsigned k=0;
	unsigned port0=xbox_get_playerinput(0);
	unsigned port1=xbox_get_playerinput(1);
	unsigned port2=xbox_get_playerinput(2);
	unsigned port3=xbox_get_playerinput(3);

		 if(port0) k = 1 + 0*16 + flag_to_index(port0);
	else if(port1) k = 1 + 1*16 + flag_to_index(port1);
	else if(port2) k = 1 + 2*16 + flag_to_index(port2);
	else if(port3) k = 1 + 3*16 + flag_to_index(port3);

	if(ready && k) {
		ready = 0;
		return k;
	}
	ready = (!k);
	return 0;
}

char * control_getkeyname(unsigned keycode){

	if(keycode >= PAD_START && keycode <= PAD_END) return (char*)padnames[keycode];
	return "...";
}

void control_update(s_playercontrols ** playercontrols, int numplayers){

	unsigned long k;
	unsigned long i;
	int player;
	int t;
	s_playercontrols * pcontrols;
	unsigned port[4];
	xbox_check_events();
	port[0]=xbox_get_playerinput(0);
	port[1]=xbox_get_playerinput(1);
	port[2]=xbox_get_playerinput(2);
	port[3]=xbox_get_playerinput(3);
	for(player=0; player<numplayers; player++){
		pcontrols = playercontrols[player];
		k = 0;
		for(i=0; i<32; i++){
			t = pcontrols->settings[i];
			if(t >= PAD_START && t <= PAD_END){
				int portnum = (t-1) / 16;
				int shiftby = (t-1) % 16;
				if(portnum >= 0 && portnum <= 3){
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
