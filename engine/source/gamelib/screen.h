/*
 * OpenBOR - http://www.chronocrash.com
 * -----------------------------------------------------------------------
 * All rights reserved, see LICENSE in OpenBOR root for details.
 *
 * Copyright (c) 2004 - 2014 OpenBOR Team
 */

#ifndef SCREEN_H
#define SCREEN_H
#include "types.h"
#include "globals.h"
s_screen *allocscreen(int width, int height, int pixelformat);
void freescreen(s_screen **screen);
void copyscreen(s_screen *dest, s_screen *src);
void copyscreen_o(s_screen *dest, s_screen *src, int x, int y);

void clearscreen(s_screen *s);
void scalescreen(s_screen *dest, s_screen *src);

void copyscreen_trans(s_screen *dest, s_screen *src, int x, int y);
void copyscreen_remap(s_screen *dest, s_screen *src, int x, int y, unsigned char *remap);
void blendscreen(s_screen *dest, s_screen *src, int x, int y, unsigned char *lut);
void putscreen(s_screen *dest, s_screen *src, int x, int y, s_drawmethod *drawmethod);
void zoomscreen(s_screen *dest, s_screen *src, int centerx, int centery, int scalex, int scaley);

//------------------------------16

void putscreenx8p16(s_screen *dest, s_screen *src, int x, int y, int key, unsigned short *remap, unsigned short(*blendfp)(unsigned short, unsigned short));
void blendscreen16(s_screen *dest, s_screen *src, int x, int y, int key, unsigned short(*blendfp)(unsigned short, unsigned short));
void scalescreen16(s_screen *dest, s_screen *src);


//------------------------------24

void putscreenx8p24(s_screen *dest, s_screen *src, int x, int y, int key, unsigned *remap, unsigned(*blendfp)(unsigned, unsigned));
void blendscreen24(s_screen *dest, s_screen *src, int x, int y, int key, unsigned(*blendfp)(unsigned, unsigned));


//------------------------------32

void putscreenx8p32(s_screen *dest, s_screen *src, int x, int y, int key, unsigned *remap, unsigned(*blendfp)(unsigned, unsigned));
void blendscreen32(s_screen *dest, s_screen *src, int x, int y, int key, unsigned(*blendfp)(unsigned, unsigned));
void scalescreen32(s_screen *dest, s_screen *src);


#endif

