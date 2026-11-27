/*
 * OpenBOR - https://www.chronocrash.com
 * Copyright (c) OpenBOR Team. All rights reserved. See LICENSE.
 * Damon V. Caskey - 2026-11-27
 */
#ifndef OPENBOR_MESHMAP_RENDER_H
#define OPENBOR_MESHMAP_RENDER_H

#include "meshmap.h"
#include "gpu.h"

/* Three row-major rows map world XYZ1 to screen-X numerator, screen-Y
 * numerator, and positive homogeneous W. This transforms the view only;
 * the authored terrain and its collision grid keep their fixed world axes. */
typedef struct s_meshmap_view
{
    double projection[12];
    double near_w;
} s_meshmap_view;

typedef struct s_meshmap_render_buffer
{
    gpu_mesh_vertex *vertices;
    size_t triangle_count, triangle_capacity;
} s_meshmap_render_buffer;

void meshmap_view_orthographic(s_meshmap_view *view, double offset_x, double offset_y);
int meshmap_view_valid(const s_meshmap_view *view);
int meshmap_project(const s_meshmap *map, const s_meshmap_view *view,
                     unsigned target_width, unsigned target_height,
                     unsigned artwork_width, unsigned artwork_height,
                     s_meshmap_render_buffer *buffer);
void meshmap_render_buffer_free(s_meshmap_render_buffer *buffer);
/* Explicit draws honor active ancestry, but do not require MESHMAP_DRAW;
 * that flag enables automatic native scene queueing only. */
int meshmap_draw(const s_meshmap_store *store, uint32_t id, s_screen *target,
                  const s_meshmap_view *view, const s_drawmethod *method);
/* Queue stores IDs, resolving current resources during playback. Clear the
 * sprite queue before destroying its owning level collection. */
void meshmap_queue_native(s_meshmap_store *store, int offset_x, int offset_y);
void meshmap_render_shutdown(void);

#endif
