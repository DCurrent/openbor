/*
 * OpenBOR - http://www.chronocrash.com
 * Copyright (c) OpenBOR Team. All rights reserved. See LICENSE.
 *
 * Damon V. Caskey
 * 2026-11-16
 * Exercise the Android launcher compositor in a genuine GLES2 context. Pixel
 * expectations expand the original font and source palettes independently;
 * observed driver calls detect unwanted CPU scene uploads and readbacks.
 */
#include <assert.h>
#include <limits.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "SDL.h"
#include "SDL_opengles2_khrplatform.h"
#include "SDL_opengles2_gl2platform.h"
#include "SDL_opengles2_gl2.h"
#include "gpu.h"
#include "gpu_launcher_android.h"
#include "gpu_output_filter_android.h"
#include "launcher_text.h"
#include "video.h"
#include "videocommon.h"
#include "output_filter_reference.h"
#define hankaku_font10 launcher_reference_font
#include "hankaku.h"
#undef hankaku_font10

int pixelbytes[5] = { 1, 1, 0, 0, 4 };
static unsigned warnings, readbacks, cpu_uploads, draws, font_uploads;
static unsigned texture_allocations, texture_deletions, images_checked, pixels_checked;
static unsigned shader_compilations, driver_lookups;
static unsigned renderer_creations;
static unsigned formatter_failures;
static size_t reject_malloc_size;
static int reject_shader, reject_link, reject_draw, reject_font, missing_function;
static int reject_present_copy;
static int draw_error_pending;
static void (GL_APIENTRY *real_get_shader)(GLuint, GLenum, GLint *);
static void (GL_APIENTRY *real_compile_shader)(GLuint);
static void (GL_APIENTRY *real_get_program)(GLuint, GLenum, GLint *);
static void (GL_APIENTRY *real_draw_arrays)(GLenum, GLint, GLsizei);
static void (GL_APIENTRY *real_texture_image)(GLenum, GLint, GLint, GLsizei,
    GLsizei, GLint, GLenum, GLenum, const void *);
static void (GL_APIENTRY *real_gen_textures)(GLsizei, GLuint *);
static void (GL_APIENTRY *real_delete_textures)(GLsizei, const GLuint *);
static GLenum (GL_APIENTRY *real_get_error)(void);
static void (GL_APIENTRY *get_integer)(GLenum, GLint *);
static void (GL_APIENTRY *get_boolean)(GLenum, GLboolean *);
static void (GL_APIENTRY *get_float)(GLenum, GLfloat *);
static void (GL_APIENTRY *get_attribute)(GLuint, GLenum, GLint *);
static void (GL_APIENTRY *get_pointer)(GLuint, GLenum, void **);
static GLboolean (GL_APIENTRY *is_enabled)(GLenum);
static void (GL_APIENTRY *active_texture)(GLenum);
static void (GL_APIENTRY *color_mask)(GLboolean, GLboolean, GLboolean, GLboolean);
static void (GL_APIENTRY *enable)(GLenum);
static void (GL_APIENTRY *disable)(GLenum);
static void (GL_APIENTRY *pixel_store)(GLenum, GLint);
static void (GL_APIENTRY *clear_color)(GLfloat, GLfloat, GLfloat, GLfloat);

void *__real_SDL_GL_GetProcAddress(const char *name);
void *__real_malloc(size_t size);
int __real_SDL_RenderReadPixels(SDL_Renderer *, const SDL_Rect *, Uint32, void *, int);
int __real_SDL_UpdateTexture(SDL_Texture *, const SDL_Rect *, const void *, int);
void __real_SDL_RenderPresent(SDL_Renderer *renderer);
int __real_SDL_RenderCopy(SDL_Renderer *, SDL_Texture *, const SDL_Rect *, const SDL_Rect *);
SDL_Renderer *__real_SDL_CreateRenderer(SDL_Window *, int, Uint32);

/* Damon V. Caskey - 2026-11-16: retain actual production diagnostics. */
void gpu_log(const char *format, ...)
{
    va_list arguments;
    va_start(arguments, format);
    vfprintf(stderr, format, arguments);
    fputc('\n', stderr);
    va_end(arguments);
    ++warnings;
}

/* Damon V. Caskey - 2026-11-16: target only one long formatting allocation. */
void *__wrap_malloc(size_t size)
{
    if(reject_malloc_size && size == reject_malloc_size)
    {
        ++formatter_failures;
        return NULL;
    }
    return __real_malloc(size);
}

/* Damon V. Caskey - 2026-11-16: reject a compiled real shader on request. */
static void GL_APIENTRY shader_status(GLuint shader, GLenum name, GLint *value)
{
    real_get_shader(shader, name, value);
    if(reject_shader && name == GL_COMPILE_STATUS) *value = GL_FALSE;
}

/* Damon V. Caskey - 2026-11-16: verify rejected startup does not recompile per frame. */
static void GL_APIENTRY compile_shader(GLuint shader)
{
    ++shader_compilations;
    real_compile_shader(shader);
}

/* Damon V. Caskey - 2026-11-16: reject the real program's link boundary. */
static void GL_APIENTRY program_status(GLuint program, GLenum name, GLint *value)
{
    real_get_program(program, name, value);
    if(reject_link && name == GL_LINK_STATUS) *value = GL_FALSE;
}

/* Damon V. Caskey - 2026-11-16: count native batches and fail one submission. */
static void GL_APIENTRY draw_arrays(GLenum mode, GLint first, GLsizei count)
{
    ++draws;
    if(reject_draw)
    {
        reject_draw = 0;
        draw_error_pending = 1;
        return;
    }
    real_draw_arrays(mode, first, count);
}

/* Damon V. Caskey - 2026-11-16: report an injected failure after submission. */
static GLenum GL_APIENTRY get_error(void)
{
    GLenum error = real_get_error();
    if(draw_error_pending)
    {
        draw_error_pending = 0;
        return GL_OUT_OF_MEMORY;
    }
    return error;
}

/* Damon V. Caskey - 2026-11-16: observe and narrowly reject the font atlas. */
static void GL_APIENTRY texture_image(GLenum target, GLint level, GLint format,
    GLsizei width, GLsizei height, GLint border, GLenum external_format,
    GLenum type, const void *pixels)
{
    if(width == 80 && height == 100)
    {
        ++font_uploads;
        if(reject_font) width = -1;
    }
    real_texture_image(target, level, format, width, height, border,
                       external_format, type, pixels);
}

/* Damon V. Caskey - 2026-11-16: count private allocations independently. */
static void GL_APIENTRY gen_textures(GLsizei count, GLuint *textures)
{
    real_gen_textures(count, textures);
    texture_allocations += (unsigned)count;
}

/* Damon V. Caskey - 2026-11-16: observe retirement before SDL context loss. */
static void GL_APIENTRY delete_textures(GLsizei count, const GLuint *textures)
{
    real_delete_textures(count, textures);
    texture_deletions += (unsigned)count;
}

/* Damon V. Caskey - 2026-11-16: production uses its actual GLES function table. */
void *__wrap_SDL_GL_GetProcAddress(const char *name)
{
    void *address = __real_SDL_GL_GetProcAddress(name);
    ++driver_lookups;
    if(missing_function && !strcmp(name, "glDrawArrays")) return NULL;
#define WRAP_GL(symbol, storage, wrapper) \
    if(!strcmp(name, symbol)) { storage = address; return address ? (void *)wrapper : NULL; }
    WRAP_GL("glGetShaderiv", real_get_shader, shader_status)
    WRAP_GL("glCompileShader", real_compile_shader, compile_shader)
    WRAP_GL("glGetProgramiv", real_get_program, program_status)
    WRAP_GL("glDrawArrays", real_draw_arrays, draw_arrays)
    WRAP_GL("glGetError", real_get_error, get_error)
    WRAP_GL("glTexImage2D", real_texture_image, texture_image)
    WRAP_GL("glGenTextures", real_gen_textures, gen_textures)
    WRAP_GL("glDeleteTextures", real_delete_textures, delete_textures)
#undef WRAP_GL
    return address;
}

/* Damon V. Caskey - 2026-11-16: implicit downloads are never needed here. */
int __wrap_SDL_RenderReadPixels(SDL_Renderer *renderer, const SDL_Rect *rectangle,
    Uint32 format, void *pixels, int pitch)
{
    ++readbacks;
    return __real_SDL_RenderReadPixels(renderer, rectangle, format, pixels, pitch);
}

/* Damon V. Caskey - 2026-11-16: detect the old CPU-composed scene upload. */
int __wrap_SDL_UpdateTexture(SDL_Texture *texture, const SDL_Rect *rectangle,
    const void *pixels, int pitch)
{
    ++cpu_uploads;
    return __real_SDL_UpdateTexture(texture, rectangle, pixels, pitch);
}

/* Damon V. Caskey - 2026-11-16: expose a failed final SDL composition. */
int __wrap_SDL_RenderCopy(SDL_Renderer *renderer, SDL_Texture *texture,
    const SDL_Rect *source, const SDL_Rect *destination)
{
    if(reject_present_copy)
    {
        reject_present_copy = 0;
        return SDL_SetError("Injected Android launcher presentation failure");
    }
    return __real_SDL_RenderCopy(renderer, texture, source, destination);
}

/* Damon V. Caskey - 2026-11-16: failed reset recovery must not recreate per frame. */
SDL_Renderer *__wrap_SDL_CreateRenderer(SDL_Window *window, int index, Uint32 flags)
{
    ++renderer_creations;
    return __real_SDL_CreateRenderer(window, index, flags);
}

/* Damon V. Caskey - 2026-11-16: allocate exact engine RGB/indexed layouts. */
static s_screen *make_screen(int width, int height, int format)
{
    size_t bytes = (size_t)width * height * (format == PIXEL_32 ? 4 : 1);
    s_screen *screen = calloc(1, sizeof(*screen) + bytes + 1024);
    assert(screen);
    screen->magic = screen_magic;
    screen->width = width;
    screen->height = height;
    screen->pixelformat = format;
    if(format == PIXEL_x8) screen->palette = screen->data + bytes;
    return screen;
}

/* Damon V. Caskey - 2026-11-16: use the original repertoire independently. */
static unsigned reference_index(unsigned character)
{
    if(character >= 32 && character <= 127) return character - 32;
    if(character >= 160 && character <= 223) return character - 64;
    return 0;
}

/* Damon V. Caskey - 2026-11-16: expand the immutable five-bit font rows. */
static void reference_text(unsigned *pixels, int width, int height,
    int x, int y, unsigned foreground, unsigned background, int fill,
    const unsigned char *text, const unsigned char *font)
{
    size_t index;
    int row, column;
    for(index = 0; text[index]; ++index)
        for(row = 0; row < 10; ++row)
            for(column = 0; column < 5; ++column)
            {
                int64_t dx = (int64_t)x + (int64_t)index * 5 + column;
                int64_t dy = (int64_t)y + row;
                int ink = (font[reference_index(text[index]) * 10 + row] >> column) & 1;
                if(dx >= 0 && dx < width && dy >= 0 && dy < height && (ink || fill))
                    pixels[(size_t)dy * width + (size_t)dx] =
                        (ink ? foreground : background) & 0xFFFFFFu;
            }
}

/* Damon V. Caskey - 2026-11-16: source copies are opaque, including index zero. */
static void reference_screen(unsigned *pixels, int width, int height,
    const s_screen *source, int x, int y)
{
    int row, column;
    for(row = 0; row < source->height; ++row)
        for(column = 0; column < source->width; ++column)
        {
            int64_t dx = (int64_t)x + column, dy = (int64_t)y + row;
            size_t offset = (size_t)row * source->width + column;
            if(dx >= 0 && dx < width && dy >= 0 && dy < height)
            {
                unsigned color;
                if(source->pixelformat == PIXEL_32)
                    memcpy(&color, source->data + offset * 4, 4);
                else memcpy(&color, source->palette + source->data[offset] * 4, 4);
                pixels[(size_t)dy * width + (size_t)dx] = color & 0xFFFFFFu;
            }
        }
}

/* Damon V. Caskey - 2026-11-16: clipped rectangles use wide arithmetic. */
static void reference_box(unsigned *pixels, int width, int height,
    int x, int y, int box_width, int box_height, unsigned color)
{
    int row, column;
    for(row = 0; row < height; ++row)
        for(column = 0; column < width; ++column)
            if(column >= x && (int64_t)column < (int64_t)x + box_width &&
               row >= y && (int64_t)row < (int64_t)y + box_height)
                pixels[row * width + column] = color & 0xFFFFFFu;
}

/* Damon V. Caskey - 2026-11-16: verify complete images and opaque alpha. */
static void compare_pixels(const unsigned *actual, const unsigned *expected,
    int width, int height, const char *label)
{
    int index;
    for(index = 0; index < width * height; ++index)
    {
        if((actual[index] & 0xFFFFFFu) != expected[index] || (actual[index] >> 24) != 255)
        {
            fprintf(stderr, "%s at (%d,%d): got %08x, expected ff%06x\n",
                label, index % width, index / width, actual[index], expected[index]);
            assert(0);
        }
        ++pixels_checked;
    }
    ++images_checked;
}

/* Damon V. Caskey - 2026-11-16: make readback an explicit test-only boundary. */
static void verify_texture(SDL_Renderer *renderer, SDL_Texture *target,
    const unsigned *expected, int width, int height, const char *label)
{
    SDL_Texture *saved = SDL_GetRenderTarget(renderer);
    unsigned *actual = malloc((size_t)width * height * 4);
    assert(actual && target);
    assert(SDL_SetRenderTarget(renderer, target) == 0);
    assert(SDL_RenderReadPixels(renderer, NULL, SDL_PIXELFORMAT_ABGR8888,
        actual, width * 4) == 0);
    assert(SDL_SetRenderTarget(renderer, saved) == 0);
    compare_pixels(actual, expected, width, height, label);
    free(actual);
}

/* Damon V. Caskey - 2026-11-16: test every input byte, both fills and exact RGB. */
static void test_glyphs(SDL_Renderer *renderer)
{
    gpu_launcher_android *compositor = gpu_launcher_android_create(renderer, 9, 12);
    unsigned expected[108];
    unsigned first_font_upload = font_uploads;
    int character, fill;
    assert(compositor);
    for(fill = 0; fill <= 1; ++fill)
        for(character = 1; character <= 255; ++character)
        {
            unsigned char text[] = {(unsigned char)character, 0};
            unsigned char glyph = (unsigned char)reference_index(character);
            unsigned foreground = character % 3 ? 0x9F0317u : 0;
            unsigned background = character % 2 ? 0xE5A70Bu : 0;
            unsigned before_reads = readbacks, before_uploads = cpu_uploads;
            int index;
            for(index = 0; index < 108; ++index) expected[index] = 0x456789u;
            assert(gpu_launcher_android_begin(compositor));
            assert(gpu_launcher_android_box(compositor, 0, 0, 9, 12, 0x456789u));
            assert(gpu_launcher_android_text(compositor, 2, 1, foreground, background,
                fill, &glyph, 1, launcher_reference_font, 160));
            assert(readbacks == before_reads && cpu_uploads == before_uploads);
            reference_text(expected, 9, 12, 2, 1, foreground, background,
                fill, text, launcher_reference_font);
            verify_texture(renderer, gpu_launcher_android_texture(compositor),
                expected, 9, 12, "every byte/fill/RGB");
        }
    assert(font_uploads == first_font_upload + 1);
    gpu_launcher_android_destroy(compositor);
}

/* Damon V. Caskey - 2026-11-16: preserve ordered source, text, preview and box draws. */
static void test_sources_and_ordering(SDL_Renderer *renderer)
{
    gpu_launcher_android *compositor = gpu_launcher_android_create(renderer, 31, 19);
    s_screen *background = make_screen(31, 19, PIXEL_32);
    s_screen *preview = make_screen(9, 7, PIXEL_x8);
    unsigned expected[31 * 19];
    unsigned char text[] = {'A', '\n', 'B', '\t', 0xA6, 0xFF, 'C', 0};
    unsigned char glyphs[7];
    static const int clips[][2] = {{0,0}, {-3,-2}, {28,17}, {-20,8},
        {INT_MIN,0}, {INT_MAX,0}, {0,INT_MIN}, {0,INT_MAX}};
    unsigned pass, index, before_reads, before_uploads;
    assert(compositor);
    for(index = 0; index < sizeof(glyphs); ++index)
        glyphs[index] = (unsigned char)reference_index(text[index]);
    for(index = 0; index < 31 * 19; ++index)
        ((unsigned *)background->data)[index] = (index * 73951u & 0xFFFFFFu) |
            ((index % 256u) << 24);
    for(index = 0; index < 256; ++index)
        ((unsigned *)preview->palette)[index] = (index * 17291u + 0x172B3Fu) & 0xFFFFFFu;
    for(index = 0; index < 63; ++index) preview->data[index] = (unsigned char)(index % 9);
    for(pass = 0; pass < sizeof(clips) / sizeof(clips[0]); ++pass)
    {
        memset(expected, 0, sizeof(expected));
        before_reads = readbacks;
        before_uploads = cpu_uploads;
        assert(gpu_launcher_android_begin(compositor));
        assert(gpu_launcher_android_screen(compositor, background, 0, 0));
        reference_screen(expected, 31, 19, background, 0, 0);
        assert(gpu_launcher_android_text(compositor, clips[pass][0], clips[pass][1],
            0x1907F3u, 0xD1B587u, pass & 1, glyphs, sizeof(glyphs),
            launcher_reference_font, 160));
        reference_text(expected, 31, 19, clips[pass][0], clips[pass][1],
            0x1907F3u, 0xD1B587u, pass & 1, text, launcher_reference_font);
        assert(gpu_launcher_android_screen(compositor, preview, 3, 2));
        reference_screen(expected, 31, 19, preview, 3, 2);
        assert(gpu_launcher_android_box(compositor, -2, 9, 11, 6, 0xA35729u));
        reference_box(expected, 31, 19, -2, 9, 11, 6, 0xA35729u);
        assert(gpu_launcher_android_text(compositor, 4, 6, 0, 0xE7A319u, 0,
            glyphs, 1, launcher_reference_font, 160));
        reference_text(expected, 31, 19, 4, 6, 0, 0, 0,
            (const unsigned char *)"A", launcher_reference_font);
        assert(readbacks == before_reads && cpu_uploads == before_uploads);
        verify_texture(renderer, gpu_launcher_android_texture(compositor),
            expected, 31, 19, "ordered RGB/indexed preview/text/box");
        /* Reusing the same source allocation must refresh pixels and palette. */
        preview->data[pass] ^= 7;
        ((unsigned *)preview->palette)[0] ^= 0xABCDEFu;
        ((unsigned *)background->data)[pass] ^= 0xFFFFFFu;
    }
    gpu_launcher_android_destroy(compositor);
    free(preview);
    free(background);
}

typedef struct renderer_state
{
    SDL_Texture *target;
    SDL_Rect viewport, clip;
    SDL_bool clipped;
    SDL_BlendMode blend;
    Uint8 red, green, blue, alpha;
    float scale_x, scale_y;
    int logical_width, logical_height;
    GLint program, active_texture, textures[2], array_buffer, framebuffer;
    GLint unpack_alignment, attributes[2][6];
    void *pointers[2];
    GLfloat clear[4];
    GLboolean mask[4], capabilities[8];
} renderer_state;

/* Damon V. Caskey - 2026-11-16: observe public SDL and its cached raw GL state. */
static renderer_state capture_state(SDL_Renderer *renderer)
{
    static const GLenum names[6] = { GL_VERTEX_ATTRIB_ARRAY_ENABLED,
        GL_VERTEX_ATTRIB_ARRAY_SIZE, GL_VERTEX_ATTRIB_ARRAY_TYPE,
        GL_VERTEX_ATTRIB_ARRAY_NORMALIZED, GL_VERTEX_ATTRIB_ARRAY_STRIDE,
        GL_VERTEX_ATTRIB_ARRAY_BUFFER_BINDING };
    static const GLenum capabilities[8] = { GL_BLEND, GL_SCISSOR_TEST,
        GL_DEPTH_TEST, GL_STENCIL_TEST, GL_CULL_FACE, GL_DITHER,
        GL_SAMPLE_ALPHA_TO_COVERAGE, GL_SAMPLE_COVERAGE };
    renderer_state result;
    int attribute, item;
    memset(&result, 0, sizeof(result));
    result.target = SDL_GetRenderTarget(renderer);
    SDL_RenderGetViewport(renderer, &result.viewport);
    SDL_RenderGetClipRect(renderer, &result.clip);
    result.clipped = SDL_RenderIsClipEnabled(renderer);
    SDL_RenderGetScale(renderer, &result.scale_x, &result.scale_y);
    SDL_RenderGetLogicalSize(renderer, &result.logical_width, &result.logical_height);
    assert(SDL_GetRenderDrawBlendMode(renderer, &result.blend) == 0);
    assert(SDL_GetRenderDrawColor(renderer, &result.red, &result.green,
        &result.blue, &result.alpha) == 0);
    get_integer(GL_CURRENT_PROGRAM, &result.program);
    get_integer(GL_ARRAY_BUFFER_BINDING, &result.array_buffer);
    get_integer(GL_FRAMEBUFFER_BINDING, &result.framebuffer);
    get_integer(GL_ACTIVE_TEXTURE, &result.active_texture);
    for(item = 0; item < 2; ++item)
    {
        active_texture(GL_TEXTURE0 + item);
        get_integer(GL_TEXTURE_BINDING_2D, &result.textures[item]);
    }
    active_texture(result.active_texture);
    get_integer(GL_UNPACK_ALIGNMENT, &result.unpack_alignment);
    get_float(GL_COLOR_CLEAR_VALUE, result.clear);
    get_boolean(GL_COLOR_WRITEMASK, result.mask);
    for(item = 0; item < 8; ++item) result.capabilities[item] = is_enabled(capabilities[item]);
    for(attribute = 0; attribute < 2; ++attribute)
    {
        for(item = 0; item < 6; ++item)
            get_attribute(attribute, names[item], &result.attributes[attribute][item]);
        get_pointer(attribute, GL_VERTEX_ATTRIB_ARRAY_POINTER, &result.pointers[attribute]);
    }
    return result;
}

/* Damon V. Caskey - 2026-11-16: SDL batching must resume with its cached state. */
static void assert_state(SDL_Renderer *renderer, const renderer_state *expected)
{
    renderer_state actual = capture_state(renderer);
    if(memcmp(&actual, expected, sizeof(actual)))
    {
        const unsigned char *first = (const unsigned char *)expected;
        const unsigned char *second = (const unsigned char *)&actual;
        size_t index;
        for(index = 0; index < sizeof(actual); ++index)
            if(first[index] != second[index])
            {
                fprintf(stderr, "Android launcher changed renderer state byte %zu: %u -> %u\n",
                    index, first[index], second[index]);
                break;
            }
        assert(0);
    }
}

/* Damon V. Caskey - 2026-11-16: exercise all commands under foreign SDL state. */
static void test_state(SDL_Renderer *renderer)
{
    gpu_launcher_android *compositor = gpu_launcher_android_create(renderer, 13, 11);
    SDL_Texture *outer = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_BGR888,
        SDL_TEXTUREACCESS_TARGET, 31, 19);
    SDL_Texture *sample = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_BGR888,
        SDL_TEXTUREACCESS_STREAMING, 1, 1);
    s_screen *source = make_screen(3, 2, PIXEL_x8);
    unsigned pixel = 0xFF815327u, expected[143] = { 0 };
    unsigned char glyph = 'A' - 32;
    SDL_Rect viewport = {2,1,18,12}, clip = {1,2,10,6};
    renderer_state state;
    assert(compositor && outer && sample);
    ((unsigned *)source->palette)[0] = 0x815327u;
    assert(SDL_UpdateTexture(sample, NULL, &pixel, 4) == 0);
    assert(SDL_SetRenderTarget(renderer, outer) == 0);
    assert(SDL_RenderSetLogicalSize(renderer, 23, 17) == 0);
    assert(SDL_RenderSetScale(renderer, 1.25f, 1.5f) == 0);
    assert(SDL_RenderSetViewport(renderer, &viewport) == 0);
    assert(SDL_RenderSetClipRect(renderer, &clip) == 0);
    assert(SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND) == 0);
    assert(SDL_SetRenderDrawColor(renderer, 17, 83, 129, 197) == 0);
    assert(SDL_SetTextureBlendMode(sample, SDL_BLENDMODE_BLEND) == 0);
    assert(SDL_RenderCopy(renderer, sample, NULL, NULL) == 0);
    assert(SDL_RenderFlush(renderer) == 0);
    color_mask(GL_TRUE, GL_FALSE, GL_TRUE, GL_FALSE);
    enable(GL_CULL_FACE);
    pixel_store(GL_UNPACK_ALIGNMENT, 8);
    clear_color(0.125f, 0.25f, 0.5f, 0.75f);
    active_texture(GL_TEXTURE1);
    state = capture_state(renderer);
    assert(gpu_launcher_android_begin(compositor));
    assert_state(renderer, &state);
    assert(gpu_launcher_android_screen(compositor, source, 0, 0));
    assert_state(renderer, &state);
    reference_screen(expected, 13, 11, source, 0, 0);
    assert(gpu_launcher_android_text(compositor, 4, 1, 0x7D095Bu, 0, 0,
        &glyph, 1, launcher_reference_font, 160));
    assert_state(renderer, &state);
    reference_text(expected, 13, 11, 4, 1, 0x7D095Bu, 0, 0,
        (const unsigned char *)"A", launcher_reference_font);
    assert(gpu_launcher_android_box(compositor, 10, 8, 9, 5, 0x31A7EDu));
    assert_state(renderer, &state);
    reference_box(expected, 13, 11, 10, 8, 9, 5, 0x31A7EDu);
    reject_draw = 1;
    assert(!gpu_launcher_android_box(compositor, 0, 0, 1, 1, 0));
    assert_state(renderer, &state);
    assert(!gpu_launcher_android_texture(compositor));
    assert(gpu_launcher_android_begin(compositor));
    assert_state(renderer, &state);
    memset(expected, 0, sizeof(expected));
    color_mask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    disable(GL_CULL_FACE);
    pixel_store(GL_UNPACK_ALIGNMENT, 4);
    clear_color(0, 0, 0, 0);
    active_texture(GL_TEXTURE0);
    assert(SDL_SetRenderTarget(renderer, NULL) == 0);
    assert(SDL_RenderSetLogicalSize(renderer, 0, 0) == 0);
    assert(SDL_RenderSetScale(renderer, 1, 1) == 0);
    assert(SDL_RenderSetViewport(renderer, NULL) == 0);
    assert(SDL_RenderSetClipRect(renderer, NULL) == 0);
    verify_texture(renderer, gpu_launcher_android_texture(compositor), expected,
        13, 11, "state-preserved failed frame recovery");
    gpu_launcher_android_destroy(compositor);
    SDL_DestroyTexture(sample);
    SDL_DestroyTexture(outer);
    free(source);
}

/* Damon V. Caskey - 2026-11-16: failures suppress the entire frame until begin. */
static void test_failures(SDL_Renderer *renderer)
{
    gpu_launcher_android *compositor;
    unsigned char glyphs[] = {'A' - 32, 160};
    unsigned char alternate_font[1600];
    unsigned expected[80] = { 0 };
    unsigned before_draws, before_fonts;
    s_screen *source = make_screen(1, 1, PIXEL_x8);
    assert(!gpu_launcher_android_create(NULL, 8, 10));
    assert(!gpu_launcher_android_create(renderer, INT_MAX, 10));
    assert(!gpu_launcher_android_create(renderer, 8, 0));
    reject_shader = 1;
    assert(!gpu_launcher_android_create(renderer, 8, 10));
    reject_shader = 0;
    reject_link = 1;
    assert(!gpu_launcher_android_create(renderer, 8, 10));
    reject_link = 0;
    missing_function = 1;
    assert(!gpu_launcher_android_create(renderer, 8, 10));
    missing_function = 0;
    compositor = gpu_launcher_android_create(renderer, 8, 10);
    assert(compositor);
    assert(gpu_launcher_android_begin(compositor));
    before_draws = draws;
    before_fonts = font_uploads;
    assert(gpu_launcher_android_text(compositor, 0, 0, 0, 0, 0, NULL, 0, NULL, 0));
    assert(gpu_launcher_android_text(compositor, INT_MIN, INT_MAX, 0, 0, 1,
        glyphs, 1, launcher_reference_font, 160));
    assert(draws == before_draws && font_uploads == before_fonts);
    assert(!gpu_launcher_android_text(compositor, 0, 0, 0, 0, 0,
        glyphs, 2, launcher_reference_font, 160));
    assert(!gpu_launcher_android_texture(compositor));
    assert(!gpu_launcher_android_box(compositor, 0, 0, 1, 1, 0));
    assert(draws == before_draws);
    assert(gpu_launcher_android_begin(compositor));
    assert(!gpu_launcher_android_text(compositor, 0, 0, 0, 0, 0,
        glyphs, SIZE_MAX, launcher_reference_font, 160));
    assert(gpu_launcher_android_begin(compositor));
    assert(!gpu_launcher_android_text(compositor, 0, 0, 0, 0, 0,
        glyphs, 1, launcher_reference_font, 159));
    assert(gpu_launcher_android_begin(compositor));
    assert(gpu_launcher_android_text(compositor, 0, 0, 0xC1390Bu, 0, 0,
        glyphs, 1, launcher_reference_font, 160));
    memcpy(alternate_font, launcher_reference_font, sizeof(alternate_font));
    alternate_font[('A' - 32) * 10 + 1] ^= 31;
    reject_font = 1;
    assert(!gpu_launcher_android_text(compositor, 0, 0, 0xF1D3B5u, 0, 0,
        glyphs, 1, alternate_font, 160));
    reject_font = 0;
    assert(!gpu_launcher_android_texture(compositor));
    before_fonts = font_uploads;
    assert(gpu_launcher_android_begin(compositor));
    assert(gpu_launcher_android_text(compositor, 0, 0, 0xC1390Bu, 0, 0,
        glyphs, 1, launcher_reference_font, 160));
    assert(font_uploads == before_fonts);
    reference_text(expected, 8, 10, 0, 0, 0xC1390Bu, 0, 0,
        (const unsigned char *)"A", launcher_reference_font);
    verify_texture(renderer, gpu_launcher_android_texture(compositor), expected,
        8, 10, "transactional atlas failure and recovery");
    source->palette = NULL;
    assert(!gpu_launcher_android_screen(compositor, source, 0, 0));
    assert(!gpu_launcher_android_texture(compositor));
    assert(gpu_launcher_android_begin(compositor));
    gpu_launcher_android_invalidate(compositor);
    assert(!gpu_launcher_android_texture(compositor));
    assert(gpu_launcher_android_begin(compositor));
    memset(expected, 0, sizeof(expected));
    verify_texture(renderer, gpu_launcher_android_texture(compositor), expected,
        8, 10, "begin clears failed frame");
    gpu_launcher_android_destroy(compositor);
    free(source);
}

/* Production video functions below use deterministic assets, while their
 * rendering, source selection, reset recovery and presentation stay intact. */
SDL_Window *window;
SDL_Renderer *renderer;
SDL_Texture *texture, *texture_base, *buttons;
static SDL_Texture *movie_texture;
static const void *movie_owner;
static gpu_output_filter_android *output_filter;
static gpu_launcher_android *launcher;
static const s_screen *launcher_owner;
static SDL_GLContext launcher_context;
static SDL_atomic_t launcher_reset_pending;
static int launcher_watch_installed, launcher_frame_valid, launcher_init_failed, launcher_warning;
static int launcher_rebuild_pending, video_mode_ready;
s_videomodes stored_videomodes;
static struct { int swfilter, hwfilter, fullscreen, stretch; } savedata;
static char windowTitle[] = "Android launcher regression";
static const unsigned char buttonpng[1] = { 0 };
int nativeWidth = 64, nativeHeight = 48, brightness, stretch = 1, hide_t;
static int textureWidth, textureHeight;
#define MAXTOUCHB 1
static unsigned touchstates[MAXTOUCHB];
static SDL_Rect btnsrc[MAXTOUCHB], btndes[MAXTOUCHB];
static unsigned surface_requests, presented_frames;
static unsigned presented_pixels[64 * 48];

/* Damon V. Caskey - 2026-11-16: asset decoding is outside compositor coverage. */
static SDL_Surface *pngToSurface(const unsigned char *bytes)
{
    SDL_Surface *surface;
    (void)bytes;
    surface = SDL_CreateRGBSurface(0, 1, 1, 32, 0xFF, 0xFF00, 0xFF0000, 0xFF000000);
    assert(surface);
    assert(SDL_FillRect(surface, NULL, SDL_MapRGBA(surface->format, 0, 255, 0, 255)) == 0);
    assert(SDL_SetSurfaceBlendMode(surface, SDL_BLENDMODE_BLEND) == 0);
    return surface;
}

/* Damon V. Caskey - 2026-11-16: retain one visible touch button for ordering. */
static void setup_touch(void)
{
    btnsrc[0] = (SDL_Rect){0, 0, 1, 1};
    btndes[0] = (SDL_Rect){2, 3, 5, 4};
}

/* Damon V. Caskey - 2026-11-16: the fixture does not depend on wall-clock time. */
static int timer_gettick(void)
{
    return 100;
}

/* Damon V. Caskey - 2026-11-16: preprocessing dimensions are already RGB32. */
s_videomodes setupPreBlitProcessing(s_videomodes mode)
{
    return mode;
}

/* Damon V. Caskey - 2026-11-16: detect entry into the ordinary CPU scene path. */
s_videosurface *getVideoSurface(s_screen *screen)
{
    static s_videosurface surface;
    ++surface_requests;
    if(!screen || screen->pixelformat != PIXEL_32) return NULL;
    surface = (s_videosurface){screen->width, screen->height, screen->width * 4, screen->data};
    return &surface;
}

/* Damon V. Caskey - 2026-11-16: capture only completed presentation boundaries. */
void __wrap_SDL_RenderPresent(SDL_Renderer *presenter)
{
    assert(SDL_GetRenderTarget(presenter) == NULL);
    assert(__real_SDL_RenderReadPixels(presenter, NULL, SDL_PIXELFORMAT_ABGR8888,
        presented_pixels, nativeWidth * 4) == 0);
    ++presented_frames;
    __real_SDL_RenderPresent(presenter);
}

/* Generated verbatim by the runner, including actual mode/reset/presenter code. */
#include "android_launcher_video_functions.inc"

static s_screen *vscreen;
static int launcher_redraw, launcher_frame_ready;
/* The menu's failure accumulation is production code, not a test surrogate. */
#include "android_launcher_menu_functions.inc"

/* Damon V. Caskey - 2026-11-16: run the production shared variadic formatter. */
static int launcher_printf(s_screen *screen, int x, int y, unsigned foreground,
    unsigned background, int fill, const char *format, ...)
{
    int result;
    va_list arguments;
    va_start(arguments, format);
    result = launcher_text_vprintf(screen, x, y, foreground, background, fill,
        format, arguments);
    va_end(arguments);
    return result;
}

/* Damon V. Caskey - 2026-11-16: initialize a real production RGB32 mode. */
static void set_mode(int width, int height, int filter)
{
    s_videomodes mode = { 0 };
    mode.hRes = (short)width;
    mode.vRes = (short)height;
    mode.pixel = 4;
    savedata.swfilter = filter;
    savedata.hwfilter = 1;
    assert(video_set_mode(mode));
    assert(video_mode_ready && textureWidth == width && textureHeight == height);
}

/* Damon V. Caskey - 2026-11-16: shared formatting preserves long and NUL runs. */
static void test_formatting(void)
{
    s_screen *owner = make_screen(19, 10, PIXEL_32);
    unsigned expected[190] = { 0 };
    unsigned char text[201], terminated[] = {'A', 0, 'Z', 0};
    unsigned before_reads, before_uploads, before_draws;
    gpu_launcher_android_stats before;
    size_t index;
    set_mode(19, 10, 0);
    memset(owner->data, 0x5A, 19 * 10 * 4);
    assert(video_launcher_begin(owner));
    for(index = 0; index < 200; ++index) text[index] = index < 195 ? 'X' : 'A' + index % 3;
    text[200] = 0;
    before_reads = readbacks;
    before_uploads = cpu_uploads;
    assert(launcher_printf(owner, -195 * 5, 0, 0x971B4Du, 0x3F5D7Bu, 1, "%s", text));
    reference_text(expected, 19, 10, -195 * 5, 0, 0x971B4Du, 0x3F5D7Bu, 1,
        text, launcher_reference_font);
    assert(readbacks == before_reads && cpu_uploads == before_uploads);
    verify_texture(renderer, gpu_launcher_android_texture(launcher), expected,
        19, 10, "long format with visible suffix");
    assert(video_launcher_begin(owner));
    memset(expected, 0, sizeof(expected));
    before = *gpu_launcher_android_get_stats(launcher);
    assert(launcher_printf(owner, 0, 0, 0x17A9D3u, 0x4D658Bu, 1, "%c%c%c", 'A', 0, 'Z'));
    reference_text(expected, 19, 10, 0, 0, 0x17A9D3u, 0x4D658Bu, 1,
        terminated, launcher_reference_font);
    assert(gpu_launcher_android_get_stats(launcher)->text_glyphs == before.text_glyphs + 1);
    verify_texture(renderer, gpu_launcher_android_texture(launcher), expected,
        19, 10, "embedded NUL stops filled cells");
    before_draws = draws;
    assert(!launcher_printf(owner, 0, 0, 1, 2, 1, NULL));
    reject_malloc_size = sizeof(text);
    assert(!launcher_printf(owner, 0, 0, 1, 2, 1, "%s", text));
    reject_malloc_size = 0;
    assert(formatter_failures == 1 && draws == before_draws);
    for(index = 0; index < 19 * 10 * 4; ++index) assert(owner->data[index] == 0x5A);
    video_launcher_retire(0);
    free(owner);
}

/* Damon V. Caskey - 2026-11-16: formatting failure blocks an incomplete menu. */
static void test_menu_failure(void)
{
    unsigned char text[201];
    unsigned before_present, before_draws, before_failures;
    set_mode(8, 10, 0);
    vscreen = make_screen(8, 10, PIXEL_32);
    memset(text, 'A', sizeof(text) - 1);
    text[sizeof(text) - 1] = 0;
    savedata.stretch = 0;
    launcher_frame_ready = video_launcher_begin(vscreen);
    printText(0, 0, 0xAF3197, 0, 0, "%s", "A");
    assert(launcher_frame_ready);
    before_present = presented_frames;
    before_draws = draws;
    before_failures = formatter_failures;
    reject_malloc_size = sizeof(text);
    printText(0, 0, 0xAF3197, 0, 0, "%s", text);
    reject_malloc_size = 0;
    assert(formatter_failures == before_failures + 1 && !launcher_frame_ready);
    blit_video_menu(vscreen);
    assert(launcher_redraw && stretch == savedata.stretch);
    assert(presented_frames == before_present && draws == before_draws);
    launcher_frame_ready = video_launcher_begin(vscreen);
    printText(0, 0, 0xAF3197, 0, 0, "%s", "A");
    blit_video_menu(vscreen);
    assert(!launcher_redraw && launcher_frame_ready && stretch == savedata.stretch);
    assert(presented_frames == before_present + 1);
    video_launcher_retire(0);
    free(vscreen);
    vscreen = NULL;
}

/* Damon V. Caskey - 2026-11-16: independently reproduce final nearest/aspect mapping. */
static void reference_display(unsigned *pixels, const unsigned *source,
    int width, int height, int filter, int stretched)
{
    int x, y, out_width = filter ? width * 2 : width;
    int out_height = filter ? height * 2 : height;
    int drawn_width = stretched ? nativeWidth : nativeHeight * width / height;
    int left = stretched ? 0 : (nativeWidth - drawn_width) / 2;
    for(y = 0; y < nativeHeight; ++y)
        for(x = 0; x < nativeWidth; ++x)
        {
            unsigned color = 0;
            if(x >= left && x < left + drawn_width)
            {
                int sx = (2 * (x - left) + 1) * out_width / (2 * drawn_width);
                int sy = (2 * y + 1) * out_height / (2 * nativeHeight);
                color = filter ? output_filter_expected(source, width, height, filter, sx, sy) :
                    source[sy * width + sx];
            }
            pixels[y * nativeWidth + x] = color;
        }
}

/* Damon V. Caskey - 2026-11-16: construct one realistic ordered launcher frame. */
static void compose_frame(s_screen *owner, s_screen *background, s_screen *preview,
    unsigned *expected)
{
    memset(expected, 0, (size_t)owner->width * owner->height * 4);
    assert(video_launcher_begin(owner));
    assert(video_launcher_screen(owner, background, 0, 0));
    reference_screen(expected, owner->width, owner->height, background, 0, 0);
    assert(launcher_printf(owner, 0, 1, 0, 0xBDA753u, 1, "%s", "AB"));
    reference_text(expected, owner->width, owner->height, 0, 1, 0, 0xBDA753u, 1,
        (const unsigned char *)"AB", launcher_reference_font);
    assert(video_launcher_screen(owner, preview, 3, 4));
    reference_screen(expected, owner->width, owner->height, preview, 3, 4);
    assert(launcher_printf(owner, 6, 2, 0xE50791u, 0, 0, "%s", "C"));
    reference_text(expected, owner->width, owner->height, 6, 2, 0xE50791u, 0, 0,
        (const unsigned char *)"C", launcher_reference_font);
}

/* Damon V. Caskey - 2026-11-16: real final blit retains filters, aspect and touch. */
static void test_presentation(void)
{
    s_screen *owner = make_screen(12, 12, PIXEL_32);
    s_screen *background = make_screen(12, 12, PIXEL_32);
    s_screen *preview = make_screen(5, 4, PIXEL_x8);
    unsigned expected[144], display[64 * 48];
    int filter, aspect, index;
    memset(owner->data, 0xA5, 12 * 12 * 4);
    for(index = 0; index < 144; ++index)
        ((unsigned *)background->data)[index] = (index * 73951u) & 0xFFFFFFu;
    for(index = 0; index < 256; ++index)
        ((unsigned *)preview->palette)[index] = (index * 12973u + 0x1379B5u) & 0xFFFFFFu;
    for(index = 0; index < 20; ++index) preview->data[index] = (unsigned char)(index % 4);
    for(filter = 0; filter < BLITTER_MAX; ++filter)
    {
        SDL_Texture *game_texture;
        unsigned before_reads, before_uploads, before_surfaces;
        set_mode(12, 12, filter);
        game_texture = texture;
        brightness = 0;
        hide_t = 0;
        touchstates[0] = 0;
        compose_frame(owner, background, preview, expected);
        before_reads = readbacks;
        before_uploads = cpu_uploads;
        before_surfaces = surface_requests;
        for(aspect = 0; aspect < 2; ++aspect)
        {
            gpu_launcher_android_stats before = *gpu_launcher_android_get_stats(launcher);
            stretch = aspect;
            assert(video_copy_screen(owner));
            assert(texture == game_texture && textureWidth == 12 && textureHeight == 12);
            assert(readbacks == before_reads && cpu_uploads == before_uploads &&
                surface_requests == before_surfaces);
            assert(!memcmp(&before, gpu_launcher_android_get_stats(launcher), sizeof(before)));
            reference_display(display, expected, 12, 12, filter, stretch);
            compare_pixels(presented_pixels, display, 64, 48, "final filter/aspect launcher presentation");
        }
    }
    set_mode(12, 12, 0);
    assert(video_launcher_begin(owner));
    stretch = 1;
    hide_t = 0;
    brightness = 129;
    assert(video_copy_screen(owner));
    for(index = 0; index < 64 * 48; ++index) display[index] = 0x808080u;
    compare_pixels(presented_pixels, display, 64, 48, "positive brightness after launcher");
    assert(video_launcher_box(owner, 0, 0, 12, 12, 0xFFFFFFu));
    brightness = -256;
    hide_t = INT_MAX;
    touchstates[0] = 1;
    assert(video_copy_screen(owner));
    memset(display, 0, sizeof(display));
    reference_box(display, 64, 48, 2, 3, 5, 4, 0x008000u);
    compare_pixels(presented_pixels, display, 64, 48, "touch drawn after negative brightness");
    for(index = 0; index < 12 * 12 * 4; ++index) assert(owner->data[index] == 0xA5);
    brightness = 0;
    hide_t = 0;
    touchstates[0] = 0;
    video_launcher_retire(0);
    free(preview);
    free(background);
    free(owner);
}

/* Damon V. Caskey - 2026-11-16: failed frames never enter the CPU upload branch. */
static void test_video_failures_and_reset(void)
{
    s_screen *owner = make_screen(8, 10, PIXEL_32);
    s_screen *ordinary = make_screen(8, 10, PIXEL_32);
    unsigned expected[80] = { 0 };
    unsigned before_present, before_surfaces, before_uploads, before_reads, before_draws;
    unsigned before_fonts, before_deletes;
    unsigned before_compiles, before_lookups, before_warnings, before_allocations;
    unsigned before_renderers;
    SDL_Texture *game_texture;
    SDL_Event event;
    set_mode(8, 10, 0);
    memset(owner->data, 0xA7, 80 * 4);
    game_texture = texture;
    before_present = presented_frames;
    before_surfaces = surface_requests;
    before_uploads = cpu_uploads;
    before_reads = readbacks;
    reject_shader = 1;
    assert(!video_launcher_begin(owner));
    reject_shader = 0;
    assert(launcher_owner == owner && !video_copy_screen(owner));
    assert(surface_requests == before_surfaces && cpu_uploads == before_uploads &&
        readbacks == before_reads && presented_frames == before_present);
    assert(launcher_init_failed);
    before_compiles = shader_compilations;
    before_lookups = driver_lookups;
    before_warnings = warnings;
    before_allocations = texture_allocations;
    before_draws = draws;
    assert(!video_launcher_begin(owner));
    assert(!video_launcher_begin(owner));
    assert(!video_copy_screen(owner));
    assert(shader_compilations == before_compiles && driver_lookups == before_lookups &&
        warnings == before_warnings && texture_allocations == before_allocations &&
        draws == before_draws);
    assert(surface_requests == before_surfaces && cpu_uploads == before_uploads &&
        readbacks == before_reads && presented_frames == before_present);
    set_mode(8, 10, 0);
    assert(!launcher_init_failed);
    game_texture = texture;
    before_uploads = cpu_uploads;
    assert(video_launcher_begin(owner));
    assert(shader_compilations > before_compiles);
    before_draws = draws;
    {
        uint64_t before_sources = gpu_launcher_android_get_stats(launcher)->source_uploads;
        assert(!video_launcher_screen(owner, owner, 0, 0));
        assert(!gpu_launcher_android_texture(launcher));
        assert(gpu_launcher_android_get_stats(launcher)->source_uploads == before_sources);
    }
    assert(!video_copy_screen(owner));
    assert(draws == before_draws && surface_requests == before_surfaces &&
        cpu_uploads == before_uploads && readbacks == before_reads &&
        presented_frames == before_present);
    assert(video_launcher_begin(owner));
    reject_draw = 1;
    assert(!launcher_printf(owner, 0, 0, 0x1739A5u, 0, 0, "%s", "A"));
    before_draws = draws;
    assert(!video_launcher_box(owner, 0, 0, 8, 10, 0xFFFFFFu));
    assert(!video_copy_screen(owner) && draws == before_draws);
    assert(video_launcher_begin(owner));
    assert(launcher_printf(owner, 0, 0, 0x1739A5u, 0, 0, "%s", "A"));
    reject_present_copy = 1;
    assert(!video_copy_screen(owner));
    assert(texture == game_texture && textureWidth == 8 && textureHeight == 10);
    assert(!video_copy_screen(owner));
    assert(presented_frames == before_present && surface_requests == before_surfaces &&
        cpu_uploads == before_uploads && readbacks == before_reads);
    assert(video_launcher_begin(owner));
    assert(launcher_printf(owner, 0, 0, 0x1739A5u, 0, 0, "%s", "A"));
    assert(video_copy_screen(owner));
    assert(presented_frames == before_present + 1);
    before_present = presented_frames;
    before_draws = draws;
    memset(&event, 0, sizeof(event));
    event.type = SDL_RENDER_TARGETS_RESET;
    assert(SDL_PushEvent(&event) == 1);
    assert(draws == before_draws && SDL_AtomicGet(&launcher_reset_pending));
    assert(!video_copy_screen(owner));
    assert(presented_frames == before_present && surface_requests == before_surfaces &&
        cpu_uploads == before_uploads && readbacks == before_reads);
    before_fonts = font_uploads;
    assert(video_launcher_begin(owner));
    assert(!SDL_AtomicGet(&launcher_reset_pending) && !launcher_rebuild_pending);
    assert(gpu_launcher_android_get_stats(launcher)->font_uploads == 0);
    assert(launcher_printf(owner, 0, 0, 0x1739A5u, 0, 0, "%s", "A"));
    assert(font_uploads == before_fonts + 1);
    reference_text(expected, 8, 10, 0, 0, 0x1739A5u, 0, 0,
        (const unsigned char *)"A", launcher_reference_font);
    verify_texture(renderer, gpu_launcher_android_texture(launcher), expected,
        8, 10, "SDL reset recreates context and atlas");
    assert(video_copy_screen(owner));
    before_deletes = texture_deletions;
    set_mode(8, 10, BLITTER_BILINEAR);
    assert(!launcher && !launcher_owner && !launcher_watch_installed);
    assert(texture_deletions > before_deletes);
    assert(video_launcher_begin(owner));
    assert(launcher_printf(owner, 0, 0, 0x1739A5u, 0, 0, "%s", "A"));
    before_present = presented_frames;
    before_surfaces = surface_requests;
    before_uploads = cpu_uploads;
    reject_draw = 1;
    assert(!video_copy_screen(owner));
    assert(!video_copy_screen(owner));
    assert(presented_frames == before_present && surface_requests == before_surfaces &&
        cpu_uploads == before_uploads);
    assert(video_launcher_begin(owner));
    assert(launcher_printf(owner, 0, 0, 0x1739A5u, 0, 0, "%s", "A"));
    assert(video_copy_screen(owner));
    /* Reset must also bound retries when rebuilding the video mode fails
     * before a launcher compositor exists, here at output-filter compilation. */
    before_present = presented_frames;
    memset(&event, 0, sizeof(event));
    event.type = SDL_RENDER_DEVICE_RESET;
    assert(SDL_PushEvent(&event) == 1);
    reject_shader = 1;
    assert(!video_launcher_begin(owner));
    reject_shader = 0;
    assert(!video_mode_ready && !video_copy_screen(owner));
    before_renderers = renderer_creations;
    before_compiles = shader_compilations;
    before_lookups = driver_lookups;
    before_warnings = warnings;
    before_allocations = texture_allocations;
    before_draws = draws;
    before_uploads = cpu_uploads;
    before_surfaces = surface_requests;
    assert(!video_launcher_begin(owner));
    assert(!video_launcher_begin(owner));
    assert(!video_copy_screen(owner));
    assert(renderer_creations == before_renderers && shader_compilations == before_compiles &&
        driver_lookups == before_lookups && warnings == before_warnings &&
        texture_allocations == before_allocations && draws == before_draws &&
        cpu_uploads == before_uploads && surface_requests == before_surfaces &&
        presented_frames == before_present);
    assert(SDL_PushEvent(&event) == 1);
    assert(SDL_AtomicGet(&launcher_reset_pending));
    assert(video_launcher_begin(owner));
    assert(video_mode_ready && renderer_creations == before_renderers + 1 &&
        shader_compilations > before_compiles);
    assert(launcher_printf(owner, 0, 0, 0x1739A5u, 0, 0, "%s", "A"));
    assert(video_copy_screen(owner) && presented_frames == before_present + 1);
    video_launcher_retire(0);
    before_uploads = cpu_uploads;
    before_surfaces = surface_requests;
    assert(video_copy_screen(ordinary));
    assert(cpu_uploads == before_uploads + 1 && surface_requests == before_surfaces + 1);
    before_deletes = texture_deletions;
    assert(video_launcher_begin(owner));
    assert(launcher_printf(owner, 0, 0, 0x1739A5u, 0, 0, "%s", "A"));
    {
        s_videomodes shutdown = { 0 };
        assert(!video_set_mode(shutdown));
        assert(!video_mode_ready && !launcher && !launcher_owner && !launcher_watch_installed);
        assert(!texture && !texture_base && !buttons && !output_filter);
        assert(texture_deletions > before_deletes);
    }
    for(before_reads = 0; before_reads < 80 * 4; ++before_reads)
        assert(owner->data[before_reads] == 0xA7);
    free(ordinary);
    free(owner);
}

/* Damon V. Caskey - 2026-11-16: resolve observation entry points from this context. */
static void load_observers(void)
{
#define LOAD_OBSERVER(variable, name) variable = __real_SDL_GL_GetProcAddress(name); assert(variable)
    LOAD_OBSERVER(get_integer, "glGetIntegerv");
    LOAD_OBSERVER(get_boolean, "glGetBooleanv");
    LOAD_OBSERVER(get_float, "glGetFloatv");
    LOAD_OBSERVER(get_attribute, "glGetVertexAttribiv");
    LOAD_OBSERVER(get_pointer, "glGetVertexAttribPointerv");
    LOAD_OBSERVER(is_enabled, "glIsEnabled");
    LOAD_OBSERVER(active_texture, "glActiveTexture");
    LOAD_OBSERVER(color_mask, "glColorMask");
    LOAD_OBSERVER(enable, "glEnable");
    LOAD_OBSERVER(disable, "glDisable");
    LOAD_OBSERVER(pixel_store, "glPixelStorei");
    LOAD_OBSERVER(clear_color, "glClearColor");
#undef LOAD_OBSERVER
}

/* Damon V. Caskey - 2026-11-16: require the actual Android GLES shader profile. */
int main(void)
{
    SDL_RendererInfo info;
    const GLubyte *(GL_APIENTRY *get_string)(GLenum);
    int index, driver = -1;
    if(SDL_Init(SDL_INIT_VIDEO) < 0) goto unavailable;
    SDL_SetHint(SDL_HINT_RENDER_BATCHING, "1");
    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "nearest");
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
    for(index = 0; index < SDL_GetNumRenderDrivers(); ++index)
        if(SDL_GetRenderDriverInfo(index, &info) == 0 && !strcmp(info.name, "opengles2")) driver = index;
    if(driver < 0) goto unavailable;
    window = SDL_CreateWindow("Android launcher regression", 0, 0, nativeWidth, nativeHeight,
        SDL_WINDOW_OPENGL | SDL_WINDOW_HIDDEN);
    if(!window) goto unavailable;
    renderer = SDL_CreateRenderer(window, driver, SDL_RENDERER_ACCELERATED | SDL_RENDERER_TARGETTEXTURE);
    if(!renderer) goto unavailable;
    get_string = __real_SDL_GL_GetProcAddress("glGetString");
    assert(get_string && strstr((const char *)get_string(GL_VERSION), "OpenGL ES"));
    printf("Android launcher host context: %s\n", get_string(GL_VERSION));
    load_observers();
    test_glyphs(renderer);
    test_sources_and_ordering(renderer);
    test_state(renderer);
    test_failures(renderer);
    test_formatting();
    test_menu_failure();
    test_presentation();
    test_video_failures_and_reset();
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    printf("Android GPU launcher passed: %u complete images, %u exact RGB8 pixels; original font, RGB/indexed ordering, safe formatting, no implicit readbacks/scene uploads, SDL/GL state, native failure recovery and context retirement.\n",
        images_checked, pixels_checked);
    return 0;
unavailable:
    fprintf(stderr, "SKIP: Android launcher GLES2 context unavailable: %s\n", SDL_GetError());
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 77;
}
