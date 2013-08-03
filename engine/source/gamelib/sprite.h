/*
 * OpenBOR - http://www.LavaLit.com
 * -----------------------------------------------------------------------
 * All rights reserved, see LICENSE in OpenBOR root for details.
 *
 * Copyright (c) 2004 - 2011 OpenBOR Team
 */

#ifndef SPRITE_H
#define SPRITE_H

#ifndef TRANSPARENT_IDX
#define		TRANSPARENT_IDX		0x00
#endif

unsigned fakey_encodesprite(s_bitmap *bitmap);
unsigned encodesprite(int offsx, int offsy, s_bitmap *bitmap, s_sprite *dest);

// common sprite draw function, dispatch all formats
void putsprite(int x, int y, s_sprite *frame, s_screen *screen, s_drawmethod *drawmethod);

// Normal putsprite
void putsprite_8(int x, int y, int is_flip, s_sprite *frame, s_screen *screen, unsigned char *remap, unsigned char *blend);
//with speical effects
void putsprite_ex(int x, int y, s_sprite *frame, s_screen *screen, s_drawmethod *drawmethod);
/*
// 8bit pixel transpixel functions
unsigned char remapcolor(unsigned char* table, unsigned char color, unsigned char unused);
unsigned char blendcolor(unsigned char* table, unsigned char color1, unsigned char color2);
unsigned char blendfillcolor(unsigned char* table, unsigned char unused, unsigned char color);
*/
//----------------------------------------------------------------------------------------------------
//              16/24/32bit palette version
///////////////////////////////////////////////////////////////////////////////////////////////////////
// Normal putsprite
void putsprite_x8p16(int x, int y, int is_flip, s_sprite *frame, s_screen *screen, unsigned short *remap, unsigned short(*fp)(unsigned short, unsigned short));
//with speical effects
void putsprite_ex_x8p16(int x, int y, s_sprite *frame, s_screen *screen, s_drawmethod *drawmethod);

// Normal putsprite
void putsprite_x8p24(int x, int y, int is_flip, s_sprite *frame, s_screen *screen, unsigned char *remap, unsigned(*fp)(unsigned, unsigned));
//with speical effects
void putsprite_ex_x8p24(int x, int y, s_sprite *frame, s_screen *screen, s_drawmethod *drawmethod);

// Normal putsprite
void putsprite_x8p32(int x, int y, int is_flip, s_sprite *frame, s_screen *screen, unsigned *remap, unsigned(*fp)(unsigned, unsigned));
//with speical effects
void putsprite_ex_x8p32(int x, int y, s_sprite *frame, s_screen *screen, s_drawmethod *drawmethod);

/*
unsigned blend_multiply(unsigned color1, unsigned color2);
unsigned blend_screen(unsigned color1, unsigned color2);
unsigned blend_overlay(unsigned color1, unsigned color2);
unsigned blend_hardlight(unsigned color1, unsigned color2);
unsigned blend_dodge(unsigned color1, unsigned color2);
unsigned blend_half(unsigned color1, unsigned color2);
*/
unsigned blend_channel32(register unsigned color1, register unsigned color2, register unsigned a);
unsigned short blend_channel16(unsigned short color1, unsigned short color2, register unsigned a);
#endif
