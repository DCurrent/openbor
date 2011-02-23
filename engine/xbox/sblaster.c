/*
 * OpenBOR - http://www.LavaLit.com
 * -----------------------------------------------------------------------
 * Licensed under the BSD license, see LICENSE in OpenBOR root for details.
 *
 * Copyright (c) 2004 - 2011 OpenBOR Team
 */

int SB_playstart(int bits, int samplerate){
    xbox_pause_audio(0);
	return 1;
}

void SB_playstop(){
    xbox_pause_audio(1);
}

char SB_getvolume(char dev){
	return 0;
}

char SB_setvolume(char dev, char volume){
	return 0;
}
