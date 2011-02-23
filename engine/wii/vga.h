/*
 * OpenBOR - http://www.LavaLit.com
 * -----------------------------------------------------------------------
 * Licensed under the BSD license, see LICENSE in OpenBOR root for details.
 *
 * Copyright (c) 2004 - 2011 OpenBOR Team
 */

#ifndef		VGA_H
#define		VGA_H


// Set VGA-type palette
void vga_setpalette(unsigned char* palette);

// Set brightness/gamma correction
void  vga_set_color_correction(int br, int gm);

// Wait for a vertical retrace
void vga_vwait();

#endif


