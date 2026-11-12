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
    gpu_vertex vertices[4] = {
        {left, top, u0, v0},
        {right, top, u1, v0},
        {right, bottom, u1, v1},
        {left, bottom, u0, v1}
    };
    return emit(vertices, wrap, userdata) ? GPU_TRANSFORM_DRAWN : GPU_TRANSFORM_FAILED;
}

/*
 * Damon V. Caskey
 * 2026-11-12
 *
 * Reproduce the existing plane's row-wise scale and accumulated vertical
 * sampling. This is not a projective replacement: tile/stretch/no-perspective
 * retain their established meanings, including wavetime texture scrolling.
 */
static int gpu_draw_plane(const gpu_transform_parameters *p, const s_drawmethod *m,
                          gpu_transform_emit_quad emit, void *userdata)
{
    float origin_x = (float)p->x - p->centerx - m->centerx;
    float origin_y = (float)p->y - p->centery - m->centery;
    float left = fmaxf(0.0f, origin_x);
    float right = fminf((float)p->target_width, origin_x + p->source_width);
    float bottom = fminf((float)p->target_height, origin_y + p->source_height);
    float begin = m->water.beginsize, end = m->water.endsize;
    float factor, size, sy = 0.0f, center, origin_u;
    int row, rows;

    if(!gpu_finite(begin) || !gpu_finite(end) || begin <= 0.0f || end < 0.0f)
        return GPU_TRANSFORM_FAILED;
    if(left >= right || origin_y >= bottom || bottom <= 0.0f)
        return GPU_TRANSFORM_EMPTY;

    factor = (end - begin) / p->source_height;
    rows = (int)(bottom - origin_y);
    center = p->target_width / 2.0f - left;
    origin_u = m->water.wavetime - origin_x + p->source_width;

    /* Validate the entire row sequence before the callback can modify a target. */
    size = begin;
    for(row = 0; row < rows; ++row, size += factor)
    {
        float step, y_step;
        if(!gpu_finite(size) || size <= FLT_MIN) return GPU_TRANSFORM_FAILED;
        step = 1.0f / size;
        y_step = m->water.perspective == WATER_PERSPECTIVE_TILE ? step
               : m->water.perspective == WATER_PERSPECTIVE_STRETCH
                 ? step * p->source_height / p->source_width : 1.0f;
        if(!gpu_finite(step) || !gpu_finite(sy) || fabsf(sy) > INT_MAX / 2.0f
           || !gpu_finite(origin_u - center * step)
           || fabsf(origin_u - center * step) > INT_MAX / 2.0f
           || !gpu_finite(origin_u + (right - left - center) * step)
           || fabsf(origin_u + (right - left - center) * step) > INT_MAX / 2.0f)
            return GPU_TRANSFORM_FAILED;
        sy += y_step;
    }

    sy = 0.0f;
    size = begin;
    for(row = 0; row < rows; ++row, size += factor)
    {
        float step = 1.0f / size;
        float top = origin_y + row;
        float u = origin_u - center * step;
        float y_step = m->water.perspective == WATER_PERSPECTIVE_TILE ? step
                     : m->water.perspective == WATER_PERSPECTIVE_STRETCH
                       ? step * p->source_height / p->source_width : 1.0f;
        int source_y = (int)sy % p->source_height;

        if(source_y < 0) source_y += p->source_height;
        if(top >= 0.0f)
        {
            /* The old loop samples at integer destination x, not x+0.5. */
            u -= 0.5f * step;
            if(gpu_emit_rect(left, top, right, top + 1.0f,
                             u, (float)source_y, u + (right - left) * step,
                             (float)source_y, 1, emit, userdata) == GPU_TRANSFORM_FAILED)
                return GPU_TRANSFORM_FAILED;
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
    int table_index = (int)phase % 256;
    if(table_index < 0) table_index += 256;
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
    int first, last, row;

    if(!gpu_finite(wavelength) || wavelength <= 0.0f
       || m->water.amplitude < 0 || m->water.amplitude > INT_MAX / 8)
        return GPU_TRANSFORM_FAILED;
    if(top >= p->target_height || top + p->source_height <= 0.0f)
        return GPU_TRANSFORM_EMPTY;

    first = top < 0.0f ? (int)-top : 0;
    last = p->source_height;
    if(top + last > p->target_height) last = (int)(p->target_height - top);
    phase_step = 256.0f / wavelength;
    if(!gpu_finite(phase_step) || phase_step * p->source_height > INT_MAX / 2.0f)
        return GPU_TRANSFORM_FAILED;
    phase = (float)(m->water.wavetime % 256);
    if(phase < 0.0f) phase += 256.0f;
    origin_shift = m->water.watermode == WATER_MODE_SINE
                 ? gpu_wave_offset(phase, m->water.amplitude) : (float)m->water.amplitude;
    phase += first * phase_step;

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
                return GPU_TRANSFORM_FAILED;
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
    double zx = m->scalex / 256.0, zy = m->scaley / 256.0;
    double angle, sine, cosine, ax, ay, bx, by, determinant;
    double cx = (double)p->centerx + m->centerx;
    double cy = (double)p->centery + m->centery;
    double minx = DBL_MAX, miny = DBL_MAX, maxx = -DBL_MAX, maxy = -DBL_MAX;
    gpu_vertex vertices[4];
    int index, degrees = (int)fmod(m->rotate, 360.0f);

    if(m->config & DRAWMETHOD_CONFIG_FLIP_X) zx = -zx;
    else degrees = -degrees;
    if(m->config & DRAWMETHOD_CONFIG_FLIP_Y) { zy = -zy; degrees = -degrees; }
    angle = degrees * (GPU_PI / 180.0);
    sine = sin(angle);
    cosine = cos(angle);
    /* Suppress residuals at exact quarter-turns, preserving pixel boundaries. */
    if(degrees % 90 == 0)
    {
        sine = round(sine);
        cosine = round(cosine);
    }
    ax = cosine / zx;
    ay = sine / zx;
    bx = -sine / zy;
    by = cosine / zy;
    determinant = ax * by - ay * bx;

    for(index = 0; index < 4; ++index)
    {
        double u = (index == 1 || index == 2 ? p->source_width : 0) - cx;
        double v = (index >= 2 ? p->source_height : 0) - cy;
        double x = p->x + (by * u - bx * v) / determinant;
        double y = p->y + (-ay * u + ax * v) / determinant;
        minx = fmin(minx, x); maxx = fmax(maxx, x);
        miny = fmin(miny, y); maxy = fmax(maxy, y);
    }
    /* Rotation samples integer destination positions. A reflected source-zero
     * boundary can therefore lie exactly on the maximum destination integer;
     * include it and let source clipping reject the opposite exclusive edge. */
    minx = fmax(0.0, floor(minx)); maxx = fmin(p->target_width, ceil(maxx) + 1.0);
    miny = fmax(0.0, floor(miny)); maxy = fmin(p->target_height, ceil(maxy) + 1.0);
    if(minx >= maxx || miny >= maxy) return GPU_TRANSFORM_EMPTY;

    for(index = 0; index < 4; ++index)
    {
        double x = index == 1 || index == 2 ? maxx : minx;
        double y = index >= 2 ? maxy : miny;
        vertices[index].x = (float)x;
        vertices[index].y = (float)y;
        /* GL samples pixel centers; software rotation uses integer positions. */
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
    return value >= 0 ? value / 256 : -((-value + 255) / 256);
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
    int64_t count, source = origin_source;
    float delta = (float)-step / 256.0f;
    if(begin >= end) return 0;
    count = (end - begin) / 256;
    if(!count) return 0;
    if(dx + width > xmax) source += gpu_fixed_floor(step * (dx + width - xmax));
    *right = (float)gpu_fixed_floor(end);
    *left = *right - count;
    *u0 = (float)(source + (count - 1) * step) / 256.0f - 0.5f * delta;
    *u1 = *u0 + count * delta;
    return 1;
}

/*
 * Damon V. Caskey
 * 2026-11-12
 *
 * Keep existing fixed-point scale/flip sampling instead of shifting downscaled
 * artwork to a different texel. Ordinary scaling remains one GPU quad. Shear
 * uses row quads because the legacy rasterizer snaps each shifted row to whole
 * destination pixels; a continuously skewed quad would change its sampling.
 */
static int gpu_draw_scale(const gpu_transform_parameters *p, const s_drawmethod *m,
                          gpu_transform_emit_quad emit, void *userdata)
{
    int64_t x = (int64_t)p->x * 256, y = (int64_t)p->y * 256;
    int64_t width = (int64_t)p->source_width * m->scalex;
    int64_t height = (int64_t)p->source_height * m->scaley;
    int64_t cx = ((int64_t)p->centerx + m->centerx) * m->scalex;
    int64_t cy = ((int64_t)p->centery + m->centery) * m->scaley;
    int64_t xmin = 0, ymin = 0, xmax = (int64_t)p->target_width * 256;
    int64_t ymax = (int64_t)p->target_height * 256;
    int64_t dx, dy, shift = m->shiftx, step_x, step_y, source_x, source_y;
    int64_t begin_y, end_y, rows, row;
    float left, right, u0, u1;

    /* Wider intermediates make ordinary clipping safe; reject inputs whose
     * original fixed-point math was already outside a sensible signed range. */
    if(x < -536870912 || x > 536870912 || y < -536870912 || y > 536870912
       || cx < -536870912 || cx > 536870912 || cy < -536870912 || cy > 536870912
       || width > 536870912 || height > 536870912
       || p->source_width > INT_MAX / 256 || p->source_height > INT_MAX / 256)
        return GPU_TRANSFORM_FAILED;

    if(m->clipw > 0 && m->cliph > 0)
    {
        int64_t clipright = ((int64_t)m->clipx + m->clipw) * 256;
        int64_t clipbottom = ((int64_t)m->clipy + m->cliph) * 256;
        if(m->clipx > 0) xmin = (int64_t)m->clipx * 256;
        if(m->clipy > 0) ymin = (int64_t)m->clipy * 256;
        if(clipright < xmax) xmax = clipright;
        if(clipbottom < ymax) ymax = clipbottom;
    }
    if(xmin >= xmax || ymin >= ymax) return GPU_TRANSFORM_EMPTY;

    dx = m->config & DRAWMETHOD_CONFIG_FLIP_X ? cx - width + x : x - cx;
    dy = m->config & DRAWMETHOD_CONFIG_FLIP_Y ? cy - height + y : y - cy;
    if(m->config & DRAWMETHOD_CONFIG_FLIP_Y) shift = -shift;
    dx += gpu_fixed_floor((dy + height - y) * shift);
    step_x = 65536 / m->scalex;
    step_y = 65536 / m->scaley;
    if(!(m->config & DRAWMETHOD_CONFIG_FLIP_X)) step_x = -step_x;
    if(!(m->config & DRAWMETHOD_CONFIG_FLIP_Y)) step_y = -step_y;
    source_x = m->config & DRAWMETHOD_CONFIG_FLIP_X ? 0
               : (int64_t)p->source_width * 256 + step_x;
    source_y = m->config & DRAWMETHOD_CONFIG_FLIP_Y ? 0
               : (int64_t)p->source_height * 256 + step_y;
    begin_y = dy > ymin ? dy : ymin;
    end_y = dy + height < ymax ? dy + height : ymax;
    if(begin_y >= end_y) return GPU_TRANSFORM_EMPTY;
    rows = (end_y - begin_y) / 256;
    if(!rows) return GPU_TRANSFORM_EMPTY;
    if(dy + height > ymax)
    {
        dx -= gpu_fixed_floor(shift * (dy + height - end_y));
        source_y += gpu_fixed_floor(step_y * (dy + height - end_y));
    }

    if(!shift)
    {
        float delta = (float)-step_y / 256.0f;
        float bottom = (float)gpu_fixed_floor(end_y);
        float v0 = (float)(source_y + (rows - 1) * step_y) / 256.0f - 0.5f * delta;
        if(!gpu_scale_span(dx, width, xmin, xmax, source_x, step_x,
                           &left, &right, &u0, &u1)) return GPU_TRANSFORM_EMPTY;
        return gpu_emit_rect(left, bottom - rows, right, bottom,
                             u0, v0, u1, v0 + rows * delta, GPU_SAMPLE_FIXED8, emit, userdata);
    }

    /* Shear may produce a large horizontal position, but after row clipping
     * any sampled span differs from an edge by at most the validated width. */
    for(row = 0; row < rows; ++row, dx -= shift, source_y += step_y)
    {
        float top = (float)gpu_fixed_floor(end_y) - 1.0f - row;
        if(gpu_scale_span(dx, width, xmin, xmax, source_x, step_x,
                          &left, &right, &u0, &u1))
        {
            float v = (float)source_y / 256.0f;
            if(gpu_emit_rect(left, top, right, top + 1.0f, u0, v, u1, v,
                             GPU_SAMPLE_FIXED8, emit, userdata) == GPU_TRANSFORM_FAILED)
                return GPU_TRANSFORM_FAILED;
        }
    }
    return GPU_TRANSFORM_DRAWN;
}

/*
 * Damon V. Caskey
 * 2026-11-12
 *
 * Select exactly one existing transformation family. Validate before emission,
 * and leave repeats, painter ordering, materials and target synchronization to
 * the renderer. Disabled methods retain the ordinary untransformed fast path.
 */
int gpu_transform_draw(const gpu_transform_parameters *p, const s_drawmethod *method,
                       gpu_transform_emit_quad emit, void *userdata)
{
    s_drawmethod plain;
    const s_drawmethod *m = method;
    if(!p || !emit || p->source_width <= 0 || p->source_height <= 0
       || p->target_width <= 0 || p->target_height <= 0)
        return GPU_TRANSFORM_FAILED;
    if(!m || !(m->config & DRAWMETHOD_CONFIG_ENABLED))
    {
        memset(&plain, 0, sizeof(plain));
        plain.scalex = plain.scaley = 256;
        m = &plain;
    }
    if(p->is_sprite && (!m->scalex || !m->scaley)) return GPU_TRANSFORM_EMPTY;
    if(m->scalex < 0 || m->scaley < 0 || !gpu_finite(m->rotate))
        return GPU_TRANSFORM_FAILED;
    if(m->water.watermode == WATER_MODE_SHEAR && m->water.beginsize > 0.0f)
        return gpu_draw_plane(p, m, emit, userdata);
    if(m->water.watermode && m->water.amplitude)
    {
        if(m->water.watermode == WATER_MODE_SHEAR) return GPU_TRANSFORM_FAILED;
        return gpu_draw_water(p, m, emit, userdata);
    }
    if(!m->scalex || !m->scaley) return GPU_TRANSFORM_EMPTY;
    if(m->rotate) return gpu_draw_rotate(p, m, emit, userdata);
    return gpu_draw_scale(p, m, emit, userdata);
}
