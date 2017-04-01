/*
 * OpenBOR - http://www.chronocrash.com
 * -----------------------------------------------------------------------
 * All rights reserved, see LICENSE in OpenBOR root for details.
 *
 * Copyright (c) 2004 - 2014 OpenBOR Team
 */

// A soundblaster interface. No bugs?

#ifndef SBLASTER_H
#define SBLASTER_H

#define	SB_MASTERVOL	0x22
#define	SB_VOICEVOL		0x04

// The interface
int SB_playstart(int bits, int samplerate);
void SB_playstop(void);

//int SB_getvolume(char dev);
void SB_setvolume(char dev, char volume);
void SB_updatevolume(int volume);

#endif
