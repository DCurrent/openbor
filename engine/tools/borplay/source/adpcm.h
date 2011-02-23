/*
 * OpenBOR - http://www.LavaLit.com
 * -----------------------------------------------------------------------
 * Licensed under the BSD license, see LICENSE in OpenBOR root for details.
 *
 * Copyright (c) 2004 - 2011 OpenBOR Team
 */

#ifndef ADPCM_H
#define ADPCM_H

char adpcm_index(int channel);
short adpcm_valprev(int channel);
void adpcm_loop_reset(int channel, short valprev, char index);

void adpcm_reset();

int adpcm_encode(short * indata, unsigned char * outdata, int len, int channels);
int adpcm_decode(unsigned char * indata, short * outdata, int len, int channels);

#endif

