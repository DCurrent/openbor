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

typedef struct gpu_affine_sampling
{
    /* Homogeneous local-source numerators at the draw position's pixel center.
     * Each row contains X step, Y step and origin. Approximate floats choose
     * a candidate; whole parts plus seven signed fractional bytes correct its
     * boundary without relying on optimizer-sensitive compensated arithmetic. */
    float approximate_x[3], approximate_y[3], approximate_denominator;
    float anchor[2], pivot[2];
    float x[8][3], y[8][3], denominator[8];
} gpu_affine_sampling;

typedef struct gpu_shear_sampling
{
    /* Canonical whole/byte pairs retain exact legacy 8.8 row coverage. The
     * bottom-relative row is evaluated by the shader instead of CPU geometry.
     * Read packed values only when enabled is nonzero. */
    int enabled;
    float origin[3]; /* Whole X, byte X, exclusive destination bottom. */
    float extent[3]; /* Whole width, byte width, destination row count. */
    float shift[2];  /* Whole/byte signed step toward the previous row. */
    /* Nearest uses 8.8 origins/increments. Filtered draws use source-edge
     * coordinates: source X at the fixed row origin, Y at the bottom row's
     * pixel center, then true inverse X and bottom-to-top Y increments. */
    float source[2], step[2];
    float clip[2];   /* Destination clip: inclusive left, exclusive right. */
} gpu_shear_sampling;

typedef struct gpu_water_sampling
{
    /* Shader sampling begins at the first visible source row. Keeping its
     * already reduced phase avoids repeating top-clipped work per fragment. */
    int enabled;
    float origin[3]; /* Unclipped left/top and first visible source row. */
    float wave[3];   /* First visible phase, phase increment and amplitude. */
    float anchor;    /* Rounded first-row sine offset, or centered amplitude. */
} gpu_water_sampling;

typedef struct gpu_plane_row
{
    /* One RGBA texel retains both the exact nearest row and its continuous
     * recurrence. The last channels store wrapped fractional Y and the local
     * vertical increment; filtered sampling adds half that increment. */
    float step, source_y, source_y_continuous, step_y;
} gpu_plane_row;

typedef struct gpu_plane_sampling
{
    /* The renderer owns row storage and may reuse a validated prefix. Draw
     * setup fills the remaining uniforms without changing cached row data. */
    int enabled;
    const gpu_plane_row *rows;
    int valid_rows;
    float origin[3]; /* Clipped left, unclipped top and horizontal scroll. */
    float info[2];   /* Target midpoint relative to left, row texture height. */
} gpu_plane_sampling;

enum gpu_transform_result
{
    /* FAILED reports rejected geometry; EMPTY and DRAWN complete handling. */
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
    GPU_SAMPLE_CONTINUOUS = 3,
    GPU_SAMPLE_AFFINE = 4,
    GPU_SAMPLE_SHEAR = 5,
    GPU_SAMPLE_WATER = 6,
    GPU_SAMPLE_PLANE = 7,
    GPU_SAMPLE_CURVE_REPEAT = 9,
    /* Damon V. Caskey - 2026-11-20: opaque preview resizing stabilizes
     * large-source interpolation and clamps valid samples to source edges. */
    GPU_SAMPLE_PREVIEW = 8
};

/*
 * Called synchronously, with vertices in perimeter order. The caller may
 * append the four vertices to a reusable indexed-triangle submission buffer.
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

/* Optional native sampling removes expanded row geometry. NULL retains each
 * established native geometry path; plane rows must be prepared beforehand. */
int gpu_transform_draw_sampled(const gpu_transform_parameters *parameters,
                               const s_drawmethod *method, gpu_shear_sampling *shear,
                               gpu_water_sampling *water, gpu_plane_sampling *plane,
                               gpu_transform_emit_quad emit, void *userdata);

/* Prepare the exact plane recurrence once into caller-owned source_height
 * records. Return its valid prefix length, including top-clipped rows; an
 * invalid later row does not invalidate an earlier drawable prefix. */
int gpu_transform_prepare_plane_rows(const gpu_transform_parameters *parameters,
                                     const s_drawmethod *method, gpu_plane_row *rows);

/* The native renderer retains the affine sampling state alongside its quad.
 * Generic geometry callers can use gpu_transform_draw without retaining it. */
int gpu_transform_draw_affine(const gpu_transform_parameters *parameters,
                              const s_drawmethod *method, gpu_affine_sampling *sampling,
                              gpu_transform_emit_quad emit, void *userdata);

#endif
