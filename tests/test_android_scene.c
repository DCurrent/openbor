/*
 * Damon V. Caskey
 * 2026-11-20
 * Exercise the production Android video flow in genuine GLES3. Existing
 * independent filter/movie/font oracles supply expected complete images.
 * Test-only window reads are isolated at swap; production never sees them.
 */
#define main unused_android_movie_oracle_suite
#define movie_screen android_oracle_movie_screen
#include "test_gpu_movie.c"
#undef movie_screen
#undef main
#include <stdarg.h>
#include <signal.h>
#include <sys/wait.h>
#include <unistd.h>
#include "screen.h"
#include "video.h"
#include "launcher_text.h"
#include "output_filter_reference.h"
#define hankaku_font10 android_reference_font
#include "hankaku.h"
#undef hankaku_font10

#define ANDROID_WIDTH 32
#define ANDROID_HEIGHT 24
#define ANDROID_PIXELS (ANDROID_WIDTH * ANDROID_HEIGHT)
#define MAXTOUCHB 3

static struct
{
    int swfilter, hwfilter, fpslimit, usegl, fullscreen, stretch;
} savedata;
static unsigned touchstates[MAXTOUCHB];
static SDL_Rect btnsrc[MAXTOUCHB], btndes[MAXTOUCHB];
static const unsigned char buttonpng[] = {0};
static int hide_t, fixture_tick = 100;
static unsigned presents, renderer_creations, cpu_uploads, context_creations;
static SDL_GLContext live_contexts[32];
static unsigned live_context_count, context_deletions;
static unsigned shader_compilations, diagnostic_count, images_checked, pixels_checked;
static unsigned formatter_failures;
static unsigned presented[ANDROID_PIXELS];
static int reject_shader, reject_draw, reset_during_size, reject_context;
static size_t reject_malloc_size;
static s_screen *vscreen;
static int launcher_frame_ready, launcher_redraw;

#include "android_scene_video_globals.inc"
#include "android_scene_allocation_functions.inc"

/* Damon V. Caskey - 2026-11-20: replace asset decoding, never scene drawing. */
static SDL_Surface *pngToSurface(const unsigned char *unused)
{
    static const unsigned pixels[4] = {
        0xffc17123u, 0x8043b52du, 0x009ea353u, 0xff9257cfu
    };
    SDL_Surface *surface = SDL_CreateRGBSurfaceWithFormat(0, 2, 2, 32,
                                                        SDL_PIXELFORMAT_ABGR8888);
    (void)unused;
    assert(surface);
    memcpy(surface->pixels, pixels, sizeof(pixels));
    return surface;
}

static void setup_touch(void)
{
    SDL_Surface *surface = pngToSurface(buttonpng);
    assert(video_upload_buttons(surface));
    SDL_FreeSurface(surface);
}

static int timer_gettick(void)
{
    return fixture_tick;
}

#include "android_scene_video_functions.inc"
#include "android_scene_menu_functions.inc"

void __real_SDL_GL_SwapWindow(SDL_Window *target);
SDL_GLContext __real_SDL_GL_CreateContext(SDL_Window *target);
void __real_SDL_GL_DeleteContext(SDL_GLContext context);
void __real_SDL_GL_GetDrawableSize(SDL_Window *target, int *width, int *height);
int __real_SDL_UpdateTexture(SDL_Texture *, const SDL_Rect *, const void *, int);
SDL_Renderer *__real_SDL_CreateRenderer(SDL_Window *, int, Uint32);
void GL_APIENTRY __real_glGetShaderiv(GLuint shader, GLenum name, GLint *value);
void GL_APIENTRY __real_glCompileShader(GLuint shader);
void GL_APIENTRY __real_glDrawArrays(GLenum mode, GLint first, GLsizei count);
void *__real_malloc(size_t size);

/* Damon V. Caskey - 2026-11-20: observe the complete frame before real swap. */
void __wrap_SDL_GL_SwapWindow(SDL_Window *target)
{
    unsigned bottom_up[ANDROID_PIXELS];
    int x, y, width, height;
    __real_SDL_GL_GetDrawableSize(target, &width, &height);
    assert(width == ANDROID_WIDTH && height == ANDROID_HEIGHT);
    glReadPixels(0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, bottom_up);
    assert(glGetError() == GL_NO_ERROR);
    for(y = 0; y < height; ++y)
        for(x = 0; x < width; ++x)
        {
            presented[y * width + x] = bottom_up[(height - 1 - y) * width + x];
            /* RGB32 source padding is never display alpha. Touch keeps its
             * own fractional source alpha while blending onto opaque output. */
            assert((presented[y * width + x] >> 24) == 255);
        }
    ++presents;
    __real_SDL_GL_SwapWindow(target);
}

SDL_GLContext __wrap_SDL_GL_CreateContext(SDL_Window *target)
{
    SDL_GLContext context;
    unsigned index;
    ++context_creations;
    if(reject_context) return NULL;
    context = __real_SDL_GL_CreateContext(target);
    if(!context) return NULL;
    for(index = 0; index < sizeof(live_contexts) / sizeof(live_contexts[0]); ++index)
        if(!live_contexts[index])
        {
            live_contexts[index] = context;
            ++live_context_count;
            return context;
        }
    assert(!"context lifetime fixture capacity exceeded");
    return NULL;
}

/* Damon V. Caskey - 2026-11-20: every owned handle must retire exactly once. */
void __wrap_SDL_GL_DeleteContext(SDL_GLContext context)
{
    unsigned index;
    assert(context);
    for(index = 0; index < sizeof(live_contexts) / sizeof(live_contexts[0]); ++index)
        if(live_contexts[index] == context)
        {
            live_contexts[index] = NULL;
            --live_context_count;
            ++context_deletions;
            __real_SDL_GL_DeleteContext(context);
            return;
        }
    assert(!"double deletion or unowned SDL context");
}

/* Damon V. Caskey - 2026-11-20: reset after borrowing a texture must not restore it in place. */
void __wrap_SDL_GL_GetDrawableSize(SDL_Window *target, int *width, int *height)
{
    __real_SDL_GL_GetDrawableSize(target, width, height);
    if(reset_during_size)
    {
        reset_during_size = 0;
        SDL_AtomicSet(&video_reset_pending, 1);
    }
}

int __wrap_SDL_UpdateTexture(SDL_Texture *texture, const SDL_Rect *rectangle,
                              const void *pixels, int pitch)
{
    ++cpu_uploads;
    return __real_SDL_UpdateTexture(texture, rectangle, pixels, pitch);
}

SDL_Renderer *__wrap_SDL_CreateRenderer(SDL_Window *target, int index, Uint32 flags)
{
    ++renderer_creations;
    return __real_SDL_CreateRenderer(target, index, flags);
}

void GL_APIENTRY __wrap_glGetShaderiv(GLuint shader, GLenum name, GLint *value)
{
    __real_glGetShaderiv(shader, name, value);
    if(reject_shader && name == GL_COMPILE_STATUS) *value = GL_FALSE;
}

void GL_APIENTRY __wrap_glCompileShader(GLuint shader)
{
    ++shader_compilations;
    __real_glCompileShader(shader);
}

void GL_APIENTRY __wrap_glDrawArrays(GLenum mode, GLint first, GLsizei count)
{
    if(reject_draw)
    {
        reject_draw = 0;
        __real_glDrawArrays((GLenum)-1, first, count);
        return;
    }
    __real_glDrawArrays(mode, first, count);
}

void *__wrap_malloc(size_t size)
{
    if(reject_malloc_size && size == reject_malloc_size)
    {
        ++formatter_failures;
        return NULL;
    }
    return __real_malloc(size);
}

void __wrap_gpu_log(const char *format, ...)
{
    va_list args;
    va_start(args, format);
    vfprintf(stderr, format, args);
    fputc('\n', stderr);
    va_end(args);
    ++diagnostic_count;
}

/* Damon V. Caskey - 2026-11-20: compare all RGB bytes, with explicit interpolation tolerance. */
static void android_compare(const unsigned *actual, const unsigned *expected,
                             int count, int tolerance, const char *label)
{
    int index, shift;
    for(index = 0; index < count; ++index)
        for(shift = 0; shift < 24; shift += 8)
        {
            int got = (actual[index] >> shift) & 255;
            int want = (expected[index] >> shift) & 255;
            if(abs(got - want) > tolerance)
            {
                fprintf(stderr, "%s pixel %d channel %d: got %06x expected %06x (limit %d)\n",
                    label, index, shift / 8, actual[index] & 0xffffffu,
                    expected[index] & 0xffffffu, tolerance);
                abort();
            }
        }
    ++images_checked;
    pixels_checked += (unsigned)count;
}

/* Damon V. Caskey - 2026-11-20: reproduce documented display controls in scalar arithmetic. */
static unsigned android_correction(unsigned pixel, int gamma, int light)
{
    unsigned result = 0;
    int shift;
    for(shift = 0; shift < 24; shift += 8)
    {
        double value = ((pixel >> shift) & 255) / 255.0;
        double g = MAX(-256, MIN(256, gamma)) / 256.0;
        double b = MAX(-256, MIN(256, light)) / 256.0;
        if(g > 0) value = 1 - (1 - value) * (1 - value * g);
        else value *= 1 - (1 - value) * -g;
        if(b > 0) value = value * (1 - b) + b;
        else value *= 1 + b;
        result |= (unsigned)floor(value * 255 + 0.5) << shift;
    }
    return result;
}

/* Damon V. Caskey - 2026-11-20: final nearest sampling after independent filter reconstruction. */
static void android_expected_scene(const unsigned *source, int width, int height,
                                    int filter, int gamma, int light, unsigned *expected)
{
    int x, y;
    int filtered_width = filter ? width * 2 : width;
    int filtered_height = filter ? height * 2 : height;
    int output_width = stretch ? ANDROID_WIDTH : (int)(ANDROID_HEIGHT * ((float)width / height));
    int left = stretch ? 0 : (int)(ANDROID_WIDTH / 2.0f - ANDROID_HEIGHT * ((float)width / height) / 2.0f);
    for(y = 0; y < ANDROID_HEIGHT; ++y)
        for(x = 0; x < ANDROID_WIDTH; ++x)
        {
            unsigned pixel = 0;
            if(x >= left && x < left + output_width)
            {
                int sx = (int)(((x - left) + 0.5) * filtered_width / output_width);
                int sy = (int)((y + 0.5) * filtered_height / ANDROID_HEIGHT);
                pixel = output_filter_expected(source, width, height, filter, sx, sy);
            }
            expected[y * ANDROID_WIDTH + x] = android_correction(pixel, gamma, light);
        }
}

static s_videomodes android_mode(void)
{
    s_videomodes mode = {0};
    mode.hRes = 16;
    mode.vRes = 12;
    mode.pixel = 4;
    return mode;
}

static void android_hidden_touch(void)
{
    hide_t = 0;
    memset(touchstates, 0, sizeof(touchstates));
    memset(btndes, 0, sizeof(btndes));
}

/* Damon V. Caskey - 2026-11-20: normal composition/presentation stays resident across filters. */
static void android_scene(s_screen *screen, unsigned *pixels)
{
    unsigned expected[ANDROID_PIXELS], shadow[16 * 12];
    gpu_renderer_stats before;
    int x, y, filter;
    for(y = 0; y < screen->height; ++y)
        for(x = 0; x < screen->width; ++x)
            pixels[y * screen->width + x] =
                ((unsigned)(17 + 11*x + 7*y) & 255) |
                (((unsigned)(41 + 19*x + 3*y) & 255) << 8) |
                (((unsigned)(97 + 5*x + 13*y) & 255) << 16);
    memcpy(screen->data, pixels, sizeof(shadow));
    memcpy(shadow, pixels, sizeof(shadow));
    assert(gpu_draw_box(screen, 3, 2, 5, 4, 0x004f29b7u, &plainmethod));
    for(y = 2; y < 6; ++y)
        for(x = 3; x < 8; ++x) pixels[y * screen->width + x] = 0x004f29b7u;
    before = *gpu_gl_get_stats();
    for(filter = BLITTER_NONE; filter < BLITTER_MAX; ++filter)
    {
        savedata.swfilter = filter;
        assert(video_copy_screen(screen));
        android_expected_scene(pixels, 16, 12, filter, 0, 0, expected);
        android_compare(presented, expected, ANDROID_PIXELS, 0, "scene/output filter");
        assert(gpu_gl_get_stats()->readbacks == before.readbacks);
        assert(gpu_gl_get_stats()->uploads == before.uploads);
        assert(gpu_gl_get_stats()->fallback_draws == 0);
        assert(!memcmp(screen->data, shadow, sizeof(shadow)));
    }
    savedata.swfilter = BLITTER_NONE;
    video_set_color_correction(117, -57);
    assert(video_copy_screen(screen));
    android_expected_scene(pixels, 16, 12, 0, 117, -57, expected);
    android_compare(presented, expected, ANDROID_PIXELS, 1, "gamma then brightness");
    video_set_color_correction(-91, 73);
    video_stretch(0);
    /* A taller authored screen exercises corrected bars independently. */
    {
        s_screen *tall = allocscreen(8, 12, PIXEL_32);
        unsigned tall_pixels[8 * 12];
        assert(tall);
        for(x = 0; x < 8 * 12; ++x) tall_pixels[x] = 0x004a769bu;
        memcpy(tall->data, tall_pixels, sizeof(tall_pixels));
        assert(video_copy_screen(tall));
        android_expected_scene(tall_pixels, 8, 12, 0, -91, 73, expected);
        android_compare(presented, expected, ANDROID_PIXELS, 1, "aspect and corrected bars");
        freescreen(&tall);
    }
    video_stretch(1);
    video_set_color_correction(0, 0);
}

/* Damon V. Caskey - 2026-11-20: touch follows correction; clipped skins retain destination size. */
static void android_touch(s_screen *screen, const unsigned *pixels)
{
    static const unsigned texels[4] = {0xffc17123u, 0x8043b52du, 0x009ea353u, 0xff9257cfu};
    unsigned expected[ANDROID_PIXELS];
    int held, x, y, shift;
    video_set_color_correction(79, -43);
    hide_t = fixture_tick + 1;
    btnsrc[0] = (SDL_Rect){-1, -1, 3, 3};
    btndes[0] = (SDL_Rect){5, 4, 8, 8};
    for(held = 0; held <= 1; ++held)
    {
        touchstates[0] = held;
        android_expected_scene(pixels, 16, 12, 0, 79, -43, expected);
        for(y = 4; y < 12; ++y)
            for(x = 5; x < 13; ++x)
            {
                unsigned texel = texels[((y - 4) / 4) * 2 + (x - 5) / 4];
                unsigned result = 0, previous = expected[y * ANDROID_WIDTH + x];
                double alpha = (texel >> 24) / 255.0 * (held ? 128 : 64) / 255.0;
                for(shift = 0; shift < 24; shift += 8)
                    result |= (unsigned)floor(((texel >> shift) & 255) * alpha +
                        ((previous >> shift) & 255) * (1 - alpha) + 0.5) << shift;
                expected[y * ANDROID_WIDTH + x] = result;
            }
        assert(video_copy_screen(screen));
        android_compare(presented, expected, ANDROID_PIXELS, 1, "touch alpha/source clipping");
    }
    btnsrc[0] = (SDL_Rect){9, 9, 2, 2};
    assert(video_copy_screen(screen));
    android_expected_scene(pixels, 16, 12, 0, 79, -43, expected);
    android_compare(presented, expected, ANDROID_PIXELS, 1, "outside touch source");
    android_hidden_touch();
    video_set_color_correction(0, 0);
}

/* Damon V. Caskey - 2026-11-20: expand original immutable font rows independently. */
static void android_reference_text(unsigned *pixels, int x, int y, const char *text)
{
    int index, row, column;
    for(index = 0; text[index]; ++index)
        for(row = 0; row < 10; ++row)
            for(column = 0; column < 5; ++column)
            {
                int dx = x + index * 5 + column, dy = y + row;
                unsigned glyph = (unsigned char)text[index] - 32;
                if(dx >= 0 && dx < 16 && dy >= 0 && dy < 12 &&
                   ((android_reference_font[glyph * 10 + row] >> column) & 1))
                    pixels[dy * 16 + dx] = 0x0091df37u;
            }
}

/* Damon V. Caskey - 2026-11-20: real launcher/menu order and complete-frame gating. */
static void android_launcher(void)
{
    unsigned pixels[16 * 12], expected[ANDROID_PIXELS];
    s_screen *preview = allocscreen(4, 2, PIXEL_x8);
    unsigned *palette;
    unsigned before;
    int index;
    char long_text[160];
    const unsigned char indexes[8] = {0, 1, 0, 1, 0, 1, 0, 1};
    vscreen = allocscreen(16, 12, PIXEL_32);
    assert(vscreen && preview);
    memset(vscreen->data, 0, sizeof(pixels));
    palette = (unsigned *)preview->palette;
    palette[0] = 0x005ea137u;
    palette[1] = 0x00c19473u;
    memcpy(preview->data, indexes, sizeof(indexes));
    launcher_frame_ready = video_launcher_begin(vscreen);
    launcher_frame_ready &= video_launcher_box(vscreen, 0, 0, 16, 12, 0x00351763u);
    for(index = 0; index < 16 * 12; ++index) pixels[index] = 0x00351763u;
    printText(1, 1, 0x0091df37u, 0, 0, "%s", "AZ");
    android_reference_text(pixels, 1, 1, "AZ");
    launcher_frame_ready &= video_launcher_screen(vscreen, preview, 4, 5);
    for(index = 0; index < 8; ++index)
        pixels[(5 + index / 4) * 16 + 4 + index % 4] = palette[index & 1];
    freescreen(&preview);
    assert(launcher_frame_ready);
    before = presents;
    blit_video_menu(vscreen);
    assert(!launcher_redraw && presents == before + 1);
    android_expected_scene(pixels, 16, 12, 0, 0, 0, expected);
    android_compare(presented, expected, ANDROID_PIXELS, 0, "launcher ordering and opaque index zero");
    for(index = 0; index < 16 * 12; ++index) assert(((unsigned *)vscreen->data)[index] == 0);

    /* Formatting can fail before the native call; the real menu gate must
     * still prevent presenting an otherwise valid but incomplete target. */
    memset(long_text, 'W', sizeof(long_text) - 1);
    long_text[sizeof(long_text) - 1] = 0;
    reject_malloc_size = sizeof(long_text);
    printText(0, 0, 1, 0, 0, "%s", long_text);
    reject_malloc_size = 0;
    assert(formatter_failures == 1 && !launcher_frame_ready);
    blit_video_menu(vscreen);
    assert(launcher_redraw && presents == before + 1);
    assert(video_launcher_begin(vscreen));
    assert(!video_launcher_screen(vscreen, NULL, 0, 0));
    assert(!video_copy_screen(vscreen) && presents == before + 1);
    freescreen(&vscreen);
}

/* Damon V. Caskey - 2026-11-20: every kernel reaches the final movie target once. */
static void android_movie(s_screen *gameplay, const unsigned *gameplay_pixels)
{
    movie_fixture fixture;
    yuv_video_mode mode = {6, 4, 1, 1};
    unsigned expected[ANDROID_PIXELS];
    unsigned long reads;
    int resample, index;
    movie_pattern(&fixture, mode.width, mode.height, 7);
    assert(video_setup_yuv_overlay(&mode));
    video_set_color_correction(-37, 53);
    video_stretch(0);
    for(resample = DC_RESAMPLE_NEAREST; resample < DC_RESAMPLE_END; ++resample)
    {
        gpu_movie_draw draw = {.x = 4, .y = 0, .width = 24, .height = 24,
            .resample = resample, .gamma = -37.0f / 256, .brightness = 53.0f / 256};
        reads = gpu_gl_get_stats()->readbacks;
        assert(video_prepare_yuv_frame(&fixture.source, resample));
        assert(video_display_yuv_frame());
        movie_expected(&fixture.source, &draw, ANDROID_WIDTH, ANDROID_HEIGHT, expected);
        for(index = 0; index < ANDROID_PIXELS; ++index)
            if(index % ANDROID_WIDTH < 4 || index % ANDROID_WIDTH >= 28)
                expected[index] = android_correction(0, -37, 53);
        android_compare(presented, expected, ANDROID_PIXELS,
                         resample == DC_RESAMPLE_NEAREST ? 1 : 3, "final-resolution movie");
        assert(gpu_gl_get_stats()->readbacks == reads);
    }
    assert(gpu_gl_get_stats()->movie_uploads == 1);
    {
        s_screen *capture = allocscreen(8, 8, PIXEL_32);
        gpu_movie_draw draw = {.width = 8, .height = 8,
            .resample = DC_RESAMPLE_BICUBIC_MITCHELL,
            .gamma = -37.0f / 256, .brightness = 53.0f / 256};
        assert(capture);
        memset(capture->data, 0x69, 8 * 8 * 4);
        reads = gpu_gl_get_stats()->readbacks;
        assert(video_capture_yuv_frame(capture, &fixture.source, draw.resample));
        assert(gpu_gl_get_stats()->readbacks == reads && capture->data[0] == 0x69);
        gpu_sync_cpu(capture);
        assert(gpu_gl_get_stats()->readbacks == reads + 1);
        movie_expected(&fixture.source, &draw, 8, 8, expected);
        android_compare((unsigned *)capture->data, expected, 64, 3, "explicit movie capture");
        freescreen(&capture);
    }
    assert(video_restore_mode());
    video_stretch(1);
    video_set_color_correction(0, 0);
    assert(video_copy_screen(gameplay));
    android_expected_scene(gameplay_pixels, 16, 12, 0, 0, 0, expected);
    android_compare(presented, expected, ANDROID_PIXELS, 0, "gameplay retained after movie");
    gpu_release_yuv(fixture.source.owner);
}

/* Damon V. Caskey - 2026-11-20: simulate SDL's actual watch, then event dispatch. */
static void android_event(Uint32 type, int watched)
{
    SDL_Event event;
    memset(&event, 0, sizeof(event));
    event.type = type;
    if(watched) video_native_watch(NULL, &event);
    video_handle_event(&event);
}

static void android_replace_context(void)
{
    SDL_GLContext replacement;
    unsigned count = live_context_count;
    /* SDL's Android resume path creates a replacement without deleting the
     * old tracked handle. The video boundary must adopt and retire it. */
    assert(SDL_GL_MakeCurrent(window, NULL) == 0);
    replacement = SDL_GL_CreateContext(window);
    assert(replacement && replacement != video_context);
    assert(live_context_count == count + 1);
    assert(LoadGLFunctions());
}

/* Damon V. Caskey - 2026-11-20: owner-level recovery, including late reset delivery. */
static void android_lifecycle(s_screen *screen, unsigned *pixels)
{
    unsigned expected[ANDROID_PIXELS];
    unsigned long reads;
    uint64_t generation;
    int index;
    movie_fixture fixture;
    yuv_video_mode movie_mode = {2, 2, 1, 1};
    assert(gpu_draw_box(screen, 0, 0, 16, 12, 0x00619ac3u, &plainmethod));
    for(index = 0; index < 16 * 12; ++index) pixels[index] = 0x00619ac3u;
    assert(video_set_mode(android_mode()));
    android_compare((unsigned *)screen->data, pixels, 16 * 12, 0, "mode change preservation");
    assert(gpu_draw_box(screen, 0, 0, 16, 12, 0x003c81b9u, &plainmethod));
    for(index = 0; index < 16 * 12; ++index) pixels[index] = 0x003c81b9u;
    reads = gpu_gl_get_stats()->readbacks;
    android_event(SDL_APP_WILLENTERBACKGROUND, 0);
    assert(gpu_gl_get_stats()->readbacks == reads + 1);
    android_event(SDL_APP_WILLENTERBACKGROUND, 0);
    assert(gpu_gl_get_stats()->readbacks == reads + 1);
    assert(!gpu_draw_box(screen, 0, 0, 1, 1, 0, &plainmethod));
    assert(!gpu_allow_cpu_operation(screen, GPU_OPERATION_BOX));
    assert(!video_copy_screen(screen));
    android_compare((unsigned *)screen->data, pixels, 16 * 12, 0, "background preservation");
    android_replace_context();
    /* SDL queues reset while resuming; dispatch can occur after foreground. */
    {
        SDL_Event event = {0};
        event.type = SDL_RENDER_DEVICE_RESET;
        video_native_watch(NULL, &event);
    }
    android_event(SDL_APP_DIDENTERFOREGROUND, 0);
    assert(live_context_count == 1);
    generation = gpu_gl_get_context_generation();
    android_event(SDL_RENDER_DEVICE_RESET, 0);
    assert(gpu_gl_get_context_generation() == generation);
    assert(video_copy_screen(screen));
    android_expected_scene(pixels, 16, 12, 0, 0, 0, expected);
    android_compare(presented, expected, ANDROID_PIXELS, 0, "pause/replacement/late reset");
    reads = gpu_gl_get_stats()->readbacks;
    assert(video_copy_screen(screen));
    assert(gpu_gl_get_stats()->readbacks == reads);

    /* Reset in the borrowed-texture boundary rejects this frame without
     * rebuilding GL names underneath it; next owner access reacquires them. */
    {
        unsigned swaps = presents;
        generation = gpu_gl_get_context_generation();
        reset_during_size = 1;
        assert(!video_copy_screen(screen));
        assert(presents == swaps && gpu_gl_get_context_generation() == generation);
        assert(video_copy_screen(screen));
        assert(gpu_gl_get_context_generation() > generation);
        android_compare(presented, expected, ANDROID_PIXELS, 0, "borrowed texture reset recovery");
    }

    /* Disposable fullscreen reconstruction must not turn a recoverable loss
     * into loss of script data, and an old prepared frame must not be shown. */
    movie_constant(&fixture, 117, 91, 183);
    assert(video_setup_yuv_overlay(&movie_mode));
    assert(video_prepare_yuv_frame(&fixture.source, DC_RESAMPLE_MOVIE));
    generation = gpu_gl_get_context_generation();
    android_replace_context();
    android_event(SDL_RENDER_DEVICE_RESET, 1);
    assert(live_context_count == 1);
    assert(gpu_gl_get_context_generation() > generation);
    assert(!video_display_yuv_frame());
    assert(video_prepare_yuv_frame(&fixture.source, DC_RESAMPLE_MOVIE));
    assert(video_display_yuv_frame());
    assert(video_restore_mode());
    assert(video_copy_screen(screen));
    android_compare(presented, expected, ANDROID_PIXELS, 0, "private movie discarded on reset");
    gpu_release_yuv(fixture.source.owner);

    /* A new explicit mode can arrive before the platform replacement has
     * been adopted. Both distinct old handles must retire exactly once. */
    android_replace_context();
    {
        unsigned deleted = context_deletions;
        assert(video_set_mode(android_mode()));
        assert(context_deletions == deleted + 2 && live_context_count == 1);
    }
    assert(video_copy_screen(screen));
    android_compare(presented, expected, ANDROID_PIXELS, 0, "mode retires tracked and replacement contexts");
}

/* Damon V. Caskey - 2026-11-20: initialization/draw failure latches never select CPU. */
static void android_failures(s_screen *screen)
{
    unsigned swaps = presents, compiles, creates, logs;
    int retry;
    reject_draw = 1;
    assert(!video_copy_screen(screen) && presents == swaps);
    assert(video_copy_screen(screen) && presents == swaps + 1);
    reject_shader = 1;
    assert(!video_set_mode(android_mode()));
    compiles = shader_compilations;
    creates = context_creations;
    logs = diagnostic_count;
    swaps = presents;
    for(retry = 0; retry < 8; ++retry)
    {
        assert(!video_copy_screen(screen));
        assert(!gpu_draw_box(screen, 0, 0, 1, 1, 0, &plainmethod));
        assert(!gpu_allow_cpu_operation(screen, GPU_OPERATION_BOX));
    }
    assert(compiles == shader_compilations && creates == context_creations);
    assert(logs == diagnostic_count && swaps == presents);
    reject_shader = 0;
    assert(video_set_mode(android_mode()) && video_copy_screen(screen));
    reject_context = 1;
    assert(!video_set_mode(android_mode()));
    creates = context_creations;
    for(retry = 0; retry < 8; ++retry) assert(!video_copy_screen(screen));
    assert(creates == context_creations && !gpu_allow_cpu_operation(screen, GPU_OPERATION_SCREEN));
    reject_context = 0;
    assert(video_set_mode(android_mode()));
}

static int android_start(void)
{
    if(SDL_Init(SDL_INIT_VIDEO) < 0) return 0;
    gpu_test_context_attributes();
    window = SDL_CreateWindow("OpenBOR Android native scene", 0, 0,
        ANDROID_WIDTH, ANDROID_HEIGHT, SDL_WINDOW_OPENGL | SDL_WINDOW_HIDDEN);
    if(!window) return 0;
    nativeWidth = ANDROID_WIDTH;
    nativeHeight = ANDROID_HEIGHT;
    savedata.hwfilter = 1;
    savedata.stretch = 1;
    android_hidden_touch();
    return video_set_mode(android_mode()) && LoadGLFunctions();
}

int main(int argc, char **argv)
{
    s_screen *screen;
    unsigned pixels[16 * 12];
    if(!android_start()) return 77;
    screen = allocscreen(16, 12, PIXEL_32);
    assert(screen);
    memset(screen->data, 0, sizeof(pixels));
    if(argc > 1 && !strcmp(argv[1], "--lost-dirty"))
    {
        assert(gpu_draw_box(screen, 0, 0, 16, 12, 0x0075a9c3u, &plainmethod));
        android_replace_context();
        android_event(SDL_RENDER_DEVICE_RESET, 1);
        return 1;
    }
    android_scene(screen, pixels);
    android_touch(screen, pixels);
    android_launcher();
    android_movie(screen, pixels);
    android_lifecycle(screen, pixels);
    android_failures(screen);
    assert(!renderer_creations && !cpu_uploads);
    freescreen(&screen);
    video_shutdown();
    assert(live_context_count == 0);
    SDL_Quit();
    /* Re-exec creates a fresh Mesa runtime, avoiding forked driver threads. */
    {
        pid_t child = fork();
        int status;
        assert(child >= 0);
        if(!child)
        {
            execl(argv[0], argv[0], "--lost-dirty", (char *)NULL);
            _exit(127);
        }
        assert(waitpid(child, &status, 0) == child);
        assert(WIFSIGNALED(status) && WTERMSIG(status) == SIGABRT);
    }
    printf("Android native video passed: %u complete images / %u pixels; resident scenes, filters, touch, launcher, movies, mode/pause/reset recovery and explicit failure.\n",
        images_checked, pixels_checked);
    return 0;
}
