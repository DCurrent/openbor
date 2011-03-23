/*
 * OpenBOR - http://www.LavaLit.com
 * -----------------------------------------------------------------------
 * Licensed under the BSD license, see LICENSE in OpenBOR root for details.
 *
 * Copyright (c) 2004 - 2011 OpenBOR Team
 */

#ifndef BITMAP_H
#define BITMAP_H


s_bitmap * allocbitmap(int width, int height, int format);
void freebitmap(s_bitmap*);
void getbitmap(int x, int y, int width, int height, s_bitmap *bitmap, s_screen *screen);
void putbitmap(int, int, s_bitmap*, s_screen*);
void clipbitmap(s_bitmap*,int *,int *,int *,int *);
void flipbitmap(s_bitmap *);

#endif

