/*
 * OpenBOR - http://www.chronocrash.com
 * -----------------------------------------------------------------------
 * All rights reserved, see LICENSE in OpenBOR root for details.
 *
 * Copyright (c)  OpenBOR Team
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
	x" Button 10",       \
	x" Button 11",       \
	x" Button 12",       \
	x" Button 13",       \
	x" Button 14",       \
	x" Button 15",       \
	x" Button 16",       \
	x" Button 17",       \
	x" Button 18",       \
	x" Button 19",       \
	x" Button 20",       \
	x" Button 21",       \
	x" Button 22",       \
	x" Button 23",       \
	x" Button 24",       \
	x" Button 25",       \
	x" Button 26",       \
	x" Button 27",       \
	x" Button 28",       \
	x" Button 29",       \
	x" Button 30",       \
	x" Button 31",       \
	x" Button 32",       \
	x" Button 33",       \
	x" Button 34",       \
	x" Button 35",       \
	x" Button 36",       \
	x" Button 37",       \
	x" Button 38",       \
	x" Button 39",       \
	x" Button 40",       \
	x" Button 41",       \
	x" Button 42",       \
	x" Button 43",       \
	x" Button 44",       \
	x" Button 45",       \
	x" Button 46",       \
	x" Button 47",       \
	x" Button 48",       \
	x" Button 49",       \
	x" Button 50",       \
	x" Button 51",       \
	x" Button 52",       \
	x" Button 53",       \
	x" Button 54",       \
	x" Button 55",       \
	x" Button 56",       \
	x" Button 57",       \
	x" Button 58",       \
	x" Button 59",       \
	x" Button 60",
	JOYSTICK_NAMES("P1")
	JOYSTICK_NAMES("P2")
	JOYSTICK_NAMES("P3")
	JOYSTICK_NAMES("P4")
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
	x" unknown 28",     \
	x" unknown 29",       \
	x" unknown 30",       \
	x" unknown 31",       \
	x" unknown 32",       \
	x" unknown 33",       \
	x" unknown 34",       \
	x" unknown 35",       \
	x" unknown 36",       \
	x" unknown 37",       \
	x" unknown 38",       \
	x" unknown 39",       \
	x" unknown 40",       \
	x" unknown 41",       \
	x" unknown 42",       \
	x" unknown 43",       \
	x" unknown 44",       \
	x" unknown 45",       \
	x" unknown 46",       \
	x" unknown 47",       \
	x" unknown 48",       \
	x" unknown 49",       \
	x" unknown 50",       \
	x" unknown 51",       \
	x" unknown 52",       \
	x" unknown 53",       \
	x" unknown 54",       \
	x" unknown 55",       \
	x" unknown 56",       \
	x" unknown 57",       \
	x" unknown 58",       \
	x" unknown 59",       \
	x" unknown 60",
	GAMEPARK_NAMES("P1")
	GAMEPARK_NAMES("P2")
	GAMEPARK_NAMES("P3")
	GAMEPARK_NAMES("P4")
	"undefined"
};

const u64 JoystickBits[JOY_MAX_INPUTS + 1] = {
	0x0000000000000000, // No Buttons Pressed
	0x0000000000000001, // Hat Up
	0x0000000000000002,	// Hat Right
	0x0000000000000004, // Hat Down
	0x0000000000000008,	// Hat Left
	0x0000000000000010,	// Button 1
	0x0000000000000020,	// Button 2
	0x0000000000000040,	// Button 3
	0x0000000000000080,	// Button 4
	0x0000000000000100,	// Button 5
	0x0000000000000200,	// Button 6
	0x0000000000000400,	// Button 7
	0x0000000000000800,	// Button 8
	0x0000000000001000, // Button 9
	0x0000000000002000,	// Button 10
	0x0000000000004000,	// Button 11
	0x0000000000008000,	// Button 12
	0x0000000000010000,	// Button 13
	0x0000000000020000,	// Button 14
	0x0000000000040000,	// Button 15
	0x0000000000080000,	// Button 16
	0x0000000000100000,	// Button 17
	0x0000000000200000,	// Button 18
	0x0000000000400000,	// Button 19
	0x0000000000800000,	// Button 20
	0x0000000001000000,	// Button 21
	0x0000000002000000,	// Button 22
	0x0000000004000000,	// Button 23
	0x0000000008000000,	// Button 24
	0x0000000010000000,	// Button 25
	0x0000000020000000,	// Button 26
	0x0000000040000000,	// Button 27
	0x0000000080000000, // Button 28

	0x0000000100000000,
	0x0000000200000000,
	0x0000000400000000,
	0x0000000800000000,
	0x0000001000000000,
	0x0000002000000000,
	0x0000004000000000,
	0x0000008000000000,
	0x0000010000000000,
	0x0000020000000000,
	0x0000040000000000,
	0x0000080000000000,
	0x0000100000000000,
	0x0000200000000000,
	0x0000400000000000,
	0x0000800000000000,
	0x0001000000000000,
	0x0002000000000000,
	0x0004000000000000,
	0x0008000000000000,
	0x0010000000000000,
	0x0020000000000000,
	0x0040000000000000,
	0x0080000000000000,
	0x0100000000000000,
	0x0200000000000000,
	0x0400000000000000,
	0x0800000000000000,
	0x1000000000000000,
	0x2000000000000000,
	0x4000000000000000,
	0x8000000000000000  // Button 60
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
	x" Button 32",      \
	x" Button 33",       \
	x" Button 34",       \
	x" Button 35",       \
	x" Button 36",       \
	x" Button 37",       \
	x" Button 38",       \
	x" Button 39",       \
	x" Button 40",       \
	x" Button 41",       \
	x" Button 42",       \
	x" Button 43",       \
	x" Button 44",       \
	x" Button 45",       \
	x" Button 46",       \
	x" Button 47",       \
	x" Button 48",       \
	x" Button 49",       \
	x" Button 50",       \
	x" Button 51",       \
	x" Button 52",       \
	x" Button 53",       \
	x" Button 54",       \
	x" Button 55",       \
	x" Button 56",       \
	x" Button 57",       \
	x" Button 58",       \
	x" Button 59",       \
    x" Button 60",       \
    x" Button 61",       \
    x" Button 62",       \
    x" Button 63",       \
    x" Button 64",
	BUTTON_NAMES("P1")
	BUTTON_NAMES("P2")
	BUTTON_NAMES("P3")
	BUTTON_NAMES("P4")
};

const char* JoystickAxisNames[JOY_NAME_SIZE] = {
	"...",
#define AXIS_BUTTONS(x,n) \
    x" Axis "n" -",       \
    x" Axis "n" +",
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
	AXIS_BUTTONS(x,"16")  \
	AXIS_BUTTONS(x,"17")  \
	AXIS_BUTTONS(x,"18")  \
	AXIS_BUTTONS(x,"19")  \
	AXIS_BUTTONS(x,"20")  \
	AXIS_BUTTONS(x,"21")  \
	AXIS_BUTTONS(x,"22")  \
	AXIS_BUTTONS(x,"23")  \
	AXIS_BUTTONS(x,"24")  \
	AXIS_BUTTONS(x,"25")  \
	AXIS_BUTTONS(x,"26")  \
	AXIS_BUTTONS(x,"27")  \
	AXIS_BUTTONS(x,"28")  \
	AXIS_BUTTONS(x,"29")  \
	AXIS_BUTTONS(x,"30")  \
	AXIS_BUTTONS(x,"31")  \
	AXIS_BUTTONS(x,"32")
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
	HAT_BUTTONS(x,"8")   \
	HAT_BUTTONS(x,"9")   \
	HAT_BUTTONS(x,"10")   \
	HAT_BUTTONS(x,"11")   \
	HAT_BUTTONS(x,"12")   \
	HAT_BUTTONS(x,"13")   \
	HAT_BUTTONS(x,"14")   \
	HAT_BUTTONS(x,"15")   \
	HAT_BUTTONS(x,"16")
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
	x" unknown 32",      \
	x" unknown 33",       \
	x" unknown 34",       \
	x" unknown 35",       \
	x" unknown 36",       \
	x" unknown 37",       \
	x" unknown 38",       \
	x" unknown 39",       \
	x" unknown 40",       \
	x" unknown 41",       \
	x" unknown 42",       \
	x" unknown 43",       \
	x" unknown 44",       \
	x" unknown 45",       \
	x" unknown 46",       \
	x" unknown 47",       \
	x" unknown 48",       \
	x" unknown 49",       \
	x" unknown 50",       \
	x" unknown 51",       \
	x" unknown 52",       \
	x" unknown 53",       \
	x" unknown 54",       \
	x" unknown 55",       \
	x" unknown 56",       \
	x" unknown 57",       \
	x" unknown 58",       \
	x" unknown 59",       \
    x" unknown 60",       \
    x" unknown 61",       \
    x" unknown 62",       \
    x" unknown 63",       \
    x" unknown 64",
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

char* JOY_GetKeyName(int keycode)
{
	return (char*)SDL_GetScancodeName(keycode);
}
