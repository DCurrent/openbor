/*
 * OpenBOR - http://www.chronocrash.com
 * Copyright (c) OpenBOR Team. All rights reserved. See LICENSE.
 *
 * Damon V. Caskey
 * 2026-11-20
 * Native preparation for the drawmethod's general surface bend. These helpers
 * prepare geometry only; source sampling and scene pixels remain on the GPU.
 */
#ifndef OPENBOR_GPU_CURVE_H
#define OPENBOR_GPU_CURVE_H

#include <stddef.h>
#include "gpu_transform.h"

#define GPU_CURVE_MAX_QUADS 8192u
#define GPU_CURVE_PIXEL_TOLERANCE 0.25

typedef struct gpu_curve_vertex
{
    float x, y;
    float u, v;
    /* Positive clip W. The vertex shader multiplies projected XY by W before
     * assigning gl_Position, preserving perspective-correct source sampling. */
    float w;
} gpu_curve_vertex;

typedef struct gpu_curve_mesh
{
    gpu_curve_vertex *vertices;
    size_t quad_count;
    size_t quad_capacity;
} gpu_curve_mesh;

/* Build one instance atomically in caller-owned reusable storage. Returns a
 * gpu_transform_result; FAILED and EMPTY leave quad_count zero. Source texels
 * include curve scroll offsets, so a renderer may zero those offsets for a
 * geometry cache and add them while collecting vertices. Viewport, drawmethod
 * clip and near-plane clipping preserve homogeneous interpolation. */
int gpu_curve_build(const gpu_transform_parameters *parameters,
                    const s_drawmethod *method, gpu_curve_mesh *mesh);

/* Project a source point using exactly the same surface and pivot conventions.
 * This helper does not clip to the source, viewport or drawmethod rectangle.
 * EMPTY means the point is behind the near plane or the placement is singular;
 * FAILED means invalid input. The output is written only on DRAWN. */
int gpu_curve_map_point(const gpu_transform_parameters *parameters,
                        const s_drawmethod *method, double source_x,
                        double source_y, gpu_curve_vertex *vertex);

void gpu_curve_free(gpu_curve_mesh *mesh);

#endif
