/*
 * OpenBOR - http://www.chronocrash.com
 * -----------------------------------------------------------------------
 * Licensed under a BSD-style license, see LICENSE in OpenBOR root for details.
 *
 * Copyright (c) 2004 - 2017 OpenBOR Team
 */

#ifndef	CONTROL_H
#define	CONTROL_H

#include <stdint.h>

#define VITA_DPAD_UP         0x00000001
#define VITA_DPAD_RIGHT      0x00000002
#define VITA_DPAD_DOWN       0x00000004
#define VITA_DPAD_LEFT       0x00000008
#define VITA_CROSS           0x00000010
#define VITA_CIRCLE          0x00000020
#define VITA_SQUARE          0x00000040
#define VITA_TRIANGLE        0x00000080
#define VITA_LEFT_TRIGGER    0x00000100
#define VITA_RIGHT_TRIGGER   0x00000200
#define VITA_START           0x00000400
#define VITA_SELECT          0x00000800
#define VITA_L1              0x00001000
#define VITA_R1              0x00002000
#define VITA_L3              0x00004000
#define VITA_R3              0x00008000

#define	CONTROL_ESC					14

#define	CONTROL_DEFAULT1_UP			1
#define	CONTROL_DEFAULT1_RIGHT		2
#define	CONTROL_DEFAULT1_DOWN		3
#define	CONTROL_DEFAULT1_LEFT		4
#define CONTROL_DEFAULT1_FIRE1		7
#define CONTROL_DEFAULT1_FIRE2		6
#define	CONTROL_DEFAULT1_FIRE3		9
#define	CONTROL_DEFAULT1_FIRE4		10
#define	CONTROL_DEFAULT1_FIRE5		5
#define	CONTROL_DEFAULT1_FIRE6		8
#define CONTROL_DEFAULT1_START		11
#define CONTROL_DEFAULT1_SCREENSHOT 12

#define	CONTROL_DEFAULT2_UP			(1+18)
#define	CONTROL_DEFAULT2_RIGHT		(2+18)
#define	CONTROL_DEFAULT2_DOWN		(3+18)
#define	CONTROL_DEFAULT2_LEFT		(4+18)
#define CONTROL_DEFAULT2_FIRE1		(7+18)
#define CONTROL_DEFAULT2_FIRE2		(6+18)
#define	CONTROL_DEFAULT2_FIRE3		(9+18)
#define	CONTROL_DEFAULT2_FIRE4		(10+18)
#define	CONTROL_DEFAULT2_FIRE5		(5+18)
#define	CONTROL_DEFAULT2_FIRE6		(8+18)
#define CONTROL_DEFAULT2_START		(11+18)
#define CONTROL_DEFAULT2_SCREENSHOT (12+18)

#define	CONTROL_DEFAULT3_UP			(1+(18*2))
#define	CONTROL_DEFAULT3_RIGHT		(2+(18*2))
#define	CONTROL_DEFAULT3_DOWN		(3+(18*2))
#define	CONTROL_DEFAULT3_LEFT		(4+(18*2))
#define CONTROL_DEFAULT3_FIRE1		(7+(18*2))
#define CONTROL_DEFAULT3_FIRE2		(6+(18*2))
#define	CONTROL_DEFAULT3_FIRE3		(9+(18*2))
#define	CONTROL_DEFAULT3_FIRE4		(10+(18*2))
#define	CONTROL_DEFAULT3_FIRE5		(5+(18*2))
#define	CONTROL_DEFAULT3_FIRE6		(8+(18*2))
#define CONTROL_DEFAULT3_START		(11+(18*2))
#define CONTROL_DEFAULT3_SCREENSHOT (12+(18*2))

#define	CONTROL_DEFAULT4_UP			(1+(18*3))
#define	CONTROL_DEFAULT4_RIGHT		(2+(18*3))
#define	CONTROL_DEFAULT4_DOWN		(3+(18*3))
#define	CONTROL_DEFAULT4_LEFT		(4+(18*3))
#define CONTROL_DEFAULT4_FIRE1		(7+(18*3))
#define CONTROL_DEFAULT4_FIRE2		(6+(18*3))
#define	CONTROL_DEFAULT4_FIRE3		(9+(18*3))
#define	CONTROL_DEFAULT4_FIRE4		(10+(18*3))
#define	CONTROL_DEFAULT4_FIRE5		(5+(18*3))
#define	CONTROL_DEFAULT4_FIRE6		(8+(18*3))
#define CONTROL_DEFAULT4_START		(11+(18*3))
#define CONTROL_DEFAULT4_SCREENSHOT (12+(18*3))

typedef struct
{
	int	settings[32];
	unsigned int keyflags, newkeyflags;
	int kb_break;
}
s_playercontrols;

void control_exit();
void control_init(int joy_enable);
int control_usejoy(int enable);
int control_getjoyenabled();
int keyboard_getlastkey();
void control_setkey(s_playercontrols * pcontrols, unsigned int flag, int key);
int control_scankey();
char* control_getkeyname(unsigned int keycode);
void control_update(s_playercontrols ** playercontrols, int numplayers);
void control_rumble(int port, int msec);

#endif

