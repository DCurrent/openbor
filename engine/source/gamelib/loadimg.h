/*
 * OpenBOR - http://www.chronocrash.com
 * -----------------------------------------------------------------------
 * All rights reserved, see LICENSE in OpenBOR root for details.
 *
 * Copyright (c)  OpenBOR Team
 */

#ifndef	LOADIMAGE_H
#define LOADIMAGE_H

// Blah.

int loadscreen(char *filename, char *packfile, unsigned char *pal, int format, s_screen **screen);
/*
* Caskey, Damon V.
* 2026-08-27
*
* Probe the indexed decoder for background loading without reporting an
* incompatible color type. All other validation errors remain enabled.
*/
int loadscreen_indexed_probe(char *filename, char *packfile, unsigned char *pal, int format, s_screen **screen);
int loadscreen32(char *filename, char *packfile, s_screen **screen);
s_bitmap *loadbitmap(char *filename, char *packfile, int format);
int loadimagepalette(char *filename, char *packfile, unsigned char *pal);
#endif


