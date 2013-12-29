/*
 * OpenBOR - http://www.chronocrash.com
 * -----------------------------------------------------------------------
 * All rights reserved, see LICENSE in OpenBOR root for details.
 *
 * Copyright (c) 2004 - 2011 OpenBOR Team
 */

#ifndef DRAW_H
#define DRAW_H

// Primitive drawing functions. Support clipping.


// Line function, not particularly fast
void line(int sx, int sy, int ex, int ey, int colour, s_screen *screen, int alpha);

void drawbox(int x, int y, int width, int height, int colour, s_screen *screen, int alpha);
//void drawbox_trans(int x, int y, int width, int height, char colour, s_screen *screen, char *lut);

// Pretty slow circle function
void circle(int x, int y, int radius, int colour, s_screen *screen, int alpha);

// Always handy
void _putpixel(int x, int y, int colour, s_screen *screen, int alpha);


///////////////////////////////////////
///////   16/32bit version ...........
///////////////////////////////////////////
void line16(int sx, int sy, int ex, int ey, unsigned short colour, s_screen *screen, int alpha);
void drawbox16(int x, int y, int width, int height, unsigned short colour, s_screen *screen, int alpha);
void _putpixel16(int x, int y, unsigned short colour, s_screen *screen, int alpha);

void line32(int sx, int sy, int ex, int ey, unsigned colour, s_screen *screen, int alpha);
void drawbox32(int x, int y, int width, int height, unsigned colour, s_screen *screen, int alpha);
void _putpixel32(int x, int y, unsigned colour, s_screen *screen, int alpha);


//======================== root methods ==================================

void putbox(int x, int y, int width, int height, int colour, s_screen *screen, s_drawmethod *drawmethod);
void putline(int sx, int sy, int ex, int ey, int colour, s_screen *screen, s_drawmethod *drawmethod);
void putpixel(unsigned x, unsigned y, int colour, s_screen *screen, s_drawmethod *drawmethod);


#endif


