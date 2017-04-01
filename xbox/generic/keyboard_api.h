/*
 * OpenBOR - http://www.LavaLit.com
 * -----------------------------------------------------------------------
 * Licensed under the BSD license, see LICENSE in OpenBOR root for details.
 *
 * Copyright (c) 2004 - 2011 OpenBOR Team
 */

#ifndef KEYBOARD_API_H
#define KEYBOARD_API_H

#define DEBUG_KEYBOARD
#include <xtl.h>
#include <xkbd.h>

/*   the joker's little keyboard api for those didn't want to figure it out themselves   */

/* mail me at joker@crusaders.no if you got any questions, angry mails or whatever.    */

extern XINPUT_DEBUG_KEYSTROKE   g_keyboardStroke;

char Keyboard_Init( int queuesize, int delay, int interval );
char Keyboard_Status( );
char Keyboard_GetVKInput();
char Keyboard_GetASCIIInput();


#endif

