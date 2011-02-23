/*
 * OpenBOR - http://www.LavaLit.com
 * -----------------------------------------------------------------------
 * Licensed under the BSD license, see LICENSE in OpenBOR root for details.
 *
 * Copyright (c) 2004 - 2011 OpenBOR Team
 */

#ifndef	CONTROL_H
#define	CONTROL_H

#define DC_DPAD_UP       0x00000001
#define DC_DPAD_RIGHT    0x00000002
#define DC_DPAD_DOWN     0x00000004
#define DC_DPAD_LEFT     0x00000008
#define DC_A             0x00000010
#define DC_B             0x00000020
#define DC_X             0x00000040
#define DC_Y             0x00000080
#define DC_LEFT_TRIGGER	 0x00000100
#define DC_RIGHT_TRIGGER 0x00000200
#define DC_START         0x00000400
#define DC_C             0x00000800
#define DC_DPAD2_UP		 0x00001000
#define DC_DPAD2_DOWN	 0x00002000
#define DC_DPAD2_LEFT	 0x00004000
#define DC_DPAD2_RIGHT	 0x00008000
#define DC_D             0x00010000
#define DC_Z             0x00020000

#define	CONTROL_ESC					73

#define	CONTROL_DEFAULT1_UP			1
#define	CONTROL_DEFAULT1_RIGHT		2
#define	CONTROL_DEFAULT1_DOWN		3
#define	CONTROL_DEFAULT1_LEFT		4
#define CONTROL_DEFAULT1_FIRE1		5
#define CONTROL_DEFAULT1_FIRE2		7
#define	CONTROL_DEFAULT1_FIRE3		9
#define	CONTROL_DEFAULT1_FIRE4		10
#define	CONTROL_DEFAULT1_FIRE5		6
#define	CONTROL_DEFAULT1_FIRE6		8
#define CONTROL_DEFAULT1_START		11
#define CONTROL_DEFAULT1_SCREENSHOT 73

#define	CONTROL_DEFAULT2_UP			(1+18)
#define	CONTROL_DEFAULT2_RIGHT		(2+18)
#define	CONTROL_DEFAULT2_DOWN		(3+18)
#define	CONTROL_DEFAULT2_LEFT		(4+18)
#define CONTROL_DEFAULT2_FIRE1		(5+18)
#define CONTROL_DEFAULT2_FIRE2		(7+18)
#define	CONTROL_DEFAULT2_FIRE3		(9+18)
#define	CONTROL_DEFAULT2_FIRE4		(10+18)
#define	CONTROL_DEFAULT2_FIRE5		(6+18)
#define	CONTROL_DEFAULT2_FIRE6		(8+18)
#define CONTROL_DEFAULT2_START		(11+18)
#define CONTROL_DEFAULT2_SCREENSHOT 73

#define	CONTROL_DEFAULT3_UP			(1+(18*2))
#define	CONTROL_DEFAULT3_RIGHT		(2+(18*2))
#define	CONTROL_DEFAULT3_DOWN		(3+(18*2))
#define	CONTROL_DEFAULT3_LEFT		(4+(18*2))
#define CONTROL_DEFAULT3_FIRE1		(5+(18*2))
#define CONTROL_DEFAULT3_FIRE2		(7+(18*2))
#define	CONTROL_DEFAULT3_FIRE3		(9+(18*2))
#define	CONTROL_DEFAULT3_FIRE4		(10+(18*2))
#define	CONTROL_DEFAULT3_FIRE5		(6+(18*2))
#define	CONTROL_DEFAULT3_FIRE6		(8+(18*2))
#define CONTROL_DEFAULT3_START		(11+(18*2))
#define CONTROL_DEFAULT3_SCREENSHOT 73

#define	CONTROL_DEFAULT4_UP			(1+(18*3))
#define	CONTROL_DEFAULT4_RIGHT		(2+(18*3))
#define	CONTROL_DEFAULT4_DOWN		(3+(18*3))
#define	CONTROL_DEFAULT4_LEFT		(4+(18*3))
#define CONTROL_DEFAULT4_FIRE1		(5+(18*3))
#define CONTROL_DEFAULT4_FIRE2		(7+(18*3))
#define	CONTROL_DEFAULT4_FIRE3		(9+(18*3))
#define	CONTROL_DEFAULT4_FIRE4		(10+(18*3))
#define	CONTROL_DEFAULT4_FIRE5		(6+(18*3))
#define	CONTROL_DEFAULT4_FIRE6		(8+(18*3))
#define CONTROL_DEFAULT4_START		(11+(18*3))
#define CONTROL_DEFAULT4_SCREENSHOT 73

typedef struct
{
	int	settings[32];
	unsigned long keyflags, newkeyflags;
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
unsigned long getPad(int port);

int dcpad_rumblepower[4];

#endif

