/*
 * Exercise the production SDL/OpenGL presenter and context lifecycle.
 * Run through tests/run_gpu_tests.sh, or compile with the native renderer test
 * flags plus video.c, opengl.c, videocommon.c, timer.c and gfx.c. The native
 * test fixtures are reused with their independent
 * entry point renamed; --gc-sections discards the unused test suite.
 */
#define main unused_backend_test_suite
#include "test_gpu_renderer.c"
#undef main
#include "savedata.h"
#include "video.h"
#include "videocommon.h"
#include "opengl.h"
#include "gfx.h"

s_savedata savedata;
extern SDL_Window *window;
extern int nativeWidth, nativeHeight;
extern s_videomodes stored_videomodes;
extern void video_gl_setup_screen(void);

/*
 * Damon V. Caskey
 * 2026-11-12
 * Supply the SDL platform sleep helper to this isolated video-backend test.
 */
void _usleep(u32 microseconds)
{
    /* SDL accepts whole milliseconds; round upward to preserve the requested
     * minimum delay when the engine supplies a fractional millisecond. */
    SDL_Delay((microseconds + 999) / 1000);
}

/*
 * Damon V. Caskey
 * 2026-11-12
 * Inspect the displayed framebuffer to catch vertical inversion or missing
 * presentation-state restoration after native render-to-texture operations.
 */
static void verify_presented_frame(void)
{
    void (APIENTRY *read_buffer)(GLenum) = SDL_GL_GetProcAddress("glReadBuffer");
    unsigned pixels[64];

    /* Inspect the front buffer after presentation. GL's bottom-up row order
     * maps the logical bottom green strip to the first sampled row and the
     * logical top red strip to the last one. Restore the normal read target. */
    assert(read_buffer);
    read_buffer(GL_FRONT);
    glReadPixels(0, 0, 8, 8, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
    assert((pixels[2] & 0x00ffffff) == 0x0000ff00);
    assert((pixels[58] & 0x00ffffff) == 0x000000ff);
    assert((pixels[18] & 0x00ffffff) == 0x00123456);
    read_buffer(GL_BACK);
}

/*
 * Damon V. Caskey
 * 2026-11-12
 * Return to cached native drawing after public presentation helpers change
 * the current program, viewport or texture bindings outside a frame handoff.
 */
static void test_presenter_state_boundaries(s_screen *screen)
{
    void (APIENTRY *get_texture_parameter)(GLenum, GLenum, GLint *) = SDL_GL_GetProcAddress("glGetTexParameteriv");
    s_screen *target = test_screen(16, 16, PIXEL_32);
    unsigned texture, external_texture;
    int width, height;
    GLint minimum_filter, maximum_filter;

    /* Runtime gamma/brightness updates select the presentation program even
     * when their values are unchanged. Native draws must reselect their own. */
    assert(gpu_draw_box(target, 10, 10, 3, 3, 0x00123456, &plainmethod));
    video_gl_set_color_correction(0, 0);
    assert(gpu_draw_box(target, 10, 10, 3, 3, 0x00654321, &plainmethod));
    gpu_sync_cpu(target);
    assert(test_pixel(target, 11, 11) == 0x00654321);

    /* The window viewport is smaller than this native target, so retaining
     * the presenter's viewport would move or discard the tested rectangle. */
    assert(gpu_draw_box(target, 10, 10, 3, 3, 0x00654321, &plainmethod));
    video_gl_setup_screen();
    assert(gpu_draw_box(target, 10, 10, 3, 3, 0x0013579b, &plainmethod));
    gpu_sync_cpu(target);
    assert(test_pixel(target, 11, 11) == 0x0013579b);

    /* Another context owner can change unit-zero ownership. Native sampling
     * must recover even though its cached source handle remains stable. */
    assert(gpu_draw_screen(target, screen, 0, 0, &plainmethod));
    gpu_gl_invalidate_state();
    glGenTextures(1, &external_texture);
    assert(external_texture);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, external_texture);
    assert(gpu_draw_screen(target, screen, 8, 0, &plainmethod));
    gpu_sync_cpu(target);
    assert(test_pixel(target, 10, 0) == 0x000000ff);
    assert(test_pixel(target, 10, 7) == 0x0000ff00);
    glDeleteTextures(1, &external_texture);

    /* The presenter temporarily requests linear display filtering. Its
     * borrowed native texture must be returned with pixel-art sampling. */
    assert(video_copy_screen(screen));
    assert(gpu_draw_screen(target, screen, 0, 8, &plainmethod));
    gpu_sync_cpu(target);
    assert(test_pixel(target, 2, 8) == 0x000000ff);
    assert(test_pixel(target, 2, 15) == 0x0000ff00);
    assert(gpu_gl_get_screen_texture(screen, &texture, &width, &height));
    assert(width == 8 && height == 8);
    gpu_gl_invalidate_state();
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture);
    assert(get_texture_parameter);
    get_texture_parameter(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, &minimum_filter);
    get_texture_parameter(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, &maximum_filter);
    assert(minimum_filter == GL_NEAREST && maximum_filter == GL_NEAREST);
    release_test_screen(target);
}

/*
 * Damon V. Caskey
 * 2026-11-16
 * Keep selected display filters on the native scene path through consecutive
 * presentations. Exact kernel pixels are covered by the independent suite.
 */
static void test_native_output_filters(s_screen *source)
{
    void (APIENTRY *read_buffer)(GLenum) = SDL_GL_GetProcAddress("glReadBuffer");
    gpu_renderer_stats before;
    unsigned stale_pixel = test_pixel(source, 2, 2), texture;
    int filter, width, height;
    assert(source->pixelformat == PIXEL_32);
    assert(stored_videomodes.pixel == 4);
    assert(stored_videomodes.hRes == 8 && stored_videomodes.vRes == 8);
    assert(gpu_draw_box(source, 0, 0, 8, 8, 0x00345678, &plainmethod));
    assert(gpu_gl_get_screen_texture(source, &texture, &width, &height));
    for(filter = 1; filter < BLITTER_MAX; ++filter)
    {
        savedata.swfilter = filter;
        before = *gpu_gl_get_stats();
        assert(video_copy_screen(source));
        assert(gpu_is_active());
        assert(!gpu_allow_cpu_operation(source, GPU_OPERATION_SCREEN));
        assert(gpu_gl_get_stats()->readbacks == before.readbacks);
        assert(gpu_gl_get_stats()->uploads == before.uploads);
        assert(gpu_gl_get_stats()->output_filter_draws == before.output_filter_draws + 1);
        assert(test_pixel(source, 2, 2) == stale_pixel);
        if(filter == BLITTER_SCANLINES)
        {
            unsigned pixels[8 * 8];
            int drawable_width, drawable_height;
            SDL_GL_GetDrawableSize(window, &drawable_width, &drawable_height);
            assert(read_buffer && drawable_width == 16 && drawable_height == 16);
            memset(pixels, 0xa5, sizeof(pixels));
            read_buffer(GL_FRONT);
            /* The offscreen SDL driver can retain its original pbuffer size
             * after a logical window resize. Sample the shared lower 8x8 area
             * so the same assertions also exercise ordinary resizable windows. */
            glReadPixels(0, 0, 8, 8, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
            read_buffer(GL_BACK);
            /* The window must consume the filtered texture itself. These
             * alternating rows also retain colors below 565 precision. */
            if((pixels[8 + 4] & 0x00ffffffu) != 0x00345678u ||
               (pixels[4] & 0x00ffffffu) != 0)
                fprintf(stderr, "Displayed scanlines: upper=%06x lower=%06x, expected 345678/000000\n",
                        pixels[8 + 4] & 0x00ffffffu, pixels[4] & 0x00ffffffu);
            assert((pixels[8 + 4] & 0x00ffffffu) == 0x00345678u);
            assert((pixels[4] & 0x00ffffffu) == 0);
        }
        assert(gpu_draw_box(source, 0, 0, 1, 1, (unsigned)filter, &plainmethod));
    }
    /* Display scale controls activation without suspending native rendering. */
    savedata.swfilter = BLITTER_BILINEAR;
    savedata.hwscale = 1.0f;
    before = *gpu_gl_get_stats();
    assert(video_copy_screen(source));
    assert(gpu_gl_get_stats()->output_filter_draws == before.output_filter_draws);
    assert(gpu_gl_get_stats()->readbacks == before.readbacks);
    savedata.hwscale = 2.0f;
}

/*
 * Damon V. Caskey
 * 2026-11-15
 * Reject non-RGB32 scene requests before changing a live context, screen
 * ownership or the valid authored mode used by later restore operations.
 */
static void test_scene_format_rejection(s_videomodes valid_mode, s_screen *screen)
{
    const int formats[] = {0, 1, 2, 3, 5};
    SDL_Window *original_window = window;
    SDL_GLContext original_context = SDL_GL_GetCurrentContext();
    unsigned original_texture, texture;
    unsigned long uploads, readbacks;
    int width, height;
    size_t i;

    /* Flush the accepted image before the snapshot so rejection itself cannot
     * hide teardown, recreation, a CPU readback or a replacement upload. */
    assert(gpu_gl_get_screen_texture(screen, &original_texture, &width, &height));
    assert(width == 8 && height == 8);
    uploads = gpu_gl_get_stats()->uploads;
    readbacks = gpu_gl_get_stats()->readbacks;
    for(i = 0; i < sizeof(formats) / sizeof(formats[0]); ++i)
    {
        s_videomodes invalid_mode = valid_mode;
        invalid_mode.pixel = formats[i];
        assert(!video_set_mode(invalid_mode));
        assert(window == original_window);
        assert(SDL_GL_GetCurrentContext() == original_context);
        assert(opengl && gpu_is_active() && savedata.usegl == 1);
        assert(stored_videomodes.pixel == 4);
        assert(stored_videomodes.hRes == valid_mode.hRes);
        assert(stored_videomodes.vRes == valid_mode.vRes);
        assert(gpu_gl_get_screen_texture(screen, &texture, &width, &height));
        assert(texture == original_texture && width == 8 && height == 8);
        assert(gpu_gl_get_stats()->uploads == uploads);
        assert(gpu_gl_get_stats()->readbacks == readbacks);
    }
    assert(!gpu_allow_cpu_operation(screen, GPU_OPERATION_SCREEN));
    assert(video_copy_screen(screen));
}

/*
 * Damon V. Caskey
 * 2026-11-15
 * Compare logical RGB channels while allowing one framebuffer rounding unit.
 */
static void test_movie_color(unsigned actual, unsigned expected)
{
    int channel;
    for(channel = 0; channel < 3; ++channel)
    {
        int value = (actual >> (channel*8)) & 255;
        int wanted = (expected >> (channel*8)) & 255;
        assert(abs(value-wanted) <= 1);
    }
}

/*
 * Damon V. Caskey
 * 2026-11-15
 * Real video entry points share resident planes between fullscreen output and
 * explicit capture. Correction must precede clipping, screenshots remain GPU
 * authoritative until requested, and movie layout never suspends scene draws.
 */
static void test_native_movie_presentation(s_screen *screen)
{
    yuv_video_mode mode = {6, 8, 8, 8};
    unsigned char luminance[48], chroma_u[12], chroma_v[12];
    gpu_yuv_source source = {
        .owner = luminance, .generation = 1, .width = 6, .height = 8,
        .y = luminance, .u = chroma_u, .v = chroma_v
    };
    s_screen *capture = test_screen(6, 8, PIXEL_32);
    SDL_GLContext context = SDL_GL_GetCurrentContext();
    void (APIENTRY *read_buffer)(GLenum) = SDL_GL_GetProcAddress("glReadBuffer");
    unsigned pixels[64], stale_pixel;
    gpu_renderer_stats before;
    int index;

    assert(read_buffer);
    assert(gpu_draw_box(screen, 0, 0, 8, 8, 0x00887766, &plainmethod));
    stale_pixel = test_pixel(screen, 2, 2);
    before = *gpu_gl_get_stats();
    assert(video_setup_yuv_overlay(&mode));
    assert(gpu_is_active());
    assert(!gpu_allow_cpu_operation(screen, GPU_OPERATION_SCREEN));
    assert(SDL_GL_GetCurrentContext() == context);
    assert(gpu_gl_get_stats()->readbacks == before.readbacks);
    assert(test_pixel(screen, 2, 2) == stale_pixel);

    /* RGB before clipping is approximately (434.54,207.12,29.98). Gamma +1
     * produces (129,246,56); an RGB32 conversion first would give R=255. */
    memset(luminance, 235, sizeof(luminance));
    memset(chroma_u, 16, sizeof(chroma_u));
    memset(chroma_v, 240, sizeof(chroma_v));
    video_gl_set_color_correction(256, 0);
    assert(video_prepare_yuv_frame(&source, DC_RESAMPLE_MOVIE));
    assert(video_display_yuv_frame());
    assert(gpu_is_active());
    assert(gpu_gl_get_stats()->movie_uploads == before.movie_uploads+1);
    assert(gpu_gl_get_stats()->readbacks == before.readbacks);
    read_buffer(GL_FRONT);
    glReadPixels(0, 0, 8, 8, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
    read_buffer(GL_BACK);
    test_movie_color(pixels[4*8+4], 0x0038f681);

    before = *gpu_gl_get_stats();
    assert(video_capture_yuv_frame(capture, &source, DC_RESAMPLE_MOVIE));
    assert(gpu_gl_get_stats()->movie_uploads == before.movie_uploads);
    assert(gpu_gl_get_stats()->readbacks == before.readbacks);
    for(index = 0; index < 48; ++index) assert(((unsigned *)capture->data)[index] == 0);
    gpu_sync_cpu(capture);
    for(index = 0; index < 48; ++index)
        test_movie_color(((unsigned *)capture->data)[index], 0x0038f681);

    /* Scene presentation is valid immediately after movies and capture.
     * Restoring a healthy normal mode retains the context and source cache. */
    video_gl_set_color_correction(0, 0);
    before = *gpu_gl_get_stats();
    assert(video_copy_screen(screen));
    assert(gpu_gl_get_stats()->readbacks == before.readbacks);
    read_buffer(GL_FRONT);
    glReadPixels(0, 0, 8, 8, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
    read_buffer(GL_BACK);
    test_movie_color(pixels[4*8+4], 0x00887766);
    assert(video_restore_mode());
    assert(SDL_GL_GetCurrentContext() == context);
    assert(gpu_is_active());
    assert(gpu_gl_get_stats()->readbacks == before.readbacks);
    assert(gpu_gl_get_stats()->movie_resources == before.movie_resources);
    assert(video_prepare_yuv_frame(&source, DC_RESAMPLE_BICUBIC_MITCHELL));
    assert(video_display_yuv_frame());
    assert(gpu_gl_get_stats()->movie_uploads == before.movie_uploads);
    assert(gpu_gl_get_stats()->readbacks == before.readbacks);
    assert(video_restore_mode());
    gpu_release_yuv(source.owner);
    release_test_screen(capture);
}

/*
 * Damon V. Caskey
 * 2026-11-12
 * 2026-11-15 - Require native RGB32 through mode failures and context teardown.
 * 2026-11-15 - Reject removed scene formats and exercise real filter storage.
 * 2026-11-16 - Keep output filters and movie composition on native RGB32.
 * Exercise production presentation, mode switching, GPU output filters and
 * YUV transitions without linking unrelated game/audio subsystems.
 */
int main(void)
{
    s_videomodes mode = { .hRes = 8, .vRes = 8, .pixel = 4 };
    s_videomodes closed = {0};
    s_screen *screen = test_screen(8, 8, PIXEL_32);
    unsigned long readbacks;

    /* Use the real video lifecycle so context selection and GPU registration
     * match engine startup. Missing platform support is reported as a skip. */
    if(SDL_Init(SDL_INIT_VIDEO) != 0)
    {
        fprintf(stderr, "SKIP: SDL video unavailable: %s\n", SDL_GetError());
        free(screen);
        return 77;
    }
    nativeWidth = nativeHeight = 64;
    /* A legacy SDL preference must not select software for migrated RGB32. */
    savedata.usegl = 0;
    savedata.hwscale = 1.0f;
    if(!video_set_mode(mode) || !opengl)
    {
        fprintf(stderr, "SKIP: GL presentation unavailable: %s\n", SDL_GetError());
        free(screen);
        SDL_Quit();
        return 77;
    }
    /* Distinct top/bottom strips expose inverted presentation, while the
     * center fill verifies pixels survive the native-target handoff. */
    assert(gpu_is_active());
    assert(savedata.usegl == 1);
    assert(gpu_draw_box(screen, 0, 0, 8, 8, 0x00123456, &plainmethod));
    assert(gpu_draw_box(screen, 0, 0, 8, 1, 0x000000ff, &plainmethod));
    assert(gpu_draw_box(screen, 0, 7, 8, 1, 0x0000ff00, &plainmethod));
    readbacks = gpu_gl_get_stats()->readbacks;
    assert(video_copy_screen(screen));
    assert(gpu_gl_get_stats()->readbacks == readbacks);
    verify_presented_frame();
    test_presenter_state_boundaries(screen);

    /* The old context must preserve live screen contents before replacement. */
    assert(video_set_mode(mode));
    assert(opengl && gpu_is_active());
    assert(test_pixel(screen, 2, 2) == 0x00123456);
    assert(gpu_draw_box(screen, 0, 0, 8, 8, 0x00abcdef, &plainmethod));
    savedata.swfilter = 1;
    savedata.hwscale = 2.0f;
    assert(video_set_mode(mode));
    assert(opengl && gpu_is_active());
    assert(!gpu_allow_cpu_operation(screen, GPU_OPERATION_SCREEN));
    assert(test_pixel(screen, 2, 2) == 0x00abcdef);
    test_native_output_filters(screen);

    /* Fullscreen movie color remains independent of scene output filtering.
     * Capture and normal scene presentation share the still-active renderer. */
    test_native_movie_presentation(screen);
    savedata.swfilter = 0;
    savedata.hwscale = 1.0f;
    assert(video_set_mode(mode));
    assert(opengl && gpu_is_active());
    test_native_movie_presentation(screen);

    /* Explicitly disabling the required GPU rejects the mode. Neither scene
     * drawing nor presentation may silently read back and select software. */
    assert(SDL_setenv("OPENBOR_GPU", "0", 1) == 0);
    assert(!video_set_mode(mode));
    assert(!opengl && !gpu_is_active() && savedata.usegl == 1);
    readbacks = gpu_gl_get_stats()->readbacks;
    assert(!video_copy_screen(screen));
    assert(gpu_gl_get_stats()->readbacks == readbacks);
    assert(SDL_setenv("OPENBOR_GPU", "1", 1) == 0);
    assert(video_set_mode(mode));
    assert(opengl && gpu_is_active());

    /* Losing the scene backend must not revive its RGB32 presentation retry. */
    gpu_gl_shutdown();
    assert(opengl && !gpu_is_active());
    readbacks = gpu_gl_get_stats()->readbacks;
    assert(!video_copy_screen(screen));
    assert(gpu_gl_get_stats()->readbacks == readbacks);
    assert(video_set_mode(mode));
    assert(opengl && gpu_is_active());

    /* A later SDL preference still cannot bypass required native drawing. */
    savedata.usegl = 0;
    assert(video_set_mode(mode));
    assert(opengl && gpu_is_active() && savedata.usegl == 1);
    assert(video_copy_screen(screen));

    test_scene_format_rejection(mode, screen);
    savedata.usegl = 1;
    assert(video_set_mode(mode));
    assert(opengl && gpu_is_active());
    assert(gpu_draw_box(screen, 0, 0, 8, 8, 0x00010203, &plainmethod));
    readbacks = gpu_gl_get_stats()->readbacks;
    assert(video_copy_screen(screen));
    assert(gpu_gl_get_stats()->readbacks == readbacks);
    /* Final teardown must preserve the last displayed version before the
     * script-visible screen allocation outlives its graphics context. */
    video_set_mode(closed);
    assert(!opengl && !gpu_is_active());
    assert(test_pixel(screen, 2, 2) == 0x00010203);
    release_test_screen(screen);
    SDL_DestroyWindow(window);
    window = NULL;
    SDL_Quit();
    puts("GPU presentation tests passed: direct texture, orientation, GL recreation, native RGB8 output filters, movie color/capture/coexistence with filters, required RGB32 failures, RGB32-only scene modes.");
    return 0;
}
