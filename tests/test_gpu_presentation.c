/*
 * Exercise the production SDL/OpenGL presenter and context lifecycle.
 * Run through tests/run_gpu_tests.sh, or compile with the native renderer test
 * flags plus video.c, opengl.c, videocommon.c, timer.c and simple2x.c. The native
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
GfxBlitterTypes GfxBlitters[BLITTER_MAX * 2];
char *GfxBlitterNames[(BLITTER_MAX * 2) + 1] = { "Disabled", "Simple 2x fixture" };
extern SDL_Window *window;
extern int nativeWidth, nativeHeight;
extern s_videomodes stored_videomodes;
extern void Simple2x(u8 *src, u32 src_pitch, u8 *delta, u8 *dest, u32 dest_pitch, int width, int height);
extern void video_gl_setup_screen(void);
extern void video_gl_init_texture(int index, int width, int height, int bytes_per_pixel);

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
    unsigned texture;
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

    /* Recreating the legacy upload texture changes unit-zero ownership.
     * Native sampling must recover even though its source handle is stable. */
    assert(gpu_draw_screen(target, screen, 0, 0, &plainmethod));
    video_gl_init_texture(0, 8, 8, 4);
    assert(gpu_draw_screen(target, screen, 8, 0, &plainmethod));
    gpu_sync_cpu(target);
    assert(test_pixel(target, 10, 0) == 0x000000ff);
    assert(test_pixel(target, 10, 7) == 0x0000ff00);

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
 * 2026-11-15
 * Verify a selected output filter consumes RGB32 scene data and produces its
 * private packed intermediate without changing the authored scene format.
 */
static void test_cpu_output_filter(s_screen *source)
{
    s_videosurface *output = getVideoSurface(source);
    int x, y;

    /* The production nearest-neighbor filter doubles both dimensions. Its
     * packed BGR565 output is private presentation storage, not a scene mode. */
    assert(source->pixelformat == PIXEL_32);
    assert(stored_videomodes.pixel == 4);
    assert(output && output->width == 16 && output->height == 16);
    assert(output->pitch == output->width * 2);
    assert(output->data != source->data);
    for(y = 0; y < output->height; ++y)
    {
        const unsigned short *row = (const unsigned short *)((const unsigned char *)output->data + y * output->pitch);
        for(x = 0; x < output->width; ++x)
        {
            /* RGB32 0x00abcdef contains R=ef, G=cd, B=ab. Quantizing these
             * channels to 5/6/5 bits produces the packed value 0xae7d. */
            assert(row[x] == 0xae7d);
        }
    }
    assert(test_pixel(source, 2, 2) == 0x00abcdef);
    assert(video_copy_screen(source));
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
 * 2026-11-12
 * 2026-11-15 - Require native RGB32 through mode failures and context teardown.
 * 2026-11-15 - Reject removed scene formats and exercise real filter storage.
 * Exercise production presentation, mode switching, unported CPU filters and
 * YUV transitions without linking unrelated game/audio subsystems.
 */
int main(void)
{
    s_videomodes mode = { .hRes = 8, .vRes = 8, .pixel = 4 };
    s_videomodes closed = {0};
    s_screen *screen = test_screen(8, 8, PIXEL_32);
    yuv_video_mode movie_mode = {8, 8, 8, 8};
    unsigned char luminance[64], chroma_u[16], chroma_v[16];
    yuv_frame frame = { .width = 8, .height = 8, .lum = luminance,
                        .cb = chroma_u, .cr = chroma_v };
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
    /* Use the actual filter implementation at the fixture's selected index. */
    GfxBlitters[1] = Simple2x;
    savedata.swfilter = 1;
    savedata.hwscale = 2.0f;
    assert(video_set_mode(mode));
    assert(opengl && !gpu_is_active());
    assert(gpu_allow_cpu_operation(screen, GPU_OPERATION_SCREEN));
    assert(test_pixel(screen, 2, 2) == 0x00abcdef);
    test_cpu_output_filter(screen);

    /* Returning from a CPU filter must restore native drawing. Entering YUV
     * mode then retires that context's scene resources with current shadows. */
    savedata.swfilter = 0;
    savedata.hwscale = 1.0f;
    assert(video_set_mode(mode));
    assert(opengl && gpu_is_active());
    assert(gpu_draw_box(screen, 0, 0, 8, 8, 0x00887766, &plainmethod));
    memset(luminance, 128, sizeof(luminance));
    memset(chroma_u, 128, sizeof(chroma_u));
    memset(chroma_v, 128, sizeof(chroma_v));
    assert(video_setup_yuv_overlay(&movie_mode));
    assert(!gpu_is_active());
    assert(gpu_allow_cpu_operation(screen, GPU_OPERATION_SCREEN));
    assert(test_pixel(screen, 2, 2) == 0x00887766);
    assert(video_prepare_yuv_frame(&frame));
    assert(video_display_yuv_frame());
    assert(video_restore_mode());
    assert(gpu_is_active());

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
    puts("GPU presentation tests passed: direct texture, orientation, GL recreation, filter intermediates, YUV, required RGB32 failures, RGB32-only scene modes.");
    return 0;
}
