/*
 * Standalone shader-equation and geometry regression tests.
 *
 * Build from the repository root (no SDL/OpenGL context required):
 * gcc -std=gnu99 -O2 -DPP_TEST -ffunction-sections -fdata-sections \
 *   -Iengine/source -Iengine/source/gamelib \
 *   tests/test_gpu_transform.c engine/source/gamelib/gpu_transform.c \
 *   engine/source/gamelib/gpu_shader.c engine/source/gamelib/pixelformat.c \
 *   -Wl,--gc-sections -lm -o test_gpu_transform
 */

#include <assert.h>
#include <float.h>
#include <limits.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#include "gpu_shader.h"
#include "gpu_transform.h"

extern unsigned blend_screen(unsigned source, unsigned destination);
extern unsigned blend_multiply(unsigned source, unsigned destination);
extern unsigned blend_overlay(unsigned source, unsigned destination);
extern unsigned blend_hardlight(unsigned source, unsigned destination);
extern unsigned blend_dodge(unsigned source, unsigned destination);
extern unsigned blend_half(unsigned source, unsigned destination);

typedef struct captured_quad
{
    gpu_vertex vertices[4];
    int wrap;
} captured_quad;

typedef struct capture_state
{
    captured_quad quads[256];
    int count;
} capture_state;

/*
 * Damon V. Caskey
 * 2026-11-12
 *
 * Retain emitted geometry for deterministic checks without a graphics driver.
 */
static int capture(const gpu_vertex vertices[4], int wrap, void *userdata)
{
    capture_state *state = (capture_state *)userdata;

    /* Keep submission order and sampling mode beside each copied quad so
     * later checks can verify row progression as well as vertex positions. */
    assert(state->count < (int)(sizeof(state->quads) / sizeof(state->quads[0])));
    memcpy(state->quads[state->count].vertices, vertices,
           sizeof(state->quads[state->count].vertices));
    state->quads[state->count].wrap = wrap;
    ++state->count;
    return 1;
}

/*
 * Damon V. Caskey
 * 2026-11-12
 *
 * Initialize a self-contained ordinary drawmethod, avoiding engine globals.
 */
static s_drawmethod method(void)
{
    s_drawmethod result;

    /* Zero disables optional effects; fixed-point 256 gives unit scale and
     * full channel values preserve unmodified source colors. */
    memset(&result, 0, sizeof(result));
    result.config = DRAWMETHOD_CONFIG_ENABLED;
    result.scalex = result.scaley = 256;
    result.channelr = result.channelg = result.channelb = 255;
    return result;
}

/*
 * Damon V. Caskey
 * 2026-11-12
 *
 * Compare geometry with a small tolerance for float interpolation setup.
 */
static void near_value(double actual, double expected)
{
    /* The tolerance covers interpolation setup roundoff while still exposing
     * a meaningful shift in source coordinates or destination pixel edges. */
    if(fabs(actual - expected) > 0.001)
    {
        fprintf(stderr, "expected %.9f, got %.9f\n", expected, actual);
        assert(0);
    }
}

/*
 * Damon V. Caskey
 * 2026-11-12
 *
 * Mirror GLSL byte-domain arithmetic with 32-bit floats. Exhaustive comparisons
 * against the actual legacy blend functions catch rounding and mode mistakes.
 */
static unsigned shader_float_blend(int mode, unsigned s, unsigned d)
{
    float source = (float)s, destination = (float)d, selector, other, value;

    /* Evaluate in shader-like float precision, preserving each mode's integer
     * division boundary. Overlay and hardlight differ only in which channel
     * selects the low/high branch. */
    switch(mode)
    {
    case 1:
        return (unsigned)(255.0f - floorf((255.0f-source)*(255.0f-destination)/255.0f + 0.0001f));
    case 2:
        return (unsigned)floorf(source * destination / 256.0f);
    case 3:
    case 4:
        selector = mode == 3 ? destination : source;
        other = mode == 3 ? source : destination;
        value = selector < 128.0f ? floorf(2.0f * selector * other / 256.0f)
              : 255.0f - floorf((511.0f-2.0f*selector)*(255.0f-other)/255.0f + 0.0001f);
        return (unsigned)value;
    case 5:
        return (unsigned)fminf(255.0f, floorf(destination*256.0f/(256.0f-source) + 0.0001f));
    default:
        return (unsigned)floorf((source + destination) / 2.0f);
    }
}

/*
 * Damon V. Caskey
 * 2026-11-12
 *
 * Verify every possible channel pair for all six blend modes, plus channel
 * opacity endpoints and midpoints. Tests call production software equations.
 */
static void test_blends(void)
{
    unsigned (*legacy[6])(unsigned, unsigned) = {
        blend_screen, blend_multiply, blend_overlay,
        blend_hardlight, blend_dodge, blend_half
    };
    const unsigned opacities[] = {0, 1, 127, 128, 254, 255};
    unsigned source, destination, a;
    int mode;

    /* Exhaust every byte pair against the production integer implementation.
     * Both the exported reference and float expression must round identically. */
    for(mode = 1; mode <= 6; ++mode)
    {
        for(source = 0; source < 256; ++source)
        {
            for(destination = 0; destination < 256; ++destination)
            {
                unsigned expected = legacy[mode - 1](source, destination);
                assert(gpu_shader_blend_reference(mode, source, destination) == expected);
                assert(shader_float_blend(mode, source, destination) == expected);
            }
        }
    }

    /* Opacity uses the engine's 255 complement and 256 divisor. Endpoints
     * intentionally follow that existing arithmetic alongside midpoint cases. */
    for(source = 0; source < 256; ++source)
    {
        for(destination = 0; destination < 256; ++destination)
        {
            for(a = 0; a < sizeof(opacities)/sizeof(opacities[0]); ++a)
            {
                unsigned opacity = opacities[a];
                unsigned expected = (source*opacity + destination*(255-opacity)) >> 8;
                unsigned sampled = (unsigned)floorf(((float)source*opacity
                                   + (float)destination*(255-opacity))/256.0f);
                assert(gpu_shader_channel_reference(source, destination, opacity) == expected);
                assert(sampled == expected);
            }
        }
    }
    assert(strstr(gpu_vertex_shader_source, "#version 120"));
    assert(strstr(gpu_fragment_shader_source, "gl_FragCoord.xy/targetSize"));
    assert(strstr(gpu_fragment_shader_source, "blendTableMask"));
}

/*
 * Damon V. Caskey
 * 2026-11-12
 *
 * Check pivots, scaling, independent flips, shear and disabled-method behavior.
 */
static void test_scale_and_flip(void)
{
    gpu_transform_parameters p = {8, 4, 64, 64, 20, 30, 2, 1, 1};
    s_drawmethod m = method();
    capture_state state;

    /* Start with the sprite's own pivot at unit scale. Geometry should cover
     * its exact image bounds using the fixed-point nearest-sample convention. */
    memset(&state, 0, sizeof(state));
    assert(gpu_transform_draw(&p, &m, capture, &state) == GPU_TRANSFORM_DRAWN);
    assert(state.count == 1 && state.quads[0].wrap == GPU_SAMPLE_FIXED8);
    near_value(state.quads[0].vertices[0].x, 18);
    near_value(state.quads[0].vertices[0].y, 29);
    near_value(state.quads[0].vertices[2].x, 26);
    near_value(state.quads[0].vertices[2].y, 33);

    /* Combining method pivots, nonuniform scale, shear and both flips forces
     * independent horizontal and vertical mapping into the emitted row strips. */
    memset(&state, 0, sizeof(state));
    m.centerx = 1; m.centery = 2;
    m.scalex = 512; m.scaley = 128; m.shiftx = 128;
    m.config |= DRAWMETHOD_CONFIG_FLIP_X | DRAWMETHOD_CONFIG_FLIP_Y;
    assert(gpu_transform_draw(&p, &m, capture, &state) == GPU_TRANSFORM_DRAWN);
    assert(state.count == 2);
    near_value(state.quads[0].vertices[0].x, 9);
    near_value(state.quads[0].vertices[0].y, 30);
    near_value(state.quads[0].vertices[2].x, 25);
    near_value(state.quads[0].vertices[2].y, 31);
    near_value(state.quads[1].vertices[0].y, 29);
    near_value(state.quads[1].vertices[0].v, 2);

    /* Disabling the method must restore the ordinary sprite geometry even
     * while its optional transform fields retain their previous values. */
    memset(&state, 0, sizeof(state));
    m.config = DRAWMETHOD_CONFIG_NONE;
    assert(gpu_transform_draw(&p, &m, capture, &state) == GPU_TRANSFORM_DRAWN);
    near_value(state.quads[0].vertices[0].x, 18);
    near_value(state.quads[0].vertices[0].y, 29);
}

/*
 * Damon V. Caskey
 * 2026-11-14
 * Check optional shear sampling against hand values and the unchanged row
 * geometry contract, including clipped phases and extreme reflected shifts.
 */
static void test_sampled_shear_contract(void)
{
    gpu_transform_parameters p = {8, 1, 10, 1, 2, 0, 0, 0, 0};
    s_drawmethod m = method();
    gpu_shear_sampling sampling;
    capture_state sampled, legacy;
    const int extreme_shifts[] = {INT_MAX, INT_MIN};
    int width, index, vertex;

    /* One fixed unit crosses the right clip at width 10. The old geometry
     * drops its leftmost pixel and retains a 255/256 source phase. Width 11
     * leaves eight pixels and an unshifted first source sample. */
    m.shiftx = 1;
    for(width = 10; width <= 11; ++width)
    {
        p.target_width = width;
        memset(&sampled, 0, sizeof(sampled));
        memset(&legacy, 0, sizeof(legacy));
        assert(gpu_transform_draw(&p, &m, capture, &legacy) == GPU_TRANSFORM_DRAWN);
        assert(gpu_transform_draw_sampled(&p, &m, &sampling, capture, &sampled) == GPU_TRANSFORM_DRAWN);
        assert(legacy.count == 1 && legacy.quads[0].wrap == GPU_SAMPLE_FIXED8);
        near_value(legacy.quads[0].vertices[0].x, width == 10 ? 3 : 2);
        near_value(legacy.quads[0].vertices[2].x, 10);
        near_value(legacy.quads[0].vertices[0].u + 0.5, width == 10 ? 255.0 / 256.0 : 0);
        assert(sampling.enabled && sampled.count == 1 && sampled.quads[0].wrap == GPU_SAMPLE_SHEAR);
        near_value(sampled.quads[0].vertices[0].x, 2);
        near_value(sampled.quads[0].vertices[2].x, 10);
        near_value(sampling.origin[0], 2);
        near_value(sampling.origin[1], 1);
        near_value(sampling.origin[2], 1);
        near_value(sampling.extent[0], 8);
        near_value(sampling.extent[1], 0);
        near_value(sampling.extent[2], 1);
        near_value(sampling.shift[0], 0);
        near_value(sampling.shift[1], 1);
        near_value(sampling.source[0], 1792);
        near_value(sampling.source[1], 0);
        near_value(sampling.step[0], -256);
        near_value(sampling.step[1], -256);
        near_value(sampling.clip[0], 0);
        near_value(sampling.clip[1], width);
    }

    /* Integer reciprocal steps become zero above scale 65536. Keep the old
     * constant source coordinate instead of inventing a minimum unit step. */
    p = (gpu_transform_parameters){2, 2, 16, 16, 0, 0, 0, 0, 0};
    m.scalex = m.scaley = 65537;
    memset(&sampled, 0, sizeof(sampled));
    memset(&legacy, 0, sizeof(legacy));
    assert(gpu_transform_draw(&p, &m, capture, &legacy) == GPU_TRANSFORM_DRAWN);
    assert(gpu_transform_draw_sampled(&p, &m, &sampling, capture, &sampled) == GPU_TRANSFORM_DRAWN);
    assert(legacy.count == 16 && sampling.enabled && sampled.count == 1);
    near_value(sampling.step[0], 0);
    near_value(sampling.step[1], 0);
    near_value(sampling.source[0], 512);
    near_value(sampling.source[1], 512);
    for(index = 0; index < legacy.count; ++index)
    {
        assert(legacy.quads[index].wrap == GPU_SAMPLE_FIXED8);
        for(vertex = 0; vertex < 4; ++vertex)
        {
            near_value(legacy.quads[index].vertices[vertex].u, 2);
            near_value(legacy.quads[index].vertices[vertex].v, 2);
        }
    }

    /* Vertical reflection negates the signed shift in wider arithmetic. Only
     * the bottom row intersects the target for either integer extreme, so
     * large dormant travel must not reject its bounded native intersection. */
    p = (gpu_transform_parameters){8, 3, 16, 16, 2, 3, 0, 0, 0};
    m = method();
    m.config |= DRAWMETHOD_CONFIG_FLIP_Y;
    for(index = 0; index < 2; ++index)
    {
        m.shiftx = extreme_shifts[index];
        memset(&sampled, 0, sizeof(sampled));
        memset(&legacy, 0, sizeof(legacy));
        assert(gpu_transform_draw(&p, &m, capture, &legacy) == GPU_TRANSFORM_DRAWN);
        assert(gpu_transform_draw_sampled(&p, &m, &sampling, capture, &sampled) == GPU_TRANSFORM_DRAWN);
        assert(legacy.count == 1 && legacy.quads[0].wrap == GPU_SAMPLE_FIXED8);
        assert(sampling.enabled && sampled.count == 1 && sampled.quads[0].wrap == GPU_SAMPLE_SHEAR);
        near_value(sampling.shift[0], index == 0 ? -8388608 : 8388608);
        near_value(sampling.shift[1], index == 0 ? 1 : 0);
        near_value(sampling.extent[2], 1);
        near_value(sampling.source[1], 0);
        near_value(sampling.step[1], 256);
        for(vertex = 0; vertex < 4; ++vertex)
        {
            near_value(sampled.quads[0].vertices[vertex].x, legacy.quads[0].vertices[vertex].x);
            near_value(sampled.quads[0].vertices[vertex].y, legacy.quads[0].vertices[vertex].y);
        }
    }
}

/*
 * Damon V. Caskey
 * 2026-11-14
 * Bound optional sampler precision without requesting CPU pixel fallback.
 * Large visible shear uses one callback; empty coverage submits no geometry.
 */
static void test_sampled_shear_limits(void)
{
    gpu_transform_parameters p;
    int *dimensions[] = {&p.source_width, &p.source_height, &p.target_width, &p.target_height};
    s_drawmethod m = method();
    gpu_shear_sampling sampling;
    capture_state sampled, legacy;
    unsigned dimension;

    m.shiftx = 1;
    for(dimension = 0; dimension < sizeof(dimensions) / sizeof(dimensions[0]); ++dimension)
    {
        p = (gpu_transform_parameters){2, 2, 8, 8, 0, 0, 0, 0, 0};
        *dimensions[dimension] = 65535;
        memset(&sampled, 0, sizeof(sampled));
        assert(gpu_transform_draw_sampled(&p, &m, &sampling, capture, &sampled) == GPU_TRANSFORM_DRAWN);
        assert(sampling.enabled && sampled.count == 1 && sampled.quads[0].wrap == GPU_SAMPLE_SHEAR);

        /* The next dimension is still valid native geometry. Explicitly
         * require the ordinary rows, rather than FAILED or partial emission. */
        *dimensions[dimension] = 65536;
        memset(&sampled, 0, sizeof(sampled));
        memset(&legacy, 0, sizeof(legacy));
        assert(gpu_transform_draw(&p, &m, capture, &legacy) == GPU_TRANSFORM_DRAWN);
        assert(gpu_transform_draw_sampled(&p, &m, &sampling, capture, &sampled) == GPU_TRANSFORM_DRAWN);
        assert(!sampling.enabled && legacy.count > 0 && sampled.count == legacy.count);
        assert(sampled.quads[0].wrap == GPU_SAMPLE_FIXED8);
        assert(!memcmp(sampled.quads, legacy.quads, (size_t)legacy.count * sizeof(legacy.quads[0])));
    }

    /* Sixty thousand visible rows must fit the same one-quad callback
     * contract without enlarging the capture helper's fixed capacity. */
    p = (gpu_transform_parameters){4, 60000, 512, 60000, 0, 0, 0, 0, 0};
    memset(&sampled, 0, sizeof(sampled));
    assert(gpu_transform_draw_sampled(&p, &m, &sampling, capture, &sampled) == GPU_TRANSFORM_DRAWN);
    assert(sampling.enabled && sampled.count == 1 && sampled.quads[0].wrap == GPU_SAMPLE_SHEAR);
    near_value(sampling.extent[2], 60000);
    near_value(sampled.quads[0].vertices[0].x, 0);
    near_value(sampled.quads[0].vertices[0].y, 0);
    near_value(sampled.quads[0].vertices[2].x, 238);
    near_value(sampled.quads[0].vertices[2].y, 60000);

    /* Entirely horizontal clipping and a width below one destination pixel
     * both complete native handling without invoking the callback. */
    p = (gpu_transform_parameters){8, 4, 16, 16, 100, 0, 0, 0, 0};
    memset(&sampled, 0, sizeof(sampled));
    assert(gpu_transform_draw_sampled(&p, &m, &sampling, capture, &sampled) == GPU_TRANSFORM_EMPTY);
    assert(!sampling.enabled && sampled.count == 0);
    p.source_width = 1;
    p.x = 0;
    m.scalex = 128;
    sampling.enabled = 1;
    assert(gpu_transform_draw_sampled(&p, &m, &sampling, capture, &sampled) == GPU_TRANSFORM_EMPTY);
    assert(!sampling.enabled && sampled.count == 0);
}

/*
 * Damon V. Caskey
 * 2026-11-12
 *
 * Check inverse rotation sampling at exact quarter-turns with nonuniform scale.
 */
static void test_rotation(void)
{
    gpu_transform_parameters p = {8, 4, 64, 64, 20, 30, 2, 1, 1};
    s_drawmethod m = method();
    capture_state state;
    int flip;

    /* Quarter-turns have exact zero/unit trigonometric coefficients. Pairing
     * them with unequal axis scales exposes swapped inverse-map denominators. */
    m.rotate = 90.0f; m.scalex = 512; m.scaley = 128;
    for(flip = 0; flip < 4; ++flip)
    {
        gpu_vertex *vertices;
        float ax, ay, bx, by, source_x, source_y;
        memset(&state, 0, sizeof(state));
        m.config = DRAWMETHOD_CONFIG_ENABLED
                 | (flip & 1 ? DRAWMETHOD_CONFIG_FLIP_X : 0)
                 | (flip & 2 ? DRAWMETHOD_CONFIG_FLIP_Y : 0);
        assert(gpu_transform_draw(&p, &m, capture, &state) == GPU_TRANSFORM_DRAWN);
        assert(state.count == 1);
        /* Recover source-coordinate gradients from the emitted quad, then
         * sample the pivot at its destination pixel center. */
        vertices = state.quads[0].vertices;
        ax = (vertices[1].u - vertices[0].u) / (vertices[1].x - vertices[0].x);
        ay = (vertices[1].v - vertices[0].v) / (vertices[1].x - vertices[0].x);
        bx = (vertices[3].u - vertices[0].u) / (vertices[3].y - vertices[0].y);
        by = (vertices[3].v - vertices[0].v) / (vertices[3].y - vertices[0].y);
        source_x = vertices[0].u + ax * (20.5f - vertices[0].x) + bx * (30.5f - vertices[0].y);
        source_y = vertices[0].v + ay * (20.5f - vertices[0].x) + by * (30.5f - vertices[0].y);
        near_value(source_x, 2.0);
        near_value(source_y, 1.0);
        near_value(ax, 0.0);
        near_value(by, 0.0);
        near_value(fabs(ay), 0.5);
        near_value(fabs(bx), 2.0);
    }
}

/*
 * Damon V. Caskey
 * 2026-11-12
 *
 * Compare plane rows with the original recurrence for all perspective modes,
 * including a clipped top and nonzero wavetime texture scroll.
 */
static void test_plane(void)
{
    gpu_transform_parameters p = {16, 12, 64, 64, 8, -3, 0, 0, 0};
    s_drawmethod m = method();
    capture_state state;
    int perspective, row;
    m.water.watermode = WATER_MODE_SHEAR;
    m.water.beginsize = 0.75f; m.water.endsize = 2.0f;
    m.water.wavetime = 5;

    /* Advance the original vertical recurrence through clipped rows too;
     * otherwise the visible texture would jump when the plane crosses y=0. */
    for(perspective = WATER_PERSPECTIVE_NONE; perspective <= WATER_PERSPECTIVE_STRETCH; ++perspective)
    {
        float size = m.water.beginsize, sy = 0.0f;
        float factor = (m.water.endsize - size) / p.source_height;
        m.water.perspective = (e_water_perspective)perspective;
        memset(&state, 0, sizeof(state));
        assert(gpu_transform_draw(&p, &m, capture, &state) == GPU_TRANSFORM_DRAWN);
        assert(state.count == 9);
        for(row = 0; row < p.source_height; ++row, size += factor)
        {
            float step = 1.0f / size;
            if(row >= 3)
            {
                /* The first three source rows lie above the target. Visible
                 * row indices therefore begin at zero while texture phase
                 * continues from the same accumulated source position. */
                captured_quad *quad = &state.quads[row - 3];
                float expected_u = m.water.wavetime - p.x + p.source_width
                                 - (p.target_width / 2.0f - p.x) * step;
                float observed_u = quad->vertices[0].u
                                 + (quad->vertices[1].u - quad->vertices[0].u)
                                   * 0.5f / p.source_width;
                near_value(observed_u, expected_u);
                near_value(quad->vertices[0].v, (int)sy % p.source_height);
                near_value(quad->vertices[0].y, row - 3);
                assert(quad->wrap == 1);
            }
            sy += perspective == WATER_PERSPECTIVE_TILE ? step
                : perspective == WATER_PERSPECTIVE_STRETCH
                  ? step * p.source_height / p.source_width : 1.0f;
        }
    }
}

/*
 * Damon V. Caskey
 * 2026-11-12
 *
 * Ensure water row phase and clipping are stable, with no per-pixel CPU work.
 */
static void test_water(void)
{
    gpu_transform_parameters p = {8, 8, 64, 64, 20, -2, 0, 0, 0};
    s_drawmethod m = method();
    capture_state state;
    int row;
    m.water.watermode = WATER_MODE_SINE;
    m.water.amplitude = 4; m.water.wavelength = 8.0f; m.water.wavetime = 0;
    memset(&state, 0, sizeof(state));
    assert(gpu_transform_draw(&p, &m, capture, &state) == GPU_TRANSFORM_DRAWN);
    assert(state.count == 6);

    /* Two rows are clipped above the target, but their sine phase remains
     * part of the original wave. Check phase, coverage and source-row centers. */
    for(row = 0; row < state.count; ++row)
    {
        double phase = (row + 2) * (2.0 * 3.14159265358979323846 / 8.0);
        float shift = (float)floor((1.0 + sin(phase)) * 4.0 + 0.5) - 4.0f;
        near_value(state.quads[row].vertices[0].x, 20 + shift);
        near_value(state.quads[row].vertices[0].y, row);
        near_value(state.quads[row].vertices[0].v, row + 2.5f);
        assert(state.quads[row].wrap == 0);
    }
}

/*
 * Damon V. Caskey
 * 2026-11-12
 *
 * Reject invalid/singular inputs without emitting partially rendered geometry.
 */
static void test_validation(void)
{
    gpu_transform_parameters p = {8, 8, 64, 64, 20, 30, 0, 0, 0};
    s_drawmethod m = method();
    capture_state state;

    /* Zero scale is a valid empty draw; nonfinite angles and singular water
     * parameters are failures. Neither result may emit a partial quad list. */
    memset(&state, 0, sizeof(state));
    m.scalex = 0;
    assert(gpu_transform_draw(&p, &m, capture, &state) == GPU_TRANSFORM_EMPTY);
    assert(state.count == 0);
    m = method(); m.rotate = NAN;
    assert(gpu_transform_draw(&p, &m, capture, &state) == GPU_TRANSFORM_FAILED);
    assert(state.count == 0);
    m = method(); m.water.watermode = WATER_MODE_SINE;
    m.water.amplitude = 4; m.water.wavelength = 0.0f;
    assert(gpu_transform_draw(&p, &m, capture, &state) == GPU_TRANSFORM_FAILED);
    assert(state.count == 0);
    m = method(); m.water.watermode = WATER_MODE_SHEAR;
    m.water.beginsize = 0.25f; m.water.endsize = -0.5f;
    assert(gpu_transform_draw(&p, &m, capture, &state) == GPU_TRANSFORM_FAILED);
    assert(state.count == 0);
}

/*
 * Damon V. Caskey
 * 2026-11-12
 *
 * Exercise exact and adjacent float boundaries before integer texel conversion.
 */
static void test_coordinate_limits(void)
{
    gpu_transform_parameters p = {1, 1, 1, 1, 0, 0, 0, 0, 0};
    s_drawmethod m = method();
    capture_state state;
    const int scroll_cases[] =
    {
        (int)nextafterf(0x1p30f, 0.0f),
        -(int)nextafterf(0x1p30f, 0.0f),
        1073741824, -1073741824, INT_MAX, INT_MIN
    };
    const float wavelengths[] =
    {
        nextafterf(0x1p-22f, INFINITY), 0x1p-22f, 0x1p-23f
    };
    unsigned index;

    /* Float rounding turns INT_MAX / 2 into exactly 2^30 if the bound itself
     * uses float precision. The nearest float below remains valid, whereas
     * either signed 2^30 endpoint and the rounded integer extremes must fail.
     * Reset the capture for every attempt to expose any partial submission. */
    m.water.watermode = WATER_MODE_SHEAR;
    m.water.beginsize = m.water.endsize = 1.0f;
    for(index = 0; index < sizeof(scroll_cases) / sizeof(scroll_cases[0]); ++index)
    {
        int expected = index < 2 ? GPU_TRANSFORM_DRAWN : GPU_TRANSFORM_FAILED;
        memset(&state, 0, sizeof(state));
        m.water.wavetime = scroll_cases[index];
        assert(gpu_transform_draw(&p, &m, capture, &state) == expected);
        assert(state.count == (expected == GPU_TRANSFORM_DRAWN ? 1 : 0));
    }

    /* Reciprocal wavelength can land on the same boundary without a large
     * input coordinate. The adjacent larger wavelength lowers the phase step
     * into range; exact 2^30 and 2^31 steps must be rejected before drawing. */
    m = method();
    m.water.watermode = WATER_MODE_SINE;
    m.water.amplitude = 1;
    for(index = 0; index < sizeof(wavelengths) / sizeof(wavelengths[0]); ++index)
    {
        int expected = index == 0 ? GPU_TRANSFORM_DRAWN : GPU_TRANSFORM_FAILED;
        memset(&state, 0, sizeof(state));
        m.water.wavelength = wavelengths[index];
        assert(gpu_transform_draw(&p, &m, capture, &state) == expected);
        assert(state.count == (expected == GPU_TRANSFORM_DRAWN ? 1 : 0));
    }

    /* These horizontal endpoints stay within the guard, but vertical texture
     * travel reaches the boundary on the second row. Whole-draw preflight must
     * reject that row before the valid first row reaches the callback. */
    p.source_height = p.target_height = 2;
    m = method();
    m.water.watermode = WATER_MODE_SHEAR;
    m.water.perspective = WATER_PERSPECTIVE_TILE;
    m.water.beginsize = m.water.endsize = 0x1p-30f;
    memset(&state, 0, sizeof(state));
    assert(gpu_transform_draw(&p, &m, capture, &state) == GPU_TRANSFORM_FAILED);
    assert(state.count == 0);

    /* Moving the scale one float above the boundary decreases its reciprocal
     * enough for both rows to remain valid, guarding against over-rejection. */
    m.water.beginsize = m.water.endsize = nextafterf(0x1p-30f, INFINITY);
    memset(&state, 0, sizeof(state));
    assert(gpu_transform_draw(&p, &m, capture, &state) == GPU_TRANSFORM_DRAWN);
    assert(state.count == 2);
}

/*
 * Damon V. Caskey
 * 2026-11-13
 *
 * Check that emitted affine geometry carries the documented source mapping,
 * including local flips and both pivot contributions, in one native quad.
 */
static void test_affine_mapping(void)
{
    const affine_transform matrices[] =
    {
        {1, 0, 0, 1, 0, 0},
        {2, 0, 0, 0.5f, 0.25f, -0.75f},
        {1, 0.5f, -0.25f, 1, -1.5f, 2.25f},
        {0, -1, 1, 0, 0, 0},
        {-1, 0, 0, 1, 0.5f, 0.5f}
    };
    gpu_transform_parameters p = {8, 6, 64, 64, 24, 25, 2, 1, 1};
    unsigned matrix;
    int flip, vertex;

    /* Recover destination coordinates from each emitted source coordinate.
     * This is independent of whether the backend chooses transformed corners
     * or a conservative rectangle with inverse-mapped texture coordinates. */
    for(matrix = 0; matrix < sizeof(matrices) / sizeof(matrices[0]); ++matrix)
    {
        for(flip = 0; flip < 4; ++flip)
        {
            s_drawmethod m = method();
            capture_state state;
            m.config |= DRAWMETHOD_CONFIG_AFFINE
                      | (flip & 1 ? DRAWMETHOD_CONFIG_FLIP_X : 0)
                      | (flip & 2 ? DRAWMETHOD_CONFIG_FLIP_Y : 0);
            m.affine = matrices[matrix];
            m.centerx = 1;
            m.centery = -2;
            memset(&state, 0, sizeof(state));
            assert(gpu_transform_draw(&p, &m, capture, &state) == GPU_TRANSFORM_DRAWN);
            assert(state.count == 1 && state.quads[0].wrap == GPU_SAMPLE_AFFINE);
            for(vertex = 0; vertex < 4; ++vertex)
            {
                const gpu_vertex *v = &state.quads[0].vertices[vertex];
                double local_x = v->u - p.centerx - m.centerx;
                double local_y = v->v - p.centery - m.centery;
                if(flip & 1) local_x = -local_x;
                if(flip & 2) local_y = -local_y;
                near_value(v->x, p.x + m.affine.tx
                           + m.affine.m00 * local_x + m.affine.m01 * local_y);
                near_value(v->y, p.y + m.affine.ty
                           + m.affine.m10 * local_x + m.affine.m11 * local_y);
            }
        }
    }
}

/*
 * Damon V. Caskey
 * 2026-11-13
 *
 * An enabled affine matrix owns geometry even when stored legacy transform
 * fields would otherwise reject or hide a draw. Disabling it restores scale.
 */
static void test_affine_precedence(void)
{
    gpu_transform_parameters p = {8, 6, 64, 64, 20, 30, 2, 1, 1};
    s_drawmethod m = method();
    capture_state state;

    /* Deliberately invalid dormant settings expose premature legacy checks.
     * The matrix remains an ordinary identity with the existing source pivot. */
    m.config |= DRAWMETHOD_CONFIG_AFFINE;
    m.affine.m00 = m.affine.m11 = 1.0f;
    m.scalex = 0;
    m.scaley = -256;
    m.rotate = NAN;
    m.shiftx = INT_MAX;
    m.water.watermode = WATER_MODE_SHEAR;
    m.water.beginsize = -1.0f;
    m.water.endsize = NAN;
    memset(&state, 0, sizeof(state));
    assert(gpu_transform_draw(&p, &m, capture, &state) == GPU_TRANSFORM_DRAWN);
    assert(state.count == 1);
    near_value(state.quads[0].vertices[0].x, 18);
    near_value(state.quads[0].vertices[0].y, 29);

    /* The flag is opt-in. A disabled matrix must not override the established
     * zero-scale sprite rule, and disabling the whole method restores identity. */
    m.config &= ~DRAWMETHOD_CONFIG_AFFINE;
    memset(&state, 0, sizeof(state));
    assert(gpu_transform_draw(&p, &m, capture, &state) == GPU_TRANSFORM_EMPTY);
    assert(state.count == 0);
    m.config = DRAWMETHOD_CONFIG_AFFINE;
    memset(&state, 0, sizeof(state));
    assert(gpu_transform_draw(&p, &m, capture, &state) == GPU_TRANSFORM_DRAWN);
    near_value(state.quads[0].vertices[0].x, 18);
    near_value(state.quads[0].vertices[0].y, 29);
}

/*
 * Damon V. Caskey
 * 2026-11-13
 *
 * Reject malformed matrices before geometry emission and handle collapsed or
 * distant finite transforms without unsafe coordinate conversion.
 */
static void test_affine_validation(void)
{
    gpu_transform_parameters p = {8, 6, 64, 64, 20, 30, 0, 0, 0};
    s_drawmethod m = method();
    capture_state state;
    float *fields[6];
    unsigned field;

    m.config |= DRAWMETHOD_CONFIG_AFFINE;
    m.affine.m00 = m.affine.m11 = 1.0f;
    fields[0] = &m.affine.m00; fields[1] = &m.affine.m01;
    fields[2] = &m.affine.m10; fields[3] = &m.affine.m11;
    fields[4] = &m.affine.tx; fields[5] = &m.affine.ty;
    for(field = 0; field < sizeof(fields) / sizeof(fields[0]); ++field)
    {
        float saved = *fields[field];
        *fields[field] = field & 1 ? INFINITY : NAN;
        memset(&state, 0, sizeof(state));
        assert(gpu_transform_draw(&p, &m, capture, &state) == GPU_TRANSFORM_FAILED);
        assert(state.count == 0);
        *fields[field] = saved;
    }

    /* A finite zero determinant has no area, even when neither basis vector
     * is zero. Whole-draw rejection must not emit a partially valid shape. */
    m.affine.m00 = m.affine.m01 = m.affine.m10 = m.affine.m11 = 1.0f;
    memset(&state, 0, sizeof(state));
    assert(gpu_transform_draw(&p, &m, capture, &state) == GPU_TRANSFORM_EMPTY);
    assert(state.count == 0);

    /* A finite translation can exceed the integer destination range. It is
     * simply offscreen and must be clipped before any integer conversion. */
    m.affine = (affine_transform){1, 0, 0, 1, FLT_MAX, -FLT_MAX};
    memset(&state, 0, sizeof(state));
    assert(gpu_transform_draw(&p, &m, capture, &state) == GPU_TRANSFORM_EMPTY);
    assert(state.count == 0);

    /* Visible extreme and subnormal scales, and a tiny nonzero shear beyond
     * the native fraction encoding, must request fallback before emission. */
    p.x = p.y = 0;
    for(field = 0; field < 3; ++field)
    {
        m.affine = (affine_transform){1, 0, 0, 1, 0, 0};
        if(field == 0) m.affine.m00 = m.affine.m11 = FLT_MAX;
        if(field == 1) m.affine.m00 = nextafterf(0.0f, 1.0f);
        if(field == 2) m.affine.m01 = 0x1p-60f;
        memset(&state, 0, sizeof(state));
        assert(gpu_transform_draw(&p, &m, capture, &state) == GPU_TRANSFORM_FAILED);
        assert(state.count == 0);
    }
}

/*
 * Damon V. Caskey
 * 2026-11-12
 * 2026-11-14 - Exercise optional shear sampling and retained native rows.
 *
 * Run the standalone renderer arithmetic suite and report a concise result.
 */
int main(void)
{
    /* Validate material arithmetic before geometry, then exercise rejection
     * paths to ensure invalid inputs cannot leave partially submitted work. */
    test_blends();
    test_scale_and_flip();
    test_sampled_shear_contract();
    test_sampled_shear_limits();
    test_rotation();
    test_plane();
    test_water();
    test_validation();
    test_coordinate_limits();
    test_affine_mapping();
    test_affine_precedence();
    test_affine_validation();
    puts("GPU transform/blend tests passed (393216 exhaustive blend pairs).");
    return 0;
}
