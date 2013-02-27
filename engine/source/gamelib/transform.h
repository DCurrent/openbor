/*
 * OpenBOR - http://www.LavaLit.com
 * -----------------------------------------------------------------------
 * All rights reserved, see LICENSE in OpenBOR root for details.
 *
 * Copyright (c) 2004 - 2011 OpenBOR Team
 */

#ifndef TRANSFORM_H
#define TRANSFORM_H
inline void gfx_draw_rotate(s_screen* dest, gfx_entry* src, int x, int y, int centerx, int centery, s_drawmethod* drawmethod);
inline void gfx_draw_scale(s_screen *dest, gfx_entry* src, int x, int y, int centerx, int centery, s_drawmethod* drawmethod);
inline void gfx_draw_water(s_screen *dest, gfx_entry* src, int x, int y, int centerx, int centery, s_drawmethod* drawmethod);
inline void gfx_draw_plane(s_screen *dest, gfx_entry* src, int x, int y, int centerx, int centery, s_drawmethod* drawmethod);
//inline void draw_pixel_gfx(s_screen* dest, gfx_entry* src, int dx, int dy, int sx, int sy);
inline void src_seek(int x, int y);
inline void dest_seek(int x, int y);
inline void src_line_inc();
inline void src_line_dec();
inline void src_inc();
inline void src_dec();
inline void dest_line_inc();
inline void dest_line_dec();
inline void dest_inc();
inline void dest_dec();
inline void write_pixel();
char sprite_get_pixel(s_sprite* sprite, int x, int y);
inline void init_gfx_global_draw_stuff(s_screen*, gfx_entry*, s_drawmethod*);
#endif
