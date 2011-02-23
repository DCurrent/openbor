/*
 * OpenBOR - http://www.LavaLit.com
 * -----------------------------------------------------------------------
 * Licensed under the BSD license, see LICENSE in OpenBOR root for details.
 *
 * Copyright (c) 2004 - 2011 OpenBOR Team
 */

#ifndef		ANIGIF_H
#define		ANIGIF_H

// Animated GIF player.

#define			ANIGIF_DECODE_END		0
#define			ANIGIF_DECODE_FRAME		1
#define			ANIGIF_DECODE_RETRY		3



// Returns true on succes
int anigif_open(char *filename, char *packfilename, unsigned char *pal);

// Returns type of action (frame, retry or end)
int anigif_decode(s_screen * screen, int *delay, int x, int y);

void anigif_close();

#endif
