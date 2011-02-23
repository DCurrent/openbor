#ifndef BITMAP_H
#define BITMAP_H


s_bitmap * allocbitmap(int width, int height);
void freebitmap(s_bitmap*);
void getbitmap(int x, int y, int width, int height, s_bitmap *bitmap, s_screen *screen);
void putbitmap(int, int, s_bitmap*, s_screen*);
void clipbitmap(s_bitmap*,int *,int *,int *,int *);
void flipbitmap(s_bitmap *);

#endif

