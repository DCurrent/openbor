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
 * 2026-11-12
 *
 * Run the standalone renderer arithmetic suite and report a concise result.
 */
int main(void)
{
    /* Validate material arithmetic before geometry, then exercise rejection
     * paths to ensure invalid inputs cannot leave partially submitted work. */
    test_blends();
    test_scale_and_flip();
    test_rotation();
    test_plane();
    test_water();
    test_validation();
    test_coordinate_limits();
    puts("GPU transform/blend tests passed (393216 exhaustive blend pairs).");
    return 0;
}
