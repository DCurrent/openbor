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
 *   engine/source/gamelib/screen32.c engine/source/gamelib/draw.c \
 *   engine/source/gamelib/draw32.c engine/source/gamelib/sprite.c \
 *   engine/source/gamelib/spritex8p32.c engine/source/gamelib/spriteq.c \
 *   engine/source/gamelib/bitmap.c engine/source/gamelib/transform.c \
 *   engine/source/gamelib/pixelformat.c engine/source/omath.c \
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
 * Record CPU ownership after a software write or complete replacement.
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

    /* Reject unsupported work before changing any pixels so the original
     * software operation can run once against the synchronized target. */
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
     * cases verify that production fallbacks acquire the correct CPU version. */
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
        assert(!gpu_allow_cpu_operation(screen, GPU_OPERATION_SCREEN));
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
 * 2026-11-15
 * Snapshot the externally visible CPU raster state so native dispatch cannot
 * accidentally retain software drawmethod setup on either success or failure.
 */
static void test_draw_globals(unsigned values[11])
{
    /* Include clipping boundaries even when clipping is disabled. */
    values[0] = channelr;
    values[1] = channelg;
    values[2] = channelb;
    values[3] = tintcolor;
    values[4] = tintmode;
    values[5] = (unsigned)usechannel;
    values[6] = (unsigned)useclip;
    values[7] = (unsigned)clipx1;
    values[8] = (unsigned)clipy1;
    values[9] = (unsigned)clipx2;
    values[10] = (unsigned)clipy2;
}

/*
 * Damon V. Caskey
 * 2026-11-15
 * Reject native work without CPU retry, including missing callbacks and a
 * lost backend. Explicit CPU mode and indexed data remain usable elsewhere
 * in this fixture as independent software reference implementations.
 */
static void test_native_required_dispatch(s_screen *source, s_sprite *sprite,
                                          s_screen *dest, s_screen *indexed)
{
    gpu_backend missing_callbacks = backend;
    s_drawmethod sentinel = plainmethod;
    s_drawmethod requested = plainmethod;
    mock_image *native;
    unsigned cpu_before[MOCK_PIXEL_CAPACITY], native_before[MOCK_PIXEL_CAPACITY];
    unsigned globals_before[11], globals_after[11];
    int previous_downloads, previous_uploads, mode, repeat;
    size_t bytes = (size_t)dest->width * dest->height * sizeof(unsigned);

    /* Deliberately leave unrelated software state selected. Successful GPU
     * draws must consume their supplied method without replacing this state. */
    sentinel.channelr = 13;
    sentinel.channelg = 27;
    sentinel.channelb = 41;
    sentinel.tintmode = 1;
    sentinel.tintcolor = 0x123456;
    sentinel.clipx = 1;
    sentinel.clipy = 2;
    sentinel.clipw = 3;
    sentinel.cliph = 4;
    gpu_set_native_required(1);
    drawmethod_global_init(&sentinel);
    test_draw_globals(globals_before);
    putbox(0, 0, dest->width, dest->height, 0x556677, dest, &requested);
    putpixel(0, 0, 0xAABBCC, dest, &requested);
    putscreen(dest, source, 0, 0, NULL);
    test_draw_globals(globals_after);
    assert(!memcmp(globals_before, globals_after, sizeof(globals_before)));

    /* Hold distinct CPU and GPU versions. Any hidden raster retry, readback,
     * upload, or global initialization will violate at least one snapshot. */
    native = mock_find(dest, 0);
    assert(native && native->gpu_dirty);
    memcpy(cpu_before, dest->data, bytes);
    memcpy(native_before, native->pixels, bytes);
    previous_downloads = downloads;
    previous_uploads = uploads;
    missing_callbacks.draw_screen = NULL;
    missing_callbacks.draw_sprite = NULL;
    missing_callbacks.draw_box = NULL;
    missing_callbacks.draw_line = NULL;
    missing_callbacks.clear_screen = NULL;
    missing_callbacks.copy_screen = NULL;
    reject_draws = 1;

    for(mode = 0; mode < 3; mode++)
    {
        if(mode == 1)
        {
            gpu_register_backend(&missing_callbacks);
        }
        else if(mode == 2)
        {
            /* Backend loss cannot clear the previously selected policy. */
            gpu_unregister_backend();
            assert(!gpu_is_active());
        }
        for(repeat = 0; repeat < 2; repeat++)
        {
            /* Public try APIs must still report native failure truthfully. */
            assert(!gpu_draw_box(dest, 0, 0, 1, 1, 0xFFFFFF, &requested));
            putscreen(dest, source, 0, 0, &requested);
            putsprite(0, 0, sprite, dest, &requested);
            putbox(0, 0, 2, 2, 0xFFFFFF, dest, &requested);
            putline(0, 0, 3, 3, 0xFFFFFF, dest, &requested);
            putpixel(1, 1, 0xFFFFFF, dest, &requested);
            putpixel(UINT_MAX, UINT_MAX, 0xFFFFFF, dest, &requested);
            clearscreen(dest);
            copyscreen(dest, source);
        }
        test_draw_globals(globals_after);
        assert(!memcmp(globals_before, globals_after, sizeof(globals_before)));
        assert(!memcmp(cpu_before, dest->data, bytes));
        assert(!memcmp(native_before, native->pixels, bytes));
        assert(downloads == previous_downloads && uploads == previous_uploads);
        assert(native->gpu_dirty && !native->cpu_dirty);
    }

    /* Indexed image storage still needs CPU clear support even while native
     * RGB32 rendering is selected and its backend is unavailable. */
    memset(indexed->data, 7, (size_t)indexed->width * indexed->height);
    clearscreen(indexed);
    for(mode = 0; mode < indexed->width * indexed->height; mode++)
    {
        assert(indexed->data[mode] == 0);
    }

    /* The remaining fixture intentionally exercises CPU raster references.
     * Select that mode explicitly; registration itself always requires GPU. */
    reject_draws = 0;
    gpu_register_backend(&backend);
    gpu_set_native_required(0);
    drawmethod_global_init(NULL);
}

/*
 * Damon V. Caskey
 * 2026-11-15
 * Inspect production PP_TEST diagnostics without replacing the logger or draw
 * dispatch. Repeated failures emit one resampling record per image operation
 * until an explicit rendering-policy change starts another selection.
 */
static void test_resample_warning_log(FILE *log, int saved_stdout)
{
    char line[1024];
    unsigned screen_warnings = 0, sprite_warnings = 0, records = 0;
    assert(fflush(stdout) == 0);
    assert(dup2(saved_stdout, STDOUT_FILENO) >= 0);
    assert(close(saved_stdout) == 0);
    rewind(log);
    while(fgets(line, sizeof(line), log))
    {
        ++records;
        assert(strstr(line, "operation skipped. CPU resampling is unavailable."));
        if(strstr(line, "GPU resampling for screen draw"))
        {
            ++screen_warnings;
        }
        if(strstr(line, "GPU resampling for sprite draw"))
        {
            ++sprite_warnings;
        }
    }
    assert(!ferror(log));
    assert(records == 4 && screen_warnings == 2 && sprite_warnings == 2);
    assert(fclose(log) == 0);
}

/*
 * Damon V. Caskey
 * 2026-11-15
 * Explicit filtered draws must never become nearest CPU draws, including an
 * unmigrated rendering selection or indexed destination. Exercise real screen
 * and sprite roots against rejected, missing and unavailable backend callbacks.
 */
static void test_resample_required_dispatch(s_screen *source, s_sprite *sprite,
                                           s_screen *dest, s_screen *indexed)
{
    gpu_backend rejecting_backend = backend;
    gpu_backend missing_callbacks = backend;
    s_drawmethod requested = plainmethod;
    s_drawmethod sentinel = plainmethod;
    s_screen *indexed_dest = allocscreen(8, 8, PIXEL_x8);
    mock_image *native_source, *native_dest;
    unsigned source_before[MOCK_PIXEL_CAPACITY], dest_before[MOCK_PIXEL_CAPACITY];
    unsigned native_source_before[MOCK_PIXEL_CAPACITY], native_dest_before[MOCK_PIXEL_CAPACITY];
    unsigned char indexed_before[MOCK_PIXEL_CAPACITY];
    unsigned globals_before[11], globals_after[11];
    size_t source_bytes = (size_t)source->width * source->height * sizeof(unsigned);
    size_t dest_bytes = (size_t)dest->width * dest->height * sizeof(unsigned);
    FILE *log;
    int saved_stdout, phase, mode, repeat, format, kind, selection;
    int previous_downloads, previous_uploads, previous_draws, previous_rejections;
    unsigned char indexed_format = indexed->pixelformat;

    assert(indexed_dest);
    memset(indexed->data, 1, (size_t)indexed->width * indexed->height);
    memset(indexed_dest->data, 0x5a, sizeof(indexed_before));
    memcpy(indexed_before, indexed_dest->data, sizeof(indexed_before));
    rejecting_backend.draw_sprite = mock_reject_sprite;
    missing_callbacks.draw_screen = NULL;
    missing_callbacks.draw_sprite = NULL;
    gpu_register_backend(&rejecting_backend);
    gpu_set_native_required(0);

    /* Both images have stale CPU shadows, making any hidden readback visible. */
    putbox(0, 0, source->width, source->height, 0x2468AC, source, NULL);
    putbox(0, 0, dest->width, dest->height, 0x13579B, dest, NULL);
    native_source = mock_find(source, 0);
    native_dest = mock_find(dest, 0);
    assert(native_source && native_dest && native_source->gpu_dirty && native_dest->gpu_dirty);
    memcpy(source_before, source->data, source_bytes);
    memcpy(dest_before, dest->data, dest_bytes);
    memcpy(native_source_before, native_source->pixels, source_bytes);
    memcpy(native_dest_before, native_dest->pixels, dest_bytes);
    sentinel.channelr = 17;
    sentinel.channelg = 29;
    sentinel.channelb = 43;
    sentinel.clipx = 2;
    sentinel.clipy = 1;
    sentinel.clipw = 3;
    sentinel.cliph = 4;
    drawmethod_global_init(&sentinel);
    test_draw_globals(globals_before);
    previous_downloads = downloads;
    previous_uploads = uploads;
    previous_draws = native_draws;
    previous_rejections = rejected_sprite_draws;
    reject_draws = 1;

    log = tmpfile();
    assert(log && fflush(stdout) == 0);
    saved_stdout = dup(STDOUT_FILENO);
    assert(saved_stdout >= 0 && dup2(fileno(log), STDOUT_FILENO) >= 0);

    for(phase = 0; phase < 3; ++phase)
    {
        if(phase == 1)
        {
            /* Teardown does not reset warning state or enable CPU filtering. */
            gpu_unregister_backend();
        }
        else if(phase == 2)
        {
            /* Registration selects native mode; explicitly restore this fixture's
             * unmigrated mode and verify missing callbacks under that policy. */
            gpu_register_backend(&missing_callbacks);
            gpu_set_native_required(0);
        }
        for(mode = DC_RESAMPLE_BILINEAR; mode < DC_RESAMPLE_END; ++mode)
        {
            requested.resample = (e_drawmethod_resample)mode;
            for(repeat = 0; repeat < 2; ++repeat)
            {
                assert(!gpu_draw_screen(dest, source, 0, 0, &requested));
                assert(!gpu_draw_sprite(dest, sprite, 0, 0, &requested));
                putscreen(dest, source, 0, 0, &requested);
                putsprite(0, 0, sprite, dest, &requested);
                for(format = 0; format < 2; ++format)
                {
                    indexed_dest->pixelformat = format ? PIXEL_x8 : PIXEL_8;
                    putscreen(indexed_dest, indexed, 0, 0, &requested);
                    putsprite(0, 0, sprite, indexed_dest, &requested);
                }
            }
        }
        test_draw_globals(globals_after);
        assert(!memcmp(globals_before, globals_after, sizeof(globals_before)));
        assert(!memcmp(source_before, source->data, source_bytes));
        assert(!memcmp(dest_before, dest->data, dest_bytes));
        assert(!memcmp(native_source_before, native_source->pixels, source_bytes));
        assert(!memcmp(native_dest_before, native_dest->pixels, dest_bytes));
        assert(!memcmp(indexed_before, indexed_dest->data, sizeof(indexed_before)));
        assert(downloads == previous_downloads && uploads == previous_uploads);
        assert(native_draws == previous_draws);
        assert(native_source->gpu_dirty && !native_source->cpu_dirty);
        assert(native_dest->gpu_dirty && !native_dest->cpu_dirty);
    }
    assert(rejected_sprite_draws > previous_rejections);
    test_resample_warning_log(log, saved_stdout);

    /* Nearest, disabled drawmethods and absent drawmethods retain the existing
     * CPU results when that rendering configuration was selected explicitly. */
    gpu_register_backend(&rejecting_backend);
    gpu_set_native_required(0);
    indexed_dest->pixelformat = PIXEL_8;
    indexed->pixelformat = PIXEL_8;
    for(selection = 0; selection < 3; ++selection)
    {
        requested = plainmethod;
        if(selection == 1)
        {
            requested.config = DRAWMETHOD_CONFIG_NONE;
            requested.resample = DC_RESAMPLE_LANCZOS_3;
        }
        for(kind = 0; kind < 2; ++kind)
        {
            s_drawmethod *method = selection == 2 ? NULL : &requested;
            gpu_prepare_cpu_write(dest);
            memset(dest->data, 0, dest_bytes);
            memset(indexed_dest->data, 0x5a, sizeof(indexed_before));
            if(kind == 0)
            {
                putscreen(dest, source, 0, 0, method);
                putscreen(indexed_dest, indexed, 0, 0, method);
                assert(((unsigned *)dest->data)[0] == 0x2468AC);
            }
            else
            {
                putsprite(0, 0, sprite, dest, method);
                putsprite(0, 0, sprite, indexed_dest, method);
                assert(((unsigned *)dest->data)[0] == 0x456789);
            }
            assert(indexed_dest->data[0] == 1);
            assert(indexed_dest->data[indexed_dest->width * indexed_dest->height - 1] == 0x5a);
        }
    }

    reject_draws = 0;
    indexed->pixelformat = indexed_format;
    gpu_register_backend(&backend);
    gpu_set_native_required(0);
    drawmethod_global_init(NULL);
    freescreen(&indexed_dest);
}

/*
 * Damon V. Caskey
 * 2026-11-13
 * Check representable repeat instances and visible affine cancellation of
 * extreme integer draw positions in both screen and sprite fallbacks.
 */
static void test_affine_repeat_limits(s_screen *source, s_sprite *sprite, s_screen *dest)
{
    unsigned palette[256] = {0};
    int kind, scenario, x, y;

    /* Equal RGB channels make repeated half blends independently predictable.
     * Native fills leave CPU shadows stale before each software operation. */
    palette[1] = 0x808080;
    putbox(0, 0, source->width, source->height, 0x808080, source, NULL);
    for(kind = 0; kind < 2; kind++)
    {
        int source_width = kind ? sprite->width : source->width;
        int source_height = kind ? sprite->height : source->height;
        for(scenario = 0; scenario < 4; scenario++)
        {
            s_drawmethod method = plainmethod;
            int draw_x, draw_y;

            method.config |= DRAWMETHOD_CONFIG_AFFINE;
            method.alpha = BLEND_HALF + 1;
            method.table = (unsigned char *)palette;
            if(scenario < 2)
            {
                /* The valid first instance starts at (-1,-1) after the
                 * matrix translation cancels INT_MAX. Later positive spans
                 * are either unused or outside the integer drawing API. */
                draw_x = draw_y = INT_MAX;
                method.affine.tx = method.affine.ty = -2147483648.0f;
                method.xrepeat = method.yrepeat = scenario ? 3 : 1;
                method.xspan = method.yspan = scenario ? 1 : INT_MAX;
            }
            else if(scenario == 2)
            {
                /* The lower boundary has a valid first image at the origin;
                 * negative spans must not wrap into additional instances. */
                draw_x = draw_y = INT_MIN;
                method.affine.tx = method.affine.ty = 2147483648.0f;
                method.xrepeat = method.yrepeat = 3;
                method.xspan = method.yspan = -1;
            }
            else
            {
                /* Ordinary overlapping repeats still blend once per covered
                 * instance in the existing row/column submission order. */
                draw_x = draw_y = 0;
                method.xrepeat = method.yrepeat = 2;
                method.xspan = method.yspan = 1;
            }

            putbox(0, 0, dest->width, dest->height, 0x202020, dest, NULL);
            if(kind)
            {
                putsprite(draw_x, draw_y, sprite, dest, &method);
            }
            else
            {
                putscreen(dest, source, draw_x, draw_y, &method);
            }
            for(y = 0; y < dest->height; y++)
            {
                for(x = 0; x < dest->width; x++)
                {
                    int count, blend;
                    unsigned channel = 0x20;
                    if(scenario < 2)
                    {
                        count = x < source_width - 1 && y < source_height - 1;
                    }
                    else if(scenario == 2)
                    {
                        count = x < source_width && y < source_height;
                    }
                    else
                    {
                        int columns = (x < source_width) + (x >= 1 && x <= source_width);
                        int rows = (y < source_height) + (y >= 1 && y <= source_height);
                        count = columns * rows;
                    }
                    for(blend = 0; blend < count; blend++)
                    {
                        channel = (0x80 + channel) / 2;
                    }
                    assert(((unsigned *)dest->data)[y * dest->width + x] ==
                           (channel << 16 | channel << 8 | channel));
                }
            }
        }
    }
}

/*
 * Damon V. Caskey
 * 2026-11-12
 * 2026-11-13 - Include affine fallback repeat boundaries under sanitizers.
 * 2026-11-15 - Verify native-only dispatch independently of CPU references.
 * 2026-11-15 - Reject removed image formats before allocation or drawing.
 * 2026-11-15 - Keep requested GPU resampling out of every CPU fallback path.
 * Cover native dispatch, synchronous CPU access, software fallback, direct
 * low-level calls, queue capture/replay, and resource lifetime together.
 */
int main(void)
{
    s_screen *small = test_screen(4, 4, 0x112233);
    s_screen *large = test_screen(8, 8, 0x445566);
    s_screen *capture = test_screen(8, 8, 0);
    s_screen *indexed;
    s_bitmap *bitmap;
    s_sprite *sprite;
    s_drawmethod method = plainmethod;
    unsigned char *tables[MAX_BLENDINGS] = {0};
    int previous_downloads, previous_uploads, previous_sprites, i;

    test_image_format_contract();
    gpu_register_backend(&backend);
    assert(gpu_is_active());

    /* GPU drawing does not eagerly overwrite the CPU shadow. */
    putbox(0, 0, 4, 4, 0x123456, small, NULL);
    assert(((unsigned *)small->data)[0] == 0x112233);
    assert(downloads == 0);
    gpu_sync_cpu(small);
    assert(((unsigned *)small->data)[0] == 0x123456);
    gpu_sync_cpu(small);
    assert(downloads == 1);

    /* A directly called software primitive preserves untouched GPU pixels. */
    putbox(0, 0, 4, 4, 0x234567, small, NULL);
    line32(0, 0, 3, 0, 0xABCDEF, small, 0);
    assert(((unsigned *)small->data)[0] == 0xABCDEF);
    assert(((unsigned *)small->data)[4] == 0x234567);
    previous_uploads = uploads;
    copyscreen(large, small);
    assert(uploads > previous_uploads);
    gpu_sync_cpu(large);
    assert(((unsigned *)large->data)[0] == 0xABCDEF);
    assert(((unsigned *)large->data)[8] == 0x234567);
    assert(((unsigned *)large->data)[7] == 0x445566);

    /* Native clear discards prior contents without a download. */
    putbox(0, 0, 8, 8, 0x334455, large, NULL);
    previous_downloads = downloads;
    clearscreen(large);
    assert(downloads == previous_downloads);
    gpu_sync_cpu(large);
    for(i = 0; i < 64; ++i)
    {
        assert(((unsigned *)large->data)[i] == 0);
    }

    /* Unsupported transform reads GPU source and preserves target borders. */
    /* This fixture explicitly selects the still-required CPU reference mode;
     * production native RGB32 rejection is checked independently below. */
    gpu_set_native_required(0);
    putbox(0, 0, 4, 4, 0x765432, small, NULL);
    putbox(0, 0, 8, 8, 0x101010, large, NULL);
    method.scalex = 128;
    putscreen(large, small, 0, 0, &method);
    assert(((unsigned *)large->data)[0] == 0x765432);
    assert(((unsigned *)large->data)[7] == 0x101010);

    /* The direct scaler and indexed blitter cannot bypass coherence. */
    putbox(0, 0, 4, 4, 0x314159, small, NULL);
    scalescreen32(large, small);
    assert(((unsigned *)large->data)[63] == 0x314159);
    indexed = allocscreen(4, 4, PIXEL_x8);
    assert(indexed);
    memset(indexed->data, 1, 16);
    memset(indexed->palette, 0, PAL_BYTES);
    ((unsigned *)indexed->palette)[1] = 0x987654;
    putbox(0, 0, 8, 8, 0x181818, large, NULL);
    putscreenx8p32(large, indexed, 0, 0, 0, NULL, NULL);
    assert(((unsigned *)large->data)[0] == 0x987654);
    assert(((unsigned *)large->data)[7] == 0x181818);

    /* RLE sprites use the preserved software route when unsupported. */
    bitmap = allocbitmap(4, 2, PIXEL_x8);
    memset(bitmap->data, 1, 8);
    memset(bitmap->palette, 0, PAL_BYTES);
    ((unsigned *)bitmap->palette)[1] = 0x456789;
    /* The fixed fixture needs only a small RLE payload plus its palette.
     * Avoid the legacy size-query routine's unrelated null-pointer arithmetic
     * so sanitizer failures remain specific to this rendering regression. */
    sprite = malloc(sizeof(*sprite) + PAL_BYTES + 128);
    assert(sprite);
    previous_sprites = released_sprites;
    encodesprite(0, 0, bitmap, sprite);
    assert(released_sprites == previous_sprites + 1);
    test_native_required_dispatch(small, sprite, large, indexed);
    test_resample_required_dispatch(small, sprite, large, indexed);
    putbox(0, 0, 8, 8, 0x212121, large, NULL);
    putsprite(0, 0, sprite, large, NULL);
    assert(((unsigned *)large->data)[0] == 0x456789);
    assert(((unsigned *)large->data)[7] == 0x212121);

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

    /* Source pixels remain live until queue replay, not frozen at enqueue. */
    spriteq_add_screen(0, 0, 0, small, NULL, 0);
    putbox(0, 0, 4, 4, 0x667788, small, NULL);
    spriteq_draw(capture, 0, INT_MIN, INT_MAX, 0, 0);
    gpu_sync_cpu(capture);
    assert(((unsigned *)capture->data)[0] == 0x667788);
    spriteq_clear();

    /* A fully rejected native draw still produces the normal CPU result. */
    putbox(0, 0, 8, 8, 0x887766, large, NULL);
    reject_draws = 1;
    putpixel(1, 1, 0xFFFFFF, large, NULL);
    assert(((unsigned *)large->data)[9] == 0xFFFFFF);
    assert(((unsigned *)large->data)[10] == 0x887766);
    reject_draws = 0;
    set_blendtables(tables);
    assert(invalidated_tables == 1);

    test_affine_repeat_limits(small, sprite, large);

    /* Free wrappers must evict resources before addresses can be recycled. */
    gpu_release_sprite(sprite);
    free(sprite);
    freebitmap(bitmap);
    freescreen(&indexed);
    freescreen(&small);
    freescreen(&large);
    freescreen(&capture);
    assert(released_screens == 5);
    for(i = 0; i < MOCK_SCREEN_CAPACITY; ++i)
    {
        assert(!images[i].screen);
    }
    assert(native_draws > 0);
    gpu_unregister_backend();
    assert(!gpu_is_active());

    /* Explicit unmigrated CPU mode remains functional without a backend. */
    gpu_set_native_required(0);
    small = test_screen(4, 4, 0);
    putbox(0, 0, 4, 4, 0x654321, small, NULL);
    assert(((unsigned *)small->data)[15] == 0x654321);
    freescreen(&small);
    puts("GPU coherence and queue capture tests passed.");
    return 0;
}
