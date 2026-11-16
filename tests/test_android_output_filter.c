/*
 * OpenBOR - http://www.chronocrash.com
 * Copyright (c) OpenBOR Team. All rights reserved. See LICENSE.
 *
 * Damon V. Caskey
 * 2026-11-16
 * Execute the Android SDL/GLES adapter with independent RGB8 pixel oracles,
 * real target composition and fault injection. Host coverage does not replace
 * Android device QA; the shader profile here is genuinely OpenGL ES.
 */
#include <assert.h>
#include <limits.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "SDL.h"
#include "SDL_opengles2_khrplatform.h"
#include "SDL_opengles2_gl2platform.h"
#include "SDL_opengles2_gl2.h"
#include "gpu_output_filter_android.h"
#include "output_filter_reference.h"

static unsigned warnings, readbacks, uploads, images_checked, pixels_checked;
static int fail_shader, fail_draw, missing_function;
static void (GL_APIENTRY *real_get_shader)(GLuint, GLenum, GLint *);
static GLenum (GL_APIENTRY *real_get_error)(void);
static void (GL_APIENTRY *get_integer)(GLenum, GLint *);
static void (GL_APIENTRY *get_boolean)(GLenum, GLboolean *);
static void (GL_APIENTRY *get_attribute)(GLuint, GLenum, GLint *);
static void (GL_APIENTRY *get_pointer)(GLuint, GLenum, void **);
static GLboolean (GL_APIENTRY *is_enabled)(GLenum);
static void (GL_APIENTRY *color_mask)(GLboolean, GLboolean, GLboolean, GLboolean);
static void (GL_APIENTRY *enable)(GLenum);
static void (GL_APIENTRY *disable)(GLenum);

void *__real_SDL_GL_GetProcAddress(const char *name);
int __real_SDL_RenderReadPixels(SDL_Renderer *, const SDL_Rect *, Uint32, void *, int);
int __real_SDL_UpdateTexture(SDL_Texture *, const SDL_Rect *, const void *, int);

/* Damon V. Caskey - 2026-11-16: retain production diagnostic call sites. */
void gpu_log(const char *format, ...)
{
    va_list arguments;
    va_start(arguments, format);
    vfprintf(stderr, format, arguments);
    fputc('\n', stderr);
    va_end(arguments);
    ++warnings;
}

/* Damon V. Caskey - 2026-11-16: reject a real shader boundary without hooks. */
static void GL_APIENTRY shader_status(GLuint shader, GLenum name, GLint *value)
{
    real_get_shader(shader, name, value);
    if(fail_shader && name == GL_COMPILE_STATUS) *value = GL_FALSE;
}

/* Damon V. Caskey - 2026-11-16: model one failed GPU draw at its error check. */
static GLenum GL_APIENTRY draw_error(void)
{
    GLenum result = real_get_error();
    if(fail_draw)
    {
        fail_draw = 0;
        return GL_OUT_OF_MEMORY;
    }
    return result;
}

/* Damon V. Caskey - 2026-11-16: intercept only explicitly armed fault cases. */
void *__wrap_SDL_GL_GetProcAddress(const char *name)
{
    void *address = __real_SDL_GL_GetProcAddress(name);
    if(missing_function && !strcmp(name, "glDrawArrays")) return NULL;
    if(!strcmp(name, "glGetShaderiv"))
    {
        real_get_shader = address;
        return address ? (void *)shader_status : NULL;
    }
    if(!strcmp(name, "glGetError"))
    {
        real_get_error = address;
        return address ? (void *)draw_error : NULL;
    }
    return address;
}

/* Damon V. Caskey - 2026-11-16: count explicit readback boundaries only. */
int __wrap_SDL_RenderReadPixels(SDL_Renderer *renderer, const SDL_Rect *rectangle,
                                Uint32 format, void *pixels, int pitch)
{
    ++readbacks;
    return __real_SDL_RenderReadPixels(renderer, rectangle, format, pixels, pitch);
}

/* Damon V. Caskey - 2026-11-16: detect any extra CPU-filter image upload. */
int __wrap_SDL_UpdateTexture(SDL_Texture *texture, const SDL_Rect *rectangle,
                             const void *pixels, int pitch)
{
    ++uploads;
    return __real_SDL_UpdateTexture(texture, rectangle, pixels, pitch);
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
    GLint program, active_texture, attributes[2][6];
    void *pointers[2];
    GLboolean mask[4], blend_enabled, scissor_enabled, cull_enabled;
} renderer_state;

/* Damon V. Caskey - 2026-11-16: snapshot both public SDL and cached GL state. */
static renderer_state capture_state(SDL_Renderer *renderer)
{
    static const GLenum names[6] = { GL_VERTEX_ATTRIB_ARRAY_ENABLED,
        GL_VERTEX_ATTRIB_ARRAY_SIZE, GL_VERTEX_ATTRIB_ARRAY_TYPE,
        GL_VERTEX_ATTRIB_ARRAY_NORMALIZED, GL_VERTEX_ATTRIB_ARRAY_STRIDE,
        GL_VERTEX_ATTRIB_ARRAY_BUFFER_BINDING };
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
    assert(SDL_GetRenderDrawColor(renderer, &result.red, &result.green, &result.blue, &result.alpha) == 0);
    get_integer(GL_CURRENT_PROGRAM, &result.program);
    get_integer(GL_ACTIVE_TEXTURE, &result.active_texture);
    get_boolean(GL_COLOR_WRITEMASK, result.mask);
    result.blend_enabled = is_enabled(GL_BLEND);
    result.scissor_enabled = is_enabled(GL_SCISSOR_TEST);
    result.cull_enabled = is_enabled(GL_CULL_FACE);
    for(attribute = 0; attribute < 2; ++attribute)
    {
        for(item = 0; item < 6; ++item)
            get_attribute(attribute, names[item], &result.attributes[attribute][item]);
        get_pointer(attribute, GL_VERTEX_ATTRIB_ARRAY_POINTER, &result.pointers[attribute]);
    }
    return result;
}

/* Damon V. Caskey - 2026-11-16: state preservation includes cached VBO pointers. */
static void assert_state(SDL_Renderer *renderer, const renderer_state *expected)
{
    renderer_state actual = capture_state(renderer);
    assert(!memcmp(&actual, expected, sizeof(actual)));
}

/* Damon V. Caskey - 2026-11-16: read a completed RGB32 target explicitly. */
static void read_target(SDL_Renderer *renderer, SDL_Texture *target, unsigned *pixels, int width)
{
    SDL_Texture *saved = SDL_GetRenderTarget(renderer);
    assert(SDL_SetRenderTarget(renderer, target) == 0);
    assert(SDL_RenderReadPixels(renderer, NULL, SDL_PIXELFORMAT_ABGR8888, pixels, width * 4) == 0);
    assert(SDL_SetRenderTarget(renderer, saved) == 0);
}

/* Damon V. Caskey - 2026-11-16: compare every pixel, including border/parity. */
static void compare_image(const unsigned *source, const unsigned *actual,
                            int width, int height, int mode)
{
    int x, y;
    for(y = 0; y < height * 2; ++y)
    {
        for(x = 0; x < width * 2; ++x)
        {
            unsigned expected = output_filter_expected(source, width, height, mode, x, y);
            unsigned got = actual[y * width * 2 + x];
            if((got & 0xFFFFFFu) != expected || (got >> 24) != 255)
            {
                fprintf(stderr, "Android filter %d %dx%d pixel %d,%d expected %06x, received %08x\n",
                        mode, width, height, x, y, expected, got);
                assert(0);
            }
            ++pixels_checked;
        }
    }
    ++images_checked;
}

/*
 * Damon V. Caskey
 * 2026-11-16
 * Check all named filters through uploads, reused shader/target storage and
 * ordinary SDL composition. Changing unused source alpha cannot affect RGB.
 */
static void check_images(SDL_Renderer *renderer, int width, int height, int pattern)
{
    static const unsigned shapes[9] = {
        0x030107, 0x030107, 0x1311fb,
        0x030107, 0x05810b, 0x1311fb,
        0x1311fb, 0x1311fb, 0x030107
    };
    unsigned *source = malloc((size_t)width * height * sizeof(*source));
    unsigned *actual = malloc((size_t)width * height * 4 * sizeof(*actual));
    SDL_Texture *input, *composition;
    int mode, x, y, generation;
    assert(source && actual);
    assert(SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "linear"));
    input = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_BGR888,
                                SDL_TEXTUREACCESS_STREAMING, width, height);
    assert(SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "nearest"));
    composition = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_BGR888,
                                    SDL_TEXTUREACCESS_TARGET, width * 2, height * 2);
    assert(input && composition);
    assert(SDL_SetTextureBlendMode(input, SDL_BLENDMODE_NONE) == 0);
    for(mode = 1; mode < BLITTER_MAX; ++mode)
    {
        gpu_output_filter_android *filter = gpu_output_filter_android_create(renderer, width, height, mode);
        assert(filter);
        for(generation = 0; generation < 2; ++generation)
        {
            SDL_Texture *target;
            unsigned before_reads, before_uploads;
            for(y = 0; y < height; ++y)
                for(x = 0; x < width; ++x)
                {
                    unsigned pixel = pattern ? shapes[(y % 3) * 3 + x % 3] :
                        ((x + y * 17) & 255) | (((x * 13 + y * 29) & 255) << 8) |
                        (((x * 23 + y * 47) & 255) << 16);
                    source[y * width + x] = (pixel ^ (generation ? 0xFFFFFFu : 0)) |
                        ((unsigned)((x * 7 + y * 19 + generation) & 255) << 24);
                }
            assert(SDL_UpdateTexture(input, NULL, source, width * 4) == 0);
            before_reads = readbacks;
            before_uploads = uploads;
            target = gpu_output_filter_android_apply(filter, input);
            assert(target && readbacks == before_reads && uploads == before_uploads);
            read_target(renderer, target, actual, width * 2);
            compare_image(source, actual, width, height, mode);
            /* SDL target copies and overlays must still select their own shader,
             * blend and texture state correctly after the shared-context pass. */
            assert(SDL_SetRenderTarget(renderer, composition) == 0);
            assert(SDL_RenderCopy(renderer, target, NULL, NULL) == 0);
            assert(SDL_RenderReadPixels(renderer, NULL, SDL_PIXELFORMAT_ABGR8888,
                                        actual, width * 8) == 0);
            compare_image(source, actual, width, height, mode);
            assert(SDL_SetRenderTarget(renderer, NULL) == 0);
        }
        gpu_output_filter_android_destroy(filter);
    }
    SDL_DestroyTexture(composition);
    SDL_DestroyTexture(input);
    free(actual);
    free(source);
}

/*
 * Damon V. Caskey
 * 2026-11-16
 * Preserve nondefault target, clip, scale, color mask and SDL shader attributes
 * across success and a failed GPU draw. Recovery uses the same GPU resources.
 */
static void check_state_and_failures(SDL_Renderer *renderer)
{
    const unsigned pixels[6] = { 0xFF1361AF, 0xFF2583CD, 0xFF4795EB,
                               0xFF59770D, 0xFF6B992F, 0xFF8DBB41 };
    unsigned actual[24], before_reads, before_uploads, before_warnings;
    SDL_Texture *input = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_BGR888,
                                            SDL_TEXTUREACCESS_STREAMING, 3, 2);
    SDL_Texture *outer = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_BGR888,
                                            SDL_TEXTUREACCESS_TARGET, 31, 19);
    SDL_Texture *wrong = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_BGR888,
                                            SDL_TEXTUREACCESS_STREAMING, 1, 1);
    gpu_output_filter_android *filter;
    SDL_Texture *target;
    SDL_Rect viewport = { 2, 1, 18, 12 }, clip = { 1, 2, 10, 6 };
    renderer_state state;
    assert(input && outer && wrong);
    assert(SDL_UpdateTexture(input, NULL, pixels, 12) == 0);
    assert(!gpu_output_filter_android_create(renderer, INT_MAX, 1, 1));
    assert(!gpu_output_filter_android_create(renderer, 3, 2, 0));
    assert(!gpu_output_filter_android_create(renderer, 3, 2, BLITTER_MAX));
    fail_shader = 1;
    assert(!gpu_output_filter_android_create(renderer, 3, 2, 1));
    fail_shader = 0;
    missing_function = 1;
    assert(!gpu_output_filter_android_create(renderer, 3, 2, 1));
    missing_function = 0;
    filter = gpu_output_filter_android_create(renderer, 3, 2, BLITTER_BILINEAR);
    assert(filter);
    assert(SDL_SetRenderTarget(renderer, outer) == 0);
    assert(SDL_RenderSetLogicalSize(renderer, 23, 17) == 0);
    assert(SDL_RenderSetScale(renderer, 1.25f, 1.5f) == 0);
    assert(SDL_RenderSetViewport(renderer, &viewport) == 0);
    assert(SDL_RenderSetClipRect(renderer, &clip) == 0);
    assert(SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND) == 0);
    assert(SDL_SetRenderDrawColor(renderer, 17, 83, 129, 197) == 0);
    assert(SDL_SetTextureBlendMode(input, SDL_BLENDMODE_BLEND) == 0);
    assert(SDL_RenderCopy(renderer, input, NULL, NULL) == 0);
    assert(SDL_RenderFlush(renderer) == 0);
    color_mask(GL_TRUE, GL_FALSE, GL_TRUE, GL_FALSE);
    enable(GL_CULL_FACE);
    state = capture_state(renderer);
    before_reads = readbacks;
    before_uploads = uploads;
    target = gpu_output_filter_android_apply(filter, input);
    assert(target);
    assert_state(renderer, &state);
    assert(readbacks == before_reads && uploads == before_uploads);
    assert(!gpu_output_filter_android_apply(filter, wrong));
    assert_state(renderer, &state);
    before_warnings = warnings;
    assert(!gpu_output_filter_android_apply(filter, wrong));
    assert(warnings == before_warnings);
    fail_draw = 1;
    assert(!gpu_output_filter_android_apply(filter, input));
    assert_state(renderer, &state);
    assert(gpu_output_filter_android_apply(filter, input) == target);
    assert_state(renderer, &state);
    color_mask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    disable(GL_CULL_FACE);
    assert(SDL_SetRenderTarget(renderer, NULL) == 0);
    read_target(renderer, target, actual, 6);
    compare_image(pixels, actual, 3, 2, BLITTER_BILINEAR);
    gpu_output_filter_android_destroy(filter);
    SDL_DestroyTexture(wrong);
    SDL_DestroyTexture(outer);
    SDL_DestroyTexture(input);
}

/* Damon V. Caskey - 2026-11-16: require a genuine SDL GLES renderer. */
int main(void)
{
    SDL_Window *window;
    SDL_Renderer *renderer;
    SDL_RendererInfo info;
    const GLubyte *(GL_APIENTRY *get_string)(GLenum);
    int index, driver = -1;
    if(SDL_Init(SDL_INIT_VIDEO) < 0) goto unavailable;
    SDL_SetHint(SDL_HINT_RENDER_BATCHING, "1");
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
    for(index = 0; index < SDL_GetNumRenderDrivers(); ++index)
        if(SDL_GetRenderDriverInfo(index, &info) == 0 && !strcmp(info.name, "opengles2")) driver = index;
    if(driver < 0) goto unavailable;
    window = SDL_CreateWindow("Android output-filter regression", 0, 0, 67, 43,
                                SDL_WINDOW_OPENGL | SDL_WINDOW_HIDDEN);
    if(!window) goto unavailable;
    renderer = SDL_CreateRenderer(window, driver, SDL_RENDERER_ACCELERATED | SDL_RENDERER_TARGETTEXTURE);
    if(!renderer) { SDL_DestroyWindow(window); goto unavailable; }
    get_string = __real_SDL_GL_GetProcAddress("glGetString");
    get_integer = __real_SDL_GL_GetProcAddress("glGetIntegerv");
    get_boolean = __real_SDL_GL_GetProcAddress("glGetBooleanv");
    get_attribute = __real_SDL_GL_GetProcAddress("glGetVertexAttribiv");
    get_pointer = __real_SDL_GL_GetProcAddress("glGetVertexAttribPointerv");
    is_enabled = __real_SDL_GL_GetProcAddress("glIsEnabled");
    color_mask = __real_SDL_GL_GetProcAddress("glColorMask");
    enable = __real_SDL_GL_GetProcAddress("glEnable");
    disable = __real_SDL_GL_GetProcAddress("glDisable");
    assert(get_string && get_integer && get_boolean && get_attribute && get_pointer);
    assert(is_enabled && color_mask && enable && disable);
    assert(strstr((const char *)get_string(GL_VERSION), "OpenGL ES"));
    printf("Android output-filter host context: %s\n", get_string(GL_VERSION));
    check_images(renderer, 1, 1, 0);
    check_images(renderer, 5, 3, 1);
    check_images(renderer, 256, 3, 0);
    check_state_and_failures(renderer);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    printf("Android GPU output filters passed: %u images, %u exact RGB8 pixels; no filtering readbacks or extra uploads.\n",
            images_checked, pixels_checked);
    return 0;
unavailable:
    fprintf(stderr, "SKIP: SDL GLES2 output-filter context unavailable: %s\n", SDL_GetError());
    SDL_Quit();
    return 77;
}
