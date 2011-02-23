#ifndef SPRITE_H
#define SPRITE_H


#define		TRANSPARENT_IDX		0x00


unsigned int fakey_encodesprite(s_bitmap *bitmap);
unsigned int encodesprite(int offsx, int offsy, s_bitmap *bitmap, void *dest);

// Normal putsprite
void putsprite(int x, int y, s_sprite *frame, s_screen *screen);

// Remap, uses 256-byte table
void putsprite_remap(int x, int y, s_sprite *frame, s_screen *screen, char *lut);

// Blend, uses 64K table
void putsprite_blend(int x, int y, s_sprite *frame, s_screen *screen, char *lut);


#endif
