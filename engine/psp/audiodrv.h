/*
 * OpenBOR - http://www.LavaLit.com
 * -----------------------------------------------------------------------
 * Licensed under the BSD license, see LICENSE in OpenBOR root for details.
 *
 * Copyright (c) 2004 - 2011 OpenBOR Team
 */

#ifndef AUDIODRV_H
#define AUDIODRV_H

void audio_SetVolume(int channel, int left, int right);
int audio_Init(void);
void audio_Term(void);

#endif

