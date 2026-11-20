/*
 * OpenBOR - http://www.chronocrash.com
 * -----------------------------------------------------------------------
 * All rights reserved, see LICENSE in OpenBOR root for details.
 *
 * Copyright (c)  OpenBOR Team
 */

#ifndef DRAW_H
#define DRAW_H

/*
 * Damon V. Caskey
 * 2026-11-20
 * RGB32 scene primitives share the GPU drawmethod and clipping contract.
 */
void putbox(int x, int y, int width, int height, int colour, s_screen *screen, s_drawmethod *drawmethod);
void putline(int sx, int sy, int ex, int ey, int colour, s_screen *screen, s_drawmethod *drawmethod);
void putpixel(unsigned x, unsigned y, int colour, s_screen *screen, s_drawmethod *drawmethod);

#endif
