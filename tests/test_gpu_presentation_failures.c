/*
 * Damon V. Caskey
 * 2026-11-16
 * Inject failures only into the presenter's first core shader-status query while
 * retaining real contexts, native scene shaders, ownership and recovery.
 */
#define main unused_presenter_failure_backend_suite
#include "test_gpu_renderer.c"
#undef main
#include "savedata.h"
#include "video.h"
#include "opengl.h"
#include "gfx.h"
#include "video_settings.h"

s_savedata savedata;
extern SDL_Window *window;
extern int nativeWidth, nativeHeight;
static GLenum presentation_failed_status;
static unsigned presentation_injected_failures;
static PFNGLGETSHADERIVPROC presentation_real_shader_parameter;
static PFNGLGETPROGRAMIVPROC presentation_real_program_parameter;
void *__real_SDL_GL_GetProcAddress(const char *name);

/*
 * Damon V. Caskey
 * 2026-11-16
 * Supply the real platform timing boundary to the isolated presenter fixture.
 */
void _usleep(u32 microseconds)
{
    SDL_Delay((microseconds + 999) / 1000);
}

/*
 * Damon V. Caskey
 * 2026-11-16
 * Fail the first presenter compile query after the real driver operation.
 * Native shader queries remain unchanged if initialization incorrectly carries
 * on, preventing a later renderer failure from hiding the missing gate.
 */
static void APIENTRY presentation_shader_parameter(GLuint object, GLenum parameter, GLint *value)
{
    presentation_real_shader_parameter(object, parameter, value);
    if(presentation_failed_status == GL_COMPILE_STATUS && parameter == GL_COMPILE_STATUS &&
       presentation_injected_failures == 0)
    {
        *value = 0;
        ++presentation_injected_failures;
    }
}

/*
 * Damon V. Caskey
 * 2026-11-16
 * Fail only the first presenter link query while retaining genuine native
 * program linking and all subsequent driver status queries.
 */
static void APIENTRY presentation_program_parameter(GLuint object, GLenum parameter, GLint *value)
{
    presentation_real_program_parameter(object, parameter, value);
    if(presentation_failed_status == GL_LINK_STATUS && parameter == GL_LINK_STATUS &&
       presentation_injected_failures == 0)
    {
        *value = 0;
        ++presentation_injected_failures;
    }
}

/*
 * Damon V. Caskey
 * 2026-11-16
 * Restrict injection to the existing loader boundary instead of adding test
 * switches or alternate shader code to production initialization.
 */
void *__wrap_SDL_GL_GetProcAddress(const char *name)
{
    void *address = __real_SDL_GL_GetProcAddress(name);
    if(!strcmp(name, "glGetShaderiv"))
    {
        presentation_real_shader_parameter = (PFNGLGETSHADERIVPROC)address;
        return address ? (void *)presentation_shader_parameter : NULL;
    }
    if(!strcmp(name, "glGetProgramiv"))
    {
        presentation_real_program_parameter = (PFNGLGETPROGRAMIVPROC)address;
        return address ? (void *)presentation_program_parameter : NULL;
    }
    return address;
}

/*
 * Damon V. Caskey
 * 2026-11-16
 * Reject partial presenter initialization, preserve surviving scene contents,
 * and recover native presentation without any software rendering retry.
 */
static void presentation_test_failure(s_screen *screen, s_videomodes mode,
                                      GLenum status, unsigned color)
{
    unsigned long readbacks;
    assert(gpu_is_active());
    assert(gpu_draw_box(screen, 0, 0, 8, 8, color, &plainmethod));
    presentation_injected_failures = 0;
    presentation_failed_status = status;
    assert(!video_set_mode(mode));
    assert(presentation_injected_failures == 1);
    assert(!opengl && !gpu_is_active());
    assert(!SDL_GL_GetCurrentContext());
    assert(test_pixel(screen, 3, 3) == color);
    assert(savedata.usegl == VIDEO_SETTINGS_NATIVE);
    readbacks = gpu_gl_get_stats()->readbacks;
    assert(!video_copy_screen(screen));
    assert(gpu_gl_get_stats()->readbacks == readbacks);

    presentation_failed_status = 0;
    assert(video_set_mode(mode));
    assert(opengl && gpu_is_active());
    assert(video_copy_screen(screen));
    assert(test_pixel(screen, 3, 3) == color);
    readbacks = gpu_gl_get_stats()->readbacks;
    assert(gpu_draw_box(screen, 1, 1, 2, 2, color ^ 0x00ffffffu, &plainmethod));
    assert(video_copy_screen(screen));
    assert(gpu_gl_get_stats()->readbacks == readbacks);
}

/*
 * Damon V. Caskey
 * 2026-11-16
 * Distinguish unavailable infrastructure from intentionally rejected shaders.
 * The selected display filter must never make software presentation eligible.
 */
int main(void)
{
    s_videomodes mode = {.hRes = 8, .vRes = 8, .pixel = 4};
    s_videomodes closed = {0};
    s_screen *screen = test_screen(8, 8, PIXEL_32);
    assert(sizeof(void *) == 8);
    if(SDL_Init(SDL_INIT_VIDEO) != 0)
    {
        free(screen);
        return 77;
    }
    nativeWidth = nativeHeight = 64;
    savedata.usegl = 1;
    savedata.hwscale = 2.0f;
    savedata.swfilter = BLITTER_SCANLINES;
    if(!video_set_mode(mode) || !opengl)
    {
        free(screen);
        if(window) SDL_DestroyWindow(window);
        SDL_Quit();
        return 77;
    }
    presentation_test_failure(screen, mode, GL_COMPILE_STATUS, 0x0013579bu);
    presentation_test_failure(screen, mode, GL_LINK_STATUS, 0x00f1e3d5u);
    video_set_mode(closed);
    assert(!opengl && !gpu_is_active());
    release_test_screen(screen);
    SDL_DestroyWindow(window);
    window = NULL;
    SDL_Quit();
    puts("GPU presenter failure tests passed: compile/link rejection, preserved scene, no CPU retry, native recovery with selected filter.");
    return 0;
}
