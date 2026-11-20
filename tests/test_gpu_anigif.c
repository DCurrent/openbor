/*
 * Damon V. Caskey
 * 2026-11-20
 * Decode deterministic GIF streams through the production decoder and native
 * renderer. Independent expected images cover palette lookup, partial frames,
 * interlace and the engine's three-file RGB OR convention. CPU reads occur
 * only at explicit verification or simulated script-edit boundaries.
 */
#include <assert.h>
#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <SDL.h>
#include "gpu.h"
#include "anigif.h"
#include "gpu_renderer.h"
#include "gpu_test_context.h"
#include "packfile.h"
#include "screen.h"
#include "fixtures/anigif_lzw_fixture.h"

enum
{
    GIF_TEST_WIDTH = 8,
    GIF_TEST_HEIGHT = 5,
    GIF_TEST_PIXELS = 1024,
    GIF_TEST_PALETTE_ENTRIES = 4,
    GIF_TEST_FILES = 12,
    GIF_TEST_HANDLES = 12,
    GIF_TEST_FILE_BYTES = 4096,
    GIF_TEST_NAME_BYTES = 64,
    GIF_TEST_SUBBLOCK_BYTES = 255,
    GIF_TEST_CLEAR = 4,
    GIF_TEST_END = 5,
    GIF_TEST_CODE_BITS = 3,
    GIF_TEST_BACKGROUND = 0x0038597b,
    GIF_TEST_CPU_EDIT = 0x00a53572,
    GIF_TEST_GPU_EDIT = 0x004ab691
};

const s_drawmethod plainmethod = {
    .config = DRAWMETHOD_CONFIG_ENABLED,
    .alpha = -1, .remap = -1,
    .scalex = 256, .scaley = 256,
    .xrepeat = 1, .yrepeat = 1,
    .channelr = 255, .channelg = 255, .channelb = 255,
    .affine = {1, 0, 0, 1, 0, 0},
    .object_type = OBJECT_TYPE_DRAWMETHOD
};

typedef struct gif_test_file
{
    char name[GIF_TEST_NAME_BYTES];
    unsigned char data[GIF_TEST_FILE_BYTES];
    size_t length;
} gif_test_file;

typedef struct gif_test_handle
{
    gif_test_file *file;
    size_t position;
} gif_test_handle;

typedef struct gif_test_bits
{
    unsigned char data[GIF_TEST_FILE_BYTES];
    size_t count;
    unsigned pending;
    unsigned used;
} gif_test_bits;

static gif_test_file gif_files[GIF_TEST_FILES];
static gif_test_handle gif_handles[GIF_TEST_HANDLES];
static unsigned gif_file_count;
static unsigned gif_images_checked;
static unsigned gif_pixels_checked;

/*
 * Damon V. Caskey
 * 2026-11-20
 * Retain fatal allocation checks without linking game startup or its logger.
 */
void *checkAlloc(void *pointer, size_t size, const char *function,
                 const char *file, int line)
{
    (void)size; (void)function; (void)file; (void)line;
    assert(pointer);
    return pointer;
}

/*
 * Damon V. Caskey
 * 2026-11-20
 * Supply the packfile interface with independent stream positions, including
 * the transient opens used to probe the three-file RGB naming convention.
 */
int openpackfile(const char *filename, const char *packfilename)
{
    unsigned file, handle;
    (void)packfilename;
    for(file = 0; file < gif_file_count; ++file)
    {
        if(strcmp(filename, gif_files[file].name)) continue;
        for(handle = 0; handle < GIF_TEST_HANDLES; ++handle)
        {
            if(gif_handles[handle].file) continue;
            gif_handles[handle].file = &gif_files[file];
            gif_handles[handle].position = 0;
            return (int)handle;
        }
    }
    return -1;
}

/*
 * Damon V. Caskey
 * 2026-11-20
 * Preserve short-read and EOF behavior so malformed decoder fixtures exercise
 * the production error path instead of receiving synthetic complete input.
 */
int readpackfile(int handle, void *buffer, int length)
{
    gif_test_handle *stream;
    size_t available;
    if(handle < 0 || handle >= GIF_TEST_HANDLES || length < 0) return -1;
    stream = &gif_handles[handle];
    if(!stream->file) return -1;
    available = stream->file->length - stream->position;
    if((size_t)length > available) length = (int)available;
    memcpy(buffer, stream->file->data + stream->position, (size_t)length);
    stream->position += (size_t)length;
    return length;
}

/*
 * Damon V. Caskey
 * 2026-11-20
 * Failed probes retain the packfile error result and never close another
 * stream accidentally. Final fixture cleanup requires every handle released.
 */
int closepackfile(int handle)
{
    if(handle < 0 || handle >= GIF_TEST_HANDLES || !gif_handles[handle].file) return -1;
    memset(&gif_handles[handle], 0, sizeof(gif_handles[handle]));
    return 0;
}

/*
 * Damon V. Caskey
 * 2026-11-20
 * The small fixture transport retains valid SEEK_SET/CUR/END semantics while
 * refusing positions outside the supplied stream.
 */
int seekpackfile(int handle, int offset, int whence)
{
    gif_test_handle *stream;
    int64_t position;
    if(handle < 0 || handle >= GIF_TEST_HANDLES) return -1;
    stream = &gif_handles[handle];
    if(!stream->file) return -1;
    if(whence == SEEK_SET) position = offset;
    else if(whence == SEEK_CUR) position = (int64_t)stream->position + offset;
    else if(whence == SEEK_END) position = (int64_t)stream->file->length + offset;
    else return -1;
    if(position < 0 || position > (int64_t)stream->file->length) return -1;
    stream->position = (size_t)position;
    return (int)position;
}

/*
 * Damon V. Caskey
 * 2026-11-20
 * Build expected RGB32 values independently from the decoder palette routine.
 */
static unsigned gif_rgb(unsigned red, unsigned green, unsigned blue)
{
#if REVERSE_COLOR
    return (red << 16) | (green << 8) | blue;
#else
    return red | (green << 8) | (blue << 16);
#endif
}

/*
 * Damon V. Caskey
 * 2026-11-20
 * Starting another fixture also verifies all prior stream owners were closed.
 */
static void gif_fixture_reset(void)
{
    unsigned handle;
    for(handle = 0; handle < GIF_TEST_HANDLES; ++handle)
        assert(!gif_handles[handle].file);
    memset(gif_files, 0, sizeof(gif_files));
    gif_file_count = 0;
}

/*
 * Damon V. Caskey
 * 2026-11-20
 * Append explicit GIF wire bytes with bounds checked by the test builder.
 */
static void gif_byte(gif_test_file *file, unsigned value)
{
    assert(file->length < sizeof(file->data));
    file->data[file->length++] = (unsigned char)value;
}

/*
 * Damon V. Caskey
 * 2026-11-20
 * Write little-endian descriptor words independently of the decoder structs.
 */
static void gif_word(gif_test_file *file, unsigned value)
{
    gif_byte(file, value);
    gif_byte(file, value >> 8);
}

/*
 * Damon V. Caskey
 * 2026-11-20
 * Store four RGB triplets without applying the engine's forced-black index-0
 * convention; expected images apply that rule explicitly where appropriate.
 */
static void gif_palette(gif_test_file *file, const unsigned *palette)
{
    unsigned entry;
    for(entry = 0; entry < GIF_TEST_PALETTE_ENTRIES; ++entry)
    {
#if REVERSE_COLOR
        gif_byte(file, palette[entry] >> 16);
        gif_byte(file, palette[entry] >> 8);
        gif_byte(file, palette[entry]);
#else
        gif_byte(file, palette[entry]);
        gif_byte(file, palette[entry] >> 8);
        gif_byte(file, palette[entry] >> 16);
#endif
    }
}

/*
 * Damon V. Caskey
 * 2026-11-20
 * Declare a complete logical screen with a four-entry global color table.
 */
static gif_test_file *gif_file(const char *name, int width, int height,
                               const unsigned *palette)
{
    static const char signature[] = "GIF89a";
    gif_test_file *file;
    unsigned byte;
    assert(gif_file_count < GIF_TEST_FILES);
    file = &gif_files[gif_file_count++];
    assert(strlen(name) < sizeof(file->name));
    strcpy(file->name, name);
    for(byte = 0; byte < sizeof(signature) - 1; ++byte) gif_byte(file, signature[byte]);
    gif_word(file, (unsigned)width);
    gif_word(file, (unsigned)height);
    gif_byte(file, 0x81); /* Global table present, four entries. */
    gif_byte(file, 0);
    gif_byte(file, 0);
    gif_palette(file, palette);
    return file;
}

/*
 * Damon V. Caskey
 * 2026-11-20
 * Pack independent LSB-first codes; explicit widths permit a hand-authored
 * dictionary-growth case in addition to reset-before-literal fixture streams.
 */
static void gif_bits(gif_test_bits *bits, unsigned code, unsigned width)
{
    bits->pending |= code << bits->used;
    bits->used += width;
    while(bits->used >= 8)
    {
        assert(bits->count < sizeof(bits->data));
        bits->data[bits->count++] = (unsigned char)bits->pending;
        bits->pending >>= 8;
        bits->used -= 8;
    }
}

/*
 * Damon V. Caskey
 * 2026-11-20
 * Finish byte packing and split payload into GIF data subblocks. Small blocks
 * deliberately make code words cross subblock boundaries.
 */
static void gif_data(gif_test_file *file, gif_test_bits *bits)
{
    size_t start = 0;
    if(bits->used)
    {
        assert(bits->count < sizeof(bits->data));
        bits->data[bits->count++] = (unsigned char)bits->pending;
    }
    gif_byte(file, 2); /* Minimum LZW code size for the four-entry palette. */
    while(start < bits->count)
    {
        size_t block = bits->count - start;
        size_t byte;
        if(block > GIF_TEST_CODE_BITS) block = GIF_TEST_CODE_BITS;
        assert(block <= GIF_TEST_SUBBLOCK_BYTES);
        gif_byte(file, (unsigned)block);
        for(byte = 0; byte < block; ++byte) gif_byte(file, bits->data[start + byte]);
        start += block;
    }
    gif_byte(file, 0);
}

/*
 * Damon V. Caskey
 * 2026-11-20
 * Emit graphic control and image records as bytes, retaining ignored disposal
 * bits so the migration must preserve OpenBOR's existing accumulation policy.
 */
static void gif_frame_header(gif_test_file *file, int left, int top, int width,
                              int height, int transparent, int delay,
                              int disposal, int interlace, const unsigned *palette)
{
    gif_byte(file, '!'); gif_byte(file, 0xf9); gif_byte(file, 4);
    gif_byte(file, (unsigned)((disposal << 2) | (transparent >= 0)));
    gif_word(file, (unsigned)delay);
    gif_byte(file, transparent >= 0 ? (unsigned)transparent : 0);
    gif_byte(file, 0);
    gif_byte(file, ',');
    gif_word(file, (unsigned)left); gif_word(file, (unsigned)top);
    gif_word(file, (unsigned)width); gif_word(file, (unsigned)height);
    gif_byte(file, (palette ? 0x81u : 0u) | (interlace ? 0x40u : 0u));
    if(palette) gif_palette(file, palette);
}

/*
 * Damon V. Caskey
 * 2026-11-20
 * Reset before each literal, keeping fixture encoding deliberately simpler
 * than the production dictionary decoder. Row order is supplied explicitly.
 */
static void gif_frame(gif_test_file *file, int left, int top, int width, int height,
                       int transparent, int delay, int disposal, int interlace,
                       const unsigned *palette, const unsigned char *indices, size_t count)
{
    gif_test_bits bits = {{0}, 0, 0, 0};
    size_t pixel;
    gif_frame_header(file, left, top, width, height, transparent, delay,
                     disposal, interlace, palette);
    for(pixel = 0; pixel < count; ++pixel)
    {
        assert(indices[pixel] < GIF_TEST_PALETTE_ENTRIES);
        gif_bits(&bits, GIF_TEST_CLEAR, GIF_TEST_CODE_BITS);
        gif_bits(&bits, indices[pixel], GIF_TEST_CODE_BITS);
    }
    gif_bits(&bits, GIF_TEST_END, GIF_TEST_CODE_BITS);
    gif_data(file, &bits);
}

/*
 * Damon V. Caskey
 * 2026-11-20
 * Assert exact RGB bytes, reporting the first mismatching coordinate. Only
 * this observation boundary explicitly requests the GPU's current pixels.
 */
static void gif_verify(s_screen *screen, const unsigned *expected, const char *label)
{
    int pixel, count = screen->width * screen->height;
    const unsigned *actual;
    assert(count <= GIF_TEST_PIXELS);
    gpu_sync_cpu(screen);
    actual = (const unsigned *)screen->data;
    for(pixel = 0; pixel < count; ++pixel)
    {
        if(actual[pixel] != expected[pixel])
            fprintf(stderr, "%s (%d,%d): expected %08x, got %08x\n", label,
                    pixel % screen->width, pixel / screen->width,
                    expected[pixel], actual[pixel]);
        assert(actual[pixel] == expected[pixel]);
    }
    ++gif_images_checked;
    gif_pixels_checked += (unsigned)count;
}

/*
 * Damon V. Caskey
 * 2026-11-20
 * Expected composition is a clipped replacement with one optional skipped
 * source index. It does not call decoder or renderer mapping helpers.
 */
static void gif_expected(unsigned *expected, int width, int height,
                          const unsigned char *indices, const unsigned *palette,
                          int left, int top, int frame_width, int frame_height,
                          int transparent)
{
    int x, y;
    for(y = 0; y < height; ++y)
        for(x = 0; x < width; ++x)
        {
            int64_t sx = (int64_t)x - left;
            int64_t sy = (int64_t)y - top;
            unsigned index;
            if(sx < 0 || sy < 0 || sx >= frame_width || sy >= frame_height) continue;
            index = indices[sy * frame_width + sx];
            if((int)index != transparent) expected[y * width + x] = palette[index];
        }
}

/*
 * Damon V. Caskey
 * 2026-11-20
 * Frame decode may update resident textures, but must not read pixels back or
 * write the RGB32 CPU shadow while performing native composition.
 */
static int gif_decode_native(anigif_info *info)
{
    unsigned shadow[GIF_TEST_PIXELS];
    s_screen *screen = anigif_getbuffer(info);
    size_t bytes = (size_t)screen->width * screen->height * sizeof(unsigned);
    gpu_renderer_stats before = *gpu_gl_get_stats();
    int done;
    assert(bytes <= sizeof(shadow));
    memcpy(shadow, screen->data, bytes);
    done = anigif_decode_frame(info);
    assert(gpu_gl_get_stats()->readbacks == before.readbacks);
    assert(gpu_gl_get_stats()->fallback_draws == before.fallback_draws);
    assert(!memcmp(shadow, screen->data, bytes));
    return done;
}

/*
 * Damon V. Caskey
 * 2026-11-20
 * Direct frame descriptors prove index zero is not intrinsically transparent,
 * arbitrary transparency works, odd rows and clipped uploads preserve phase,
 * and mutable palette addresses are matched by contents.
 */
static void gif_test_native_frames(void)
{
    unsigned palette[GPU_PALETTE_COLORS], expected[GIF_TEST_PIXELS];
    const unsigned char indices[] = {0, 1, 255, 128, 0, 2, 3, 255, 1};
    const struct {int x, y, width, height, transparent;} cases[] = {
        {2, 1, 3, 2, 255}, {-1, -1, 3, 3, 128}, {7, 4, 3, 3, -1},
        {0, 0, 1, 3, -1}, {INT_MIN, 0, 3, 3, -1}, {INT_MAX, 0, 3, 3, -1}
    };
    s_screen *target = allocscreen(GIF_TEST_WIDTH, GIF_TEST_HEIGHT, PIXEL_32);
    unsigned entry, test, pixel;
    uint64_t resources = gpu_gl_get_stats()->gif_frame_resources;
    assert(target);
    for(entry = 0; entry < GPU_PALETTE_COLORS; ++entry)
        palette[entry] = gif_rgb((entry * 37 + 11) & 255,
                                 (entry * 61 + 19) & 255, (entry * 23 + 41) & 255);
    for(test = 0; test < sizeof(cases) / sizeof(cases[0]); ++test)
    {
        gpu_indexed_frame frame = {indices, (const unsigned char *)palette,
            cases[test].width, cases[test].height, cases[test].x, cases[test].y,
            cases[test].transparent};
        gpu_renderer_stats before;
        unsigned shadow[GIF_TEST_PIXELS];
        for(pixel = 0; pixel < GIF_TEST_WIDTH * GIF_TEST_HEIGHT; ++pixel)
            expected[pixel] = GIF_TEST_BACKGROUND;
        assert(gpu_draw_box(target, 0, 0, GIF_TEST_WIDTH, GIF_TEST_HEIGHT,
                            GIF_TEST_BACKGROUND, &plainmethod));
        gif_expected(expected, target->width, target->height, indices, palette,
                      frame.x, frame.y, frame.width, frame.height, frame.transparent);
        memcpy(shadow, target->data, sizeof(unsigned) * GIF_TEST_WIDTH * GIF_TEST_HEIGHT);
        before = *gpu_gl_get_stats();
        assert(gpu_compose_indexed(target, &frame));
        assert(gpu_gl_get_stats()->readbacks == before.readbacks);
        assert(gpu_gl_get_stats()->fallback_draws == before.fallback_draws);
        assert(!memcmp(shadow, target->data, sizeof(unsigned) * GIF_TEST_WIDTH * GIF_TEST_HEIGHT));
        gif_verify(target, expected, "indexed rectangle clipping/transparency");
    }
    {
        gpu_indexed_frame frame = {indices, (const unsigned char *)palette, 3, 2, 1, 1, -1};
        assert(gpu_compose_indexed(target, &frame));
        palette[0] = gif_rgb(250, 7, 191);
        palette[1] = gif_rgb(9, 229, 74);
        assert(gpu_compose_indexed(target, &frame));
        gif_expected(expected, target->width, target->height, indices, palette, 1, 1, 3, 2, -1);
        gif_verify(target, expected, "same-address palette content replacement");
        frame.width = 0;
        assert(!gpu_compose_indexed(target, &frame));
        frame.width = 3; frame.transparent = GPU_PALETTE_COLORS;
        assert(!gpu_compose_indexed(target, &frame));
        frame.transparent = -1; frame.indices = NULL;
        assert(!gpu_compose_indexed(target, &frame));
        gif_verify(target, expected, "invalid frame leaves destination unchanged");
    }
    {
        unsigned char full[GIF_TEST_WIDTH * GIF_TEST_HEIGHT];
        gpu_indexed_frame frame = {full, (const unsigned char *)palette,
            GIF_TEST_WIDTH, GIF_TEST_HEIGHT, 0, 0, -1};
        gpu_renderer_stats before;
        memset(full, 1, sizeof(full));
        gpu_prepare_cpu_write(target);
        for(pixel = 0; pixel < GIF_TEST_WIDTH * GIF_TEST_HEIGHT; ++pixel)
            ((unsigned *)target->data)[pixel] = GIF_TEST_CPU_EDIT;
        before = *gpu_gl_get_stats();
        assert(gpu_compose_indexed(target, &frame));
        assert(gpu_gl_get_stats()->readbacks == before.readbacks);
        assert(gpu_gl_get_stats()->fallback_draws == before.fallback_draws);
        for(pixel = 0; pixel < GIF_TEST_WIDTH * GIF_TEST_HEIGHT; ++pixel)
        {
            assert(((unsigned *)target->data)[pixel] == GIF_TEST_CPU_EDIT);
            expected[pixel] = palette[1];
        }
        gif_verify(target, expected, "full replacement after CPU edit stays native");
    }
    freescreen(&target);
    assert(gpu_gl_get_stats()->gif_frame_resources == resources);
}

/*
 * Damon V. Caskey
 * 2026-11-20
 * Preserve standard GIF black-index-0 output, local/global palette transitions,
 * timing, ignored disposal and previously drawn script pixels between frames.
 */
static void gif_test_decoder_frames(void)
{
    unsigned global[] = {gif_rgb(222, 11, 33), gif_rgb(17, 83, 201),
                          gif_rgb(192, 7, 33), gif_rgb(45, 159, 2)};
    unsigned local[] = {gif_rgb(81, 23, 251), gif_rgb(237, 123, 14),
                         gif_rgb(8, 9, 10), gif_rgb(31, 181, 229)};
    unsigned global_expected[GIF_TEST_PALETTE_ENTRIES], local_expected[GIF_TEST_PALETTE_ENTRIES];
    unsigned expected[GIF_TEST_PIXELS] = {0};
    unsigned char initial[GIF_TEST_WIDTH * GIF_TEST_HEIGHT];
    const unsigned char patch[] = {0, 1, 2, 2, 3, 0};
    const unsigned char clipped[] = {1, 2, 3, 3, 1, 2};
    anigif_info info;
    gif_test_file *file;
    s_screen *screen;
    unsigned pixel;
    uint64_t resources = gpu_gl_get_stats()->gif_frame_resources;
    gif_fixture_reset();
    file = gif_file("single.gif", GIF_TEST_WIDTH, GIF_TEST_HEIGHT, global);
    for(pixel = 0; pixel < sizeof(initial); ++pixel)
        initial[pixel] = (unsigned char)((pixel % GIF_TEST_WIDTH + 2 * (pixel / GIF_TEST_WIDTH)) % GIF_TEST_PALETTE_ENTRIES);
    gif_frame(file, 0, 0, GIF_TEST_WIDTH, GIF_TEST_HEIGHT, -1, 3, 0, 0,
               NULL, initial, sizeof(initial));
    gif_frame(file, 2, 1, 3, 2, 2, 7, 2, 0, local, patch, sizeof(patch));
    gif_frame(file, 7, 3, 3, 2, 3, 11, 3, 0, NULL, clipped, sizeof(clipped));
    gif_byte(file, ';');
    memcpy(global_expected, global, sizeof(global)); global_expected[0] = 0;
    memcpy(local_expected, local, sizeof(local)); local_expected[0] = 0;
    assert(anigif_open("single.gif", "fixture", &info));
    assert(!info.isRGB && info.frame == -1);
    screen = anigif_getbuffer(&info);
    assert(screen && screen->pixelformat == PIXEL_32);
    assert(!gif_decode_native(&info));
    assert(info.frame == 0 && info.info[0].nextframe == 30);
    gif_expected(expected, screen->width, screen->height, initial, global_expected,
                  0, 0, GIF_TEST_WIDTH, GIF_TEST_HEIGHT, -1);
    gif_verify(screen, expected, "global palette and opaque black index zero");
    assert(gpu_draw_box(screen, 0, 0, 1, 1, GIF_TEST_GPU_EDIT, &plainmethod));
    expected[0] = GIF_TEST_GPU_EDIT;
    assert(!gif_decode_native(&info));
    assert(info.frame == 1 && info.info[0].nextframe == 100);
    gif_expected(expected, screen->width, screen->height, patch, local_expected,
                  2, 1, 3, 2, 2);
    gif_verify(screen, expected, "local palette partial frame preserves GPU edits");
    gpu_prepare_cpu_write(screen);
    ((unsigned *)screen->data)[1] = GIF_TEST_CPU_EDIT;
    expected[1] = GIF_TEST_CPU_EDIT;
    assert(!gif_decode_native(&info));
    assert(info.frame == 2 && info.info[0].nextframe == 210);
    gif_expected(expected, screen->width, screen->height, clipped, global_expected,
                  7, 3, 3, 2, 3);
    gif_verify(screen, expected, "global palette restored, clipping and CPU edits retained");
    assert(gif_decode_native(&info));
    assert(info.frame == 2 && info.done);
    anigif_close(&info);
    anigif_close(&info);
    assert(gpu_gl_get_stats()->gif_frame_resources == resources);
}

/*
 * Damon V. Caskey
 * 2026-11-20
 * Enumerate interlaced row classes independently, including tiny heights whose
 * empty passes must be skipped before consuming the next source sample.
 */
static void gif_test_interlace(void)
{
    const int heights[] = {1, 2, 3, 5, 7, 9};
    unsigned palette[] = {0, gif_rgb(193, 17, 91), gif_rgb(23, 211, 62), gif_rgb(43, 77, 229)};
    unsigned test;
    for(test = 0; test < sizeof(heights) / sizeof(heights[0]); ++test)
    {
        unsigned expected[GIF_TEST_PIXELS] = {0};
        unsigned char logical[GIF_TEST_PIXELS], interlaced[GIF_TEST_PIXELS];
        int x, y, pass, count = 0, height = heights[test];
        anigif_info info;
        gif_test_file *file;
        gif_fixture_reset();
        file = gif_file("interlace.gif", GIF_TEST_WIDTH, height + 2, palette);
        for(y = 0; y < height; ++y)
            for(x = 0; x < 3; ++x) logical[y * 3 + x] = (unsigned char)(1 + ((x + y * 2) % 3));
        for(pass = 0; pass < 4; ++pass)
            for(y = 0; y < height; ++y)
            {
                int belongs = (pass == 0 && y % 8 == 0) || (pass == 1 && y % 8 == 4)
                    || (pass == 2 && y % 4 == 2) || (pass == 3 && y % 2 == 1);
                if(belongs)
                    for(x = 0; x < 3; ++x) interlaced[count++] = logical[y * 3 + x];
            }
        assert(count == 3 * height);
        gif_frame(file, 1, 1, 3, height, -1, 1, 0, 1, NULL, interlaced, (size_t)count);
        gif_byte(file, ';');
        assert(anigif_open("interlace.gif", "fixture", &info));
        assert(!gif_decode_native(&info));
        gif_expected(expected, GIF_TEST_WIDTH, height + 2, logical, palette, 1, 1, 3, height, -1);
        gif_verify(anigif_getbuffer(&info), expected, "interlace pass rows and narrow frame");
        anigif_close(&info);
    }
}

/*
 * Damon V. Caskey
 * 2026-11-20
 * This independent hand-coded LZW stream expands to forty index-2 pixels.
 * Codes 6 through 12 use the special next-code case and cross the 3/4-bit
 * code-width boundary; code 8 then repeats four pixels from the dictionary.
 */
static void gif_test_dictionary(void)
{
    static const struct {unsigned code, width;} codes[] = {
        {4, 3}, {2, 3}, {6, 3}, {7, 3}, {8, 4}, {9, 4},
        {10, 4}, {11, 4}, {12, 4}, {8, 4}, {5, 4}
    };
    unsigned palette[] = {0, gif_rgb(71, 17, 191), gif_rgb(227, 93, 34), gif_rgb(7, 219, 41)};
    unsigned expected[GIF_TEST_PIXELS];
    gif_test_bits bits = {{0}, 0, 0, 0};
    gif_test_file *file;
    anigif_info info;
    unsigned code, pixel;
    gif_fixture_reset();
    file = gif_file("dictionary.gif", GIF_TEST_WIDTH, GIF_TEST_HEIGHT, palette);
    gif_frame_header(file, 0, 0, GIF_TEST_WIDTH, GIF_TEST_HEIGHT, -1, 1, 0, 0, NULL);
    for(code = 0; code < sizeof(codes) / sizeof(codes[0]); ++code)
        gif_bits(&bits, codes[code].code, codes[code].width);
    gif_data(file, &bits);
    gif_byte(file, ';');
    assert(anigif_open("dictionary.gif", "fixture", &info));
    assert(!gif_decode_native(&info));
    for(pixel = 0; pixel < GIF_TEST_WIDTH * GIF_TEST_HEIGHT; ++pixel) expected[pixel] = palette[2];
    gif_verify(anigif_getbuffer(&info), expected, "LZW dictionary expansion and code-width growth");
    anigif_close(&info);
}

/*
 * Damon V. Caskey
 * 2026-11-20
 * Exercise an independent encoder's dictionary references and 9-to-10-bit
 * code growth. The expected image follows the source pattern's LCG and RGB
 * palette definitions, without consulting any compressed input bytes.
 */
static void gif_test_independent_encoder(void)
{
    unsigned expected[GIF_TEST_PIXELS];
    uint32_t state = UINT32_C(0x13579bdf);
    unsigned pixel;
    gif_test_file *file;
    anigif_info info;
    gif_fixture_reset();
    assert(gif_file_count < GIF_TEST_FILES);
    file = &gif_files[gif_file_count++];
    strcpy(file->name, "independent.gif");
    assert(sizeof(gif_lzw_fixture) <= sizeof(file->data));
    memcpy(file->data, gif_lzw_fixture, sizeof(gif_lzw_fixture));
    file->length = sizeof(gif_lzw_fixture);
    for(pixel = 0; pixel < GIF_LZW_FIXTURE_WIDTH * GIF_LZW_FIXTURE_HEIGHT; ++pixel)
    {
        unsigned index;
        state = state * UINT32_C(1664525) + UINT32_C(1013904223);
        index = (state >> 24) & 63;
        expected[pixel] = index ? gif_rgb((index * 37 + 13) & 255,
                                           (index * 61 + 7) & 255,
                                           (index * 19 + 101) & 255) : 0;
    }
    assert(anigif_open("independent.gif", "fixture", &info));
    assert(!gif_decode_native(&info));
    gif_verify(anigif_getbuffer(&info), expected, "independently encoded GIF dictionary growth");
    assert(gif_decode_native(&info));
    anigif_close(&info);
}

/*
 * Damon V. Caskey
 * 2026-11-20
 * GIF-private canvases preserve the logical width instead of inheriting the
 * general allocscreen rounding rule. One- and three-pixel images remain whole.
 */
static void gif_test_narrow_canvases(void)
{
    const int widths[] = {1, 3};
    unsigned palette[] = {0, gif_rgb(13, 137, 221), gif_rgb(251, 87, 31), gif_rgb(31, 209, 59)};
    unsigned test;
    for(test = 0; test < sizeof(widths) / sizeof(widths[0]); ++test)
    {
        unsigned expected[GIF_TEST_PIXELS] = {0};
        unsigned char indices[GIF_TEST_PIXELS];
        int width = widths[test], pixel;
        gif_test_file *file;
        anigif_info info;
        gif_fixture_reset();
        file = gif_file("narrow.gif", width, GIF_TEST_HEIGHT, palette);
        for(pixel = 0; pixel < width * GIF_TEST_HEIGHT; ++pixel)
            indices[pixel] = (unsigned char)(1 + pixel % 3);
        gif_frame(file, 0, 0, width, GIF_TEST_HEIGHT, -1, 1, 0, 0,
                   NULL, indices, (size_t)width * GIF_TEST_HEIGHT);
        gif_byte(file, ';');
        assert(anigif_open("narrow.gif", "fixture", &info));
        assert(anigif_getbuffer(&info)->width == width);
        assert(!gif_decode_native(&info));
        gif_expected(expected, width, GIF_TEST_HEIGHT, indices, palette,
                      0, 0, width, GIF_TEST_HEIGHT, -1);
        gif_verify(anigif_getbuffer(&info), expected, "narrow GIF logical canvas");
        anigif_close(&info);
    }
}

/*
 * Damon V. Caskey
 * 2026-11-20
 * Malformed data after a valid frame cannot publish partially decoded pixels.
 * Cases distinguish early end-code, missing terminator and invalid dictionary
 * reference, while the last valid GPU-owned frame remains readable.
 */
static void gif_test_malformed(void)
{
    unsigned palette[] = {0, gif_rgb(29, 163, 229), gif_rgb(203, 41, 71), gif_rgb(67, 197, 11)};
    unsigned char initial[GIF_TEST_WIDTH * GIF_TEST_HEIGHT];
    const unsigned char patch[] = {2, 3, 2, 3, 2, 3};
    unsigned variant, pixel;
    memset(initial, 1, sizeof(initial));
    for(variant = 0; variant < 3; ++variant)
    {
        unsigned expected[GIF_TEST_PIXELS];
        anigif_info info;
        gif_test_file *file;
        gpu_renderer_stats before;
        gif_fixture_reset();
        file = gif_file("broken.gif", GIF_TEST_WIDTH, GIF_TEST_HEIGHT, palette);
        gif_frame(file, 0, 0, GIF_TEST_WIDTH, GIF_TEST_HEIGHT, -1, 1, 0, 0, NULL, initial, sizeof(initial));
        if(variant < 2)
        {
            gif_frame(file, 2, 1, 3, 2, -1, 1, 0, 0, NULL, patch, variant == 0 ? 4 : sizeof(patch));
            if(variant == 1) file->length -= 2;
        }
        else
        {
            gif_test_bits bits = {{0}, 0, 0, 0};
            gif_frame_header(file, 2, 1, 3, 2, -1, 1, 0, 0, NULL);
            gif_bits(&bits, GIF_TEST_CLEAR, GIF_TEST_CODE_BITS);
            gif_bits(&bits, 7, GIF_TEST_CODE_BITS);
            gif_bits(&bits, GIF_TEST_END, GIF_TEST_CODE_BITS);
            gif_data(file, &bits);
        }
        assert(anigif_open("broken.gif", "fixture", &info));
        assert(!gif_decode_native(&info));
        before = *gpu_gl_get_stats();
        assert(gif_decode_native(&info));
        assert(info.frame == 0);
        assert(gpu_gl_get_stats()->gif_frame_uploads == before.gif_frame_uploads);
        assert(gpu_gl_get_stats()->gif_frame_draws == before.gif_frame_draws);
        for(pixel = 0; pixel < GIF_TEST_WIDTH * GIF_TEST_HEIGHT; ++pixel) expected[pixel] = palette[1];
        gif_verify(anigif_getbuffer(&info), expected, "malformed frame leaves last valid image intact");
        anigif_close(&info);
    }
}

/*
 * Damon V. Caskey
 * 2026-11-20
 * The named RGB streams are merged by bytewise OR, including non-disjoint
 * channel values. Both GPU and CPU script edits must survive transparent
 * updates, and each stream keeps its own accumulated frame delay.
 */
static void gif_test_rgb_streams(void)
{
    const char *names[] = {"tripr.gif", "tripg.gif", "tripb.gif"};
    unsigned palettes[3][GIF_TEST_PALETTE_ENTRIES] = {
        {0, 0x00902451, 0x00051284, 0},
        {0, 0x00088012, 0x0082210a, 0},
        {0, 0x00054384, 0x00101448, 0}
    };
    unsigned char initial[GIF_TEST_WIDTH * GIF_TEST_HEIGHT];
    const unsigned char patch[] = {2, 3};
    unsigned expected[GIF_TEST_PIXELS];
    unsigned channel, pixel;
    anigif_info info;
    uint64_t resources = gpu_gl_get_stats()->gif_frame_resources;
    gif_fixture_reset();
    memset(initial, 1, sizeof(initial));
    for(channel = 0; channel < 3; ++channel)
    {
        gif_test_file *file = gif_file(names[channel], GIF_TEST_WIDTH, GIF_TEST_HEIGHT, palettes[channel]);
        gif_frame(file, 0, 0, GIF_TEST_WIDTH, GIF_TEST_HEIGHT, -1, (int)channel + 2,
                   0, 0, NULL, initial, sizeof(initial));
        gif_frame(file, 0, 0, 2, 1, 3, (int)channel + 2, 0, 0, NULL, patch, sizeof(patch));
        gif_byte(file, ';');
    }
    assert(pixelformat == PIXEL_x8);
    assert(anigif_open("trip.gif", "fixture", &info));
    assert(info.isRGB);
    assert(!gif_decode_native(&info));
    for(pixel = 0; pixel < GIF_TEST_WIDTH * GIF_TEST_HEIGHT; ++pixel)
        expected[pixel] = palettes[0][1] | palettes[1][1] | palettes[2][1];
    gif_verify(anigif_getbuffer(&info), expected, "three-stream exact RGB OR");
    assert(gpu_draw_box(info.gifbuffer[1], 1, 0, 1, 1, GIF_TEST_GPU_EDIT, &plainmethod));
    gpu_prepare_cpu_write(info.gifbuffer[2]);
    ((unsigned *)info.gifbuffer[2]->data)[1] = GIF_TEST_CPU_EDIT;
    assert(!gif_decode_native(&info));
    expected[0] = palettes[0][2] | palettes[1][2] | palettes[2][2];
    expected[1] = palettes[0][1] | GIF_TEST_GPU_EDIT | GIF_TEST_CPU_EDIT;
    gif_verify(anigif_getbuffer(&info), expected, "RGB merge preserves edited channel images");
    for(channel = 0; channel < 3; ++channel)
        assert(info.info[channel].nextframe == (channel + 2) * 20);
    assert(gif_decode_native(&info));
    assert(info.frame == 1);
    anigif_close(&info);
    assert(gpu_gl_get_stats()->gif_frame_resources == resources);
}

/*
 * Damon V. Caskey
 * 2026-11-20
 * Direct merge supports a destination that aliases a source, with no CPU
 * synchronization and no accidental arithmetic addition of overlapping bits.
 */
static void gif_test_merge_alias(void)
{
    const unsigned colors[] = {0x00902451, 0x00088012, 0x00054384};
    s_screen *channels[3];
    unsigned expected[GIF_TEST_PIXELS], channel, pixel;
    gpu_renderer_stats before;
    for(channel = 0; channel < 3; ++channel)
    {
        channels[channel] = allocscreen(GIF_TEST_WIDTH, GIF_TEST_HEIGHT, PIXEL_32);
        assert(channels[channel]);
        assert(gpu_draw_box(channels[channel], 0, 0, GIF_TEST_WIDTH,
                            GIF_TEST_HEIGHT, colors[channel], &plainmethod));
    }
    before = *gpu_gl_get_stats();
    assert(gpu_merge_channels(channels[0], channels[0], channels[1], channels[2]));
    assert(gpu_gl_get_stats()->readbacks == before.readbacks);
    assert(gpu_gl_get_stats()->fallback_draws == before.fallback_draws);
    for(pixel = 0; pixel < GIF_TEST_WIDTH * GIF_TEST_HEIGHT; ++pixel)
        expected[pixel] = colors[0] | colors[1] | colors[2];
    gif_verify(channels[0], expected, "destination aliases RGB input");
    {
        s_screen *mismatched = allocscreen(GIF_TEST_WIDTH / 2, GIF_TEST_HEIGHT, PIXEL_32);
        assert(mismatched);
        assert(!gpu_merge_channels(channels[0], channels[0], channels[1], mismatched));
        gif_verify(channels[0], expected, "incompatible merge leaves target unchanged");
        freescreen(&mismatched);
    }
    for(channel = 0; channel < 3; ++channel) freescreen(&channels[channel]);
}

extern void (APIENTRY *ptr_glTexSubImage2D)(GLenum, GLint, GLint, GLint, GLsizei,
                                           GLsizei, GLenum, GLenum, const GLvoid *);
static void (APIENTRY *gif_real_upload)(GLenum, GLint, GLint, GLint, GLsizei,
                                        GLsizei, GLenum, GLenum, const GLvoid *);
static unsigned gif_fail_next_index_upload;

/*
 * Damon V. Caskey
 * 2026-11-20
 * Inject a real GL error into one index transfer, leaving palette uploads and
 * unrelated texture operations untouched.
 */
static void APIENTRY gif_fail_upload(GLenum target, GLint level, GLint x, GLint y,
                                      GLsizei width, GLsizei height, GLenum format,
                                      GLenum type, const GLvoid *pixels)
{
#if defined(OPENBOR_GLES)
    const GLenum index_format = GL_RED;
#else
    const GLenum index_format = GL_LUMINANCE;
#endif
    if(gif_fail_next_index_upload && format == index_format)
    {
        gif_fail_next_index_upload = 0;
        target = (GLenum)0xffffffffu;
    }
    gif_real_upload(target, level, x, y, width, height, format, type, pixels);
}

/*
 * Damon V. Caskey
 * 2026-11-20
 * A rejected transfer must leave the resident destination and CPU shadow
 * intact, then allow a clean retry and ordinary native draws afterward.
 */
static void gif_test_upload_failure(void)
{
    unsigned palette[GPU_PALETTE_COLORS] = {0};
    unsigned expected[GIF_TEST_PIXELS], pixel;
    unsigned char indices[] = {1, 1, 1, 1, 1, 1};
    s_screen *target = allocscreen(GIF_TEST_WIDTH, GIF_TEST_HEIGHT, PIXEL_32);
    gpu_indexed_frame frame = {indices, (const unsigned char *)palette, 3, 2, 2, 1, -1};
    gpu_renderer_stats before;
    assert(target);
    palette[1] = gif_rgb(249, 31, 147);
    assert(gpu_draw_box(target, 0, 0, GIF_TEST_WIDTH, GIF_TEST_HEIGHT, GIF_TEST_BACKGROUND, &plainmethod));
    for(pixel = 0; pixel < GIF_TEST_WIDTH * GIF_TEST_HEIGHT; ++pixel) expected[pixel] = GIF_TEST_BACKGROUND;
    gif_real_upload = ptr_glTexSubImage2D;
    assert(gif_real_upload);
    ptr_glTexSubImage2D = gif_fail_upload;
    gif_fail_next_index_upload = 1;
    before = *gpu_gl_get_stats();
    assert(!gpu_compose_indexed(target, &frame));
    assert(!gif_fail_next_index_upload);
    assert(glGetError() == GL_NO_ERROR);
    assert(gpu_gl_get_stats()->gif_frame_draws == before.gif_frame_draws);
    assert(gpu_gl_get_stats()->readbacks == before.readbacks);
    gif_verify(target, expected, "rejected indexed upload preserves destination");
    ptr_glTexSubImage2D = gif_real_upload;
    assert(gpu_compose_indexed(target, &frame));
    gif_expected(expected, GIF_TEST_WIDTH, GIF_TEST_HEIGHT, indices, palette, 2, 1, 3, 2, -1);
    gif_verify(target, expected, "indexed transfer retries after rejection");
    assert(gpu_draw_box(target, 0, 0, 1, 1, GIF_TEST_GPU_EDIT, &plainmethod));
    expected[0] = GIF_TEST_GPU_EDIT;
    gif_verify(target, expected, "ordinary scene drawing after indexed failure");
    freescreen(&target);
}

/*
 * Damon V. Caskey
 * 2026-11-20
 * Live GIF canvases survive normal context retirement through the shared
 * coherence boundary. Transient indexed textures are discarded and recreated
 * when the next patch arrives, preserving pixels authored by scripts.
 */
static void gif_test_context_replacement(SDL_Window *window, SDL_GLContext *context)
{
    unsigned palette[] = {0, gif_rgb(31, 173, 89), gif_rgb(239, 17, 53), gif_rgb(71, 41, 227)};
    unsigned char initial[GIF_TEST_WIDTH * GIF_TEST_HEIGHT];
    const unsigned char patch[] = {2, 3, 2, 3, 2, 3};
    unsigned expected[GIF_TEST_PIXELS];
    unsigned pixel;
    gif_test_file *file;
    anigif_info info;
    gif_fixture_reset();
    memset(initial, 1, sizeof(initial));
    file = gif_file("context.gif", GIF_TEST_WIDTH, GIF_TEST_HEIGHT, palette);
    gif_frame(file, 0, 0, GIF_TEST_WIDTH, GIF_TEST_HEIGHT, -1, 1, 0, 0, NULL, initial, sizeof(initial));
    gif_frame(file, 2, 1, 3, 2, 3, 1, 0, 0, NULL, patch, sizeof(patch));
    gif_byte(file, ';');
    assert(anigif_open("context.gif", "fixture", &info));
    assert(!gif_decode_native(&info));
    assert(gpu_draw_box(anigif_getbuffer(&info), 0, 0, 1, 1, GIF_TEST_GPU_EDIT, &plainmethod));
    for(pixel = 0; pixel < GIF_TEST_WIDTH * GIF_TEST_HEIGHT; ++pixel) expected[pixel] = palette[1];
    expected[0] = GIF_TEST_GPU_EDIT;
    gpu_gl_shutdown();
    assert(gpu_gl_get_stats()->gif_frame_resources == 0);
    gif_verify(anigif_getbuffer(&info), expected, "context retirement preserves GIF canvas");
    SDL_GL_DeleteContext(*context);
    *context = SDL_GL_CreateContext(window);
    assert(*context);
    assert(LoadGLFunctions());
    assert(gpu_gl_init());
    assert(!gif_decode_native(&info));
    gif_expected(expected, GIF_TEST_WIDTH, GIF_TEST_HEIGHT, patch, palette, 2, 1, 3, 2, 3);
    gif_verify(anigif_getbuffer(&info), expected, "next frame resumes after context replacement");
    anigif_close(&info);
    assert(gpu_gl_get_stats()->gif_frame_resources == 0);
}

/*
 * Damon V. Caskey
 * 2026-11-20
 * Unavailable native rendering rejects GIF creation and releases every opened
 * stream. It never fills a CPU RGB32 image as an implicit rendering fallback.
 */
static void gif_test_unavailable_backend(void)
{
    unsigned palette[] = {0, 0x000001, 0x000100, 0x010000};
    anigif_info info;
    gif_fixture_reset();
    gif_file("inactive.gif", GIF_TEST_WIDTH, GIF_TEST_HEIGHT, palette);
    assert(!gpu_is_active());
    assert(!anigif_open("inactive.gif", "fixture", &info));
    assert(!info.backbuffer && !info.gifbuffer[0] && !info.gifbuffer[1] && !info.gifbuffer[2]);
    anigif_close(&info);
    gif_fixture_reset();
}

/*
 * Damon V. Caskey
 * 2026-11-20
 * Run exact native GIF contracts on desktop GL or the Android GLES profile.
 * Missing contexts are reported separately from failed rendering assertions.
 */
int main(void)
{
    SDL_Window *window;
    SDL_GLContext context;
    if(SDL_Init(SDL_INIT_VIDEO) != 0) return 77;
    gpu_test_context_attributes();
    window = SDL_CreateWindow("OpenBOR GIF composition tests", 0, 0, 64, 64,
                               SDL_WINDOW_OPENGL | SDL_WINDOW_HIDDEN);
    context = window ? SDL_GL_CreateContext(window) : NULL;
    if(!context)
    {
        if(window) SDL_DestroyWindow(window);
        SDL_Quit();
        return 77;
    }
    assert(LoadGLFunctions());
    assert(gpu_gl_init());
    gif_test_native_frames();
    gif_test_decoder_frames();
    gif_test_interlace();
    gif_test_dictionary();
    gif_test_independent_encoder();
    gif_test_narrow_canvases();
    gif_test_malformed();
    gif_test_rgb_streams();
    gif_test_merge_alias();
    gif_test_upload_failure();
    gif_test_context_replacement(window, &context);
    gif_fixture_reset();
    assert(gpu_gl_get_stats()->gif_frame_resources == 0);
    gpu_gl_shutdown();
    gif_test_unavailable_backend();
    SDL_GL_DeleteContext(context);
    SDL_DestroyWindow(window);
    SDL_Quit();
    printf("GPU animated GIF tests passed: %u exact images, %u pixels; indexed palettes, transparency, clipping, interlace, LZW, RGB OR, coherence and rejection.\n",
           gif_images_checked, gif_pixels_checked);
    return 0;
}
