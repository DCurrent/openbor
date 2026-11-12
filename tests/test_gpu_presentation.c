/*
 * Exercise the production SDL/OpenGL presenter and context lifecycle.
 * Run through tests/run_gpu_tests.sh, or compile with the native renderer test
 * flags plus video.c, opengl.c, videocommon.c and timer.c. The existing native
 * test fixtures are reused with their independent
 * entry point renamed; --gc-sections discards the unused test suite.
 */
#define main unused_backend_test_suite
#include "test_gpu_renderer.c"
#undef main
#include "savedata.h"
#include "video.h"
#include "opengl.h"
#include "gfx.h"

s_savedata savedata;
GfxBlitterTypes GfxBlitters[BLITTER_MAX * 2];
extern SDL_Window *window;
extern int nativeWidth, nativeHeight;

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
 * Exercise production presentation, mode switching, CPU-filter fallback and
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
    savedata.usegl = 1;
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
    assert(gpu_draw_box(screen, 0, 0, 8, 8, 0x00123456, &plainmethod));
    assert(gpu_draw_box(screen, 0, 0, 8, 1, 0x000000ff, &plainmethod));
    assert(gpu_draw_box(screen, 0, 7, 8, 1, 0x0000ff00, &plainmethod));
    readbacks = gpu_gl_get_stats()->readbacks;
    assert(video_copy_screen(screen));
    assert(gpu_gl_get_stats()->readbacks == readbacks);
    verify_presented_frame();

    /* The old context must preserve live screen contents before replacement. */
    assert(video_set_mode(mode));
    assert(opengl && gpu_is_active());
    assert(test_pixel(screen, 2, 2) == 0x00123456);
    assert(gpu_draw_box(screen, 0, 0, 8, 8, 0x00abcdef, &plainmethod));
    savedata.swfilter = 1;
    savedata.hwscale = 2.0f;
    assert(video_set_mode(mode));
    assert(opengl && !gpu_is_active());
    assert(test_pixel(screen, 2, 2) == 0x00abcdef);

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
    assert(test_pixel(screen, 2, 2) == 0x00887766);
    assert(video_prepare_yuv_frame(&frame));
    assert(video_display_yuv_frame());
    assert(video_restore_mode());
    assert(gpu_is_active());

    /* Diagnostics can retain GL presentation while disabling native drawing. */
    assert(SDL_setenv("OPENBOR_GPU", "0", 1) == 0);
    assert(video_set_mode(mode));
    assert(opengl && !gpu_is_active());
    assert(video_copy_screen(screen));
    assert(SDL_setenv("OPENBOR_GPU", "1", 1) == 0);
    assert(video_set_mode(mode));
    assert(opengl && gpu_is_active());

    /* Switching presenter family also changes window/context ownership. */
    savedata.usegl = 0;
    assert(video_set_mode(mode));
    assert(!opengl && !gpu_is_active());
    assert(video_copy_screen(screen));
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
    puts("GPU presentation tests passed: direct texture, orientation, GL recreation, filters, YUV, opt-out, SDL switch.");
    return 0;
}
