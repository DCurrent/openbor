/*
 * Damon V. Caskey
 * 2026-11-14
 * Verify color-policy validation and complete production software draws.
 * The native fixture reuses these independent pixel expectations so a GPU
 * fallback cannot conceal differences in coverage or composition.
 */
#include <assert.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "gpu.h"
#include "screen.h"
#include "sprite.h"
#include "spriteq.h"
#include "draw.h"
#include "drawmethod_color.h"

#define COLOR_EXPECTED_PIXELS 512
#define COLOR_BACKGROUND 0x00304050u

const s_drawmethod plainmethod = {
    .config = DRAWMETHOD_CONFIG_ENABLED,
    .alpha = -1, .remap = -1,
    .scalex = 256, .scaley = 256,
    .xrepeat = 1, .yrepeat = 1,
    .channelr = 255, .channelg = 255, .channelb = 255,
    .affine = {1, 0, 0, 1, 0, 0},
    .object_type = OBJECT_TYPE_DRAWMETHOD
};

static unsigned color_images_checked;

/*
 * Damon V. Caskey
 * 2026-11-14
 * Preserve fatal allocation checks without the game logging subsystem.
 */
void *checkAlloc(void *pointer, size_t size, const char *function, const char *file, int line)
{
    (void)size; (void)function; (void)file; (void)line;
    assert(pointer);
    return pointer;
}

/*
 * Damon V. Caskey
 * 2026-11-14
 * Construct small RGB or indexed images with the production storage layout.
 */
static s_screen *colors_screen(int width, int height, int format, unsigned initial)
{
    size_t bytes = (size_t)width * height * pixelbytes[format];
    s_screen *screen = calloc(1, sizeof(*screen) + bytes + PAL_BYTES);
    int index;
    assert(screen && width * height <= COLOR_EXPECTED_PIXELS);
    screen->magic = screen_magic;
    screen->width = width;
    screen->height = height;
    screen->pixelformat = format;
    if(format == PIXEL_x8)
    {
        screen->palette = (unsigned char *)screen + offsetof(s_screen, data) + bytes;
    }
    if(format == PIXEL_32)
    {
        for(index = 0; index < width * height; ++index)
        {
            ((unsigned *)screen->data)[index] = initial;
        }
    }
    return screen;
}

/*
 * Damon V. Caskey
 * 2026-11-14
 * Retire a renderer identity before its address may be reused by allocation.
 */
static void colors_release_screen(s_screen *screen)
{
    gpu_release_screen(screen);
    free(screen);
}

/*
 * Damon V. Caskey
 * 2026-11-14
 * Encode a four-pixel indexed row, optionally leaving actual RLE gaps.
 */
static s_sprite *colors_sprite(const unsigned char values[4], int gaps)
{
    s_sprite *sprite = calloc(1, sizeof(*sprite) + 64 + PAL_BYTES);
    unsigned char *run;
    assert(sprite);
    sprite->magic = sprite_magic;
    sprite->width = sprite->srcwidth = 4;
    sprite->height = sprite->srcheight = 1;
    sprite->pixelformat = PIXEL_x8;
    sprite->palette = (unsigned char *)sprite + sizeof(*sprite) + 64;
    sprite->data[0] = sizeof(int);
    run = (unsigned char *)sprite + offsetof(s_sprite, data) + sizeof(int);
    if(gaps)
    {
        /* Only locations zero and two contain encoded source coverage. */
        run[0] = 0; run[1] = 1; run[2] = values[0];
        run[3] = 1; run[4] = 1; run[5] = values[2]; run[6] = 0xff;
    }
    else
    {
        run[0] = 0; run[1] = 4;
        memcpy(run + 2, values, 4);
        run[6] = 0xff;
    }
    return sprite;
}

/*
 * Damon V. Caskey
 * 2026-11-14
 * Release a sprite without taking ownership of its separately managed mask.
 */
static void colors_release_sprite(s_sprite *sprite)
{
    gpu_release_sprite(sprite);
    free(sprite);
}

/*
 * Damon V. Caskey
 * 2026-11-14
 * Build the complete untouched-background expectation for one image.
 */
static void colors_expected(unsigned *expected, const s_screen *screen, unsigned color)
{
    int index;
    for(index = 0; index < screen->width * screen->height; ++index)
    {
        expected[index] = color;
    }
}

/*
 * Damon V. Caskey
 * 2026-11-14
 * Check every RGB pixel, including uncovered borders, after explicit readback.
 */
static void colors_verify(s_screen *screen, const unsigned *expected, const char *label)
{
    int index;
    gpu_sync_cpu(screen);
    for(index = 0; index < screen->width * screen->height; ++index)
    {
        unsigned actual = ((unsigned *)screen->data)[index] & 0xffffffu;
        if(actual != expected[index])
        {
            fprintf(stderr, "%s at (%d,%d): expected %06x, got %06x\n", label,
                    index % screen->width, index / screen->width, expected[index], actual);
            assert(actual == expected[index]);
        }
    }
    ++color_images_checked;
}

/*
 * Damon V. Caskey
 * 2026-11-14
 * Reject incomplete or conflicting changes without mutating stored settings.
 */
static void colors_test_setters(void)
{
    s_drawmethod method = plainmethod;
    s_drawmethod_colors before;
    unsigned index;
    assert(DRAWMETHOD_COLOR_SLOTS == 5);
    assert(!drawmethod_colors_active(&method));
    assert(drawmethod_set_substitute(&method, 0, 0, 0x010101));
    before = method.colors;
    assert(!drawmethod_set_substitute(&method, 5, 2, 3));
    assert(!drawmethod_set_substitute(&method, 1, 0, 2));
    assert(!drawmethod_set_substitute(&method, 1, 0x010101, 2));
    assert(!drawmethod_set_substitute(&method, 1, 2, 0));
    assert(!drawmethod_set_substitute(&method, 1, 2, 2));
    assert(!drawmethod_set_substitute(&method, 1, 0x1000000, 3));
    assert(!drawmethod_set_substitute_enabled(&method, 32));
    assert(memcmp(&before, &method.colors, sizeof(before)) == 0);

    /* Disabling preserves values; enabling validates the complete live list. */
    assert(drawmethod_set_substitute_enabled(&method, 0));
    assert(method.colors.substitute[0].replacement == 0x010101);
    assert(drawmethod_set_substitute(&method, 1, 0, 2));
    before = method.colors;
    assert(!drawmethod_set_substitute_enabled(&method, 3));
    assert(memcmp(&before, &method.colors, sizeof(before)) == 0);
    assert(drawmethod_set_substitute_enabled(&method, 1));
    assert(drawmethod_substitute_color(&method, 0xff000000) == 0x010101);
    assert(drawmethod_substitute_color(&method, 0xff123456) == 0xff123456);

    for(index = 0; index < DRAWMETHOD_COLOR_SLOTS; ++index)
    {
        assert(drawmethod_set_chroma(&method, index, index * 0x010101));
    }
    before = method.colors;
    assert(!drawmethod_set_chroma(&method, 5, 0));
    assert(!drawmethod_set_chroma(&method, 0, 0x1000000));
    assert(!drawmethod_set_chroma_enabled(&method, 32));
    assert(memcmp(&before, &method.colors, sizeof(before)) == 0);
    assert(drawmethod_chroma_matches(&method, 0xff000000));
    assert(drawmethod_chroma_matches(&method, 0xff040404));
    assert(!drawmethod_chroma_matches(&method, 0x050505));
    assert(drawmethod_set_chroma_enabled(&method, 0));
    assert(!drawmethod_chroma_matches(&method, 0));

    /* Internal callers also receive one replacement, never recursive mapping. */
    method.colors.substitute[1].match = 0x010101;
    method.colors.substitute[1].replacement = 0x020202;
    method.colors.substitute_enabled = 3;
    assert(!drawmethod_substitutes_valid(&method.colors));
    assert(drawmethod_substitute_color(&method, 0) == 0x010101);
    method.config = 0;
    assert(!drawmethod_colors_active(&method));
    method = plainmethod;
    assert(!method.colors.substitute_enabled && !method.colors.chroma_enabled);
}

/*
 * Damon V. Caskey
 * 2026-11-14
 * Verify all five substitutions, exact matching, high-byte handling and reset.
 */
static void colors_test_five_substitutes(void)
{
    const unsigned source_colors[8] = {0, 0xffffff, 0x0000ff, 0x00ff00,
                                      0xff0000, 0xff000000, 1, 0x123456};
    const unsigned changed[8] = {0x010101, 0xfefefe, 0x0101fd, 0x01fd01,
                                0xfd0101, 0x010101, 1, 0x123456};
    s_screen *source = colors_screen(8, 1, PIXEL_32, 0);
    s_screen *target = colors_screen(12, 4, PIXEL_32, COLOR_BACKGROUND);
    s_drawmethod method = plainmethod;
    unsigned expected[COLOR_EXPECTED_PIXELS];
    int index;
    memcpy(source->data, source_colors, sizeof(source_colors));
    colors_expected(expected, target, COLOR_BACKGROUND);
    for(index = 0; index < 5; ++index)
    {
        assert(drawmethod_set_substitute(&method, index, source_colors[index], changed[index]));
    }
    putscreen(target, source, 2, 1, &method);
    for(index = 0; index < 8; ++index) expected[12 + 2 + index] = changed[index];
    assert(drawmethod_set_substitute_enabled(&method, 0));
    putscreen(target, source, 2, 2, &method);
    for(index = 0; index < 8; ++index) expected[24 + 2 + index] = source_colors[index] & 0xffffffu;
    assert(drawmethod_set_substitute_enabled(&method, 31));
    method.config |= DRAWMETHOD_CONFIG_BACKGROUND_TRANSPARENCY;
    putscreen(target, source, 2, 3, &method);
    for(index = 0; index < 8; ++index)
    {
        if(source_colors[index] & 0xffffffu) expected[36 + 2 + index] = changed[index];
    }
    colors_verify(target, expected, "five substitutes and disabled slots");
    colors_release_screen(target);
    colors_release_screen(source);
}

/*
 * Damon V. Caskey
 * 2026-11-14
 * Apply all five source keys without repairing discarded or unlisted pixels.
 */
static void colors_test_five_chroma(void)
{
    const unsigned values[8] = {0, 0xffffff, 0x0000ff, 0x00ff00, 0xff0000,
                                1, 0x123456, 0xff000000};
    s_screen *source = colors_screen(8, 1, PIXEL_32, 0);
    s_screen *target = colors_screen(12, 3, PIXEL_32, COLOR_BACKGROUND);
    s_drawmethod method = plainmethod;
    unsigned expected[COLOR_EXPECTED_PIXELS];
    int index;
    memcpy(source->data, values, sizeof(values));
    colors_expected(expected, target, COLOR_BACKGROUND);
    for(index = 0; index < 5; ++index) assert(drawmethod_set_chroma(&method, index, values[index]));
    assert(drawmethod_set_substitute(&method, 0, 0, 0x010101));
    putscreen(target, source, 2, 1, &method);
    expected[12 + 7] = 1;
    expected[12 + 8] = 0x123456;
    colors_verify(target, expected, "five source keys preserve holes");
    colors_release_screen(target);
    colors_release_screen(source);
}

/*
 * Damon V. Caskey
 * 2026-11-14
 * Palette changes alter keyed coverage; source keying precedes fill effects.
 */
static void colors_test_palette_and_fill(void)
{
    s_screen *source = colors_screen(4, 1, PIXEL_x8, 0);
    s_screen *target = colors_screen(8, 5, PIXEL_32, COLOR_BACKGROUND);
    s_drawmethod method = plainmethod;
    unsigned expected[COLOR_EXPECTED_PIXELS];
    unsigned *palette = (unsigned *)source->palette;
    const unsigned char indices[4] = {0, 1, 2, 3};
    memcpy(source->data, indices, sizeof(indices));
    palette[0] = 0x123456; palette[1] = 0xcc00cc; palette[2] = 0; palette[3] = 0x224466;
    method.config |= DRAWMETHOD_CONFIG_BACKGROUND_TRANSPARENCY;
    assert(drawmethod_set_chroma(&method, 0, 0xcc00cc));
    assert(drawmethod_set_substitute(&method, 0, 0, 0x010101));
    colors_expected(expected, target, COLOR_BACKGROUND);
    putscreen(target, source, 2, 1, &method);
    expected[8 + 4] = 0x010101; expected[8 + 5] = 0x224466;
    palette[1] = 0x557799;
    putscreen(target, source, 2, 2, &method);
    expected[16 + 3] = 0x557799; expected[16 + 4] = 0x010101; expected[16 + 5] = 0x224466;
    palette[1] = 0xcc00cc;
    method.fillcolor = 0x654321;
    putscreen(target, source, 2, 3, &method);
    expected[24 + 4] = expected[24 + 5] = 0x654321;
    colors_verify(target, expected, "palette-resolved coverage before fill");
    colors_release_screen(target);
    colors_release_screen(source);
}

/*
 * Damon V. Caskey
 * 2026-11-14
 * A fill-only material needs indexed coverage but no source RGB palette.
 */
static void colors_test_fill_without_palette(void)
{
    const unsigned char indices[4] = {0, 1, 2, 3};
    s_screen *source = colors_screen(4, 1, PIXEL_x8, 0);
    s_screen *target = colors_screen(8, 3, PIXEL_32, COLOR_BACKGROUND);
    s_drawmethod method = plainmethod;
    unsigned expected[COLOR_EXPECTED_PIXELS];
    memcpy(source->data, indices, sizeof(indices));
    source->palette = NULL;
    method.config |= DRAWMETHOD_CONFIG_BACKGROUND_TRANSPARENCY;
    method.fillcolor = 0x123456;
    assert(drawmethod_set_substitute(&method, 0, 0x123456, 0x654321));
    colors_expected(expected, target, COLOR_BACKGROUND);
    putscreen(target, source, 2, 1, &method);
    expected[8 + 3] = expected[8 + 4] = expected[8 + 5] = 0x654321;
    colors_verify(target, expected, "indexed fill needs coverage but no palette");
    colors_release_screen(target);
    colors_release_screen(source);
}

/*
 * Damon V. Caskey
 * 2026-11-14
 * Tint and destination blending may create a matched result after source keying.
 */
static void colors_test_final_composition(void)
{
    s_screen *source = colors_screen(4, 1, PIXEL_32, 0x010101);
    s_screen *target = colors_screen(8, 4, PIXEL_32, 0x808080);
    s_drawmethod method = plainmethod;
    unsigned expected[COLOR_EXPECTED_PIXELS];
    int index;
    colors_expected(expected, target, 0x808080);
    assert(drawmethod_set_chroma(&method, 0, 0));
    assert(drawmethod_set_substitute(&method, 0, 0, 0x010203));
    method.alpha = 2;
    putscreen(target, source, 2, 1, &method);
    for(index = 2; index < 6; ++index) expected[8 + index] = 0x010203;
    method.alpha = -1;
    method.tintmode = 2;
    method.tintcolor = 0x010101;
    method.channelr = method.channelg = method.channelb = 0;
    putscreen(target, source, 2, 2, &method);
    for(index = 2; index < 6; ++index) expected[16 + index] = 0x010203;
    colors_verify(target, expected, "final byte composition substitutes");
    colors_release_screen(target);
    colors_release_screen(source);
}

/*
 * Damon V. Caskey
 * 2026-11-14
 * Transparent masks and channel opacity must not activate output repair.
 */
static void colors_test_opacity(void)
{
    const unsigned char indices[4] = {1, 2, 3, 4};
    const unsigned char opacity[4] = {0, 64, 128, 255};
    s_sprite *sprite = colors_sprite(indices, 0);
    s_sprite *mask = colors_sprite(opacity, 0);
    s_screen *target = colors_screen(8, 4, PIXEL_32, 0);
    s_screen *source = colors_screen(4, 1, PIXEL_32, 0x808080);
    s_drawmethod method = plainmethod;
    unsigned expected[COLOR_EXPECTED_PIXELS];
    int index;
    sprite->mask = mask;
    colors_expected(expected, target, 0);
    assert(drawmethod_set_substitute(&method, 0, 0, 0x010101));
    putsprite(2, 1, sprite, target, &method);
    expected[8 + 3] = expected[8 + 4] = expected[8 + 5] = 0x010101;

    /* Legacy byte opacity still floors 1*255/256 to zero. The new policy
     * must not turn that zero-opacity result into its nonzero substitute. */
    gpu_prepare_cpu_write(target);
    for(index = 2; index < 6; ++index)
    {
        ((unsigned *)target->data)[16 + index] = 0x010101;
        expected[16 + index] = 0;
    }
    method.alpha = 6;
    method.channelr = method.channelg = method.channelb = 0;
    putscreen(target, source, 2, 2, &method);
    colors_verify(target, expected, "zero opacity excludes output substitution");
    sprite->mask = NULL;
    colors_release_sprite(mask);
    colors_release_sprite(sprite);
    colors_release_screen(target);
    colors_release_screen(source);
}

/*
 * Damon V. Caskey
 * 2026-11-14
 * Real encoded sprite gaps retain destination pixels even when black is mapped.
 */
static void colors_test_sprite_gaps(void)
{
    const unsigned char indices[4] = {1, 0, 2, 0};
    s_sprite *sprite = colors_sprite(indices, 1);
    s_screen *target = colors_screen(8, 3, PIXEL_32, 0);
    s_drawmethod method = plainmethod;
    unsigned expected[COLOR_EXPECTED_PIXELS];
    ((unsigned *)sprite->palette)[1] = 0;
    ((unsigned *)sprite->palette)[2] = 0x112233;
    colors_expected(expected, target, 0);
    assert(drawmethod_set_substitute(&method, 0, 0, 0x010101));
    putsprite(2, 1, sprite, target, &method);
    expected[8 + 2] = 0x010101; expected[8 + 4] = 0x112233;
    colors_verify(target, expected, "RLE gaps remain uncovered");
    colors_release_sprite(sprite);
    colors_release_screen(target);
}

/*
 * Damon V. Caskey
 * 2026-11-14
 * Exercise clipping and repetition with the existing scale and affine paths.
 */
static void colors_test_transforms(void)
{
    const unsigned values[8] = {0, 0xcc00cc, 0x224466, 0xffffff,
                                0xffffff, 0x224466, 0xcc00cc, 0};
    s_screen *source = colors_screen(4, 2, PIXEL_32, 0);
    unsigned expected[COLOR_EXPECTED_PIXELS];
    int affine, repetition, x, y;
    memcpy(source->data, values, sizeof(values));
    for(affine = 0; affine < 2; ++affine)
    {
        s_screen *target = colors_screen(20, 8, PIXEL_32, COLOR_BACKGROUND);
        s_drawmethod method = plainmethod;
        method.scalex = method.scaley = 512;
        method.xrepeat = 2; method.xspan = 7;
        method.clipx = 3; method.clipy = 2; method.clipw = 12; method.cliph = 3;
        if(affine)
        {
            method.config |= DRAWMETHOD_CONFIG_AFFINE;
            method.affine.m00 = method.affine.m11 = 2;
        }
        assert(drawmethod_set_chroma(&method, 0, 0xcc00cc));
        assert(drawmethod_set_substitute(&method, 0, 0, 0x010101));
        assert(drawmethod_set_substitute(&method, 4, 0xffffff, 0xfefefe));
        colors_expected(expected, target, COLOR_BACKGROUND);
        for(repetition = 0; repetition < 2; ++repetition)
        {
            int left = 2 + repetition * 7;
            for(y = 1; y < 5; ++y)
            {
                for(x = left; x < left + 8; ++x)
                {
                    unsigned color = values[((y - 1) / 2) * 4 + (x - left) / 2];
                    if(x < 3 || x >= 15 || y < 2 || y >= 5 || color == 0xcc00cc) continue;
                    if(color == 0) color = 0x010101;
                    else if(color == 0xffffff) color = 0xfefefe;
                    expected[y * 20 + x] = color;
                }
            }
        }
        putscreen(target, source, 2, 1, &method);
        colors_verify(target, expected, affine ? "affine clipped repeated policy" : "legacy clipped repeated policy");
        colors_release_screen(target);
    }
    colors_release_screen(source);
}

/*
 * Damon V. Caskey
 * 2026-11-14
 * Primitive keys compare the supplied color, not the renderer's white texture.
 */
static void colors_test_primitives(void)
{
    s_screen *target = colors_screen(12, 8, PIXEL_32, COLOR_BACKGROUND);
    s_drawmethod method = plainmethod;
    unsigned expected[COLOR_EXPECTED_PIXELS];
    int x, y;
    colors_expected(expected, target, COLOR_BACKGROUND);
    assert(drawmethod_set_chroma(&method, 0, 0xffffff));
    assert(drawmethod_set_substitute(&method, 0, 0, 0x010101));
    putbox(1, 1, 3, 2, 0, target, &method);
    for(y = 1; y < 3; ++y) for(x = 1; x < 4; ++x) expected[y * 12 + x] = 0x010101;
    putbox(5, 1, 3, 2, 0xffffff, target, &method);
    putpixel(9, 1, 0, target, &method); expected[12 + 9] = 0x010101;
    putpixel(10, 1, 0xffffff, target, &method);
    method.clipx = 2; method.clipy = 4; method.clipw = 6; method.cliph = 2;
    putline(0, 4, 10, 4, 0, target, &method);
    for(x = 2; x < 8; ++x) expected[4 * 12 + x] = 0x010101;
    putline(0, 5, 10, 5, 0xffffff, target, &method);
    method.clipw = method.cliph = 0;
    putline(8, 6, 9, 6, 0, target, &method);
    expected[6 * 12 + 8] = expected[6 * 12 + 9] = 0x010101;
    putline(10, 5, 10, 6, 0, target, &method);
    expected[5 * 12 + 10] = expected[6 * 12 + 10] = 0x010101;
    colors_verify(target, expected, "primitive coverage and supplied-color keying");
    colors_release_screen(target);
}

/*
 * Damon V. Caskey
 * 2026-11-14
 * Queue entries retain inline color state while later edits change new draws.
 */
static void colors_test_queue_snapshot(void)
{
    s_screen *source = colors_screen(4, 1, PIXEL_32, 0);
    s_screen *target = colors_screen(16, 3, PIXEL_32, COLOR_BACKGROUND);
    s_drawmethod method = plainmethod;
    unsigned expected[COLOR_EXPECTED_PIXELS];
    int x;
    colors_expected(expected, target, COLOR_BACKGROUND);
    spriteq_unlock(); spriteq_clear();
    assert(drawmethod_set_substitute(&method, 0, 0, 0x010101));
    spriteq_add_screen(0, 1, 10, source, &method, 0);
    assert(drawmethod_set_substitute(&method, 0, 0, 0x020202));
    spriteq_add_screen(4, 1, 10, source, &method, 0);
    assert(drawmethod_set_chroma(&method, 0, 0));
    spriteq_add_screen(8, 1, 10, source, &method, 0);
    method = plainmethod;
    spriteq_add_screen(12, 1, 10, source, &method, 0);
    spriteq_draw(target, 0, 10, 10, 0, 0);
    for(x = 0; x < 4; ++x) expected[16 + x] = 0x010101;
    for(x = 4; x < 8; ++x) expected[16 + x] = 0x020202;
    for(x = 12; x < 16; ++x) expected[16 + x] = 0;
    colors_verify(target, expected, "queued color settings are immutable");
    spriteq_clear();
    colors_release_screen(target);
    colors_release_screen(source);
}

/*
 * Damon V. Caskey
 * 2026-11-14
 * Each overlapping self-draw samples the completed previous repeat instance.
 */
static void colors_test_self_draw(void)
{
    /* Odd widths catch accidental alignment changes in the source snapshot. */
    s_screen *target = colors_screen(11, 1, PIXEL_32, 0);
    s_drawmethod method = plainmethod;
    unsigned expected[COLOR_EXPECTED_PIXELS], previous[11];
    int x, repetition;
    for(x = 0; x < 11; ++x)
    {
        expected[x] = (unsigned)x * 0x010101;
        ((unsigned *)target->data)[x] = expected[x];
    }
    assert(drawmethod_set_substitute(&method, 0, 0, 0x101010));
    assert(drawmethod_set_chroma(&method, 0, 0x020202));
    method.xrepeat = 2; method.xspan = 1;
    for(repetition = 0; repetition < 2; ++repetition)
    {
        int left = 1 + repetition;
        memcpy(previous, expected, sizeof(previous));
        for(x = left; x < 11; ++x)
        {
            unsigned color = previous[x - left];
            if(color != 0x020202) expected[x] = color == 0 ? 0x101010 : color;
        }
    }
    putscreen(target, target, 1, 0, &method);
    colors_verify(target, expected, "self-draw snapshots preserve repeat order");
    colors_release_screen(target);
}

/*
 * Damon V. Caskey
 * 2026-11-14
 * Run the same complete-image assertions on software and real native drawing.
 */
static void colors_run_suite(void)
{
    colors_test_five_substitutes();
    colors_test_five_chroma();
    colors_test_palette_and_fill();
    colors_test_fill_without_palette();
    colors_test_final_composition();
    colors_test_opacity();
    colors_test_sprite_gaps();
    colors_test_transforms();
    colors_test_primitives();
    colors_test_queue_snapshot();
    colors_test_self_draw();
}

#ifndef COLOR_GPU_TEST
/*
 * Damon V. Caskey
 * 2026-11-14
 * Unsupported output formats must decline rather than drop active rules.
 */
static void colors_test_format_rejection(void)
{
    s_screen *source = colors_screen(4, 1, PIXEL_16, 0);
    s_screen *target = colors_screen(8, 2, PIXEL_16, 0);
    s_drawmethod method = plainmethod;
    unsigned char before[32];
    memset(target->data, 0x5a, sizeof(before));
    memcpy(before, target->data, sizeof(before));
    assert(drawmethod_set_substitute(&method, 0, 0, 0x010101));
    putscreen(target, source, 2, 1, &method);
    assert(memcmp(before, target->data, sizeof(before)) == 0);
    colors_release_screen(target);
    colors_release_screen(source);
}

/*
 * Damon V. Caskey
 * 2026-11-14
 * Chroma needs resolved RGB even when a fill would replace visible colors.
 */
static void colors_test_missing_palette_rejection(void)
{
    s_screen *source = colors_screen(4, 1, PIXEL_x8, 0);
    s_screen *target = colors_screen(8, 3, PIXEL_32, COLOR_BACKGROUND);
    s_drawmethod method = plainmethod;
    unsigned expected[COLOR_EXPECTED_PIXELS];
    source->palette = NULL;
    memset(source->data, 1, 4);
    method.fillcolor = 0x123456;
    assert(drawmethod_set_chroma(&method, 0, 0xcc00cc));
    colors_expected(expected, target, COLOR_BACKGROUND);
    putscreen(target, source, 2, 1, &method);
    colors_verify(target, expected, "missing palette is an explicit failure before writes");
    colors_release_screen(target);
    colors_release_screen(source);
}

/*
 * Damon V. Caskey
 * 2026-11-14
 * Execute validation and CPU pixel coverage without requiring a GL context.
 */
int main(void)
{
    assert(sizeof(void *) == 8);
    pixelformat = PIXEL_32;
    colors_test_setters();
    colors_run_suite();
    colors_test_format_rejection();
    colors_test_missing_palette_rejection();
    printf("Drawmethod color tests passed: validation and %u complete software images.\n", color_images_checked);
    return 0;
}
#endif
