/*
 * OpenBOR - http://www.LavaLit.com
 * -----------------------------------------------------------------------
 * All rights reserved, see LICENSE in OpenBOR root for details.
 *
 * Copyright (c) 2004 - 2011 OpenBOR Team
 */

#ifndef	FONT_H
#define	FONT_H

extern int font_heights[];
extern int font_monowidths[];
void font_unload(int which);
int font_load(int which, char *filename, char *packfile, int monospace);
int font_string_width(int which, char* buf, ...);
void font_printf(int x, int y, int which, int layeroffset, char *format, ...);
void screen_printf(s_screen * screen, int x, int y, int which, char *format, ...);

#endif
