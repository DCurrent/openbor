/*
 * OpenBOR - https://www.chronocrash.com
 * -----------------------------------------------------------------------
 * Licensed under the BSD license, see LICENSE in OpenBOR root for details.
 *
 * Copyright (c)  OpenBOR Team
 */

#ifndef	CONTROL_H
#define	CONTROL_H

#define MAX_BUTTONS				    21

#define WII_UP						0x00000001
#define WII_RIGHT					0x00000002
#define WII_DOWN					0x00000004
#define WII_LEFT					0x00000008
#define WII_1_A						0x00000010
#define WII_2_B						0x00000020
#define WII_A_Y_X					0x00000040
#define WII_B_X_Y					0x00000080
#define WII_MINUS					0x00000100
#define WII_PLUS_START				0x00000200
#define WII_HOME					0x00000400
#define WII_R_TRIGGER				0x00000800
#define WII_L_TRIGGER				0x00001000
#define WII_ZR						0x00002000
#define WII_ZL						0x00004000
#define WII_C_R						0x00008000
#define WII_Z_L						0x00010000
#define WII_SUB_UP					0x00020000
#define WII_SUB_RIGHT				0x00040000
#define WII_SUB_DOWN				0x00080000
#define WII_SUB_LEFT				0x00100000

#define	CONTROL_ESC					11

#define	CONTROL_DEFAULT1_UP			1
#define	CONTROL_DEFAULT1_RIGHT		2
#define	CONTROL_DEFAULT1_DOWN		3
#define	CONTROL_DEFAULT1_LEFT		4
#define CONTROL_DEFAULT1_FIRE1		5
#define CONTROL_DEFAULT1_FIRE2		7
#define	CONTROL_DEFAULT1_FIRE3		16
#define	CONTROL_DEFAULT1_FIRE4		17
#define	CONTROL_DEFAULT1_FIRE5		6
#define	CONTROL_DEFAULT1_FIRE6		8
#define CONTROL_DEFAULT1_START		10
#define CONTROL_DEFAULT1_SCREENSHOT 9

#define CONTROL_DEFAULT_UP(x)         (1+(MAX_BUTTONS*(x)))
#define CONTROL_DEFAULT_RIGHT(x)      (2+(MAX_BUTTONS*(x)))
#define CONTROL_DEFAULT_DOWN(x)       (3+(MAX_BUTTONS*(x)))
#define CONTROL_DEFAULT_LEFT(x)       (4+(MAX_BUTTONS*(x)))
#define CONTROL_DEFAULT_FIRE1(x)      (5+(MAX_BUTTONS*(x)))
#define CONTROL_DEFAULT_FIRE2(x)      (7+(MAX_BUTTONS*(x)))
#define CONTROL_DEFAULT_FIRE3(x)      (16+(MAX_BUTTONS*(x)))
#define CONTROL_DEFAULT_FIRE4(x)      (17+(MAX_BUTTONS*(x)))
#define CONTROL_DEFAULT_FIRE5(x)      (6+(MAX_BUTTONS*(x)))
#define CONTROL_DEFAULT_FIRE6(x)      (8+(MAX_BUTTONS*(x)))
#define CONTROL_DEFAULT_START(x)      (10+(MAX_BUTTONS*(x)))
#define CONTROL_DEFAULT_SCREENSHOT(x) (9+(MAX_BUTTONS*(x)))

#define	CONTROL_NONE				(1+(MAX_BUTTONS*99)) //Kratus (20-04-21) value used to clear all keys

#define WII_SHUTDOWN                -1
#define WII_RESET                   -2

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
void control_update(s_playercontrols * playercontrols, int numplayers);
void control_rumble(int port, int ratio, int msec);
unsigned long getPad(int port);

#endif

