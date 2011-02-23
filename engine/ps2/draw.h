#ifndef DRAW_H
#define DRAW_H

// Primitive drawing functions. Support clipping.


// Line function, not particularly fast
void line(int sx, int sy, int ex, int ey, int colour, s_screen *screen);

void drawbox(int x, int y, int width, int height, char colour, s_screen *screen);
void drawbox_trans(int x, int y, int width, int height, char colour, s_screen *screen, char *lut);

// Pretty slow circle function
void circle(int x, int y, int radius, char colour, s_screen *screen);

// Always handy
void putpixel(int x, int y, char colour, s_screen *screen);



#endif


