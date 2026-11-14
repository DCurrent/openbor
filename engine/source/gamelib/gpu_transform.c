/*
 * OpenBOR - http://www.chronocrash.com
 * Copyright (c) OpenBOR Team. All rights reserved. See LICENSE.
 */

#include "gpu_transform.h"
#include <float.h>
#include <limits.h>
#include <math.h>
#include <string.h>

#define GPU_PI 3.14159265358979323846

/*
 * Damon V. Caskey
 * 2026-11-12
 *
 * Reject unusable floating-point parameters before any target is modified.
 */
static int gpu_finite(double value)
{
    /* NaN fails its own equality check. The remaining bounds ensure that
     * values accepted during double-precision setup survive float vertices. */
    return value == value && value >= -FLT_MAX && value <= FLT_MAX;
}

/*
 * Damon V. Caskey
 * 2026-11-12
 *
 * Build a rectangular row or source rectangle without allocating a mesh.
 * Raw texel coordinates permit wrapping and independent palette lookup later.
 */
static int gpu_emit_rect(float left, float top, float right, float bottom,
                         float u0, float v0, float u1, float v1, int wrap,
                         gpu_transform_emit_quad emit, void *userdata)
{
    /* Perimeter order keeps each source corner paired with its destination
     * corner. Stack storage is valid for the synchronous callback contract. */
    gpu_vertex vertices[4] =
    {
        {left, top, u0, v0},
        {right, top, u1, v0},
        {right, bottom, u1, v1},
        {left, bottom, u0, v1}
    };
    return emit(vertices, wrap, userdata) ? GPU_TRANSFORM_DRAWN : GPU_TRANSFORM_FAILED;
}

/*
 * Damon V. Caskey
 * 2026-11-13
 *
 * Encode a bounded homogeneous term as an integer and seven fractional bytes.
 * Signed digits retain tiny negative values without subtracting them from one.
 */
static int gpu_affine_digits(double value, float digits[8])
{
    int index;
    for(index = 0; index < 8; ++index)
    {
        double whole = trunc(value);
        digits[index] = (float)whole;
        value = (value - whole) * 256.0;
    }
    /* Unsupported precision selects software; never silently quantize it. */
    return value == 0.0;
}

/*
 * Damon V. Caskey
 * 2026-11-13
 *
 * Render the opt-in source-to-destination affine matrix as one native quad.
 * The fragment shader evaluates homogeneous inverse coordinates at pixel
 * centers. Keeping numerators separate from their determinant avoids rounding
 * a repeating reciprocal, such as 1/3, onto the wrong side of a texel edge.
 */
int gpu_transform_draw_affine(const gpu_transform_parameters *p, const s_drawmethod *m,
                              gpu_affine_sampling *sampling,
                              gpu_transform_emit_quad emit, void *userdata)
{
    double a, b, c, d, pivot_x, pivot_y;
    double origin_x, origin_y, determinant;
    double inverse00, inverse01, inverse10, inverse11;
    double scale, numerator_x[3], numerator_y[3], dx, dy, extent_x, extent_y;
    double min_x = DBL_MAX, min_y = DBL_MAX;
    double max_x = -DBL_MAX, max_y = -DBL_MAX;
    gpu_affine_sampling local_sampling;
    gpu_vertex vertices[4];
    int index, exponent, digit;

    if(!p || !m || !emit || p->source_width <= 0 || p->source_height <= 0
       || p->target_width <= 0 || p->target_height <= 0)
    {
        return GPU_TRANSFORM_FAILED;
    }
    a = m->affine.m00;
    b = m->affine.m01;
    c = m->affine.m10;
    d = m->affine.m11;
    pivot_x = (double)p->centerx + m->centerx;
    pivot_y = (double)p->centery + m->centery;

    /* Validate all authored coefficients before bounding or inverting them.
     * A collapsed source has no drawable area and succeeds without a draw. */
    if(!gpu_finite(a) || !gpu_finite(b) || !gpu_finite(c) || !gpu_finite(d)
       || !gpu_finite(m->affine.tx) || !gpu_finite(m->affine.ty))
    {
        return GPU_TRANSFORM_FAILED;
    }
    if(m->config & DRAWMETHOD_CONFIG_FLIP_X)
    {
        a = -a;
        c = -c;
    }
    if(m->config & DRAWMETHOD_CONFIG_FLIP_Y)
    {
        b = -b;
        d = -d;
    }
    determinant = a * d - b * c;
    if(determinant == 0.0)
    {
        return GPU_TRANSFORM_EMPTY;
    }
    origin_x = (double)p->x + m->affine.tx - a * pivot_x - b * pivot_y;
    origin_y = (double)p->y + m->affine.ty - c * pivot_x - d * pivot_y;

    /* Bound source pixel edges in double precision before intersecting with
     * the target. Large offscreen coordinates never reach integer casts or
     * the GPU; clipping does not change the continuous source sample phase. */
    for(index = 0; index < 4; ++index)
    {
        double u = index == 1 || index == 2 ? p->source_width : 0;
        double v = index >= 2 ? p->source_height : 0;
        double x = origin_x + a * u + b * v;
        double y = origin_y + c * u + d * v;

        min_x = fmin(min_x, x);
        max_x = fmax(max_x, x);
        min_y = fmin(min_y, y);
        max_y = fmax(max_y, y);
    }
    min_x = fmax(0.0, floor(min_x));
    min_y = fmax(0.0, floor(min_y));
    max_x = fmin((double)p->target_width, ceil(max_x));
    max_y = fmin((double)p->target_height, ceil(max_y));
    if(min_x >= max_x || min_y >= max_y)
    {
        return GPU_TRANSFORM_EMPTY;
    }

    /* Fractional bytes multiply exact integer pixel coordinates. Bound the
     * sum of all operands, including the corrected source candidate, so each
     * signed product/addition remains below float's exact 24-bit range. */
    extent_x = fmax(fabs((double)p->x), fabs((double)p->target_width - 1.0 - p->x));
    extent_y = fmax(fabs((double)p->y), fabs((double)p->target_height - 1.0 - p->y));
    if(extent_x + extent_y + fmax(p->source_width + fabs(pivot_x),
                                 p->source_height + fabs(pivot_y)) + 2.0 > 65535.0)
    {
        return GPU_TRANSFORM_FAILED;
    }

    inverse00 = d / determinant;
    inverse01 = -b / determinant;
    inverse10 = -c / determinant;
    inverse11 = a / determinant;

    /* Scale all homogeneous terms by the same exact power of two. A positive
     * denominator near one keeps shader division and integer products in
     * range without altering their ratio or introducing another reciprocal. */
    frexp(fabs(determinant), &exponent);
    scale = ldexp(determinant < 0.0 ? -1.0 : 1.0, -exponent);
    /* Keep integer placement and source pivot separate from the rational
     * mapping. Pre-canceling either in a double origin can move an exact
     * pivot sample across a texel boundary for an oblique authored matrix. */
    dx = 0.5 - (double)m->affine.tx;
    dy = 0.5 - (double)m->affine.ty;
    numerator_x[0] = d * scale;
    numerator_x[1] = -b * scale;
    numerator_x[2] = (d * dx - b * dy) * scale;
    numerator_y[0] = -c * scale;
    numerator_y[1] = a * scale;
    numerator_y[2] = (a * dy - c * dx) * scale;
    if(!sampling)
    {
        sampling = &local_sampling;
    }
    sampling->anchor[0] = (float)p->x;
    sampling->anchor[1] = (float)p->y;
    sampling->pivot[0] = (float)pivot_x;
    sampling->pivot[1] = (float)pivot_y;
    for(index = 0; index < 3; ++index)
    {
        float digits_x[8], digits_y[8];
        /* Bound approximate-quotient error below one texel, even after
         * cancellation. Byte-carry residuals then correct at most one step.
         * Extreme mappings remain atomic software operations. */
        double extent = index == 0 ? fmax(1.0, extent_x) : index == 1 ? fmax(1.0, extent_y) : 1;
        if(!gpu_finite(numerator_x[index]) || !gpu_finite(numerator_y[index])
           || fabs(numerator_x[index]) * extent > 262144.0
           || fabs(numerator_y[index]) * extent > 262144.0
           || !gpu_affine_digits(numerator_x[index], digits_x)
           || !gpu_affine_digits(numerator_y[index], digits_y))
        {
            return GPU_TRANSFORM_FAILED;
        }
        sampling->approximate_x[index] = (float)numerator_x[index];
        sampling->approximate_y[index] = (float)numerator_y[index];
        for(digit = 0; digit < 8; ++digit)
        {
            sampling->x[digit][index] = digits_x[digit];
            sampling->y[digit][index] = digits_y[digit];
        }
    }
    determinant *= scale;
    sampling->approximate_denominator = (float)determinant;
    if(!gpu_affine_digits(determinant, sampling->denominator))
    {
        return GPU_TRANSFORM_FAILED;
    }
    /* A single callback follows validation of all vertices. Unlike legacy
     * rotation, affine samples at pixel centers, so no half-pixel subtraction
     * belongs in the inverse mapping attached to destination pixel edges. */
    for(index = 0; index < 4; ++index)
    {
        double x = index == 1 || index == 2 ? max_x : min_x;
        double y = index >= 2 ? max_y : min_y;
        double u = inverse00 * (x - origin_x) + inverse01 * (y - origin_y);
        double v = inverse10 * (x - origin_x) + inverse11 * (y - origin_y);

        if(!gpu_finite(u) || !gpu_finite(v))
        {
            return GPU_TRANSFORM_FAILED;
        }
        vertices[index].x = (float)x;
        vertices[index].y = (float)y;
        vertices[index].u = (float)u;
        vertices[index].v = (float)v;
    }
    return emit(vertices, GPU_SAMPLE_AFFINE, userdata) ? GPU_TRANSFORM_DRAWN : GPU_TRANSFORM_FAILED;
}

/*
 * Damon V. Caskey
 * 2026-11-12
 *
 * Reproduce the existing plane's row-wise scale and accumulated vertical
 * sampling. Tile/stretch/no-perspective retain their established meanings,
 * including wavetime texture scrolling.
 */
static int gpu_draw_plane(const gpu_transform_parameters *p, const s_drawmethod *m,
                          gpu_transform_emit_quad emit, void *userdata)
{
    float origin_x = (float)p->x - p->centerx - m->centerx;
    float origin_y = (float)p->y - p->centery - m->centery;
    float left = fmaxf(0.0f, origin_x);
    float right = fminf((float)p->target_width, origin_x + p->source_width);
    float bottom = fminf((float)p->target_height, origin_y + p->source_height);
    float begin = m->water.beginsize;
    float end = m->water.endsize;
    float factor, size, center, origin_u;
    float sy = 0.0f;
    const double coordinate_limit = (double)INT_MAX / 2.0;
    int row, rows;

    /* The plane occupies the source-sized destination rectangle. Its row
     * scale changes texture sampling within that rectangle. Validate both
     * scale endpoints before deriving reciprocals or emitting geometry. */
    if(!gpu_finite(begin) || !gpu_finite(end) || begin <= 0.0f || end < 0.0f)
    {
        return GPU_TRANSFORM_FAILED;
    }
    if(left >= right || origin_y >= bottom || bottom <= 0.0f)
    {
        return GPU_TRANSFORM_EMPTY;
    }

    /* Horizontal samples are centered on the destination screen's midpoint.
     * Keep the unclipped origin in the scrolling term so left-edge clipping
     * reveals the same texture positions that an unclipped plane would use. */
    factor = (end - begin) / p->source_height;
    rows = (int)(bottom - origin_y);
    center = p->target_width / 2.0f - left;
    origin_u = m->water.wavetime - origin_x + p->source_width;

    /* Preflight the full row sequence before any callback can change a target.
     * This keeps fallback atomic: an invalid later row cannot leave earlier
     * rows rendered before the software path retries the same draw. Include
     * top-clipped rows because their vertical steps affect visible samples. */
    size = begin;
    for(row = 0; row < rows; ++row, size += factor)
    {
        float step, y_step;
        if(!gpu_finite(size) || size <= FLT_MIN)
        {
            return GPU_TRANSFORM_FAILED;
        }
        step = 1.0f / size;

        /* Tiled perspective advances both source axes by the reciprocal row
         * scale. Stretched perspective adjusts vertical travel by the source
         * aspect ratio; ordinary mode advances one source row per output row. */
        if(m->water.perspective == WATER_PERSPECTIVE_TILE)
        {
            y_step = step;
        }
        else if(m->water.perspective == WATER_PERSPECTIVE_STRETCH)
        {
            y_step = step * p->source_height / p->source_width;
        }
        else
        {
            y_step = 1.0f;
        }

        /* Source coordinates are eventually truncated to integer texels.
         * Bound both row endpoints and the accumulated vertical position
         * before that conversion, including strongly foreshortened rows.
         * Widen the computed float values before comparing with a double
         * limit: converting INT_MAX to float first rounds it upward to 2^31
         * and would incorrectly admit 2^30 at this half-range safety bound. */
        if(!gpu_finite(step) || !gpu_finite(sy) || (double)fabsf(sy) > coordinate_limit
           || !gpu_finite(origin_u - center * step)
           || (double)fabsf(origin_u - center * step) > coordinate_limit
           || !gpu_finite(origin_u + (right - left - center) * step)
           || (double)fabsf(origin_u + (right - left - center) * step) > coordinate_limit)
        {
            return GPU_TRANSFORM_FAILED;
        }
        sy += y_step;
    }

    /* Replay the validated recurrence and submit only visible rows. Horizontal
     * texels interpolate across each quad; the chosen source row stays constant
     * across its height, preserving the plane's row-based vertical sampling. */
    sy = 0.0f;
    size = begin;
    for(row = 0; row < rows; ++row, size += factor)
    {
        float step = 1.0f / size;
        float top = origin_y + row;
        float u = origin_u - center * step;
        float y_step;
        int source_y = (int)sy % p->source_height;

        if(m->water.perspective == WATER_PERSPECTIVE_TILE)
        {
            y_step = step;
        }
        else if(m->water.perspective == WATER_PERSPECTIVE_STRETCH)
        {
            y_step = step * p->source_height / p->source_width;
        }
        else
        {
            y_step = 1.0f;
        }

        if(source_y < 0)
        {
            source_y += p->source_height;
        }
        if(top >= 0.0f)
        {
            /* Interpolation runs through destination pixel centers. Offset
             * the quad's source edge by half a step so each center receives
             * the plane sampler's intended integer-destination-x sample. */
            u -= 0.5f * step;
            if(gpu_emit_rect(left, top, right, top + 1.0f,
                             u, (float)source_y, u + (right - left) * step,
                             (float)source_y, 1, emit, userdata) == GPU_TRANSFORM_FAILED)
            {
                return GPU_TRANSFORM_FAILED;
            }
        }
        sy += y_step;
    }
    return GPU_TRANSFORM_DRAWN;
}

/*
 * Damon V. Caskey
 * 2026-11-12
 *
 * Evaluate the legacy 256-step wave table analytically during row setup.
 * Only positions are generated on the CPU; pixels remain on the GPU.
 */
static float gpu_wave_offset(float phase, int amplitude)
{
    /* Reduce to the same integer phase used by the 256-entry wave table.
     * Adding one shifts the sine range to [0, 2]; the caller then removes
     * the selected origin offset to produce the final row displacement. */
    int table_index = (int)phase % 256;
    if(table_index < 0)
    {
        table_index += 256;
    }
    return floorf((1.0f + sinf((float)(table_index * (2.0 * GPU_PI / 256.0))))
                  * amplitude + 0.5f);
}

/*
 * Damon V. Caskey
 * 2026-11-12
 *
 * Emit the existing sine-water displacement as one short quad per visible row.
 * Water overrides the partial scale/rotate transform, matching old dispatch.
 */
static int gpu_draw_water(const gpu_transform_parameters *p, const s_drawmethod *m,
                          gpu_transform_emit_quad emit, void *userdata)
{
    float left = (float)p->x - p->centerx - m->centerx;
    float top = (float)p->y - p->centery - m->centery;
    float wavelength = m->water.wavelength;
    float phase, origin_shift, phase_step;
    const double coordinate_limit = (double)INT_MAX / 2.0;
    int first, last, row;

    /* The wave phase will be converted to a table index, and its displacement
     * must remain a representable destination coordinate. Reject unusable
     * wavelength/amplitude inputs before any visible rows are submitted. */
    if(!gpu_finite(wavelength) || wavelength <= 0.0f
       || m->water.amplitude < 0 || m->water.amplitude > INT_MAX / 8)
    {
        return GPU_TRANSFORM_FAILED;
    }
    if(top >= p->target_height || top + p->source_height <= 0.0f)
    {
        return GPU_TRANSFORM_EMPTY;
    }

    /* Advance the phase over top-clipped rows while retaining source row
     * numbering. This keeps waves stationary relative to their source image
     * as that image moves through the screen's upper edge. */
    first = top < 0.0f ? (int)-top : 0;
    last = p->source_height;
    if(top + last > p->target_height)
    {
        last = (int)(p->target_height - top);
    }
    phase_step = 256.0f / wavelength;

    /* Compare the actual float phase extent against an exact double limit.
     * This retains headroom for clipped-row advancement without rounding
     * INT_MAX upward before a later phase-to-integer conversion. */
    if(!gpu_finite(phase_step)
       || (double)(phase_step * p->source_height) > coordinate_limit)
    {
        return GPU_TRANSFORM_FAILED;
    }
    phase = (float)(m->water.wavetime % 256);
    if(phase < 0.0f)
    {
        phase += 256.0f;
    }

    /* Sine mode anchors displacement to the current first-row phase. The
     * alternate water mode centers displacement around the amplitude. */
    origin_shift = m->water.watermode == WATER_MODE_SINE
                 ? gpu_wave_offset(phase, m->water.amplitude) : (float)m->water.amplitude;
    phase += first * phase_step;

    /* Each visible row becomes a translated, one-pixel-high quad. Texture
     * sampling and material compositing remain in the fragment shader. */
    for(row = first; row < last; ++row)
    {
        float shift;
        phase = fmodf(phase, 256.0f);
        shift = gpu_wave_offset(phase, m->water.amplitude) - origin_shift;
        if(left + shift < p->target_width && left + shift + p->source_width > 0.0f)
        {
            if(gpu_emit_rect(left + shift, top + row,
                             left + shift + p->source_width, top + row + 1.0f,
                             0.0f, row + 0.5f, (float)p->source_width, row + 0.5f,
                             0, emit, userdata) == GPU_TRANSFORM_FAILED)
            {
                return GPU_TRANSFORM_FAILED;
            }
        }
        phase += phase_step;
    }
    return GPU_TRANSFORM_DRAWN;
}

/*
 * Damon V. Caskey
 * 2026-11-12
 *
 * Emit one inverse-mapped rectangle for rotation. The established transform
 * scales in destination axes after rotation and uses flip-dependent angle
 * signs; those conventions are retained instead of silently changing poses.
 * Bounds come from that same matrix, fixing the old nonuniform-scale clipping.
 */
static int gpu_draw_rotate(const gpu_transform_parameters *p, const s_drawmethod *m,
                           gpu_transform_emit_quad emit, void *userdata)
{
    double zx = m->scalex / 256.0;
    double zy = m->scaley / 256.0;
    double angle, sine, cosine, ax, ay, bx, by, determinant;
    double cx = (double)p->centerx + m->centerx;
    double cy = (double)p->centery + m->centery;
    double minx = DBL_MAX;
    double miny = DBL_MAX;
    double maxx = -DBL_MAX;
    double maxy = -DBL_MAX;
    gpu_vertex vertices[4];
    int index;
    int degrees = (int)fmod(m->rotate, 360.0f);

    /* Preserve drawmethod's flip-dependent angle convention. A sign change
     * in either scale reflects that source axis, while the angle changes
     * determine how the established rotation direction follows those flips. */
    if(m->config & DRAWMETHOD_CONFIG_FLIP_X)
    {
        zx = -zx;
    }
    else
    {
        degrees = -degrees;
    }
    if(m->config & DRAWMETHOD_CONFIG_FLIP_Y)
    {
        zy = -zy;
        degrees = -degrees;
    }
    angle = degrees * (GPU_PI / 180.0);
    sine = sin(angle);
    cosine = cos(angle);
    /* Suppress residuals at exact quarter-turns, preserving pixel boundaries. */
    if(degrees % 90 == 0)
    {
        sine = round(sine);
        cosine = round(cosine);
    }

    /* This matrix maps destination displacement back into source texels.
     * Each column describes source movement for one destination-axis step.
     * Its inverse places source corners in destination space for clipping. */
    ax = cosine / zx;
    ay = sine / zx;
    bx = -sine / zy;
    by = cosine / zy;
    determinant = ax * by - ay * bx;

    /* Derive coverage from the same mapping used to sample pixels. Computing
     * a separate rotated/scaled rectangle would give inconsistent bounds when
     * the two scale factors differ. Pivots stay in source texel coordinates. */
    for(index = 0; index < 4; ++index)
    {
        double u = (index == 1 || index == 2 ? p->source_width : 0) - cx;
        double v = (index >= 2 ? p->source_height : 0) - cy;
        double x = p->x + (by * u - bx * v) / determinant;
        double y = p->y + (-ay * u + ax * v) / determinant;
        minx = fmin(minx, x);
        maxx = fmax(maxx, x);
        miny = fmin(miny, y);
        maxy = fmax(maxy, y);
    }
    /* Rotation samples integer destination positions. A reflected source-zero
     * boundary can therefore lie exactly on the maximum destination integer;
     * include it and let source clipping reject the opposite exclusive edge. */
    minx = fmax(0.0, floor(minx));
    maxx = fmin(p->target_width, ceil(maxx) + 1.0);
    miny = fmax(0.0, floor(miny));
    maxy = fmin(p->target_height, ceil(maxy) + 1.0);
    if(minx >= maxx || miny >= maxy)
    {
        return GPU_TRANSFORM_EMPTY;
    }

    /* Attach inverse-mapped source coordinates to the clipped rectangle.
     * Interpolation then evaluates the same linear mapping for every fragment;
     * source-range checks in the shader trim the rectangle to actual coverage. */
    for(index = 0; index < 4; ++index)
    {
        double x = index == 1 || index == 2 ? maxx : minx;
        double y = index >= 2 ? maxy : miny;
        vertices[index].x = (float)x;
        vertices[index].y = (float)y;
        /* Subtract half a destination pixel before mapping so rasterized
         * pixel centers reproduce integer-position source sampling. */
        vertices[index].u = (float)(cx + ax * (x - p->x - 0.5) + bx * (y - p->y - 0.5));
        vertices[index].v = (float)(cy + ay * (x - p->x - 0.5) + by * (y - p->y - 0.5));
    }
    return emit(vertices, GPU_SAMPLE_CONTINUOUS, userdata) ? GPU_TRANSFORM_DRAWN : GPU_TRANSFORM_FAILED;
}

/*
 * Damon V. Caskey
 * 2026-11-12
 *
 * Divide signed fixed-point values with the floor behavior of the old shifts,
 * without relying on implementation-defined right shifts of negative integers.
 */
static int64_t gpu_fixed_floor(int64_t value)
{
    /* C division truncates toward zero. Bias the negative magnitude before
     * division so both signs produce the floor required by 8.8 coordinates. */
    return value >= 0 ? value / 256 : -((-value + 255) / 256);
}

typedef struct gpu_shear_setup
{
    int64_t origin_x, width, clip_left, clip_right, bottom, rows, shift;
    int64_t source_x, source_y, step_x, step_y;
} gpu_shear_setup;

/*
 * Damon V. Caskey
 * 2026-11-14
 * Floor a signed numerator by a positive divisor when trimming the monotonic
 * shear sequence. C truncation alone would lose negative boundary rows.
 */
static int64_t gpu_floor_divide(int64_t numerator, int64_t divisor)
{
    int64_t quotient = numerator / divisor;
    return quotient - (numerator % divisor < 0);
}

/*
 * Damon V. Caskey
 * 2026-11-14
 * Replace a legacy row walk with bounded shader sampling parameters. Exact
 * whole/byte pairs preserve snapped coverage and clipping-dependent phase.
 * Unsupported precision retains the existing native row geometry.
 */
static int gpu_prepare_shear(const gpu_transform_parameters *parameters,
                             const gpu_shear_setup *setup,
                             gpu_shear_sampling *sampling, float rectangle[4])
{
    int64_t lower, upper, first, last, divisor, origin, final_origin, bottom, rows;
    int64_t origin_whole, width_whole, shift_whole, source_y;
    int64_t left, right;

    /* Byte products fit exact 24-bit integer-valued floats for this domain.
     * The existing scale setup separately bounds fixed extents to 2^29.
     * Larger images retain row geometry without changing their pixels. */
    if(parameters->source_width > 65535 || parameters->source_height > 65535 ||
       parameters->target_width > 65535 || parameters->target_height > 65535)
    {
        return GPU_TRANSFORM_FAILED;
    }
    if(setup->width < 256)
    {
        return GPU_TRANSFORM_EMPTY;
    }

    /* Rows visit at least one pixel exactly when their fixed-point origin
     * lies inside [clip_left + 256 - width, clip_right - 256]. Intersect that
     * interval with the monotonic bottom-to-top recurrence in constant time. */
    lower = setup->clip_left + 256 - setup->width;
    upper = setup->clip_right - 256;
    divisor = setup->shift > 0 ? setup->shift : -setup->shift;
    if(setup->shift > 0)
    {
        first = -gpu_floor_divide(upper - setup->origin_x, divisor);
        last = gpu_floor_divide(setup->origin_x - lower, divisor);
    }
    else
    {
        first = -gpu_floor_divide(setup->origin_x - lower, divisor);
        last = gpu_floor_divide(upper - setup->origin_x, divisor);
    }
    if(first < 0) first = 0;
    if(last >= setup->rows) last = setup->rows - 1;
    if(first > last)
    {
        return GPU_TRANSFORM_EMPTY;
    }

    /* Rebase every phase together after trimming. The remaining origins are
     * bounded by the clip interval and source extent, even for large shifts
     * or distant pivots that produce only a small visible intersection. */
    origin = setup->origin_x - first * setup->shift;
    rows = last - first + 1;
    final_origin = origin - (rows - 1) * setup->shift;
    source_y = setup->source_y + first * setup->step_y;
    bottom = setup->bottom - first;
    origin_whole = gpu_fixed_floor(origin);
    width_whole = gpu_fixed_floor(setup->width);
    shift_whole = gpu_fixed_floor(setup->shift);

    /* Bound a single destination rectangle around all surviving row spans.
     * The shader rejects staircase gaps using each row's original clipped
     * fixed-point coverage, rather than continuously skewing the artwork. */
    left = gpu_fixed_floor(origin < final_origin ? origin : final_origin);
    right = gpu_fixed_floor((origin > final_origin ? origin : final_origin) + setup->width);
    if(left < setup->clip_left / 256) left = setup->clip_left / 256;
    if(right > setup->clip_right / 256) right = setup->clip_right / 256;
    rectangle[0] = (float)left;
    rectangle[1] = (float)(bottom - rows);
    rectangle[2] = (float)right;
    rectangle[3] = (float)bottom;

    /* Each byte is nonnegative even for negative shear. Products of a row
     * index or reciprocal step with a byte stay below 2^24; splitting the
     * whole part avoids rounding large fixed-point origins or extents. */
    sampling->origin[0] = (float)origin_whole;
    sampling->origin[1] = (float)(origin - origin_whole * 256);
    sampling->origin[2] = (float)bottom;
    sampling->extent[0] = (float)width_whole;
    sampling->extent[1] = (float)(setup->width - width_whole * 256);
    sampling->extent[2] = (float)rows;
    sampling->shift[0] = (float)shift_whole;
    sampling->shift[1] = (float)(setup->shift - shift_whole * 256);
    sampling->source[0] = (float)setup->source_x;
    sampling->source[1] = (float)source_y;
    sampling->step[0] = (float)setup->step_x;
    sampling->step[1] = (float)setup->step_y;
    sampling->clip[0] = (float)(setup->clip_left / 256);
    sampling->clip[1] = (float)(setup->clip_right / 256);
    sampling->enabled = 1;
    return GPU_TRANSFORM_DRAWN;
}

/*
 * Damon V. Caskey
 * 2026-11-12
 *
 * Recover a clipped row's exact legacy 8.8 fixed-point source coordinates.
 * Clipping the right edge can change the old sampler's fractional phase, so
 * it must be accounted for before GL scissoring rather than afterward.
 */
static int gpu_scale_span(int64_t dx, int64_t width, int64_t xmin, int64_t xmax,
                          int64_t origin_source, int64_t step,
                          float *left, float *right, float *u0, float *u1)
{
    int64_t begin = dx > xmin ? dx : xmin;
    int64_t end = dx + width < xmax ? dx + width : xmax;
    int64_t count;
    int64_t source = origin_source;
    float delta = (float)-step / 256.0f;

    /* The old row walker visits whole destination pixels from right to left.
     * Convert its clipped 8.8 extent into that exact number of pixel visits. */
    if(begin >= end)
    {
        return 0;
    }
    count = (end - begin) / 256;
    if(!count)
    {
        return 0;
    }

    /* Move the source starting point past the clipped right edge before
     * reversing the span into left-to-right quad interpolation. Preserve the
     * fractional phase even when the first surviving destination pixel moves. */
    if(dx + width > xmax)
    {
        source += gpu_fixed_floor(step * (dx + width - xmax));
    }
    *right = (float)gpu_fixed_floor(end);
    *left = *right - count;

    /* Vertex attributes describe pixel edges. Offset half a source step so
     * interpolation at the first pixel center lands on the intended sample. */
    *u0 = (float)(source + (count - 1) * step) / 256.0f - 0.5f * delta;
    *u1 = *u0 + count * delta;
    return 1;
}

/*
 * Damon V. Caskey
 * 2026-11-12
 *
 * 2026-11-14 - Supply exact shear sampling when the native shader supports it.
 * Keep existing fixed-point scale/flip sampling instead of shifting downscaled
 * artwork to a different texel. Ordinary scaling remains one GPU quad. Shear
 * uses one shader-sampled quad or retains its established row geometry when
 * that representation is unavailable; continuous skew would change sampling.
 */
static int gpu_draw_scale(const gpu_transform_parameters *p, const s_drawmethod *m,
                          gpu_shear_sampling *shear,
                          gpu_transform_emit_quad emit, void *userdata)
{
    int64_t x = (int64_t)p->x * 256;
    int64_t y = (int64_t)p->y * 256;
    int64_t width = (int64_t)p->source_width * m->scalex;
    int64_t height = (int64_t)p->source_height * m->scaley;
    int64_t cx = ((int64_t)p->centerx + m->centerx) * m->scalex;
    int64_t cy = ((int64_t)p->centery + m->centery) * m->scaley;
    int64_t xmin = 0;
    int64_t ymin = 0;
    int64_t xmax = (int64_t)p->target_width * 256;
    int64_t ymax = (int64_t)p->target_height * 256;
    int64_t dx, dy, step_x, step_y, source_x, source_y;
    int64_t shift = m->shiftx;
    int64_t begin_y, end_y, rows, row;
    float left, right, u0, u1;

    /* Positions, extents and pivots are in 8.8 fixed-point units here. Wider
     * intermediates keep products safe; the 2^29 bounds also reserve headroom
     * for subsequent shear and clipping arithmetic before callback emission. */
    if(x < -536870912 || x > 536870912 || y < -536870912 || y > 536870912
       || cx < -536870912 || cx > 536870912 || cy < -536870912 || cy > 536870912
       || width > 536870912 || height > 536870912
       || p->source_width > INT_MAX / 256 || p->source_height > INT_MAX / 256)
    {
        return GPU_TRANSFORM_FAILED;
    }

    /* Intersect drawmethod clipping with the target in the same fixed-point
     * domain. Clipping participates in source phase calculation below, so it
     * must be resolved before geometry reaches the backend's GL scissor. */
    if(m->clipw > 0 && m->cliph > 0)
    {
        int64_t clipright = ((int64_t)m->clipx + m->clipw) * 256;
        int64_t clipbottom = ((int64_t)m->clipy + m->cliph) * 256;
        if(m->clipx > 0)
        {
            xmin = (int64_t)m->clipx * 256;
        }
        if(m->clipy > 0)
        {
            ymin = (int64_t)m->clipy * 256;
        }
        if(clipright < xmax)
        {
            xmax = clipright;
        }
        if(clipbottom < ymax)
        {
            ymax = clipbottom;
        }
    }
    if(xmin >= xmax || ymin >= ymax)
    {
        return GPU_TRANSFORM_EMPTY;
    }

    /* Flips reflect the scaled extents around the requested pivot. Vertical
     * reflection also reverses row-to-row shear, retaining the image's source
     * orientation as the legacy sampler walks upward from the bottom row. */
    dx = m->config & DRAWMETHOD_CONFIG_FLIP_X ? cx - width + x : x - cx;
    dy = m->config & DRAWMETHOD_CONFIG_FLIP_Y ? cy - height + y : y - cy;
    if(m->config & DRAWMETHOD_CONFIG_FLIP_Y)
    {
        shift = -shift;
    }
    dx += gpu_fixed_floor((dy + height - y) * shift);

    /* Reciprocal scale becomes an 8.8 source step: 256 * 256 / scale.
     * Its sign follows the right-to-left, bottom-to-top traversal. Starting
     * source coordinates select the corresponding first sample on each axis. */
    step_x = 65536 / m->scalex;
    step_y = 65536 / m->scaley;
    if(!(m->config & DRAWMETHOD_CONFIG_FLIP_X))
    {
        step_x = -step_x;
    }
    if(!(m->config & DRAWMETHOD_CONFIG_FLIP_Y))
    {
        step_y = -step_y;
    }
    source_x = m->config & DRAWMETHOD_CONFIG_FLIP_X ? 0
               : (int64_t)p->source_width * 256 + step_x;
    source_y = m->config & DRAWMETHOD_CONFIG_FLIP_Y ? 0
               : (int64_t)p->source_height * 256 + step_y;
    begin_y = dy > ymin ? dy : ymin;
    end_y = dy + height < ymax ? dy + height : ymax;
    if(begin_y >= end_y)
    {
        return GPU_TRANSFORM_EMPTY;
    }
    rows = (end_y - begin_y) / 256;
    if(!rows)
    {
        return GPU_TRANSFORM_EMPTY;
    }

    /* Bottom clipping skips traversal steps before the first visible row.
     * Advance both its horizontal shear position and vertical source phase. */
    if(dy + height > ymax)
    {
        dx -= gpu_fixed_floor(shift * (dy + height - end_y));
        source_y += gpu_fixed_floor(step_y * (dy + height - end_y));
    }

    if(!shift)
    {
        /* With no shear, every row shares horizontal coverage. One quad can
         * interpolate both fixed-point source steps across the entire image. */
        float delta = (float)-step_y / 256.0f;
        float bottom = (float)gpu_fixed_floor(end_y);
        float v0 = (float)(source_y + (rows - 1) * step_y) / 256.0f - 0.5f * delta;
        if(!gpu_scale_span(dx, width, xmin, xmax, source_x, step_x,
                           &left, &right, &u0, &u1))
        {
            return GPU_TRANSFORM_EMPTY;
        }
        return gpu_emit_rect(left, bottom - rows, right, bottom,
                             u0, v0, u1, v0 + rows * delta, GPU_SAMPLE_FIXED8, emit, userdata);
    }

    if(shear)
    {
        gpu_shear_setup setup = {dx, width, xmin, xmax, gpu_fixed_floor(end_y),
                                rows, shift, source_x, source_y, step_x, step_y};
        float rectangle[4];
        int result = gpu_prepare_shear(p, &setup, shear, rectangle);
        if(result == GPU_TRANSFORM_EMPTY) return result;
        if(result == GPU_TRANSFORM_DRAWN)
        {
            /* UV attributes are unused for this mode. Pixel centers and the
             * exact setup above supply source coordinates in the shader. */
            return gpu_emit_rect(rectangle[0], rectangle[1], rectangle[2], rectangle[3],
                                 0.0f, 0.0f, 0.0f, 0.0f, GPU_SAMPLE_SHEAR, emit, userdata);
        }
    }

    /* Sheared rows receive independently clipped, integer-aligned spans.
     * Their source row stays constant across each one-pixel-high quad. After
     * clipping, span offsets remain bounded by the validated image width. */
    for(row = 0; row < rows; ++row, dx -= shift, source_y += step_y)
    {
        float top = (float)gpu_fixed_floor(end_y) - 1.0f - row;
        if(gpu_scale_span(dx, width, xmin, xmax, source_x, step_x,
                          &left, &right, &u0, &u1))
        {
            float v = (float)source_y / 256.0f;
            if(gpu_emit_rect(left, top, right, top + 1.0f, u0, v, u1, v,
                             GPU_SAMPLE_FIXED8, emit, userdata) == GPU_TRANSFORM_FAILED)
            {
                return GPU_TRANSFORM_FAILED;
            }
        }
    }
    return GPU_TRANSFORM_DRAWN;
}

/*
 * Damon V. Caskey
 * 2026-11-12
 * 2026-11-13 - Dispatch explicit affine matrices before dormant legacy state.
 * 2026-11-14 - Prepare optional shader shear without changing family priority.
 *
 * Select exactly one existing transformation family. Validate before emission,
 * and leave repeats, painter ordering, materials and target synchronization to
 * the renderer. Disabled methods retain the ordinary untransformed fast path.
 */
int gpu_transform_draw_sampled(const gpu_transform_parameters *p, const s_drawmethod *method,
                               gpu_shear_sampling *shear,
                               gpu_transform_emit_quad emit, void *userdata)
{
    s_drawmethod plain;
    const s_drawmethod *m = method;

    if(shear) shear->enabled = 0;

    /* Invalid geometry cannot supply safe reciprocal scales, clipping bounds,
     * or texture coordinates. Reject it before choosing a transform family. */
    if(!p || !emit || p->source_width <= 0 || p->source_height <= 0
       || p->target_width <= 0 || p->target_height <= 0)
    {
        return GPU_TRANSFORM_FAILED;
    }

    /* A disabled method contributes identity scale and zero offsets. The
     * source's own sprite pivot remains in the separate parameter structure. */
    if(!m || !(m->config & DRAWMETHOD_CONFIG_ENABLED))
    {
        memset(&plain, 0, sizeof(plain));
        plain.scalex = 256;
        plain.scaley = 256;
        m = &plain;
    }

    /* Affine owns geometry while enabled. Stored legacy scale, rotation and
     * water values do not participate, including zero or invalid old values. */
    if(m->config & DRAWMETHOD_CONFIG_AFFINE)
    {
        return gpu_transform_draw_affine(p, m, NULL, emit, userdata);
    }

    /* Sprite dispatch treats zero scale as invisible before water selection.
     * Screen water effects choose their own sampling extent and retain the
     * separate scale check after water selection below. */
    if(p->is_sprite && (!m->scalex || !m->scaley))
    {
        return GPU_TRANSFORM_EMPTY;
    }
    if(m->scalex < 0 || m->scaley < 0 || !gpu_finite(m->rotate))
    {
        return GPU_TRANSFORM_FAILED;
    }

    /* Keep established precedence: plane or sine-water mapping owns geometry
     * while active; otherwise rotation takes priority over scale/shear. Each
     * family receives the same pivot and drawmethod rather than composing
     * transformations that the original dispatch treated as alternatives. */
    if(m->water.watermode == WATER_MODE_SHEAR && m->water.beginsize > 0.0f)
    {
        return gpu_draw_plane(p, m, emit, userdata);
    }
    if(m->water.watermode && m->water.amplitude)
    {
        /* The union's amplitude bits can be nonzero for an invalid plane
         * size. Keep that case out of the sine-water parameter interpretation. */
        if(m->water.watermode == WATER_MODE_SHEAR)
        {
            return GPU_TRANSFORM_FAILED;
        }
        return gpu_draw_water(p, m, emit, userdata);
    }
    if(!m->scalex || !m->scaley)
    {
        return GPU_TRANSFORM_EMPTY;
    }
    if(m->rotate)
    {
        return gpu_draw_rotate(p, m, emit, userdata);
    }
    return gpu_draw_scale(p, m, shear, emit, userdata);
}

/*
 * Damon V. Caskey
 * 2026-11-14
 * Retain geometry-only callers and the native row fallback contract. Optional
 * sampling state is supplied explicitly by a capable renderer instead.
 */
int gpu_transform_draw(const gpu_transform_parameters *p, const s_drawmethod *method,
                       gpu_transform_emit_quad emit, void *userdata)
{
    return gpu_transform_draw_sampled(p, method, NULL, emit, userdata);
}
