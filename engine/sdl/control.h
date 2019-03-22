/*
 * OpenBOR - http://www.chronocrash.com
 * -----------------------------------------------------------------------
 * All rights reserved, see LICENSE in OpenBOR root for details.
 *
 * Copyright (c) 2004 - 2019 OpenBOR Team
 */

#ifndef	CONTROL_H
#define	CONTROL_H
// Generic control stuff (keyboard+joystick).

#include <stdint.h>

// 32 is an arbitrary number larger than the number of input devices that will ever be available
#define MAX_DEVICES                32
#define CONTROL_DEVICE_NAME_SIZE   64

typedef struct {
    int deviceID;
    uint32_t keyflags;
    uint32_t newkeyflags;
} s_playercontrols;

void control_init();
void control_exit();

/* Listen to input from deviceID. The first input from deviceID will be returned by the next call to
   control_getremappedkey(). Call with deviceID=-1 to finish remapping. */
void control_remapdevice(int deviceID);

// Returns the keycode of the first key pressed on the device being remapped, or -1 if nothing has been pressed yet
int control_getremappedkey();

// Returns an array of size SDID_COUNT
int *control_getmappings(int deviceID);

// Resets mappings for device to default
void control_resetmappings(int deviceID);
void control_update(s_playercontrols **allPlayerControls, int numPlayers);
void control_update_keyboard(s_playercontrols *keyboardControls);
const char *control_getkeyname(int deviceID, int keycode);
bool control_isvaliddevice(int deviceID);
const char *control_getdevicename(int deviceID);
void control_rumble(int deviceID, int ratio, int msec);

bool control_loadmappings(const char *filename);
bool control_savemappings(const char *filename);

// clears saved mappings and resets every device's mappings to defaults
void control_clearmappings();


#define control_getmappedkeyname(deviceID, key) control_getkeyname(deviceID, control_getmappings(deviceID)[key])

#ifdef ANDROID
bool is_touchpad_vibration_enabled();
#endif /* defined(ANDROID) */

#endif /* defined(CONTROL_H) */

