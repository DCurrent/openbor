/*
 * OpenBOR - http://www.chronocrash.com
 * -----------------------------------------------------------------------
 * All rights reserved, see LICENSE in OpenBOR root for details.
 *
 * Copyright (c) 2004 - 2011 OpenBOR Team
 */

#ifndef	LOADIMAGE_H
#define LOADIMAGE_H

// Blah.

int loadscreen(char *filename, char *packfile, unsigned char *pal, int format, s_screen **screen);
s_bitmap *loadbitmap(char *filename, char *packfile, int format);
int loadimagepalette(char *filename, char *packfile, unsigned char *pal);
#endif


