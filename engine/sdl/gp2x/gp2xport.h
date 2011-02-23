/*
 * OpenBOR - http://www.LavaLit.com
 * -----------------------------------------------------------------------
 * Licensed under the BSD license, see LICENSE in OpenBOR root for details.
 *
 * Copyright (c) 2004 - 2011 OpenBOR Team
 */

#ifndef GP2XPORT_H
#define GP2XPORT_H

int gp2x_init();
void gp2x_end();
void gp2x_sound_set_volume(int l, int r);
void gp2x_set_clock(int mhz);
void gp2x_video_wait_vsync();
void * UpperMalloc(int size);
void UpperFree(void* mem);

#endif
