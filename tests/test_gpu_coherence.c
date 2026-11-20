/*
 * Exercise production rendering entry points with a deliberately separate
 * mock GPU image. The separate storage makes missing ownership transitions
 * observable without a window or graphics driver.
 *
 * Build from the repository root:
 * gcc -std=c99 -O1 -DPP_TEST=1 -ffunction-sections -fdata-sections \
 *   -Iengine -Iengine/source -Iengine/source/gamelib \
 *   tests/test_gpu_coherence.c engine/source/gamelib/gpu.c \
 *   engine/source/gamelib/screen.c \
 *   engine/source/gamelib/draw.c engine/source/gamelib/sprite.c \
 *   engine/source/gamelib/spriteq.c engine/source/gamelib/bitmap.c \
 *   engine/source/gamelib/pixelformat.c \
 *   -Wl,--gc-sections -lm -o test_gpu_coherence
 */

#include <assert.h>
#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "gpu.h"
#include "screen.h"
#include "draw.h"
#include "sprite.h"
#include "spriteq.h"
#include "bitmap.h"

#define MOCK_SCREEN_CAPACITY 16
#define MOCK_PIXEL_CAPACITY 64

typedef struct mock_image
{
    s_screen *screen;
    unsigned pixels[MOCK_PIXEL_CAPACITY];
    int cpu_dirty;
    int gpu_dirty;
} mock_image;

static mock_image images[MOCK_SCREEN_CAPACITY];
static int uploads;
static int downloads;
static int native_draws;
static int released_screens;
static int released_sprites;
static int invalidated_tables;
static int reject_draws;
static int rejected_sprite_draws;

const s_drawmethod plainmethod = {
    .config = DRAWMETHOD_CONFIG_ENABLED,
    .scalex = 256, .scaley = 256,
    .xrepeat = 1, .yrepeat = 1,
    .channelr = 255, .channelg = 255, .channelb = 255,
    .affine = {.m00 = 1.0f, .m11 = 1.0f}
};

/*
 * Damon V. Caskey
 * 2026-11-12
 * Supply the allocation assertion used by the production gamelib without
 * linking the engine's logging, script, and platform subsystems.
 */
void *checkAlloc(void *ptr, size_t size, const char *func, const char *file, int line)
{
    /* Keep allocation failure fatal while omitting the engine log context. */
    (void)size;
    (void)func;
    (void)file;
    (void)line;
    assert(ptr != NULL);
    return ptr;
}

/*
 * Damon V. Caskey
 * 2026-11-12
 * Find a mock GPU allocation by screen identity, optionally allocating a
 * new slot whose first GPU use must upload the initial CPU contents.
 */
static mock_image *mock_find(s_screen *screen, int create)
{
    int i;
    mock_image *empty = NULL;

    /* Only RGB32 images participate in this fixture's native path. Indexed
     * source artwork uses CPU image helpers without a target cache slot. */
    if(!screen || screen->pixelformat != PIXEL_32)
    {
        return NULL;
    }
    /* Reuse identity before choosing a free slot. Read-only lookups must never
     * manufacture a resource and hide an absent upload or release. */
    for(i = 0; i < MOCK_SCREEN_CAPACITY; ++i)
    {
        if(images[i].screen == screen)
        {
            return &images[i];
        }
        if(!images[i].screen && !empty)
        {
            empty = &images[i];
        }
    }
    if(!create)
    {
        return NULL;
    }
    /* New resources start CPU-owned; their first native use performs upload. */
    assert(empty);
    assert(screen->width * screen->height <= MOCK_PIXEL_CAPACITY);
    memset(empty, 0, sizeof(*empty));
    empty->screen = screen;
    empty->cpu_dirty = 1;
    return empty;
}

/*
 * Damon V. Caskey
 * 2026-11-12
 * Upload only CPU-owned versions, leaving GPU-owned images undisturbed.
 */
static mock_image *mock_ready(s_screen *screen)
{
    mock_image *image = mock_find(screen, 1);

    /* Copy only a changed CPU version. Otherwise the independent GPU image
     * remains authoritative, even while the CPU shadow contains older pixels. */
    if(image && image->cpu_dirty)
    {
        memcpy(image->pixels, screen->data,
               (size_t)screen->width * screen->height * sizeof(unsigned));
        image->cpu_dirty = 0;
        ++uploads;
    }
    return image;
}

/*
 * Damon V. Caskey
 * 2026-11-12
 * Simulate lazy readback. Repeated reads of the same version must not
 * transfer pixels again.
 */
static int mock_sync(s_screen *screen)
{
    mock_image *image = mock_find(screen, 0);

    /* Clear the pending-readback flag after copying so repeated pixel queries
     * share the same current shadow until another native draw occurs. */
    if(image && image->gpu_dirty)
    {
        memcpy(screen->data, image->pixels,
               (size_t)screen->width * screen->height * sizeof(unsigned));
        image->gpu_dirty = 0;
        ++downloads;
    }
    return 1;
}

/*
 * Damon V. Caskey
 * 2026-11-12
 * Record CPU ownership after an explicit image edit or complete replacement.
 */
static void mock_dirty(s_screen *screen)
{
    mock_image *image = mock_find(screen, 0);

    /* CPU ownership supersedes the prior native version. Partial writers are
     * responsible for synchronizing retained pixels before reaching this hook. */
    if(image)
    {
        image->cpu_dirty = 1;
        image->gpu_dirty = 0;
    }
}

/*
 * Damon V. Caskey
 * 2026-11-12
 * Provide the minimal opaque native rectangle used by queue and coherence
 * tests; unsupported modes return before changing destination pixels.
 */
static int mock_box(s_screen *screen, int x, int y, int width, int height,
                    unsigned color, const s_drawmethod *method)
{
    mock_image *image;
    int px, py;

    /* Rejection must happen before uploads, readback, ownership changes
     * or pixel writes. The public wrapper must leave that failure intact. */
    if(reject_draws || (method && (method->config & DRAWMETHOD_CONFIG_ENABLED) &&
       method->alpha > 0))
    {
        return 0;
    }
    image = mock_ready(screen);
    if(!image)
    {
        return 0;
    }
    /* Clip the opaque fill to image bounds and leave the CPU shadow stale. */
    for(py = y; py < y + height; ++py)
    {
        for(px = x; px < x + width; ++px)
        {
            if(px >= 0 && py >= 0 && px < screen->width && py < screen->height)
            {
                image->pixels[py * screen->width + px] = color & 0xFFFFFF;
            }
        }
    }
    image->gpu_dirty = 1;
    ++native_draws;
    return 1;
}

/*
 * Damon V. Caskey
 * 2026-11-12
 * 2026-11-15 - Permit deterministic native rejection before any ownership change.
 * Simulate native clear without reading the prior GPU image to the CPU.
 */
static int mock_clear(s_screen *screen)
{
    mock_image *image;

    /* The rejection fixture must leave both versions untouched. */
    if(reject_draws)
    {
        return 0;
    }
    image = mock_find(screen, 1);

    /* Full replacement needs no upload of the pixels being discarded. */
    if(!image)
    {
        return 0;
    }
    memset(image->pixels, 0, sizeof(image->pixels));
    image->cpu_dirty = 0;
    image->gpu_dirty = 1;
    return 1;
}

/*
 * Damon V. Caskey
 * 2026-11-12
 * Keep ordinary screen copies entirely in mock GPU memory, preserving
 * destination pixels outside the source and rejecting self-sampling.
 */
static int mock_screen(s_screen *dest, s_screen *src, int x, int y,
                       const s_drawmethod *method)
{
    mock_image *source, *target;
    int px, py;

    /* Restrict this mock to distinct, untransformed RGB32 images. The rejected
     * cases verify that public wrappers do not attempt a CPU retry. */
    if(reject_draws || dest == src || dest->pixelformat != PIXEL_32 ||
       src->pixelformat != PIXEL_32 ||
       (method && (method->config & DRAWMETHOD_CONFIG_ENABLED)))
    {
        return 0;
    }
    source = mock_ready(src);
    target = mock_ready(dest);
    assert(source && target);
    /* Copy the overlap while retaining target pixels outside the source. */
    for(py = 0; py < src->height; ++py)
    {
        for(px = 0; px < src->width; ++px)
        {
            if(x + px >= 0 && y + py >= 0 && x + px < dest->width &&
               y + py < dest->height)
            {
                target->pixels[(y + py) * dest->width + x + px] =
                    source->pixels[py * src->width + px];
            }
        }
    }
    target->gpu_dirty = 1;
    ++native_draws;
    return 1;
}

/*
 * Damon V. Caskey
 * 2026-11-12
 * Adapt untransformed copies to the mock native screen operation.
 */
static int mock_copy(s_screen *dest, s_screen *src)
{
    /* Copy uses replacement at the origin, independent of global draw state. */
    return mock_screen(dest, src, 0, 0, NULL);
}

/*
 * Damon V. Caskey
 * 2026-11-15
 * Distinguish an explicit sprite callback rejection from an absent callback
 * while leaving both image ownership and pixel storage completely unchanged.
 */
static int mock_reject_sprite(s_screen *dest, s_sprite *sprite, int x, int y,
                              const s_drawmethod *method)
{
    (void)dest;
    (void)sprite;
    (void)x;
    (void)y;
    (void)method;
    ++rejected_sprite_draws;
    return 0;
}

/*
 * Damon V. Caskey
 * 2026-11-12
 * Retire screen identities so a recycled allocation cannot inherit stale
 * native image storage.
 */
static void mock_release_screen(s_screen *screen)
{
    mock_image *image = mock_find(screen, 0);

    /* Remove identity as well as pixel ownership so allocator address reuse
     * cannot inherit the retired target's contents. */
    if(image)
    {
        memset(image, 0, sizeof(*image));
    }
    ++released_screens;
}

/*
 * Damon V. Caskey
 * 2026-11-12
 * Observe sprite cache eviction without reading an uninitialized allocation.
 */
static void mock_release_sprite(s_sprite *sprite)
{
    /* Re-encoding calls this with fresh storage as well as existing sprites;
     * eviction depends on identity, so the allocation's fields stay unread. */
    (void)sprite;
    ++released_sprites;
}

/*
 * Damon V. Caskey
 * 2026-11-12
 * Verify that replacing blend-table pointers invalidates native lookup data.
 */
static void mock_invalidate_tables(void)
{
    /* The count verifies that the public setter reaches native lookup state. */
    ++invalidated_tables;
}

static const gpu_backend backend = {
    .draw_screen = mock_screen,
    .draw_sprite = mock_reject_sprite,
    .draw_box = mock_box,
    .clear_screen = mock_clear,
    .copy_screen = mock_copy,
    .sync_cpu = mock_sync,
    .mark_cpu_dirty = mock_dirty,
    .release_screen = mock_release_screen,
    .release_sprite = mock_release_sprite,
    .invalidate_tables = mock_invalidate_tables
};

/*
 * Damon V. Caskey
 * 2026-11-15
 * Keep invalid image formats out of allocation and CPU drawing while retaining
 * indexed artwork and RGB32 images under the 64-bit application contract.
 */
static void test_image_format_contract(void)
{
    const int supported[] = {PIXEL_8, PIXEL_x8, PIXEL_32};
    const int unsupported[] = {-1, 2, 3, 5, INT_MAX};
    unsigned char before[4 * 2 * sizeof(unsigned)];
    s_screen *screen;
    s_bitmap *bitmap;
    size_t i;

    /* Every test is compiled as C99 against the production types.h, whose
     * architecture guard must accept this supported 64-bit test executable. */
    assert(sizeof(void *) == 8);
    for(i = 0; i < sizeof(unsupported) / sizeof(unsupported[0]); ++i)
    {
        assert(allocscreen(4, 2, unsupported[i]) == NULL);
        assert(allocbitmap(4, 2, unsupported[i]) == NULL);
    }
    for(i = 0; i < sizeof(supported) / sizeof(supported[0]); ++i)
    {
        screen = allocscreen(4, 2, supported[i]);
        assert(screen && screen->pixelformat == supported[i]);
        memset(screen->data, 0x5a, (size_t)4 * 2 * pixelbytes[supported[i]]);
        freescreen(&screen);
        assert(screen == NULL);
        bitmap = allocbitmap(4, 2, supported[i]);
        assert(bitmap && bitmap->pixelformat == supported[i]);
        memset(bitmap->data, 0x5a, (size_t)4 * 2 * pixelbytes[supported[i]]);
        freebitmap(bitmap);
    }

    /* Malformed headers can still reach a public draw entry point. Removed
     * formats must not revive CPU writes even when no GPU backend is loaded. */
    screen = allocscreen(4, 2, PIXEL_32);
    assert(screen);
    memset(screen->data, 0x5a, sizeof(before));
    memcpy(before, screen->data, sizeof(before));
    for(i = 2; i <= 3; ++i)
    {
        screen->pixelformat = (unsigned char)i;
        clearscreen(screen);
        putbox(0, 0, 4, 2, 0x00123456, screen, NULL);
        assert(memcmp(before, screen->data, sizeof(before)) == 0);
    }
    screen->pixelformat = PIXEL_32;
    freescreen(&screen);
}

/*
 * Damon V. Caskey
 * 2026-11-12
 * Allocate a small deterministic CPU image before its first GPU use.
 */
static s_screen *test_screen(int width, int height, unsigned color)
{
    int i;
    s_screen *screen = allocscreen(width, height, PIXEL_32);

    /* Initialize the CPU-owned image before any native resource can upload it. */
    assert(screen);
    for(i = 0; i < width * height; ++i)
    {
        ((unsigned *)screen->data)[i] = color;
    }
    return screen;
}

/*
 * Damon V. Caskey
 * 2026-11-20
 * Exercise real public draw roots with rejecting callbacks, missing callbacks
 * and an absent backend. Distinct CPU/GPU contents expose hidden readback,
 * upload, ownership changes or rasterization on every failure route.
 */
static void test_rejected_dispatch(s_screen *source, s_sprite *sprite,
                                   s_screen *dest, s_screen *indexed)
{
    gpu_backend missing = backend;
    s_screen *indexed_dest = allocscreen(8, 8, PIXEL_x8);
    mock_image *native_source, *native_dest;
    unsigned source_before[MOCK_PIXEL_CAPACITY], dest_before[MOCK_PIXEL_CAPACITY];
    unsigned native_source_before[MOCK_PIXEL_CAPACITY], native_dest_before[MOCK_PIXEL_CAPACITY];
    unsigned char indexed_before[MOCK_PIXEL_CAPACITY];
    size_t source_bytes = (size_t)source->width * source->height * sizeof(unsigned);
    size_t dest_bytes = (size_t)dest->width * dest->height * sizeof(unsigned);
    int prior_downloads, prior_uploads, prior_draws, prior_rejections;
    int phase, selection, repeat, indexed_format;
    int saved_stdout;
    FILE *log;
    char line[1024];
    unsigned records = 0;

    assert(indexed_dest);
    memset(indexed_dest->data, 0x5a, sizeof(indexed_before));
    memcpy(indexed_before, indexed_dest->data, sizeof(indexed_before));
    gpu_register_backend(&backend);
    putbox(0, 0, source->width, source->height, 0x2468AC, source, NULL);
    putbox(0, 0, dest->width, dest->height, 0x13579B, dest, NULL);
    native_source = mock_find(source, 0);
    native_dest = mock_find(dest, 0);
    assert(native_source && native_dest && native_source->gpu_dirty && native_dest->gpu_dirty);
    memcpy(source_before, source->data, source_bytes);
    memcpy(dest_before, dest->data, dest_bytes);
    memcpy(native_source_before, native_source->pixels, source_bytes);
    memcpy(native_dest_before, native_dest->pixels, dest_bytes);
    prior_downloads = downloads;
    prior_uploads = uploads;
    prior_draws = native_draws;
    prior_rejections = rejected_sprite_draws;
    missing.draw_screen = NULL;
    missing.draw_sprite = NULL;
    missing.draw_box = NULL;
    missing.draw_line = NULL;
    missing.clear_screen = NULL;
    missing.copy_screen = NULL;
    reject_draws = 1;

    /* Capture ordinary diagnostics so repeated fixture draws remain quiet;
     * the separate logger suite checks the exact per-operation suppression. */
    log = tmpfile();
    assert(log && fflush(stdout) == 0);
    saved_stdout = dup(STDOUT_FILENO);
    assert(saved_stdout >= 0 && dup2(fileno(log), STDOUT_FILENO) >= 0);
    for(phase = 0; phase < 3; ++phase)
    {
        if(phase == 1) gpu_register_backend(&missing);
        if(phase == 2) gpu_unregister_backend();
        for(selection = 0; selection < DC_RESAMPLE_END + 2; ++selection)
        {
            s_drawmethod requested = plainmethod;
            s_drawmethod *method = &requested;
            if(selection < DC_RESAMPLE_END)
            {
                requested.resample = (e_drawmethod_resample)selection;
                requested.scalex = 128;
                requested.config |= DRAWMETHOD_CONFIG_AFFINE;
                requested.affine.m00 = 0.5f;
            }
            else if(selection == DC_RESAMPLE_END)
            {
                requested.config = DRAWMETHOD_CONFIG_NONE;
                requested.resample = DC_RESAMPLE_LANCZOS_3;
            }
            else method = NULL;
            for(repeat = 0; repeat < 2; ++repeat)
            {
                assert(!gpu_draw_box(dest, 0, 0, 1, 1, 0xFFFFFF, method));
                assert(!gpu_draw_screen(dest, source, 0, 0, method));
                assert(!gpu_draw_sprite(dest, sprite, 0, 0, method));
                putscreen(dest, source, 0, 0, method);
                putsprite(0, 0, sprite, dest, method);
                putbox(0, 0, 2, 2, 0xFFFFFF, dest, method);
                putline(0, 0, 3, 3, 0xFFFFFF, dest, method);
                putpixel(1, 1, 0xFFFFFF, dest, method);
                putpixel(UINT_MAX, UINT_MAX, 0xFFFFFF, dest, method);
                clearscreen(dest);
                copyscreen(dest, source);
                for(indexed_format = 0; indexed_format < 2; ++indexed_format)
                {
                    indexed_dest->pixelformat = indexed_format ? PIXEL_x8 : PIXEL_8;
                    putscreen(indexed_dest, indexed, 0, 0, method);
                    putsprite(0, 0, sprite, indexed_dest, method);
                    putbox(0, 0, 2, 2, 0xFFFFFF, indexed_dest, method);
                    putline(0, 0, 3, 3, 0xFFFFFF, indexed_dest, method);
                    putpixel(1, 1, 0xFFFFFF, indexed_dest, method);
                }
            }
        }
        assert(!memcmp(source_before, source->data, source_bytes));
        assert(!memcmp(dest_before, dest->data, dest_bytes));
        assert(!memcmp(native_source_before, native_source->pixels, source_bytes));
        assert(!memcmp(native_dest_before, native_dest->pixels, dest_bytes));
        assert(!memcmp(indexed_before, indexed_dest->data, sizeof(indexed_before)));
        assert(downloads == prior_downloads && uploads == prior_uploads && native_draws == prior_draws);
        assert(native_source->gpu_dirty && !native_source->cpu_dirty);
        assert(native_dest->gpu_dirty && !native_dest->cpu_dirty);
    }
    assert(rejected_sprite_draws > prior_rejections);
    assert(fflush(stdout) == 0 && dup2(saved_stdout, STDOUT_FILENO) >= 0);
    assert(close(saved_stdout) == 0);
    rewind(log);
    while(fgets(line, sizeof(line), log))
    {
        assert(strstr(line, "[GPU] WARNING:") && strstr(line, "operation skipped"));
        ++records;
    }
    assert(!ferror(log) && records > 0);
    assert(fclose(log) == 0);

    /* Indexed buffers remain source assets. Their narrow copy/reset operations
     * still work without a renderer and never alter an RGB scene destination. */
    indexed_dest->pixelformat = PIXEL_x8;
    memset(indexed->data, 7, (size_t)indexed->width * indexed->height);
    copyscreen(indexed_dest, indexed);
    for(selection = 0; selection < indexed_dest->width * indexed_dest->height; ++selection)
    {
        int x = selection % indexed_dest->width, y = selection / indexed_dest->width;
        assert(indexed_dest->data[selection] == (x < indexed->width && y < indexed->height ? 7 : 0x5a));
    }
    clearscreen(indexed_dest);
    for(selection = 0; selection < indexed_dest->width * indexed_dest->height; ++selection)
        assert(indexed_dest->data[selection] == 0);
    reject_draws = 0;
    gpu_register_backend(&backend);
    freescreen(&indexed_dest);
}

/*
 * Damon V. Caskey
 * 2026-11-20
 * Cover GPU-only public dispatch, explicit CPU image edits, queue captures,
 * indexed source maintenance and native identity lifetime in one driver-free
 * fixture. Separate native pixel suites verify actual rendered appearance.
 */
int main(void)
{
    s_screen *small = test_screen(4, 4, 0x112233);
    s_screen *large = test_screen(8, 8, 0x445566);
    s_screen *capture = test_screen(8, 8, 0);
    s_screen *indexed;
    s_bitmap *bitmap;
    s_sprite *sprite;
    const unsigned char sprite_indices[8] = {0, 1, 0, 255, 3, 0, 4, 0};
    unsigned char *tables[MAX_BLENDINGS] = {0};
    int previous_downloads, previous_uploads, previous_sprites, i;

    test_image_format_contract();
    /* No registered backend exists at startup. A primitive cannot silently
     * enable a software renderer even before the first successful GPU draw. */
    assert(!gpu_is_active());
    putbox(0, 0, 4, 4, 0xFFFFFF, small, NULL);
    assert(((unsigned *)small->data)[0] == 0x112233);
    assert(uploads == 0 && downloads == 0);
    gpu_register_backend(&backend);

    putbox(0, 0, 4, 4, 0x123456, small, NULL);
    assert(((unsigned *)small->data)[0] == 0x112233 && downloads == 0);
    gpu_sync_cpu(small);
    assert(((unsigned *)small->data)[0] == 0x123456);
    gpu_sync_cpu(small);
    assert(downloads == 1);

    /* Explicit CPU edits remain supported for real image access. They acquire
     * the native version first, preserve untouched pixels, then upload lazily. */
    putbox(0, 0, 4, 4, 0x234567, small, NULL);
    gpu_prepare_cpu_write(small);
    ((unsigned *)small->data)[0] = 0xABCDEF;
    assert(((unsigned *)small->data)[4] == 0x234567);
    previous_uploads = uploads;
    copyscreen(large, small);
    assert(uploads > previous_uploads);
    gpu_sync_cpu(large);
    assert(((unsigned *)large->data)[0] == 0xABCDEF);
    assert(((unsigned *)large->data)[8] == 0x234567);
    assert(((unsigned *)large->data)[7] == 0x445566);

    putbox(0, 0, 8, 8, 0x334455, large, NULL);
    previous_downloads = downloads;
    clearscreen(large);
    assert(downloads == previous_downloads);
    gpu_sync_cpu(large);
    for(i = 0; i < 64; ++i) assert(((unsigned *)large->data)[i] == 0);

    indexed = allocscreen(4, 4, PIXEL_x8);
    assert(indexed);
    memset(indexed->data, 1, 16);
    memset(indexed->palette, 0, PAL_BYTES);
    ((unsigned *)indexed->palette)[1] = 0x987654;
    bitmap = allocbitmap(4, 2, PIXEL_x8);
    memcpy(bitmap->data, sprite_indices, sizeof(sprite_indices));
    memset(bitmap->palette, 0, PAL_BYTES);
    ((unsigned *)bitmap->palette)[1] = 0x456789;
    sprite = malloc(sizeof(*sprite) + PAL_BYTES + 128);
    assert(sprite);
    previous_sprites = released_sprites;
    encodesprite(0, 0, bitmap, sprite);
    assert(released_sprites == previous_sprites + 1);
    for(i = 0; i < 8; ++i)
        assert((unsigned char)sprite_get_pixel(sprite, i % 4, i / 4) == sprite_indices[i]);
    assert(sprite_get_pixel(sprite, -1, 0) == 0);
    assert(sprite_get_pixel(sprite, 4, 0) == 0);
    assert(sprite_get_pixel(NULL, 0, 0) == 0);
    test_rejected_dispatch(small, sprite, large, indexed);

    /* Captures retain their contents and queue; newonly skips locked prefix. */
    spriteq_clear();
    spriteq_add_dot(0, 0, 10, 0xFF0000, NULL);
    spriteq_lock();
    spriteq_add_dot(1, 0, 20, 0x0000FF, NULL);
    spriteq_draw(capture, 1, INT_MIN, INT_MAX, 1, 1);
    assert(spriteq_get_sprite_count() == 2);
    gpu_sync_cpu(capture);
    assert(((unsigned *)capture->data)[9] == 0);
    assert(((unsigned *)capture->data)[10] == 0x0000FF);
    spriteq_draw(capture, 0, 10, 10, 0, 0);
    gpu_sync_cpu(capture);
    assert(((unsigned *)capture->data)[0] == 0xFF0000);
    assert(((unsigned *)capture->data)[10] == 0x0000FF);
    spriteq_clear();
    assert(spriteq_get_sprite_count() == 1);
    spriteq_unlock();
    spriteq_clear();

    spriteq_add_screen(0, 0, 0, small, NULL, 0);
    putbox(0, 0, 4, 4, 0x667788, small, NULL);
    spriteq_draw(capture, 0, INT_MIN, INT_MAX, 0, 0);
    gpu_sync_cpu(capture);
    assert(((unsigned *)capture->data)[0] == 0x667788);
    spriteq_clear();
    set_blendtables(tables);
    assert(invalidated_tables == 1);

    gpu_release_sprite(sprite);
    free(sprite);
    freebitmap(bitmap);
    freescreen(&indexed);
    freescreen(&small);
    freescreen(&large);
    freescreen(&capture);
    assert(released_screens == 5);
    for(i = 0; i < MOCK_SCREEN_CAPACITY; ++i) assert(!images[i].screen);
    assert(native_draws > 0);
    gpu_unregister_backend();
    assert(!gpu_is_active());
    small = test_screen(4, 4, 0);
    putbox(0, 0, 4, 4, 0x654321, small, NULL);
    for(i = 0; i < 16; ++i) assert(((unsigned *)small->data)[i] == 0);
    freescreen(&small);
    puts("GPU-only dispatch, explicit image access and queue capture tests passed.");
    return 0;
}
