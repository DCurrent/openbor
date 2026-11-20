/*
 * OpenBOR - http://www.chronocrash.com
 * -----------------------------------------------------------------------
 * All rights reserved, see LICENSE in OpenBOR root for details.
 *
 * Copyright (c)  OpenBOR Team
 */

#ifndef SCREEN_H
#define SCREEN_H
#include "types.h"
#include "globals.h"
s_screen *allocscreen(int width, int height, int pixelformat);
void freescreen(s_screen **screen);
void copyscreen(s_screen *dest, s_screen *src);
void clearscreen(s_screen *s);
void putscreen(s_screen *dest, s_screen *src, int x, int y, s_drawmethod *drawmethod);
#endif

