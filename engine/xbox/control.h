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

#define CONTROL_DEFAULT_UP(x)         (1+(16*(x)))
#define CONTROL_DEFAULT_RIGHT(x)      (2+(16*(x)))
#define CONTROL_DEFAULT_DOWN(x)       (3+(16*(x)))
#define CONTROL_DEFAULT_LEFT(x)       (4+(16*(x)))
#define CONTROL_DEFAULT_FIRE1(x)      (5+(16*(x)))
#define CONTROL_DEFAULT_FIRE2(x)      (6+(16*(x)))
#define CONTROL_DEFAULT_FIRE3(x)      (7+(16*(x)))
#define CONTROL_DEFAULT_FIRE4(x)      (8+(16*(x)))
#define CONTROL_DEFAULT_FIRE5(x)      (13+(16*(x)))
#define CONTROL_DEFAULT_FIRE6(x)      (14+(16*(x)))
#define CONTROL_DEFAULT_START(x)      (11+(16*(x)))
#define CONTROL_DEFAULT_SCREENSHOT(x) (10+(16*(x)))

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
void control_update(s_playercontrols * playercontrols, int numplayers);
void control_rumble(int port, int ratio, int msec);

#endif

