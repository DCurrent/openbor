/*
 * OpenBOR - http://www.LavaLit.com
 * -----------------------------------------------------------------------
 * Licensed under the BSD license, see LICENSE in OpenBOR root for details.
 *
 * Copyright (c) 2004 - 2011 OpenBOR Team
 */

#include "vertex.h"

void setVertexTexture(
	tVertexTexture *t,
	short texture_width,
	short texture_height,
	short texture_step,
	float vertex_width,
	float vertex_height,
	float vertex_x,
	float vertex_y)
{
	t->texture_width           = texture_width;
	t->texture_width_remaining = texture_width;
	t->texture_step            = texture_step;
	t->output_texture_y_start = 0;
	t->output_texture_x_end   = 0;
	t->output_texture_y_end   = texture_height;
	t->output_last = 0;
	t->vertex_step = vertex_width * (float) texture_step / (float) texture_width;
	t->vertex_x_plus_vertex_width = vertex_x + vertex_width;
	t->output_vertex_y_start = vertex_y;
	t->output_vertex_x_end   = vertex_x;
	t->output_vertex_y_end   = vertex_y + vertex_height;
}

void getVertexTexture(tVertexTexture *t)
{
	t->output_texture_x_start = t->output_texture_x_end;
	t->output_vertex_x_start  = t->output_vertex_x_end;
	if (t->texture_width_remaining > t->texture_step)
	{
		t->texture_width_remaining -= t->texture_step;
		t->output_texture_x_end    += t->texture_step;
		t->output_vertex_x_end     += t->vertex_step;
	}
	else
	{
		t->output_texture_x_end = t->texture_width;
		t->output_vertex_x_end  = t->vertex_x_plus_vertex_width;
		t->output_last = 1;
	}
}
