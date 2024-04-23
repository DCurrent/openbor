/*
 * OpenBOR - http://www.chronocrash.com
 * -----------------------------------------------------------------------
 * All rights reserved, see LICENSE in OpenBOR root for details.
 *
 * Copyright (c)  OpenBOR Team
 */

// Kratus (10-2021) Include the translation file
#include "translation.h"

#ifndef	JOYSTICKS_H
#define	JOYSTICKS_H

// Kratus (10-2021) Added constants and automatic translation for some Joystick status
#define JOY_UNKNOWN_NAME Tr("Unknown")
#define JOY_DISCONNECTED_NAME Tr("Disconnected")
#define JOY_NONE_NAME Tr("None")

#define JOY_TYPE_DEFAULT   0
#define JOY_TYPE_GAMEPARK  1
#define JOY_AXIS_X         0
#define JOY_AXIS_Y         1
#define JOY_MAX_INPUTS     64
#define	JOY_LIST_FIRST     600
#define JOY_LIST_TOTAL     4
#define	JOY_LIST_LAST      JOY_LIST_FIRST + JOY_MAX_INPUTS * JOY_LIST_TOTAL
#define JOY_NAME_SIZE      1 + 1 + JOY_MAX_INPUTS * JOY_LIST_TOTAL

/* Real-Time Joystick Data */
typedef struct{
	char Name[MAX_BUFFER_LEN];
	char KeyName[JOY_MAX_INPUTS + 1][MAX_BUFFER_LEN];
	int Type;
	int NumHats;
	int NumAxes;
	int NumButtons;
	u32 Hats;
	u32 Axes;
	u32 Buttons;
	u64 Data;
}s_joysticks;
extern s_joysticks joysticks[JOY_LIST_TOTAL];


extern const char *JoystickKeyName[JOY_NAME_SIZE];
extern const char *GameparkKeyName[JOY_NAME_SIZE];
extern const u64 JoystickBits[JOY_MAX_INPUTS + 1];

const char* PC_GetJoystickKeyName(int portnum, int keynum);
char* JOY_GetKeyName(int keycode);


#endif
