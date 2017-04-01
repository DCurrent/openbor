/*
 * OpenBOR - http://www.LavaLit.com
 * -----------------------------------------------------------------------
 * Licensed under the BSD license, see LICENSE in OpenBOR root for details.
 *
 * Copyright (c) 2004 - 2011 OpenBOR Team
 */

#ifndef __VERTEX_H__
#define __VERTEX_H__

typedef struct
{
	short texture_width;
	short texture_width_remaining;
	short texture_step;
	float vertex_step;
	float vertex_x_plus_vertex_width;
	short output_texture_x_start;
	short output_texture_y_start;
	short output_texture_x_end;
	short output_texture_y_end;
	float output_vertex_x_start;
	float output_vertex_y_start;
	float output_vertex_x_end;
	float output_vertex_y_end;
	int output_last;
}
tVertexTexture;

void setVertexTexture(
	tVertexTexture *t,
	short texture_width,
	short texture_height,
	short texture_step,
	float vertex_width,
	float vertex_height,
	float vertex_x,
	float vertex_y);

void getVertexTexture(tVertexTexture *t);

#endif
