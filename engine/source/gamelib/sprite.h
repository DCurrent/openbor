/*
 * OpenBOR - http://www.chronocrash.com
 * -----------------------------------------------------------------------
 * All rights reserved, see LICENSE in OpenBOR root for details.
 *
 * Copyright (c)  OpenBOR Team
 */

#ifndef SPRITE_H
#define SPRITE_H

#ifndef TRANSPARENT_IDX
#define TRANSPARENT_IDX 0x00
#endif

unsigned fakey_encodesprite(s_bitmap *bitmap);
unsigned encodesprite(int offsx, int offsy, s_bitmap *bitmap, s_sprite *dest);

/*
 * Damon V. Caskey
 * 2026-11-20
 * Sprite drawing is GPU-only; explicit source-index reads remain available
 * to the script image-access API independently of scene rendering.
 */
void putsprite(int x, int y, s_sprite *frame, s_screen *screen, s_drawmethod *drawmethod);
char sprite_get_pixel(s_sprite *sprite, int x, int y);

#endif
