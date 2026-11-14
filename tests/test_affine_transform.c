/*
 * Damon V. Caskey
 * 2026-11-13
 * Exercise the production affine rasterizer without a video backend.
 *
 * Build from the repository root, keeping generated binaries outside the tree:
 * cc -std=c99 -O2 -Wall -Werror -D_DEFAULT_SOURCE -DPP_TEST \
 *   -ffunction-sections -fdata-sections \
 *   -Iengine -Iengine/source -Iengine/source/gamelib \
 *   tests/test_affine_transform.c engine/source/gamelib/transform.c \
 *   engine/source/gamelib/gpu.c engine/source/gamelib/engine_profile.c \
 *   -Wl,--gc-sections -lm -o /tmp/test_affine_transform
 */

#include <assert.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../engine/source/gamelib/types.h"
#include "../engine/source/gamelib/transform.h"

/* Minimal pixel-format state used by the real transform.c. */
int pixelbytes[5] = {1, 1, 2, 3, 4};
int useclip, clipx1, clipy1, clipx2, clipy2;
unsigned char *blendtables[MAX_BLENDINGS];

/*
 * Damon V. Caskey
 * 2026-11-14
 * Keep the optional color-policy material hook outside geometry-only tests.
 */
void drawmethod_global_init(s_drawmethod *method)
{
    /* The full color fixture links the real pixel-format implementation. */
    assert(!method || !(method->colors.chroma_enabled | method->colors.substitute_enabled));
    (void)method;
}

/*
 * Damon V. Caskey
 * 2026-11-13
 * Supply the unused 16-bit blend callback for this geometry-only fixture.
 */
blend16fp getblendfunction16(int alpha)
{
    /* Geometry tests use direct color writes, so no blend function is selected. */
    (void)alpha;
    return NULL;
}

/*
 * Damon V. Caskey
 * 2026-11-13
 * Keep 32-bit geometry assertions independent of a particular blend operator.
 */
blend32fp getblendfunction32(int alpha)
{
    /* Native renderer fixtures cover actual palette, mask and blend behavior. */
    (void)alpha;
    return NULL;
}

/*
 * Damon V. Caskey
 * 2026-11-13
 * Satisfy the unselected mask writer without adding blending dependencies.
 */
unsigned blend_channel32(unsigned source, unsigned dest, unsigned opacity)
{
    /* These geometry cases do not supply a partial-opacity mask. */
    return opacity == 0 ? dest : source;
}

/*
 * Damon V. Caskey
 * 2026-11-13
 * Allocate a zeroed, software-owned image with the production screen layout.
 */
static s_screen *new_screen(int width, int height, int format)
{
    /* Extra inline pixel storage belongs to the same allocation as the header. */
    s_screen *screen = calloc(1, sizeof(*screen) + (size_t)width * height * pixelbytes[format]);
    assert(screen);
    screen->magic = screen_magic;
    screen->width = width;
    screen->height = height;
    screen->pixelformat = format;
    return screen;
}

/*
 * Damon V. Caskey
 * 2026-11-13
 * Access fixture pixels only after checking their storage format.
 */
static unsigned *pixels32(s_screen *screen)
{
    /* Catch an accidental indexed fixture before interpreting it as RGB. */
    assert(screen->pixelformat == PIXEL_32);
    return (unsigned *)screen->data;
}

/*
 * Damon V. Caskey
 * 2026-11-13
 * Enable affine identity independently of the legacy scale fields.
 */
static s_drawmethod identity_method(void)
{
    /* Zero legacy scales intentionally prove that affine owns its geometry. */
    s_drawmethod result = {0};
    result.config = DRAWMETHOD_CONFIG_ENABLED | DRAWMETHOD_CONFIG_AFFINE;
    result.affine.m00 = 1.0f;
    result.affine.m11 = 1.0f;
    return result;
}

/*
 * Damon V. Caskey
 * 2026-11-13
 * Assign unique nonzero colors so sampling errors identify their source texel.
 */
static void set_source_pixels(s_screen *source)
{
    int x, y;
    /* Reserve zero for destination pixels that should remain untouched. */
    for(y = 0; y < source->height; y++)
    {
        for(x = 0; x < source->width; x++)
        {
            pixels32(source)[x + y * source->width] = 1u + (unsigned)x + (unsigned)(y * source->width);
        }
    }
}

/*
 * Damon V. Caskey
 * 2026-11-13
 * Check exact identity placement and composition of translation with pivots.
 */
static void test_identity_and_offsets(void)
{
    s_screen *source = new_screen(3, 2, PIXEL_32);
    s_screen *destination = new_screen(12, 9, PIXEL_32);
    s_drawmethod method = identity_method();
    gfx_entry entry;
    int x, y;

    /* Identity must preserve each texel and the untouched destination border. */
    entry.screen = source;
    set_source_pixels(source);
    gfx_draw_affine(destination, &entry, 4, 3, 0, 0, &method);
    for(y = 0; y < destination->height; y++)
    {
        for(x = 0; x < destination->width; x++)
        {
            unsigned expected = x >= 4 && x < 7 && y >= 3 && y < 5
                ? pixels32(source)[(x - 4) + (y - 3) * source->width] : 0;
            assert(pixels32(destination)[x + y * destination->width] == expected);
        }
    }

    memset(destination->data, 0, (size_t)destination->width * destination->height * 4);
    method.affine.tx = 2.0f;
    method.affine.ty = -1.0f;
    method.centerx = 1;
    method.centery = 1;
    gfx_draw_affine(destination, &entry, 4, 3, 0, 0, &method);
    assert(pixels32(destination)[5 + 1 * destination->width] == 1);
    assert(pixels32(destination)[7 + 2 * destination->width] == 6);

    free(destination);
    free(source);
}

/*
 * Damon V. Caskey
 * 2026-11-13
 * Verify reflection and quarter-turn coverage around the same source pivot.
 */
static void test_flip_and_rotate(void)
{
    s_screen *source = new_screen(3, 2, PIXEL_32);
    s_screen *destination = new_screen(12, 10, PIXEL_32);
    s_drawmethod method = identity_method();
    gfx_entry entry;
    int x, y;

    entry.screen = source;
    set_source_pixels(source);
    /* Flips reverse source axes before the affine matrix is applied. */
    method.config |= DRAWMETHOD_CONFIG_FLIP_X | DRAWMETHOD_CONFIG_FLIP_Y;
    gfx_draw_affine(destination, &entry, 7, 5, 0, 0, &method);
    for(y = 0; y < source->height; y++)
    {
        for(x = 0; x < source->width; x++)
        {
            int destx = 7 - 1 - x;
            int desty = 5 - 1 - y;
            assert(pixels32(destination)[destx + desty * destination->width] ==
                   pixels32(source)[x + y * source->width]);
        }
    }

    memset(destination->data, 0, (size_t)destination->width * destination->height * 4);
    method = identity_method();
    method.affine.m00 = 0.0f;
    method.affine.m01 = -1.0f;
    method.affine.m10 = 1.0f;
    method.affine.m11 = 0.0f;
    gfx_draw_affine(destination, &entry, 7, 2, 0, 0, &method);
    for(y = 0; y < source->height; y++)
    {
        for(x = 0; x < source->width; x++)
        {
            int destx = 7 - 1 - y;
            int desty = 2 + x;
            assert(pixels32(destination)[destx + desty * destination->width] ==
                   pixels32(source)[x + y * source->width]);
        }
    }

    free(destination);
    free(source);
}

/*
 * Damon V. Caskey
 * 2026-11-13
 * Verify magnification and shearing against independently calculated samples.
 */
static void test_scale_shear_and_clip(void)
{
    s_screen *source = new_screen(3, 2, PIXEL_32);
    s_screen *destination = new_screen(12, 10, PIXEL_32);
    s_drawmethod method = identity_method();
    gfx_entry entry;
    int x, y;

    entry.screen = source;
    set_source_pixels(source);
    method.affine.m00 = 2.0f;
    method.affine.m11 = 2.0f;
    gfx_draw_affine(destination, &entry, 1, 2, 0, 0, &method);
    for(y = 0; y < 4; y++)
    {
        for(x = 0; x < 6; x++)
        {
            assert(pixels32(destination)[1 + x + (2 + y) * destination->width] ==
                   pixels32(source)[x / 2 + (y / 2) * source->width]);
        }
    }

    /* Shear and scale must compose in one pass. Clip should still be honored. */
    memset(destination->data, 0, (size_t)destination->width * destination->height * 4);
    method = identity_method();
    method.affine.m00 = 2.0f;
    method.affine.m01 = 1.0f;
    method.affine.m11 = 2.0f;
    useclip = 1;
    clipx1 = 4;
    clipy1 = 3;
    clipx2 = 8;
    clipy2 = 6;
    gfx_draw_affine(destination, &entry, 2, 2, 0, 0, &method);
    for(y = 0; y < destination->height; y++)
    {
        for(x = 0; x < destination->width; x++)
        {
            double srcy = (y + 0.5 - 2.0) / 2.0;
            double srcx = (x + 0.5 - 2.0 - srcy) / 2.0;
            unsigned expected = x >= clipx1 && x < clipx2 &&
                y >= clipy1 && y < clipy2 && srcx >= 0 && srcx < 3 &&
                srcy >= 0 && srcy < 2
                    ? pixels32(source)[(int)srcx + (int)srcy * source->width] : 0;
            assert(pixels32(destination)[x + y * destination->width] == expected);
        }
    }
    useclip = 0;

    free(destination);
    free(source);
}

/*
 * Damon V. Caskey
 * 2026-11-13
 * Exercise indexed sprite decoding and transparent runs through affine geometry.
 */
static void test_rle_sprite_transparency(void)
{
    /* Two RLE rows: [7, transparent, 8] and [9, 10, 11]. */
    static const unsigned char first_row[] = {0, 1, 7, 1, 1, 8, 0xFF};
    static const unsigned char second_row[] = {0, 3, 9, 10, 11, 0xFF};
    unsigned palette[256] = {0};
    s_sprite *sprite = calloc(1, sizeof(*sprite) + sizeof(first_row) + sizeof(second_row));
    s_screen *destination = new_screen(12, 10, PIXEL_32);
    s_drawmethod method = identity_method();
    gfx_entry entry;
    unsigned char *sprite_data;
    int i;

    assert(sprite);
    for(i = 1; i < 256; i++)
    {
        palette[i] = 0x120000u + (unsigned)i;
    }
    sprite->magic = sprite_magic;
    sprite->pixelformat = PIXEL_x8;
    sprite->width = 3;
    sprite->height = 2;
    sprite->centerx = 1;
    sprite->centery = 1;
    sprite->palette = (unsigned char *)palette;
    sprite->data[0] = 2 * (int)sizeof(int);
    sprite->data[1] = (int)sizeof(int) + (int)sizeof(first_row);
    sprite_data = (unsigned char *)sprite->data;
    memcpy(sprite_data + 2 * sizeof(int), first_row, sizeof(first_row));
    memcpy(sprite_data + 2 * sizeof(int) + sizeof(first_row), second_row, sizeof(second_row));

    entry.sprite = sprite;
    gfx_draw_affine(destination, &entry, 5, 4, sprite->centerx, sprite->centery, &method);
    assert(pixels32(destination)[4 + 3 * destination->width] == palette[7]);
    assert(pixels32(destination)[5 + 3 * destination->width] == 0);
    assert(pixels32(destination)[6 + 3 * destination->width] == palette[8]);
    assert(pixels32(destination)[4 + 4 * destination->width] == palette[9]);
    assert(pixels32(destination)[5 + 4 * destination->width] == palette[10]);
    assert(pixels32(destination)[6 + 4 * destination->width] == palette[11]);

    free(destination);
    free(sprite);
}

/*
 * Damon V. Caskey
 * 2026-11-13
 * Compare fractional coefficients to a direct inverse-matrix reference.
 */
static void test_fractional_affine_reference(void)
{
    s_screen *source = new_screen(4, 3, PIXEL_32);
    s_screen *destination = new_screen(22, 16, PIXEL_32);
    s_drawmethod method = identity_method();
    gfx_entry entry;
    double a, b, c, d, determinant;
    double originx, originy;
    int x, y;

    entry.screen = source;
    set_source_pixels(source);
    method.affine.m00 = 1.8f;
    method.affine.m01 = 0.35f;
    method.affine.m10 = -0.25f;
    method.affine.m11 = 1.4f;
    method.affine.tx = 1.25f;
    method.affine.ty = 0.75f;
    gfx_draw_affine(destination, &entry, 4, 5, 1, 1, &method);

    /* Evaluate each destination center independently of the incremental loop. */
    a = method.affine.m00;
    b = method.affine.m01;
    c = method.affine.m10;
    d = method.affine.m11;
    originx = 4 + method.affine.tx - a - b;
    originy = 5 + method.affine.ty - c - d;
    determinant = a * d - b * c;
    for(y = 0; y < destination->height; y++)
    {
        for(x = 0; x < destination->width; x++)
        {
            double dx = x + 0.5 - originx;
            double dy = y + 0.5 - originy;
            double sx = (d * dx - b * dy) / determinant;
            double sy = (a * dy - c * dx) / determinant;
            unsigned expected = sx >= 0 && sx < source->width &&
                sy >= 0 && sy < source->height
                    ? pixels32(source)[(int)sx + (int)sy * source->width] : 0;
            assert(pixels32(destination)[x + y * destination->width] == expected);
        }
    }

    free(destination);
    free(source);
}

/*
 * Damon V. Caskey
 * 2026-11-13
 * Check exact texel boundaries independently of the production sampling loop.
 */
static void assert_affine_reference(s_screen *source, s_screen *destination,
                                    s_drawmethod *method, int drawx, int drawy,
                                    int centerx, int centery)
{
    gfx_entry entry;
    long double a = method->affine.m00, b = method->affine.m01;
    long double c = method->affine.m10, d = method->affine.m11;
    long double determinant;
    int x, y;

    /* Use wider arithmetic for the oracle and evaluate every pixel directly.
     * No epsilon may turn a nearby authored sample into an integer boundary. */
    if(method->config & DRAWMETHOD_CONFIG_FLIP_X) { a = -a; c = -c; }
    if(method->config & DRAWMETHOD_CONFIG_FLIP_Y) { b = -b; d = -d; }
    determinant = a * d - b * c;
    assert(determinant != 0);
    entry.screen = source;
    memset(destination->data, 0, (size_t)destination->width * destination->height * 4);
    gfx_draw_affine(destination, &entry, drawx, drawy, centerx, centery, method);
    for(y = 0; y < destination->height; y++)
    {
        for(x = 0; x < destination->width; x++)
        {
            long double dx = x + 0.5L - drawx - method->affine.tx;
            long double dy = y + 0.5L - drawy - method->affine.ty;
            long double sx = (d * dx - b * dy) / determinant + centerx + method->centerx;
            long double sy = (a * dy - c * dx) / determinant + centery + method->centery;
            unsigned expected = sx >= 0 && sx < source->width &&
                sy >= 0 && sy < source->height
                    ? pixels32(source)[(int)sx + (int)sy * source->width] : 0;
            unsigned actual = pixels32(destination)[x + y * destination->width];

            if(actual != expected)
            {
                fprintf(stderr, "affine boundary (%d,%d): %u != %u, matrix "
                        "[%g,%g;%g,%g]+(%g,%g), pivot (%d,%d), flags %d\n",
                        x, y, actual, expected,
                        method->affine.m00, method->affine.m01,
                        method->affine.m10, method->affine.m11,
                        method->affine.tx, method->affine.ty,
                        centerx + method->centerx, centery + method->centery,
                        method->config);
                assert(actual == expected);
            }
        }
    }
}

/*
 * Damon V. Caskey
 * 2026-11-13
 * Preserve exact half-pixel ties and distinct representable neighboring offsets.
 */
static void test_exact_sample_boundaries(void)
{
    static const affine_transform matrices[] =
    {
        {3, 0, 0, 3, 0, 0},
        {1.5f, 0, 0, 0.75f, 0, 0},
        {2, 0.5f, 0, 1, 0, 0},
        {1, 0.5f, -0.5f, 0.75f, 0, 0},
        {1, 0, 0, 1, 0, 0},
        {0, -1, 1, 0, 0, 0}
    };
    const float translations[] =
    {
        0.5f, nextafterf(0.5f, 0.0f), nextafterf(0.5f, 1.0f),
        0.49995f, 0.50005f, -0.5f,
        nextafterf(-0.5f, -1.0f), nextafterf(-0.5f, 0.0f)
    };
    s_screen *source = new_screen(7, 5, PIXEL_32);
    s_screen *destination = new_screen(56, 48, PIXEL_32);
    unsigned matrix, translation;
    int pivot, flip;

    /* Threefold scale exposes truncation of a 1/3 sampling step. Vary the
     * pivot and facing flags so agreement cannot depend on the scan origin. */
    set_source_pixels(source);
    for(matrix = 0; matrix < sizeof(matrices) / sizeof(matrices[0]); matrix++)
    {
        for(translation = 0; translation < sizeof(translations) / sizeof(translations[0]); translation++)
        {
            for(pivot = 0; pivot < 2; pivot++)
            {
                for(flip = 0; flip < 4; flip++)
                {
                    s_drawmethod method = identity_method();
                    method.affine = matrices[matrix];
                    method.affine.tx = translations[translation];
                    method.affine.ty = translations[translation];
                    method.centerx = pivot ? 1 : 0;
                    method.centery = pivot ? 3 : 0;
                    if(flip & 1) method.config |= DRAWMETHOD_CONFIG_FLIP_X;
                    if(flip & 2) method.config |= DRAWMETHOD_CONFIG_FLIP_Y;
                    assert_affine_reference(source, destination, &method, 24, 20,
                                            pivot ? 3 : 0, pivot ? -2 : 0);
                }
            }
        }
    }

    free(destination);
    free(source);
}

/*
 * Damon V. Caskey
 * 2026-11-13
 * Reject invalid, collapsed and distant geometry without touching pixels.
 */
static void test_singular_and_offscreen(void)
{
    s_screen *source = new_screen(3, 2, PIXEL_32);
    s_screen *destination = new_screen(12, 10, PIXEL_32);
    s_drawmethod method = identity_method();
    gfx_entry entry;
    int x, y;

    entry.screen = source;
    set_source_pixels(source);
    /* Include extreme inverse coordinates as well as invalid float values. */
    method.affine.m11 = 0;
    gfx_draw_affine(destination, &entry, 4, 4, 0, 0, &method);
    method = identity_method();
    method.affine.tx = 1000000000.0f;
    gfx_draw_affine(destination, &entry, 4, 4, 0, 0, &method);
    method.affine.tx = -1000000000.0f;
    gfx_draw_affine(destination, &entry, 4, 4, 0, 0, &method);
    method.affine.tx = 0.0f;
    method.affine.m00 = 1.0e-12f;
    gfx_draw_affine(destination, &entry, 4, 4, 0, 0, &method);
    method.affine.m00 = NAN;
    gfx_draw_affine(destination, &entry, 4, 4, 0, 0, &method);
    method.affine.m00 = INFINITY;
    gfx_draw_affine(destination, &entry, 4, 4, 0, 0, &method);
    for(y = 0; y < destination->height; y++)
    {
        for(x = 0; x < destination->width; x++)
        {
            assert(pixels32(destination)[x + y * destination->width] == 0);
        }
    }

    free(destination);
    free(source);
}

/*
 * Damon V. Caskey
 * 2026-11-13
 * Run deterministic affine raster checks without requiring a graphics context.
 */
int main(void)
{
    /* Keep numeric edge cases alongside ordinary sprite and screen geometry. */
    test_identity_and_offsets();
    test_flip_and_rotate();
    test_scale_shear_and_clip();
    test_rle_sprite_transparency();
    test_fractional_affine_reference();
    test_exact_sample_boundaries();
    test_singular_and_offscreen();
    puts("affine transform tests passed");
    return 0;
}
