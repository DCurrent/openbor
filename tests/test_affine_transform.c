/*
 * Exercise the production affine rasterizer without a video backend.
 *
 * Build from the repository root:
 * gcc -std=gnu99 -O2 -ffunction-sections -fdata-sections \
 *   -Iengine -Iengine/source -Iengine/source/gamelib \
 *   tests/test_affine_transform.c engine/source/gamelib/transform.c \
 *   -Wl,--gc-sections -lm -o test_affine_transform
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

blend16fp getblendfunction16(int alpha)
{
    (void)alpha;
    return NULL;
}

blend32fp getblendfunction32(int alpha)
{
    (void)alpha;
    return NULL;
}

unsigned blend_channel32(unsigned source, unsigned dest, unsigned opacity)
{
    return opacity == 0 ? dest : source;
}

static s_screen *new_screen(int width, int height, int format)
{
    s_screen *screen = calloc(1, sizeof(*screen) + (size_t)width * height * pixelbytes[format]);
    assert(screen);
    screen->magic = screen_magic;
    screen->width = width;
    screen->height = height;
    screen->pixelformat = format;
    return screen;
}

static unsigned *pixels32(s_screen *screen)
{
    assert(screen->pixelformat == PIXEL_32);
    return (unsigned *)screen->data;
}

static s_drawmethod identity_method(void)
{
    s_drawmethod result = {0};
    result.config = DRAWMETHOD_CONFIG_ENABLED | DRAWMETHOD_CONFIG_AFFINE;
    result.affine.m00 = 1.0f;
    result.affine.m11 = 1.0f;
    return result;
}

static void set_source_pixels(s_screen *source)
{
    int x, y;
    for(y = 0; y < source->height; y++)
    {
        for(x = 0; x < source->width; x++)
        {
            pixels32(source)[x + y * source->width] = 1u + (unsigned)x + (unsigned)(y * source->width);
        }
    }
}

static void test_identity_and_offsets(void)
{
    s_screen *source = new_screen(3, 2, PIXEL_32);
    s_screen *destination = new_screen(12, 9, PIXEL_32);
    s_drawmethod method = identity_method();
    gfx_entry entry;
    int x, y;

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

static void test_flip_and_rotate(void)
{
    s_screen *source = new_screen(3, 2, PIXEL_32);
    s_screen *destination = new_screen(12, 10, PIXEL_32);
    s_drawmethod method = identity_method();
    gfx_entry entry;
    int x, y;

    entry.screen = source;
    set_source_pixels(source);
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

static void test_singular_and_offscreen(void)
{
    s_screen *source = new_screen(3, 2, PIXEL_32);
    s_screen *destination = new_screen(12, 10, PIXEL_32);
    s_drawmethod method = identity_method();
    gfx_entry entry;
    int x, y;

    entry.screen = source;
    set_source_pixels(source);
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

int main(void)
{
    test_identity_and_offsets();
    test_flip_and_rotate();
    test_scale_shear_and_clip();
    test_rle_sprite_transparency();
    test_fractional_affine_reference();
    test_singular_and_offscreen();
    puts("affine transform tests passed");
    return 0;
}
