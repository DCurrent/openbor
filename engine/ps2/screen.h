#ifndef SCREEN_H
#define SCREEN_H


s_screen * allocscreen(int width, int height);
void freescreen(s_screen * screen);
void copyscreen(s_screen * dest, s_screen * src);
void copyscreen_o(s_screen * dest, s_screen * src, int x, int y);
void clearscreen(s_screen * s);
void scalescreen(s_screen * dest, s_screen * src);

#endif

