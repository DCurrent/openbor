/*
 * OpenBOR - http://www.LavaLit.com
 * -----------------------------------------------------------------------
 * All rights reserved, see LICENSE in OpenBOR root for details.
 *
 * Copyright (c) 2004 - 2011 OpenBOR Team
 */

#ifndef	TEXTURE_H
#define	TEXTURE_H


void texture_set_wave(float amp);
void apply_texture_wave(s_screen *screen, int x, int y, int width, int height, int offsx, int offsy, s_bitmap *bitmap, int offsd, int step, s_drawmethod* drawmethod);
void apply_texture_plane(s_screen *screen, int x, int y, int width, int height, int fixp_offs, int factor, s_bitmap *bitmap, s_drawmethod* drawmethod);
#endif

