/*
 * OpenBOR - http://www.LavaLit.com
 * -----------------------------------------------------------------------
 * Licensed under the BSD license, see LICENSE in OpenBOR root for details.
 *
 * Copyright (c) 2004 - 2011 OpenBOR Team
 */

#ifndef VIDEO_H
#define VIDEO_H

extern int scaleVideo;

int video_set_mode(s_videomodes);
int video_copy_screen(s_screen*);
void video_clearscreen();

#endif
