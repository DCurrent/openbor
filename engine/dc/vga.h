/*
 * OpenBOR - http://www.LavaLit.com
 * -----------------------------------------------------------------------
 * Licensed under the BSD license, see LICENSE in OpenBOR root for details.
 *
 * Copyright (c) 2004 - 2011 OpenBOR Team
 */

#ifndef		VGA_H
#define		VGA_H

// Set video mode
void vga_setmode(int m);

// Wait for new vertical retrace (interrupts stay ON)
void vga_vwait(void);

// Set VGA-type palette
void vga_setpalette(char*p);

// Set VGA border colour index
void vga_setborderindex(int n);

// Set mode X (320x240x256 planar VGA)
void vga_setmodex(void);

// Clear video RAM in mode X
void vga_clearmodex(void);

// Activate one of the 3 video pages in mode X.
// Returns a pointer to the active video page (the active video RAM)
// or NULL if the page does not exist.
char *vga_xpageflip(int p);

// Set widescreen mode (320x204x256 linear VGA)
void vga_setwidemode(void);

// Set mode T (288x216x256 VGA)
void vga_setmodet(void);

#endif


