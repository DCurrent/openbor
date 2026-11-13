/*
 * Exercise production rendering entry points with a deliberately separate
 * mock GPU image. The separate storage makes missing ownership transitions
 * observable without a window or graphics driver.
 *
 * Build from the repository root:
 * gcc -std=c99 -O1 -DPP_TEST=1 -ffunction-sections -fdata-sections \
 *   -Iengine -Iengine/source -Iengine/source/gamelib \
 *   tests/test_gpu_coherence.c engine/source/gamelib/gpu.c \
 *   engine/source/gamelib/screen.c engine/source/gamelib/screen16.c \
 *   engine/source/gamelib/screen32.c engine/source/gamelib/draw.c \
 *   engine/source/gamelib/draw16.c engine/source/gamelib/draw32.c \
 *   engine/source/gamelib/sprite.c engine/source/gamelib/spritex8p16.c \
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

    /* Only RGB32 images participate in this fixture's native path. Other
     * formats exercise the production software fallback without a cache slot. */
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
 * Simulate native clear without reading the prior GPU image to the CPU.
 */
static int mock_clear(s_screen *screen)
{
    mock_image *image = mock_find(screen, 1);

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
    assert(released_screens == 4);
    for(i = 0; i < MOCK_SCREEN_CAPACITY; ++i)
    {
        assert(!images[i].screen);
    }
    assert(native_draws > 0);
    gpu_unregister_backend();
    assert(!gpu_is_active());

    /* No backend remains a valid, fully functional software configuration. */
    small = test_screen(4, 4, 0);
    putbox(0, 0, 4, 4, 0x654321, small, NULL);
    assert(((unsigned *)small->data)[15] == 0x654321);
    freescreen(&small);
    puts("GPU coherence and queue capture tests passed.");
    return 0;
}
