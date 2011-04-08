/*
 * OpenBOR - http://www.LavaLit.com
 * -----------------------------------------------------------------------
 * All rights reserved, see LICENSE in OpenBOR root for details.
 *
 * Copyright (c) 2004 - 2011 OpenBOR Team
 */

#ifndef		VGA_H
#define		VGA_H


// Set VGA-type palette
void vga_setpalette(unsigned char* palette);
void vga_set_color_correction(int gm, int br);
void vga_vwait();

#endif


