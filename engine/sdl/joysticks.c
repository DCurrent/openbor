/*
 * OpenBOR - http://www.chronocrash.com
 * -----------------------------------------------------------------------
 * All rights reserved, see LICENSE in OpenBOR root for details.
 *
 * Copyright (c) 2004 - 2011 OpenBOR Team
 */

#include "sdlport.h"
#include "joysticks.h"

s_joysticks joysticks[JOY_LIST_TOTAL];

const char *JoystickKeyName[JOY_NAME_SIZE] = {
	"...",
#define JOYSTICK_NAMES(x) \
	x" Up",             \
	x" Right",          \
	x" Down",           \
	x" Left",           \
	x" Button 1",       \
	x" Button 2",       \
	x" Button 3",       \
	x" Button 4",       \
	x" Button 5",       \
	x" Button 6",       \
	x" Button 7",       \
	x" Button 8",       \
	x" Button 9",       \
	x" Button 10",      \
	x" Button 11",      \
	x" Button 12",      \
	x" Button 13",      \
	x" Button 14",      \
	x" Button 15",      \
	x" Button 16",      \
	x" Button 17",      \
	x" Button 18",      \
	x" Button 19",      \
	x" Button 20",      \
	x" Button 21",      \
	x" Button 22",      \
	x" Button 23",      \
	x" Button 24",      \
	x" Button 25",      \
	x" Button 26",      \
	x" Button 27",      \
	x" Button 28",
	JOYSTICK_NAMES("P1")
	JOYSTICK_NAMES("P2")
	JOYSTICK_NAMES("P3")
	JOYSTICK_NAMES("P4")
	"undefined"
};

const char *SonyKeyName[JOY_NAME_SIZE] = {
	"...",
#define SONY_NAMES(x)   \
	x" Up",             \
	x" Right",          \
	x" Down",           \
	x" Left",           \
	x" Select",         \
	x" L3",             \
	x" R3",             \
	x" Start",          \
	x" L2",             \
	x" R2",             \
	x" L1",             \
	x" R1",             \
	x" /\\",            \
	x" O",              \
	x" X",              \
	x" []",             \
	x" PS",             \
	x" unknown 14",     \
	x" unknown 15",     \
	x" unknown 16",     \
	x" unknown 17",     \
	x" unknown 18",     \
	x" unknown 19",     \
	x" unknown 20",     \
	x" unknown 21",     \
	x" unknown 22",     \
	x" unknown 23",     \
	x" unknown 24",     \
	x" unknown 25",     \
	x" unknown 26",     \
	x" unknown 27",     \
	x" unknown 28",
	SONY_NAMES("P1")
	SONY_NAMES("P2")
	SONY_NAMES("P3")
	SONY_NAMES("P4")
	"undefined"
};

const char *MicrosoftKeyName[JOY_NAME_SIZE] = {
	"...",
#define MICROSOFT_NAMES(x) \
	x" Up",             \
	x" Right",          \
	x" Down",           \
	x" Left",           \
	x" A",              \
	x" B",              \
	x" X",              \
	x" Y",              \
	x" Left Back",      \
	x" Right Back",     \
	x" Back",           \
	x" Start",          \
	x" L-Thumb",        \
	x" R-Thumb",        \
	x" unknown 11",     \
	x" unknown 12",     \
	x" unknown 13",     \
	x" unknown 14",     \
	x" unknown 15",     \
	x" unknown 16",     \
	x" unknown 17",     \
	x" unknown 18",     \
	x" unknown 19",     \
	x" unknown 20",     \
	x" unknown 21",     \
	x" unknown 22",     \
	x" unknown 23",     \
	x" unknown 24",     \
	x" unknown 25",     \
	x" unknown 26",     \
	x" unknown 27",     \
	x" unknown 28",
	MICROSOFT_NAMES("P1")
	MICROSOFT_NAMES("P2")
	MICROSOFT_NAMES("P3")
	MICROSOFT_NAMES("P4")
	"undefined"
};

const char *GameparkKeyName[JOY_NAME_SIZE] = {
	"...",
#define GAMEPARK_NAMES(x) \
	x" Up",             \
	x" Right",          \
	x" Down",           \
	x" Left",           \
	x" Start",          \
	x" Select",         \
	x" L-Trigger",      \
	x" R-Trigger",      \
	x" A",              \
	x" B",              \
	x" X",              \
	x" Y",              \
	x" Volume Up",      \
	x" Volume Down",    \
	x" Click",          \
	x" unknown 16",     \
	x" unknown 17",     \
	x" unknown 18",     \
	x" unknown 19",     \
	x" unknown 20",     \
	x" unknown 21",     \
	x" unknown 22",     \
	x" unknown 23",     \
	x" unknown 24",     \
	x" unknown 25",     \
	x" unknown 26",     \
	x" unknown 27",     \
	x" unknown 28",
	GAMEPARK_NAMES("P1")
	GAMEPARK_NAMES("P2")
	GAMEPARK_NAMES("P3")
	GAMEPARK_NAMES("P4")
	"undefined"
};

const int JoystickBits[JOY_MAX_INPUTS + 1] = {
	0x00000000, // No Buttons Pressed
	0x00000001, // Hat Up
	0x00000002,	// Hat Right
	0x00000004, // Hat Down
	0x00000008,	// Hat Left
	0x00000010,	// Button 1
	0x00000020,	// Button 2
	0x00000040,	// Button 3
	0x00000080,	// Button 4
	0x00000100,	// Button 5
	0x00000200,	// Button 6
	0x00000400,	// Button 7
	0x00000800,	// Button 8
	0x00001000, // Button 9
	0x00002000,	// Button 10
	0x00004000,	// Button 11
	0x00008000,	// Button 12
	0x00010000,	// Button 13
	0x00020000,	// Button 14
	0x00040000,	// Button 15
	0x00080000,	// Button 16
	0x00100000,	// Button 17
	0x00200000,	// Button 18
	0x00400000,	// Button 19
	0x00800000,	// Button 20
	0x01000000,	// Button 21
	0x02000000,	// Button 22
	0x04000000,	// Button 23
	0x08000000,	// Button 24
	0x10000000,	// Button 25
	0x20000000,	// Button 26
	0x40000000,	// Button 27
	0x80000000 	// Button 28
};

const char* JoystickButtonNames[JOY_NAME_SIZE] = {
	"...",
#define BUTTON_NAMES(x) \
	x" Button 1",       \
	x" Button 2",       \
	x" Button 3",       \
	x" Button 4",       \
	x" Button 5",       \
	x" Button 6",       \
	x" Button 7",       \
	x" Button 8",       \
	x" Button 9",       \
	x" Button 10",      \
	x" Button 11",      \
	x" Button 12",      \
	x" Button 13",      \
	x" Button 14",      \
	x" Button 15",      \
	x" Button 16",      \
	x" Button 17",      \
	x" Button 18",      \
	x" Button 19",      \
	x" Button 20",      \
	x" Button 21",      \
	x" Button 22",      \
	x" Button 23",      \
	x" Button 24",      \
	x" Button 25",      \
	x" Button 26",      \
	x" Button 27",      \
	x" Button 28",      \
	x" Button 29",      \
	x" Button 30",      \
	x" Button 31",      \
	x" Button 32",
	BUTTON_NAMES("P1")
	BUTTON_NAMES("P2")
	BUTTON_NAMES("P3")
	BUTTON_NAMES("P4")
};

const char* JoystickAxisNames[JOY_NAME_SIZE] = {
	"...",
#define AXIS_BUTTONS(x,n)   x" Axis "n" -", x" Axis "n" +",
#define AXIS_NAMES(x)     \
	AXIS_BUTTONS(x,"1")   \
	AXIS_BUTTONS(x,"2")   \
	AXIS_BUTTONS(x,"3")   \
	AXIS_BUTTONS(x,"4")   \
	AXIS_BUTTONS(x,"5")   \
	AXIS_BUTTONS(x,"6")   \
	AXIS_BUTTONS(x,"7")   \
	AXIS_BUTTONS(x,"8")   \
	AXIS_BUTTONS(x,"9")   \
	AXIS_BUTTONS(x,"10")  \
	AXIS_BUTTONS(x,"11")  \
	AXIS_BUTTONS(x,"12")  \
	AXIS_BUTTONS(x,"13")  \
	AXIS_BUTTONS(x,"14")  \
	AXIS_BUTTONS(x,"15")  \
	AXIS_BUTTONS(x,"16")
	AXIS_NAMES("P1")
	AXIS_NAMES("P2")
	AXIS_NAMES("P3")
	AXIS_NAMES("P4")
};

const char* JoystickHatNames[JOY_NAME_SIZE] = {
	"...",
#define HAT_BUTTONS(x,n) \
	x" Hat "n" Up",      \
	x" Hat "n" Right",   \
	x" Hat "n" Down",    \
	x" Hat "n" Left",
#define HAT_NAMES(x)     \
	HAT_BUTTONS(x,"1")   \
	HAT_BUTTONS(x,"2")   \
	HAT_BUTTONS(x,"3")   \
	HAT_BUTTONS(x,"4")   \
	HAT_BUTTONS(x,"5")   \
	HAT_BUTTONS(x,"6")   \
	HAT_BUTTONS(x,"7")   \
	HAT_BUTTONS(x,"8")
	HAT_NAMES("P1")
	HAT_NAMES("P2")
	HAT_NAMES("P3")
	HAT_NAMES("P4")
};

const char* JoystickUnknownNames[JOY_NAME_SIZE] = {
	"...",
#define UNKNOWN_NAMES(x) \
	x" unknown 1",       \
	x" unknown 2",       \
	x" unknown 3",       \
	x" unknown 4",       \
	x" unknown 5",       \
	x" unknown 6",       \
	x" unknown 7",       \
	x" unknown 8",       \
	x" unknown 9",       \
	x" unknown 10",      \
	x" unknown 11",      \
	x" unknown 12",      \
	x" unknown 13",      \
	x" unknown 14",      \
	x" unknown 15",      \
	x" unknown 16",      \
	x" unknown 17",      \
	x" unknown 18",      \
	x" unknown 19",      \
	x" unknown 20",      \
	x" unknown 21",      \
	x" unknown 22",      \
	x" unknown 23",      \
	x" unknown 24",      \
	x" unknown 25",      \
	x" unknown 26",      \
	x" unknown 27",      \
	x" unknown 28",      \
	x" unknown 29",      \
	x" unknown 30",      \
	x" unknown 31",      \
	x" unknown 32",
	UNKNOWN_NAMES("P1")
	UNKNOWN_NAMES("P2")
	UNKNOWN_NAMES("P3")
	UNKNOWN_NAMES("P4")
};

// Numbering order: buttons, then axes, then hats
const char* PC_GetJoystickKeyName(int portnum, int keynum)
{
	int keycode = (portnum*JOY_MAX_INPUTS) + keynum;
	int firstAxis = joysticks[portnum].NumButtons;
	int firstHat = firstAxis + (2*joysticks[portnum].NumAxes);
	int firstUnknown = firstHat + (4*joysticks[portnum].NumHats);

	     if (keynum < firstAxis+1)            return JoystickButtonNames[keycode];
	else if (keynum < firstHat+1)             return JoystickAxisNames[keycode-firstAxis];
	else if (keynum < firstUnknown+1)         return JoystickHatNames[keycode-firstHat];
	else                                      return JoystickUnknownNames[keycode-firstUnknown];
}

#ifdef OPENDINGUX
char* OPENDINGUX_GetKeyName(int keycode)
{
	     if (keycode == OPENDINGUX_BUTTON_UP)     return "Up";
	else if (keycode == OPENDINGUX_BUTTON_DOWN)   return "Down";
	else if (keycode == OPENDINGUX_BUTTON_LEFT)   return "Left";
	else if (keycode == OPENDINGUX_BUTTON_RIGHT)  return "Right";
	else if (keycode == OPENDINGUX_BUTTON_A)      return "A";
	else if (keycode == OPENDINGUX_BUTTON_B)      return "B";
	else if (keycode == OPENDINGUX_BUTTON_X)      return "X";
	else if (keycode == OPENDINGUX_BUTTON_Y)      return "Y";
	else if (keycode == OPENDINGUX_BUTTON_L)      return "L";
	else if (keycode == OPENDINGUX_BUTTON_R)      return "R";
	else if (keycode == OPENDINGUX_BUTTON_START)  return "Start";
	else if (keycode == OPENDINGUX_BUTTON_SELECT) return "Select";
	else return "...";
}
#endif

char* JOY_GetKeyName(int keycode)
{
#ifdef OPENDINGUX
	return OPENDINGUX_GetKeyName(keycode);
#elif ANDROID || DARWIN || SDL2
	return (char*)SDL_GetScancodeName(keycode);
#else
	return (char*)SDL_GetKeyName(keycode);
#endif
}
