/*
 * OpenBOR - http://www.LavaLit.com
 * -----------------------------------------------------------------------
 * Licensed under the BSD license, see LICENSE in OpenBOR root for details.
 *
 * Copyright (c) 2004 - 2011 OpenBOR Team
 */

#ifndef	CONTROL_H
#define	CONTROL_H

#define	CONTROL_ESC					12

#define	CONTROL_DEFAULT1_UP			1
#define	CONTROL_DEFAULT1_RIGHT		2
#define	CONTROL_DEFAULT1_DOWN		3
#define	CONTROL_DEFAULT1_LEFT		4
#define CONTROL_DEFAULT1_FIRE1		5
#define CONTROL_DEFAULT1_FIRE2		6
#define	CONTROL_DEFAULT1_FIRE3		7
#define	CONTROL_DEFAULT1_FIRE4		8
#define	CONTROL_DEFAULT1_FIRE5		13
#define	CONTROL_DEFAULT1_FIRE6		14
#define CONTROL_DEFAULT1_START		11
#define CONTROL_DEFAULT1_SCREENSHOT 10

#define	CONTROL_DEFAULT2_UP			(1+16)
#define	CONTROL_DEFAULT2_RIGHT		(2+16)
#define	CONTROL_DEFAULT2_DOWN		(3+16)
#define	CONTROL_DEFAULT2_LEFT		(4+16)
#define CONTROL_DEFAULT2_FIRE1		(5+16)
#define CONTROL_DEFAULT2_FIRE2		(6+16)
#define	CONTROL_DEFAULT2_FIRE3		(7+16)
#define	CONTROL_DEFAULT2_FIRE4		(8+16)
#define	CONTROL_DEFAULT2_FIRE5		(13+16)
#define	CONTROL_DEFAULT2_FIRE6		(14+16)
#define CONTROL_DEFAULT2_START		(11+16)
#define CONTROL_DEFAULT2_SCREENSHOT (10+16)

#define	CONTROL_DEFAULT3_UP			(1+(16*2))
#define	CONTROL_DEFAULT3_RIGHT		(2+(16*2))
#define	CONTROL_DEFAULT3_DOWN		(3+(16*2))
#define	CONTROL_DEFAULT3_LEFT		(4+(16*2))
#define CONTROL_DEFAULT3_FIRE1		(5+(16*2))
#define CONTROL_DEFAULT3_FIRE2		(6+(16*2))
#define	CONTROL_DEFAULT3_FIRE3		(7+(16*2))
#define	CONTROL_DEFAULT3_FIRE4		(8+(16*2))
#define	CONTROL_DEFAULT3_FIRE5		(13+(16*2))
#define	CONTROL_DEFAULT3_FIRE6		(14+(16*2))
#define CONTROL_DEFAULT3_START		(11+(16*2))
#define CONTROL_DEFAULT3_SCREENSHOT (10+(16*2))

#define	CONTROL_DEFAULT4_UP			(1+(16*3))
#define	CONTROL_DEFAULT4_RIGHT		(2+(16*3))
#define	CONTROL_DEFAULT4_DOWN		(3+(16*3))
#define	CONTROL_DEFAULT4_LEFT		(4+(16*3))
#define CONTROL_DEFAULT4_FIRE1		(5+(16*3))
#define CONTROL_DEFAULT4_FIRE2		(6+(16*3))
#define	CONTROL_DEFAULT4_FIRE3		(7+(16*3))
#define	CONTROL_DEFAULT4_FIRE4		(8+(16*3))
#define	CONTROL_DEFAULT4_FIRE5		(13+(16*3))
#define	CONTROL_DEFAULT4_FIRE6		(14+(16*3))
#define CONTROL_DEFAULT4_START		(11+(16*3))
#define CONTROL_DEFAULT4_SCREENSHOT (10+(16*3))


typedef struct{
	int		settings[32];
	unsigned long	keyflags, newkeyflags;
	int		kb_break;
}s_playercontrols;


void control_exit();
void control_init(int joy_enable);
int control_usejoy(int enable);
int control_getjoyenabled();

void control_setkey(s_playercontrols * pcontrols, unsigned int flag, int key);
int control_scankey();

char * control_getkeyname(unsigned int keycode);
void control_update(s_playercontrols ** playercontrols, int numplayers);
void control_rumble(int port, int msec)
{
}

#endif

