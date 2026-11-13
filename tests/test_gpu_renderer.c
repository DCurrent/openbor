/*
 * Native renderer regression tests using a real SDL/OpenGL context.
 *
 * Build from the repository root with SDL2 development files installed:
 * cc -std=c99 -O2 -Wall -Werror -D_DEFAULT_SOURCE -DPP_TEST -DSDL=1 \
 *   -DLINUX=1 -DOPENGL=1 -DLOADGL=1 -ffunction-sections -fdata-sections \
 *   $(sdl2-config --cflags) -Iengine -Iengine/sdl -Iengine/source \
 *   -Iengine/source/gamelib -Iengine/source/gfxlib \
 *   tests/test_gpu_renderer.c engine/source/gamelib/gpu.c \
 *   engine/source/gamelib/gpu_shader.c engine/source/gamelib/gpu_transform.c \
 *   engine/sdl/gpu_renderer.c engine/sdl/loadgl.c \
 *   -Wl,--gc-sections $(sdl2-config --libs) -lm -o test_gpu_renderer
 * SDL_VIDEODRIVER=offscreen ./test_gpu_renderer
 *
 * Exit status 77 means the required SDL/OpenGL context was unavailable.
 */

#include <assert.h>
#include <float.h>
#include <limits.h>
#include <math.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <SDL.h>
#include "gpu.h"
#include "gpu_renderer.h"
#include "gpu_shader.h"
#include "loadgl.h"

/* Exercise the production queue's filtering, offsets, locking and sort. */
#include "../engine/source/gamelib/spriteq.c"

int pixelbytes[5] = {1, 1, 2, 0, 4};
int pixelformat = PIXEL_32;
int useclip, clipx1, clipx2, clipy1, clipy2;
unsigned char *blendtables[MAX_BLENDINGS];
const s_drawmethod plainmethod = {
    .config = DRAWMETHOD_CONFIG_ENABLED,
    .alpha = -1,
    .remap = -1,
    .scalex = 256, .scaley = 256,
    .xrepeat = 1, .yrepeat = 1,
    .channelr = 255, .channelg = 255, .channelb = 255,
    .affine = {1, 0, 0, 1, 0, 0},
    .object_type = OBJECT_TYPE_DRAWMETHOD
};

/*
 * Damon V. Caskey
 * 2026-11-12
 * Retain the engine allocation contract in this isolated test executable.
 */
void *checkAlloc(void *pointer, size_t size, const char *function, const char *file, int line)
{
    /* Preserve fatal allocation checks while keeping this fixture independent
     * of the engine's logging subsystem. */
    (void)size;
    (void)function;
    (void)file;
    (void)line;
    assert(pointer);
    return pointer;
}

/*
 * Damon V. Caskey
 * 2026-11-12
 * Route queue dots through the native backend; no software fallback is allowed
 * in this test, so a successful image proves GPU command coverage.
 */
void putpixel(unsigned x, unsigned y, int color, s_screen *screen, s_drawmethod *method)
{
    /* Dots share the rectangle path, with exactly one destination pixel. */
    assert(gpu_draw_box(screen, (int)x, (int)y, 1, 1, (unsigned)color, method));
}

/*
 * Damon V. Caskey
 * 2026-11-12
 * Route queued sprite records through the real native renderer.
 */
void putsprite(int x, int y, s_sprite *sprite, s_screen *screen, s_drawmethod *method)
{
    /* Require native acceptance so software rendering cannot mask a missing
     * sprite feature when queue fixtures are replayed. */
    assert(gpu_draw_sprite(screen, sprite, x, y, method));
}

/*
 * Damon V. Caskey
 * 2026-11-12
 * Route queued screens through the real native renderer without CPU copies.
 */
void putscreen(s_screen *dest, s_screen *src, int x, int y, s_drawmethod *method)
{
    /* The source remains a screen handle, preserving render-target sampling
     * and replay-time contents through the same API used by engine drawing. */
    assert(gpu_draw_screen(dest, src, x, y, method));
}

/*
 * Damon V. Caskey
 * 2026-11-12
 * Route queued line records through the native renderer when a test uses one.
 */
void putline(int sx, int sy, int ex, int ey, int color, s_screen *screen, s_drawmethod *method)
{
    /* Pass both endpoint coordinates unchanged to retain line coverage order. */
    assert(gpu_draw_line(screen, sx, sy, ex, ey, (unsigned)color, method));
}

/*
 * Damon V. Caskey
 * 2026-11-12
 * Route queued rectangles through the real native renderer.
 */
void putbox(int x, int y, int width, int height, int color, s_screen *screen, s_drawmethod *method)
{
    /* Queue extents describe a filled rectangle; native rejection fails the
     * fixture before a fallback could satisfy the expected pixel assertions. */
    assert(gpu_draw_box(screen, x, y, width, height, (unsigned)color, method));
}

/*
 * Damon V. Caskey
 * 2026-11-12
 * Allocate deterministic test images using the production screen memory
 * layout, including a mutable 32-bit palette for indexed sources.
 */
static s_screen *test_screen(int width, int height, int format)
{
    size_t bytes = (size_t)width * height * pixelbytes[format];
    s_screen *screen = calloc(1, sizeof(*screen) + bytes + PAL_BYTES);
    /* Match the production header and inline pixel layout. Zeroed storage
     * gives every clipping/capture test a deterministic untouched border. */
    assert(screen);
    screen->magic = screen_magic;
    screen->width = width;
    screen->height = height;
    screen->pixelformat = format;
    if(format == PIXEL_x8)
    {
        screen->palette = (unsigned char *)screen + offsetof(s_screen, data) + bytes;
    }
    return screen;
}

/*
 * Damon V. Caskey
 * 2026-11-12
 * Release renderer ownership before freeing the test screen's CPU allocation.
 */
static void release_test_screen(s_screen *screen)
{
    /* Evict the identity before the allocator can reuse its CPU address. */
    gpu_release_screen(screen);
    free(screen);
}

/*
 * Damon V. Caskey
 * 2026-11-12
 * Compare RGB values without interpreting the unused fourth screen byte.
 * Callers must explicitly synchronize before reading native screen pixels.
 */
static unsigned test_pixel(const s_screen *screen, int x, int y)
{
    /* Read the caller-synchronized shadow and compare the engine's RGB word. */
    return ((const unsigned *)screen->data)[y * screen->width + x] & 0x00ffffffu;
}

/*
 * Damon V. Caskey
 * 2026-11-12
 * Capture selected Z layers with offsets, sample that capture into a second
 * GPU screen, and verify stable ties without a CPU readback between targets.
 */
static void test_queue_capture(void)
{
    s_screen *capture = test_screen(8, 8, PIXEL_32);
    s_screen *output = test_screen(8, 8, PIXEL_32);
    unsigned long readbacks = gpu_gl_get_stats()->readbacks;
    unsigned texture;
    int width, height, x, y;

    /* Equal-Z boxes establish insertion-order coverage. The higher layer is
     * excluded, then the selected capture becomes the next target's source. */
    spriteq_unlock();
    spriteq_clear();
    assert(gpu_clear_screen(capture));
    assert(gpu_clear_screen(output));
    spriteq_add_box(0, 0, 4, 4, 10, 0x000000ff, NULL);
    spriteq_add_box(1, 1, 2, 2, 10, 0x0000ff00, NULL);
    spriteq_add_box(0, 0, 8, 8, 30, 0x00ff0000, NULL);
    spriteq_draw(capture, 0, 10, 10, 1, 1);
    spriteq_add_screen(0, 0, 20, capture, NULL, 0);
    spriteq_draw(output, 0, 20, 20, 0, 0);
    assert(gpu_gl_get_screen_texture(output, &texture, &width, &height));
    assert(texture && width == 8 && height == 8);
    assert(gpu_gl_get_stats()->readbacks == readbacks);

    /* Both passes must stay GPU-resident; download only the final image for
     * assertions, then reconstruct the expected painter-order overlap. */
    gpu_sync_cpu(output);
    assert(gpu_gl_get_stats()->readbacks == readbacks + 1);
    for(y = 0; y < 8; ++y)
    {
        for(x = 0; x < 8; ++x)
        {
            unsigned expected = 0;
            if(x >= 1 && x < 5 && y >= 1 && y < 5)
            {
                expected = 0x000000ff;
            }
            if(x >= 2 && x < 4 && y >= 2 && y < 4)
            {
                expected = 0x0000ff00;
            }
            assert(test_pixel(output, x, y) == expected);
        }
    }
    spriteq_clear();
    release_test_screen(output);
    release_test_screen(capture);
}

/*
 * Damon V. Caskey
 * 2026-11-12
 * Confirm that captured queues still honor locked-prefix exclusion.
 */
static void test_queue_newonly(void)
{
    s_screen *screen = test_screen(8, 8, PIXEL_32);

    /* Lock after the background entry. newonly must omit that saved prefix
     * while retaining the later box, even though both layers pass the filter. */
    spriteq_clear();
    assert(gpu_clear_screen(screen));
    spriteq_add_box(0, 0, 8, 8, 1, 0x000000ff, NULL);
    spriteq_lock();
    spriteq_add_box(3, 3, 2, 2, 2, 0x0000ff00, NULL);
    spriteq_draw(screen, 1, 0, 100, 0, 0);
    gpu_sync_cpu(screen);
    assert(test_pixel(screen, 0, 0) == 0);
    assert(test_pixel(screen, 3, 3) == 0x0000ff00);
    /* Remove queued references before retiring their destination resource. */
    spriteq_unlock();
    spriteq_clear();
    release_test_screen(screen);
}

/*
 * Damon V. Caskey
 * 2026-11-12
 * Verify CPU edits are uploaded on demand and overlapping source/target draws
 * use a stable GPU snapshot rather than undefined texture feedback.
 */
static void test_coherence_and_feedback(void)
{
    s_screen *source = test_screen(8, 8, PIXEL_32);
    s_screen *target = test_screen(8, 8, PIXEL_32);
    unsigned previous[64];
    unsigned long readbacks;
    int x, y;

    /* Encode position into each pixel so stale uploads and self-copy feedback
     * produce different, readily identifiable image values. */
    for(y = 0; y < 8; ++y)
    {
        for(x = 0; x < 8; ++x)
        {
            ((unsigned *)source->data)[y * 8 + x] = 1u + (unsigned)x + ((unsigned)y << 8);
        }
    }
    assert(gpu_copy_screen(target, source));
    gpu_prepare_cpu_write(source);
    ((unsigned *)source->data)[9] = 0x00abcdef;
    assert(gpu_draw_screen(target, source, 0, 0, &plainmethod));
    gpu_sync_cpu(target);
    assert(test_pixel(target, 1, 1) == 0x00abcdef);
    /* Shift the target over itself. Every destination pixel must sample the
     * completed pre-draw image rather than a pixel already overwritten. */
    memcpy(previous, target->data, sizeof(previous));
    readbacks = gpu_gl_get_stats()->readbacks;
    assert(gpu_draw_screen(target, target, 1, 0, &plainmethod));
    assert(gpu_gl_get_stats()->readbacks == readbacks);
    gpu_sync_cpu(target);
    for(y = 0; y < 8; ++y)
    {
        for(x = 0; x < 8; ++x)
        {
            assert(test_pixel(target, x, y) == (previous[y * 8 + (x ? x - 1 : 0)] & 0x00ffffffu));
        }
    }
    release_test_screen(target);
    release_test_screen(source);
}

/*
 * Damon V. Caskey
 * 2026-11-12
 * Keep palette selection mutable without re-encoding or expanding indexed
 * source images, and preserve zero-index transparency before lookup.
 */
static void test_indexed_palette(void)
{
    s_screen *source = test_screen(4, 4, PIXEL_x8);
    s_screen *target = test_screen(4, 4, PIXEL_32);
    s_drawmethod method = plainmethod;
    unsigned *palette = (unsigned *)source->palette;

    /* Index zero stays transparent even when its palette entry is white;
     * transparency is decided from coverage before color lookup. */
    memset(source->data, 1, 16);
    source->data[0] = 0;
    palette[0] = 0x00ffffff;
    palette[1] = 0x000000ff;
    method.config |= DRAWMETHOD_CONFIG_BACKGROUND_TRANSPARENCY;
    assert(gpu_clear_screen(target));
    assert(gpu_draw_screen(target, source, 0, 0, &method));
    gpu_sync_cpu(target);
    assert(test_pixel(target, 0, 0) == 0);
    assert(test_pixel(target, 1, 0) == 0x000000ff);
    /* Edit the same palette allocation to catch pointer-only cache checks. */
    palette[1] = 0x0000ff00;
    assert(gpu_draw_screen(target, source, 0, 0, &method));
    gpu_sync_cpu(target);
    assert(test_pixel(target, 1, 0) == 0x0000ff00);
    release_test_screen(target);
    release_test_screen(source);
}

/*
 * Damon V. Caskey
 * 2026-11-12
 * Test all built-in alpha equations against integer reference values on real
 * GL fragment execution, not just independently compiled CPU helpers.
 */
static void test_blend_modes(void)
{
    s_screen *source = test_screen(4, 4, PIXEL_32);
    s_screen *target = test_screen(4, 4, PIXEL_32);
    s_drawmethod method = plainmethod;
    unsigned src = 0x00b35721, dst = 0x003b97cd;
    int mode, pixel, shift;

    /* Hold source and destination channels fixed across modes so each result
     * can be rebuilt from the canonical byte-domain equation. */
    for(pixel = 0; pixel < 16; ++pixel)
    {
        ((unsigned *)source->data)[pixel] = src;
    }
    for(mode = 1; mode <= 6; ++mode)
    {
        unsigned expected = 0;
        gpu_prepare_cpu_write(target);
        for(pixel = 0; pixel < 16; ++pixel)
        {
            ((unsigned *)target->data)[pixel] = dst;
        }
        method.alpha = mode;
        assert(gpu_draw_screen(target, source, 0, 0, &method));
        gpu_sync_cpu(target);
        for(shift = 0; shift < 24; shift += 8)
        {
            expected |= gpu_shader_blend_reference(mode, src >> shift, dst >> shift) << shift;
        }
        assert(test_pixel(target, 2, 2) == expected);
    }
    release_test_screen(target);
    release_test_screen(source);
}

/*
 * Damon V. Caskey
 * 2026-11-12
 * Construct one valid four-pixel RLE row using the engine's relative line
 * offsets, for sprite/mask decoding tests without external image assets.
 */
static s_sprite *test_sprite(const unsigned char row[4])
{
    s_sprite *sprite = calloc(1, sizeof(*sprite) + 64 + PAL_BYTES);
    unsigned char *encoded;
    /* Reserve separate encoded-row and palette areas in one stable fixture
     * allocation, mirroring the engine's relative-offset sprite format. */
    assert(sprite);
    sprite->magic = sprite_magic;
    sprite->width = sprite->srcwidth = 4;
    sprite->height = sprite->srcheight = 1;
    sprite->pixelformat = PIXEL_x8;
    sprite->palette = (unsigned char *)sprite + sizeof(*sprite) + 64;
    /* The line offset skips its own table entry. The run then contains zero
     * skipped pixels, four visible bytes, and the end-of-line marker. */
    sprite->data[0] = sizeof(int);
    encoded = (unsigned char *)sprite + offsetof(s_sprite, data) + sizeof(int);
    encoded[0] = 0;
    encoded[1] = 4;
    memcpy(encoded + 2, row, 4);
    encoded[6] = 0xff;
    return sprite;
}

/*
 * Damon V. Caskey
 * 2026-11-12
 * Verify native RLE sprites, alpha-mask endpoints and mask precedence over
 * tint, ordinary alpha and per-channel settings using real shader execution.
 */
static void test_sprite_mask(void)
{
    const unsigned char indices[4] = {1, 1, 1, 1};
    const unsigned char opacities[4] = {255, 128, 0, 64};
    s_sprite *sprite = test_sprite(indices);
    s_sprite *mask = test_sprite(opacities);
    s_screen *target = test_screen(4, 4, PIXEL_32);
    s_drawmethod method = plainmethod;
    const unsigned source = 0x00e0a060, destination = 0x00204080;
    int x, shift;

    /* Combine conflicting tint/channel settings with a mask so the expected
     * image demonstrates that mask opacity takes precedence. */
    ((unsigned *)sprite->palette)[1] = source;
    sprite->mask = mask;
    method.alpha = 6;
    method.tintmode = 1;
    method.tintcolor = 0x00ffffff;
    method.channelr = method.channelg = method.channelb = 0;
    assert(gpu_draw_box(target, 0, 0, 4, 4, destination, &plainmethod));
    assert(gpu_draw_sprite(target, sprite, 0, 0, &method));
    gpu_sync_cpu(target);
    /* Check each endpoint/intermediate opacity independently per RGB channel. */
    for(x = 0; x < 4; ++x)
    {
        unsigned expected = 0;
        for(shift = 0; shift < 24; shift += 8)
        {
            expected |= gpu_shader_channel_reference(source >> shift, destination >> shift, opacities[x]) << shift;
        }
        assert(test_pixel(target, x, 0) == expected);
    }
    gpu_release_sprite(mask);
    gpu_release_sprite(sprite);
    free(mask);
    free(sprite);
    release_test_screen(target);
}

/*
 * Damon V. Caskey
 * 2026-11-13
 *
 * Exercise affine source selection before palette, fill, tint and compositing
 * while changing material state without re-uploading the indexed image.
 */
static void test_affine_palette_materials(void)
{
    s_screen *source = test_screen(4, 4, PIXEL_x8);
    s_screen *target = test_screen(8, 8, PIXEL_32);
    unsigned *palette = (unsigned *)source->palette;
    unsigned remap[256] = {0};
    const unsigned background = 0x00317bcd;
    unsigned long uploads = 0;
    int material, x, y, shift;

    /* Index zero is uncovered even when its palette is white. Another index
     * deliberately resolves to visible black, so coverage cannot use RGB. */
    palette[0] = 0x00ffffff;
    palette[1] = 0;
    palette[2] = 0x00d98b43;
    palette[3] = 0x0012a6e8;
    remap[0] = 0x00ffffff;
    remap[1] = 0x0042a135;
    remap[2] = 0x006c2be7;
    remap[3] = 0x00e87329;
    for(y = 0; y < 4; ++y)
        for(x = 0; x < 4; ++x)
            source->data[y * 4 + x] = (unsigned char)((x + y) % 4);

    /* The quarter-turn plus 2x scale has an exact hand-calculated mapping:
     * source x = destination y / 2, source y = 3 - destination x / 2. */
    for(material = 0; material < 11; ++material)
    {
        s_drawmethod method = plainmethod;
        const unsigned *active_palette = palette;
        method.config |= DRAWMETHOD_CONFIG_AFFINE | DRAWMETHOD_CONFIG_BACKGROUND_TRANSPARENCY;
        method.affine = (affine_transform){0, -2, 2, 0, 8, 0};
        if(material == 1)
        {
            method.table = (unsigned char *)remap;
            active_palette = remap;
        }
        if(material == 2) method.fillcolor = 0x00ab731f;
        if(material == 3)
        {
            method.tintmode = 2;
            method.tintcolor = 0x0092c7ef;
            method.alpha = 1;
        }
        if(material == 4)
        {
            method.alpha = 6;
            method.channelr = 17; method.channelg = 128; method.channelb = 251;
        }
        if(material >= 5) method.alpha = material - 4;
        if(material == 6) palette[2] = 0x00a1e549;
        assert(gpu_draw_box(target, 0, 0, 8, 8, background, &plainmethod));
        assert(gpu_draw_screen(target, source, 0, 0, &method));
        /* Palette updates upload atlas rows, while the unchanged index image
         * must retain its original texture across every material change. */
        if(material == 0)
            uploads = gpu_gl_get_stats()->uploads - gpu_gl_get_stats()->palette_uploads;
        assert(gpu_gl_get_stats()->uploads - gpu_gl_get_stats()->palette_uploads == uploads);
        gpu_sync_cpu(target);

        /* Reconstruct material results from integer color equations after
         * the independently known geometric mapping and source-index test. */
        for(y = 0; y < 8; ++y)
        {
            for(x = 0; x < 8; ++x)
            {
                unsigned index = source->data[(3 - x / 2) * 4 + y / 2];
                unsigned expected = background;
                if(index)
                {
                    unsigned color = method.fillcolor ? method.fillcolor : active_palette[index];
                    const unsigned opacity[3] = {method.channelr, method.channelg, method.channelb};
                    expected = 0;
                    for(shift = 0; shift < 24; shift += 8)
                    {
                        unsigned channel = color >> shift;
                        if(method.tintmode)
                        {
                            channel = gpu_shader_blend_reference((int)method.tintmode,
                                      method.tintcolor >> shift, channel);
                        }
                        if(material == 4)
                        {
                            channel = gpu_shader_channel_reference(channel, background >> shift,
                                                                   opacity[shift / 8]);
                        }
                        else if(method.alpha > 0)
                        {
                            channel = gpu_shader_blend_reference(method.alpha, channel, background >> shift);
                        }
                        expected |= (channel & 255u) << shift;
                    }
                }
                assert(test_pixel(target, x, y) == expected);
            }
        }
    }
    release_test_screen(target);
    release_test_screen(source);
}

/*
 * Damon V. Caskey
 * 2026-11-13
 *
 * Affine RLE sprites retain pivots, palette remaps, repeated placements,
 * explicit clipping and mask precedence using the transformed source texel.
 */
static void test_affine_sprite_mask(void)
{
    const unsigned char indices[4] = {1, 2, 3, 0};
    const unsigned char opacities[4] = {255, 128, 0, 64};
    const unsigned background = 0x0032579b;
    unsigned remap[256] = {0};
    s_sprite *sprite = test_sprite(indices);
    s_sprite *mask = test_sprite(opacities);
    s_screen *target = test_screen(8, 10, PIXEL_32);
    s_drawmethod method = plainmethod;
    int x, y, shift;

    /* Both source and drawmethod pivots contribute before the matrix. Two
     * repeated strips land at x=5..6 and x=1..2; clipping retains only x=2,5,6. */
    remap[0] = 0x00ffffff;
    remap[1] = 0;
    remap[2] = 0x00e0a060;
    remap[3] = 0x0017b9da;
    sprite->centerx = 1;
    sprite->mask = mask;
    method.config |= DRAWMETHOD_CONFIG_AFFINE;
    method.affine = (affine_transform){0, -2, 2, 0, 1, 2};
    method.centerx = 1;
    method.xrepeat = 2; method.xspan = -4;
    method.clipx = 2; method.clipy = 1; method.clipw = 5; method.cliph = 6;
    method.table = (unsigned char *)remap;
    method.alpha = 6; method.tintmode = 1; method.tintcolor = 0x00ffffff;
    method.channelr = method.channelg = method.channelb = 0;
    assert(gpu_draw_box(target, 0, 0, 8, 10, background, &plainmethod));
    assert(gpu_draw_sprite(target, sprite, 6, 2, &method));
    gpu_sync_cpu(target);
    for(y = 0; y < 10; ++y)
    {
        for(x = 0; x < 8; ++x)
        {
            unsigned expected = background;
            if(y >= 1 && y < 7 && (x == 2 || x == 5 || x == 6) && indices[y / 2])
            {
                unsigned color = remap[indices[y / 2]];
                expected = 0;
                for(shift = 0; shift < 24; shift += 8)
                {
                    expected |= gpu_shader_channel_reference(color >> shift, background >> shift,
                                                             opacities[y / 2]) << shift;
                }
            }
            assert(test_pixel(target, x, y) == expected);
        }
    }
    gpu_release_sprite(mask); gpu_release_sprite(sprite);
    free(mask); free(sprite);
    release_test_screen(target);
}

/*
 * Damon V. Caskey
 * 2026-11-13
 *
 * Queue copies preserve affine settings while replay samples current screen
 * contents. Overlapping repeats must blend sequentially and captures stay GPU
 * resident until the final assertion requests a readback.
 */
static void test_affine_queue_replay(void)
{
    s_screen *source = test_screen(2, 2, PIXEL_32);
    s_screen *capture = test_screen(10, 5, PIXEL_32);
    s_screen *output = test_screen(10, 5, PIXEL_32);
    const unsigned colors[2] = {0x00c1a539, 0x002f87e1};
    const unsigned background = 0x00112335;
    s_drawmethod method = plainmethod;
    int replay, x, y, pixel, shift;

    method.config |= DRAWMETHOD_CONFIG_AFFINE;
    method.affine = (affine_transform){2, 0, 0, 1, 1, 0};
    method.alpha = 6;
    method.xrepeat = 2; method.xspan = 2;
    spriteq_unlock(); spriteq_clear();
    spriteq_add_screen(0, 1, 7, source, &method, 0);
    method.affine.tx = 1000;
    method.config &= ~DRAWMETHOD_CONFIG_AFFINE;

    /* Mutating the caller's method after enqueue must not affect its copy.
     * Mutating source pixels before the next replay must affect that replay. */
    for(replay = 0; replay < 2; ++replay)
    {
        unsigned once = 0, twice = 0;
        unsigned long readbacks;
        gpu_prepare_cpu_write(source);
        for(pixel = 0; pixel < 4; ++pixel) ((unsigned *)source->data)[pixel] = colors[replay];
        for(shift = 0; shift < 24; shift += 8)
        {
            unsigned first = gpu_shader_blend_reference(6, colors[replay] >> shift, background >> shift);
            once |= first << shift;
            twice |= gpu_shader_blend_reference(6, colors[replay] >> shift, first) << shift;
        }
        readbacks = gpu_gl_get_stats()->readbacks;
        assert(gpu_draw_box(capture, 0, 0, 10, 5, background, &plainmethod));
        spriteq_draw(capture, 0, 7, 7, 1, 0);
        assert(gpu_draw_screen(output, capture, 0, 0, &plainmethod));
        assert(gpu_gl_get_stats()->readbacks == readbacks);
        gpu_sync_cpu(output);
        for(y = 0; y < 5; ++y)
        {
            for(x = 0; x < 10; ++x)
            {
                unsigned expected = background;
                if(y >= 1 && y < 3 && x >= 2 && x < 8)
                    expected = x >= 4 && x < 6 ? twice : once;
                assert(test_pixel(output, x, y) == expected);
            }
        }
    }
    spriteq_clear();
    release_test_screen(output); release_test_screen(capture); release_test_screen(source);
}

/*
 * Damon V. Caskey
 * 2026-11-13
 *
 * Unsupported affine inputs must leave the entire destination available for
 * software fallback. Valid collapsed or offscreen maps are successful no-ops.
 */
static void test_affine_rejection_atomicity(void)
{
    s_screen *source = test_screen(2, 2, PIXEL_32);
    s_screen *target = test_screen(8, 8, PIXEL_32);
    const unsigned background = 0x0013579b;
    int scenario, pixel;

    for(pixel = 0; pixel < 4; ++pixel)
        ((unsigned *)source->data)[pixel] = 0x00e8c6a4;

    /* Every nonfinite coefficient is rejected even when other instances would
     * be on-screen. Overflowing final spans likewise reject before the first
     * ordinary, visible instance has a chance to blend into the destination. */
    for(scenario = 0; scenario < 19; ++scenario)
    {
        s_drawmethod method = plainmethod;
        float *coefficients[] = {&method.affine.m00, &method.affine.m01,
                                 &method.affine.m10, &method.affine.m11,
                                 &method.affine.tx, &method.affine.ty};
        unsigned long draws, fallbacks;
        int handled = scenario == 13 || scenario == 14;
        method.config |= DRAWMETHOD_CONFIG_AFFINE;
        method.affine = (affine_transform){1, 0, 0, 1, 0, 0};
        method.alpha = 6;
        method.xrepeat = 2; method.xspan = 3;
        if(scenario < 12)
        {
            *coefficients[scenario / 2] = scenario & 1 ? INFINITY : NAN;
        }
        else if(scenario == 12)
        {
            method.xrepeat = 3;
            method.xspan = INT_MAX;
        }
        else if(scenario == 13)
        {
            method.affine = (affine_transform){1, 1, 1, 1, 0, 0};
        }
        else if(scenario == 14)
        {
            method.affine.tx = FLT_MAX;
            method.affine.ty = -FLT_MAX;
        }
        else if(scenario == 15)
        {
            method.affine.m00 = method.affine.m11 = FLT_MAX;
        }
        else if(scenario == 16)
        {
            method.affine.m00 = nextafterf(0.0f, 1.0f);
        }
        else if(scenario == 17)
        {
            /* This finite shear exceeds the sampler's fractional precision.
             * Reject it rather than discarding its nonzero encoding remainder. */
            method.affine.m01 = 0x1p-60f;
        }
        else
        {
            /* The first wide instance is visibly supported. The later one
             * still spans the target but exceeds exact signed-offset limits,
             * so preflight must reject both before either one is composited. */
            method.affine.m00 = 1000000.0f;
            method.xspan = -70000;
            method.xrepeat = 1;
            assert(gpu_draw_screen(target, source, 0, 0, &method));
            gpu_sync_cpu(target);
            assert(test_pixel(target, 0, 0) != background);
            method.xrepeat = 2;
        }
        assert(gpu_draw_box(target, 0, 0, 8, 8, background, &plainmethod));
        draws = gpu_gl_get_stats()->draws;
        fallbacks = gpu_gl_get_stats()->fallback_draws;
        assert(gpu_draw_screen(target, source, 0, 0, &method) == handled);
        assert(gpu_gl_get_stats()->draws == draws);
        assert(gpu_gl_get_stats()->fallback_draws == fallbacks + !handled);
        gpu_sync_cpu(target);
        for(pixel = 0; pixel < 64; ++pixel)
            assert(test_pixel(target, pixel % 8, pixel / 8) == background);
    }
    release_test_screen(target); release_test_screen(source);
}

/*
 * Damon V. Caskey
 * 2026-11-12
 * Verify uploaded blend-table lookup orientation and same-pointer table edits.
 * Custom tables are explicitly invalidated by the engine after replacement.
 */
static void test_custom_blend_table(void)
{
    s_screen *source = test_screen(4, 4, PIXEL_32);
    s_screen *target = test_screen(4, 4, PIXEL_32);
    s_drawmethod method = plainmethod;
    const unsigned source_color = 0x00b35721, destination_color = 0x003b97cd;
    unsigned char *table = malloc(256 * 256);
    unsigned expected = 0;
    int s, d, x, shift;

    /* An asymmetric lookup distinguishes source rows from destination columns
     * and catches transposed texture coordinates in the shader's table atlas. */
    for(s = 0; s < 256; ++s)
    {
        for(d = 0; d < 256; ++d)
        {
            table[(s << 8) | d] = (unsigned char)(s + 3 * d);
        }
    }
    for(x = 0; x < 16; ++x)
    {
        ((unsigned *)source->data)[x] = source_color;
    }
    blendtables[0] = table;
    gpu_invalidate_tables();
    method.alpha = 1;
    assert(gpu_draw_box(target, 0, 0, 4, 4, destination_color, &plainmethod));
    assert(gpu_draw_screen(target, source, 0, 0, &method));
    gpu_sync_cpu(target);
    for(shift = 0; shift < 24; shift += 8)
    {
        expected |= (unsigned)table[(((source_color >> shift) & 255) << 8) |
                                    ((destination_color >> shift) & 255)] << shift;
    }
    assert(test_pixel(target, 1, 1) == expected);
    /* Reuse the table address with new contents; explicit invalidation must
     * replace the uploaded values even when pointer identity is unchanged. */
    memset(table, 77, 256 * 256);
    gpu_invalidate_tables();
    assert(gpu_draw_screen(target, source, 0, 0, &method));
    gpu_sync_cpu(target);
    assert(test_pixel(target, 1, 1) == 0x004d4d4d);
    blendtables[0] = NULL;
    gpu_invalidate_tables();
    free(table);
    release_test_screen(target);
    release_test_screen(source);
}

/*
 * Damon V. Caskey
 * 2026-11-12
 * Verify clean software fallback and that renderer shutdown preserves live
 * GPU screens before their graphics context is destroyed.
 */
static void test_shutdown(void)
{
    s_screen *native = test_screen(4, 4, PIXEL_32);
    s_screen *legacy = test_screen(4, 4, PIXEL_16);

    /* Unsupported target formats retain software ownership. The RGB target
     * stays GPU-owned until shutdown transfers its latest version back. */
    assert(!gpu_clear_screen(legacy));
    assert(gpu_draw_box(native, 0, 0, 4, 4, 0x00123456, &plainmethod));
    gpu_gl_shutdown();
    assert(!gpu_is_active());
    assert(test_pixel(native, 2, 2) == 0x00123456);
    release_test_screen(legacy);
    release_test_screen(native);
}

/*
 * Damon V. Caskey
 * 2026-11-12
 * Create a real hidden OpenGL context and run native rendering/coherence
 * tests. Report unavailable graphics environments separately from failures.
 */
int main(void)
{
    SDL_Window *window;
    SDL_GLContext context;

    /* Treat missing video infrastructure as an explicit skip, then require
     * successful native initialization before exercising any image contract. */
    if(SDL_Init(SDL_INIT_VIDEO) != 0)
    {
        fprintf(stderr, "SKIP: SDL video unavailable: %s\n", SDL_GetError());
        return 77;
    }
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_COMPATIBILITY);
    window = SDL_CreateWindow("OpenBOR GPU tests", 0, 0, 64, 64, SDL_WINDOW_OPENGL | SDL_WINDOW_HIDDEN);
    context = window ? SDL_GL_CreateContext(window) : NULL;
    if(!context)
    {
        fprintf(stderr, "SKIP: GL context unavailable: %s\n", SDL_GetError());
        if(window)
        {
            SDL_DestroyWindow(window);
        }
        SDL_Quit();
        return 77;
    }
    assert(LoadGLFunctions());
    assert(gpu_gl_init());
    /* Exercise resource dependencies first, then material calculations and
     * shutdown while the same context owns every cached object. */
    test_queue_capture();
    test_queue_newonly();
    test_coherence_and_feedback();
    test_indexed_palette();
    test_blend_modes();
    test_sprite_mask();
    test_affine_palette_materials();
    test_affine_sprite_mask();
    test_affine_queue_replay();
    test_affine_rejection_atomicity();
    test_custom_blend_table();
    test_shutdown();
    SDL_GL_DeleteContext(context);

    /* New context object names must not inherit stale renderer identities. */
    context = SDL_GL_CreateContext(window);
    assert(context);
    assert(LoadGLFunctions());
    assert(gpu_gl_init());
    test_queue_capture();
    test_affine_queue_replay();
    gpu_gl_shutdown();
    SDL_GL_DeleteContext(context);
    SDL_DestroyWindow(window);
    SDL_Quit();
    puts("GPU renderer tests passed: captures, ordering, palette, blend, affine, coherence, recreation.");
    return 0;
}
