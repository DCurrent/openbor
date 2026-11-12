/*
 * OpenBOR - http://www.chronocrash.com
 * Copyright (c) OpenBOR Team. All rights reserved. See LICENSE.
 */

#ifndef OPENBOR_GPU_TRANSFORM_H
#define OPENBOR_GPU_TRANSFORM_H

#include "types.h"

typedef struct gpu_vertex
{
    /* Destination pixel edges and raw source texel coordinates travel together
     * through clipping. The shader normalizes the selected texel after sampling. */
    float x, y;
    float u, v;
} gpu_vertex;

typedef struct gpu_transform_parameters
{
    int source_width, source_height;
    int target_width, target_height;
    int x, y;
    int centerx, centery; /* Sprite pivot, or zero for screen sources. */
    int is_sprite;
} gpu_transform_parameters;

enum gpu_transform_result
{
    /* FAILED requests fallback; EMPTY and DRAWN both complete native handling. */
    GPU_TRANSFORM_FAILED = -1,
    GPU_TRANSFORM_EMPTY = 0,
    GPU_TRANSFORM_DRAWN = 1
};

enum gpu_sampling_mode
{
    /* Match the shader policies while retaining one callback layout for plain
     * images, fixed-point scale/shear, wrapped planes and rotated geometry. */
    GPU_SAMPLE_FLOOR = 0,
    GPU_SAMPLE_TILE = 1,
    GPU_SAMPLE_FIXED8 = 2,
    GPU_SAMPLE_CONTINUOUS = 3
};

/*
 * Called synchronously, with vertices in perimeter order. The caller may issue
 * a GL_QUADS draw immediately or append the four vertices to a reusable batch.
 * wrap_source selects a gpu_sampling_mode for the shader's wrapSource uniform.
 * The callback returns nonzero on success; zero stops submission with FAILED.
 * The renderer owns repeats/spans and target synchronization. This helper
 * emits one transformed instance. Parameter preflight happens before emission
 * so unsupported input cannot leave a partially submitted native instance.
 */
typedef int (*gpu_transform_emit_quad)(const gpu_vertex vertices[4], int wrap_source,
                                      void *userdata);

int gpu_transform_draw(const gpu_transform_parameters *parameters,
                       const s_drawmethod *method,
                       gpu_transform_emit_quad emit, void *userdata);

#endif
