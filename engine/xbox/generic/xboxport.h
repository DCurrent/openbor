/*
 * OpenBOR - http://www.LavaLit.com
 * -----------------------------------------------------------------------
 * Licensed under the BSD license, see LICENSE in OpenBOR root for details.
 *
 * Copyright (c) 2004 - 2011 OpenBOR Team
 */

#ifndef XBOXPORT_H
#define XBOXPORT_H

#include <xtl.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <malloc.h>
#include <io.h>
#include "ram.h"
#include "globals.h"
#include "types.h"

unsigned long xbox_get_playerinput(int playernum);
void xbox_check_events(void);
void xbox_put_image(int src_w, int src_h, s_screen* source);
void xbox_set_palette(char *palette);
void xbox_clear_screen( void );
void xbox_resize(void);
void xbox_pause_audio(int state);
void openborMain(void);
void borExit(int reset);

extern char packfile[128];

#endif