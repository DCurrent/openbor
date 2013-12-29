/*
 * OpenBOR - http://www.chronocrash.com
 * -----------------------------------------------------------------------
 * All rights reserved, see LICENSE in OpenBOR root for details.
 *
 * Copyright (c) 2004 - 2011 OpenBOR Team
 */

#ifndef	SPRITEQ_H
#define	SPRITEQ_H


#define			SFX_NONE		0
#define			SFX_REMAP		1
#define			SFX_BLEND		2
#define			SFX_SPECIAL		3

extern int  pixelformat;
// Sprite queueing and sorting
void spriteq_add_sprite(int x, int y, int z, int id, s_drawmethod *pdrawmethod, int sortid);
void spriteq_add_frame(int x, int y, int z, s_sprite *frame, s_drawmethod *pdrawmethod, int sortid);
void spriteq_add_dot(int sx, int sy, int z, int colour, s_drawmethod *pdrawmethod);
void spriteq_add_line(int sx, int sy, int ex, int ey, int z, int colour, s_drawmethod *pdrawmethod);
void spriteq_add_box(int x, int y, int width, int height, int z, int colour, s_drawmethod *pdrawmethod);
void spriteq_add_screen(int x, int y, int z, s_screen *ps, s_drawmethod *pdrawmethod, int sortid);
void spriteq_draw(s_screen *screen, int newonly, int minz, int maxz, int dx, int dy);

void spriteq_lock();
void spriteq_unlock();
int  spriteq_islocked();

void spriteq_clear(void);


#endif

