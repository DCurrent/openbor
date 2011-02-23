#ifndef	TEXTURE_H
#define	TEXTURE_H


void texture_set_wave(float amp);
void texture_wave(s_screen *screen, int x, int y, int width, int height, int offsx, int offsy, s_bitmap *bitmap, int offsd, int step);
void texture_plane(s_screen *screen, int x, int y, int width, int height, int fixp_offs, int factor, s_bitmap *bitmap);

#endif

