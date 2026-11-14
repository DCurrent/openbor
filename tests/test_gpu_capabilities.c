/*
 * Startup capability and fallback warnings through the ordinary GPU log.
 * Real GL compilation/drawing is retained; the loader wrappers only restrict
 * the capability or shader under test. No production test hooks are needed.
 */
#define main unused_capability_renderer_test_suite
#include "test_gpu_renderer.c"
#undef main
#include <unistd.h>
#include "engine_profile.h"

typedef enum capability_case
{
    CAPABILITY_HEALTHY,
    CAPABILITY_LOW_UNIFORMS,
    CAPABILITY_INVALID_QUERY,
    CAPABILITY_LOW_TEXTURE_UNITS,
    CAPABILITY_SMALL_TEXTURES,
    CAPABILITY_MISSING_ENTRY,
    CAPABILITY_AFFINE_COMPILE,
    CAPABILITY_AFFINE_LINK,
    CAPABILITY_ALL_COMPILE,
    CAPABILITY_INCOMPLETE_FRAMEBUFFER,
    CAPABILITY_DISABLED,
    CAPABILITY_NO_CONTEXT,
    CAPABILITY_COLOR_COMPILE,
    CAPABILITY_COLOR_LINK,
    CAPABILITY_COMBINED_COMPILE,
    CAPABILITY_AFFINE_ONLY_UNIFORMS,
    CAPABILITY_COLOR_ONLY_UNIFORMS,
    CAPABILITY_COUNT
} capability_case;

static const char *const capability_names[CAPABILITY_COUNT] =
{
    "healthy", "low uniforms", "invalid limit query", "low texture units", "small textures",
    "missing entry point", "affine compile failure", "affine link failure",
    "all shader compilation fails", "incomplete framebuffer",
    "disabled by setting", "no current context", "color compile failure",
    "color link failure", "combined shader compile failure",
    "affine-only uniform capacity", "color-only uniform capacity"
};
static capability_case capability_mode;
static unsigned capability_injected_compiles, capability_injected_links;
static unsigned capability_affine_fragments, capability_legacy_fragments, capability_color_fragments;
static void (APIENTRY *capability_real_get_integer)(GLenum, GLint *);
static PFNGLSHADERSOURCEPROC capability_real_shader_source;
static PFNGLCHECKFRAMEBUFFERSTATUSPROC capability_real_framebuffer_status;
void *__real_SDL_GL_GetProcAddress(const char *name);

/*
 * Damon V. Caskey
 * 2026-11-13
 * 2026-11-14 - Cover independent color and affine resource thresholds.
 * Restrict one reported resource without changing the actual test context.
 */
static void APIENTRY capability_get_integer(GLenum name, GLint *value)
{
    /* Other queries preserve the driver's real values and state semantics. */
    if(capability_mode == CAPABILITY_LOW_UNIFORMS && name == GL_MAX_FRAGMENT_UNIFORM_COMPONENTS)
    {
        *value = 64;
    }
    else if(capability_mode == CAPABILITY_AFFINE_ONLY_UNIFORMS && name == GL_MAX_FRAGMENT_UNIFORM_COMPONENTS)
    {
        *value = 256;
    }
    else if(capability_mode == CAPABILITY_COLOR_ONLY_UNIFORMS && name == GL_MAX_FRAGMENT_UNIFORM_COMPONENTS)
    {
        *value = 160;
    }
    else if(capability_mode == CAPABILITY_INVALID_QUERY && name == GL_MAX_FRAGMENT_UNIFORM_COMPONENTS)
    {
        /* An unsupported limit produces a real driver error. Startup must
         * consume it before returning control to the existing presenter. */
        *value = 0;
        capability_real_get_integer((GLenum)0xffffffffu, value);
    }
    else if(capability_mode == CAPABILITY_LOW_TEXTURE_UNITS && name == GL_MAX_TEXTURE_IMAGE_UNITS)
    {
        *value = 4;
    }
    else if(capability_mode == CAPABILITY_SMALL_TEXTURES && name == GL_MAX_TEXTURE_SIZE)
    {
        *value = 1024;
    }
    else
    {
        capability_real_get_integer(name, value);
    }
}

/*
 * Damon V. Caskey
 * 2026-11-13
 * 2026-11-14 - Reject color and combined variants independently.
 * Exercise genuine compiler/linker failures for the selected shader variant.
 */
static void APIENTRY capability_shader_source(GLuint shader, GLsizei count,
                                              const GLchar *const *strings,
                                              const GLint *lengths)
{
    PFNGLGETSHADERIVPROC get_shader =
        (PFNGLGETSHADERIVPROC)__real_SDL_GL_GetProcAddress("glGetShaderiv");
    GLint type;
    int part, affine = 0, legacy = 0, color = 0;

    /* The source define identifies the optional implementation. Valid legacy
     * retries still compile and link on the real driver. */
    get_shader(shader, GL_SHADER_TYPE, &type);
    for(part = 0; part < count; ++part)
    {
        if(strstr(strings[part], "#define GPU_AFFINE 1")) affine = 1;
        if(strstr(strings[part], "#define GPU_AFFINE 0")) legacy = 1;
        if(strstr(strings[part], "#define GPU_COLOR_RULES 1")) color = 1;
    }
    if(type == GL_FRAGMENT_SHADER)
    {
        capability_affine_fragments += affine;
        capability_legacy_fragments += legacy;
        capability_color_fragments += color;
        if(capability_mode == CAPABILITY_ALL_COMPILE ||
           (capability_mode == CAPABILITY_AFFINE_COMPILE && affine) ||
           (capability_mode == CAPABILITY_COLOR_COMPILE && color) ||
           (capability_mode == CAPABILITY_COMBINED_COMPILE && affine && color))
        {
            const GLchar *invalid = "#version 120\n#error injected_capability_compile_failure\n";
            ++capability_injected_compiles;
            capability_real_shader_source(shader, 1, &invalid, NULL);
            return;
        }
        if((capability_mode == CAPABILITY_AFFINE_LINK && affine) ||
           (capability_mode == CAPABILITY_COLOR_LINK && color))
        {
            /* Vertex output uses vec2. A used vec3 input compiles separately
             * but must fail the real program's interface-link validation. */
            const GLchar *invalid = "#version 120\nvarying vec3 sourcePosition;\n"
                                    "void main() { gl_FragColor = vec4(sourcePosition, 1.0); }\n";
            ++capability_injected_links;
            capability_real_shader_source(shader, 1, &invalid, NULL);
            return;
        }
    }
    capability_real_shader_source(shader, count, strings, lengths);
}

/*
 * Damon V. Caskey
 * 2026-11-13
 * Reject actual render-target viability after all FBO functions were loaded.
 */
static GLenum APIENTRY capability_framebuffer_status(GLenum target)
{
    if(capability_mode == CAPABILITY_INCOMPLETE_FRAMEBUFFER)
    {
        return GL_FRAMEBUFFER_UNSUPPORTED;
    }
    return capability_real_framebuffer_status(target);
}

/*
 * Damon V. Caskey
 * 2026-11-13
 * Intercept only selected capabilities through the existing dynamic GL loader.
 */
void *__wrap_SDL_GL_GetProcAddress(const char *name)
{
    void *entry = __real_SDL_GL_GetProcAddress(name);

    /* Required FBO entry points are private to the native renderer, so its
     * failure can be checked after the ordinary GL loader succeeds. */
    if(capability_mode == CAPABILITY_MISSING_ENTRY &&
       (!strcmp(name, "glGenFramebuffers") || !strcmp(name, "glGenFramebuffersEXT")))
    {
        return NULL;
    }
    if(!strcmp(name, "glGetIntegerv"))
    {
        capability_real_get_integer = (void (APIENTRY *)(GLenum, GLint *))entry;
        return (void *)capability_get_integer;
    }
    if(!strcmp(name, "glShaderSource"))
    {
        capability_real_shader_source = (PFNGLSHADERSOURCEPROC)entry;
        return (void *)capability_shader_source;
    }
    if(!strcmp(name, "glCheckFramebufferStatus") || !strcmp(name, "glCheckFramebufferStatusEXT"))
    {
        capability_real_framebuffer_status = (PFNGLCHECKFRAMEBUFFERSTATUSPROC)entry;
        return entry ? (void *)capability_framebuffer_status : NULL;
    }
    return entry;
}

/*
 * Damon V. Caskey
 * 2026-11-13
 * Assert expected output with a useful scenario diagnostic on failure.
 */
static void capability_expect(const char *log, const char *text)
{
    if(!strstr(log, text))
    {
        fprintf(stderr, "Capability case '%s' lacks '%s':\n%s\n",
                capability_names[capability_mode], text, log);
        abort();
    }
}

/*
 * Damon V. Caskey
 * 2026-11-13
 * 2026-11-14 - Verify unavailable color rules leave ordinary native draws intact.
 * Check startup reporting before drawing and prohibit repeated frame warnings.
 */
static void capability_run(SDL_Window *window, capability_case mode)
{
    SDL_GLContext context;
    FILE *capture;
    int saved_stdout, native_expected, affine_expected, color_expected, result, index;
    long startup_bytes, final_bytes;
    char log[32768];

    capability_mode = mode;
    capability_injected_compiles = capability_injected_links = 0;
    capability_affine_fragments = capability_legacy_fragments = capability_color_fragments = 0;
    assert(SDL_setenv("OPENBOR_GPU", mode == CAPABILITY_DISABLED ? "0" : "1", 1) == 0);
    context = SDL_GL_CreateContext(window);
    assert(context && LoadGLFunctions());
    if(mode == CAPABILITY_NO_CONTEXT)
    {
        assert(SDL_GL_MakeCurrent(window, NULL) == 0);
    }

    /* PP_TEST sends gpu_log to stdout. Capture that ordinary log destination,
     * independently of profiler hooks and before submitting any scene draw. */
    capture = tmpfile();
    assert(capture);
    assert(fflush(stdout) == 0);
    saved_stdout = dup(STDOUT_FILENO);
    assert(saved_stdout >= 0 && dup2(fileno(capture), STDOUT_FILENO) >= 0);
    result = gpu_gl_init();
    assert(!engine_profile_enabled());
    assert(fflush(stdout) == 0);
    startup_bytes = ftell(capture);
    assert(startup_bytes > 0);

    native_expected = mode == CAPABILITY_HEALTHY || mode == CAPABILITY_LOW_UNIFORMS ||
                      mode == CAPABILITY_AFFINE_COMPILE || mode == CAPABILITY_AFFINE_LINK ||
                      mode == CAPABILITY_COLOR_COMPILE || mode == CAPABILITY_COLOR_LINK ||
                      mode == CAPABILITY_COMBINED_COMPILE || mode == CAPABILITY_AFFINE_ONLY_UNIFORMS ||
                      mode == CAPABILITY_COLOR_ONLY_UNIFORMS;
    affine_expected = mode == CAPABILITY_HEALTHY || mode == CAPABILITY_COLOR_COMPILE ||
                      mode == CAPABILITY_COLOR_LINK || mode == CAPABILITY_COMBINED_COMPILE ||
                      mode == CAPABILITY_AFFINE_ONLY_UNIFORMS;
    color_expected = mode == CAPABILITY_HEALTHY || mode == CAPABILITY_AFFINE_COMPILE ||
                     mode == CAPABILITY_AFFINE_LINK || mode == CAPABILITY_COLOR_ONLY_UNIFORMS;
    assert(result == native_expected && gpu_is_active() == native_expected);
    if(mode == CAPABILITY_INVALID_QUERY) assert(glGetError() == GL_NO_ERROR);
    if(native_expected)
    {
        s_screen *source = test_screen(2, 2, PIXEL_32);
        s_screen *target = test_screen(4, 4, PIXEL_32);
        s_drawmethod method = plainmethod;
        const unsigned background = 0x0013579b, foreground = 0x00c2a468;
        unsigned long color_fallbacks, color_draws;

        /* Startup validation must not populate scene image ownership, trigger
         * readback, or leave an error for the first real operation. */
        assert(gpu_gl_get_stats()->draws == 0);
        assert(gpu_gl_get_stats()->uploads == 0);
        assert(gpu_gl_get_stats()->readbacks == 0);
        assert(glGetError() == GL_NO_ERROR);
        for(index = 0; index < 4; ++index) ((unsigned *)source->data)[index] = foreground;
        method.config |= DRAWMETHOD_CONFIG_AFFINE;
        assert(gpu_gl_init()); /* Already initialized: no repeated report. */
        for(index = 0; index < 32; ++index)
        {
            assert(gpu_draw_box(target, 0, 0, 4, 4, background, &plainmethod));
            assert(gpu_draw_screen(target, source, 0, 0, &method) == affine_expected);
        }
        gpu_sync_cpu(target);
        assert(test_pixel(target, 0, 0) == (affine_expected ? foreground : background));
        assert(gpu_draw_screen(target, source, 1, 1, &plainmethod));
        gpu_sync_cpu(target);
        assert(test_pixel(target, 1, 1) == foreground);
        /* Unavailable optional rules are ignored without declining an
         * ordinary native draw or changing its unmodified source color. */
        color_fallbacks = gpu_gl_get_stats()->fallback_draws;
        color_draws = gpu_gl_get_stats()->draws;
        method = plainmethod;
        method.colors.substitute_enabled = 1u << 4;
        method.colors.substitute[4].match = foreground;
        method.colors.substitute[4].replacement = 0x00335577;
        for(index = 0; index < 4; ++index)
        {
            assert(gpu_draw_box(target, 0, 0, 4, 4, background, &plainmethod));
            assert(gpu_draw_screen(target, source, 0, 0, &method));
            gpu_sync_cpu(target);
            assert(test_pixel(target, 0, 0) == (color_expected ? 0x00335577u : foreground));
        }
        method.colors.substitute_enabled = 0;
        method.colors.chroma_enabled = 1u << 4;
        method.colors.chroma[4] = foreground;
        assert(gpu_draw_box(target, 0, 0, 4, 4, background, &plainmethod));
        assert(gpu_draw_screen(target, source, 0, 0, &method));
        gpu_sync_cpu(target);
        assert(test_pixel(target, 0, 0) == (color_expected ? background : foreground));

        /* Seed a matching palette and chroma mask, then remove the palette
         * without resetting shader state. An ordinary indexed fill remains
         * native: unavailable source chroma is ignored, while supported
         * Substitute still processes the final fill color. */
        {
            s_screen *indexed = test_screen(2, 2, PIXEL_x8);
            unsigned char *palette = indexed->palette;
            memset(indexed->data, 1, 4);
            ((unsigned *)palette)[1] = foreground;
            method.fillcolor = foreground;
            method.colors.substitute_enabled = 1u << 4;
            assert(gpu_draw_box(target, 0, 0, 4, 4, background, &plainmethod));
            assert(gpu_draw_screen(target, indexed, 0, 0, &method));
            gpu_sync_cpu(target);
            assert(test_pixel(target, 0, 0) == (color_expected ? background : foreground));
            indexed->palette = NULL;
            assert(gpu_draw_screen(target, indexed, 0, 0, &method));
            gpu_sync_cpu(target);
            assert(test_pixel(target, 0, 0) == (color_expected ? 0x00335577u : foreground));
            method.colors.substitute_enabled = 0;
            assert(gpu_draw_screen(target, indexed, 0, 0, &method));
            gpu_sync_cpu(target);
            assert(test_pixel(target, 0, 0) == foreground);
            /* Restoring the palette must restore valid chroma after its
             * previous ignored draw cleared the effective shader mask. */
            indexed->palette = palette;
            method.fillcolor = background;
            assert(gpu_draw_screen(target, indexed, 0, 0, &method));
            gpu_sync_cpu(target);
            assert(test_pixel(target, 0, 0) == (color_expected ? foreground : background));
            release_test_screen(indexed);
        }
        assert(gpu_gl_get_stats()->fallback_draws == color_fallbacks);
        assert(gpu_gl_get_stats()->draws > color_draws);
        release_test_screen(target);
        release_test_screen(source);
    }
    assert(fflush(stdout) == 0);
    final_bytes = ftell(capture);
    assert(final_bytes == startup_bytes);

    /* Read the complete report after restoring stdout so failures can include
     * the captured evidence without writing into their own input stream. */
    assert(dup2(saved_stdout, STDOUT_FILENO) >= 0);
    assert(close(saved_stdout) == 0);
    assert(final_bytes < (long)sizeof(log));
    rewind(capture);
    assert(fread(log, 1, (size_t)final_bytes, capture) == (size_t)final_bytes);
    log[final_bytes] = '\0';
    assert(fclose(capture) == 0);
    capability_expect(log, "[GPU] ========== GPU CAPABILITY REPORT ==========");
    assert(!strstr(log, "[ENGINE PROFILE]"));
    capability_expect(log, affine_expected ?
        "RESULT: NATIVE - native scene rendering and affine transforms available." :
        native_expected ?
        "RESULT: LIMITED NATIVE - native scene rendering available; affine transforms use CPU software rendering." :
        "RESULT: SOFTWARE - native scene rendering unavailable; draws use CPU software rendering.");
    assert(!strstr(strstr(log, "RESULT:") + 1, "RESULT:"));
    capability_expect(log, color_expected ?
        "Chroma key execution:              NATIVE - available" :
        "Chroma key execution:              UNAVAILABLE - ignored");
    capability_expect(log, color_expected ?
        "Substitute execution:              NATIVE - available" :
        "Substitute execution:              UNAVAILABLE - ignored");
    capability_expect(log, "5 chroma keys; 5 substitutes");
    capability_expect(log, "RGB32 native targets");
    capability_expect(log, "Software implementation:           None; rules ignored");
    capability_expect(log, "Other drawing paths:               Rules ignored; ordinary drawing continues");
    assert(!strstr(log, "Chroma and Substitute draws will use CPU software rendering"));
    assert(!strstr(log, "Unavailable effects will use CPU software rendering"));
    if(!color_expected)
        capability_expect(log, "WARNING: Chroma and Substitute rules are unavailable and will be ignored.");
    if(mode != CAPABILITY_HEALTHY)
    {
        capability_expect(log, "WARNING:");
        capability_expect(log, "CPU");
        capability_expect(log, "Significant slowdown is possible.");
    }
    if(capability_injected_compiles || capability_injected_links)
    {
        capability_expect(log, "If native affine is unavailable:   CPU software rendering");
        capability_expect(log, "If color rules are unavailable:    Ignored; no software implementation.");
    }
    /* A capable native backend can still be hosted by llvmpipe. Its separate
     * software-driver warning must not be confused with an engine fallback. */
    if(mode == CAPABILITY_HEALTHY)
    {
        capability_expect(log, "GLSL");
        capability_expect(log, "Maximum texture size:");
        capability_expect(log, "Fragment texture units:");
        capability_expect(log, "Fragment uniform components:");
        assert(capability_affine_fragments == 1 && capability_legacy_fragments == 0);
        assert(capability_color_fragments == 1);
    }
    if(mode == CAPABILITY_LOW_UNIFORMS)
    {
        capability_expect(log, "fragment uniform components");
        capability_expect(log, "64");
        capability_expect(log, "256");
        assert(capability_affine_fragments == 0);
    }
    if(mode == CAPABILITY_LOW_TEXTURE_UNITS)
    {
        capability_expect(log, "fragment texture units");
        capability_expect(log, "4");
        capability_expect(log, "5");
    }
    if(mode == CAPABILITY_SMALL_TEXTURES)
    {
        capability_expect(log, "maximum texture size");
        capability_expect(log, "1024");
        capability_expect(log, "1536");
    }
    if(mode == CAPABILITY_MISSING_ENTRY) capability_expect(log, "glGenFramebuffers");
    if(mode == CAPABILITY_INVALID_QUERY)
        capability_expect(log, "GPU limits could not be verified");
    if(mode == CAPABILITY_AFFINE_COMPILE || mode == CAPABILITY_ALL_COMPILE)
    {
        capability_expect(log, "shader compilation failed");
        capability_expect(log, "injected_capability_compile_failure");
        assert(capability_injected_compiles == (mode == CAPABILITY_ALL_COMPILE ? 4u : 2u));
    }
    if(mode == CAPABILITY_AFFINE_LINK)
    {
        capability_expect(log, "shader link failed");
        assert(capability_injected_links == 2);
    }
    if(mode == CAPABILITY_AFFINE_COMPILE || mode == CAPABILITY_AFFINE_LINK)
    {
        capability_expect(log, "affine");
        assert(capability_affine_fragments == 2);
    }
    if(mode == CAPABILITY_COLOR_COMPILE || mode == CAPABILITY_COMBINED_COMPILE)
    {
        capability_expect(log, "shader compilation failed");
        assert(capability_injected_compiles == 1);
        assert(capability_affine_fragments == 2 && capability_color_fragments == 1);
    }
    if(mode == CAPABILITY_COLOR_LINK)
    {
        capability_expect(log, "shader link failed");
        assert(capability_injected_links == 1);
        assert(capability_affine_fragments == 2 && capability_color_fragments == 1);
    }
    if(mode == CAPABILITY_AFFINE_ONLY_UNIFORMS)
    {
        assert(capability_affine_fragments == 1 && capability_color_fragments == 0);
    }
    if(mode == CAPABILITY_COLOR_ONLY_UNIFORMS)
    {
        assert(capability_affine_fragments == 0 && capability_color_fragments == 1);
    }
    if(mode == CAPABILITY_INCOMPLETE_FRAMEBUFFER)
        capability_expect(log, "RGB32 framebuffer probe failed");
    if(mode == CAPABILITY_DISABLED) capability_expect(log, "OPENBOR_GPU=0");
    if(mode == CAPABILITY_NO_CONTEXT) capability_expect(log, "no current OpenGL context");
    if(native_expected && !affine_expected) assert(capability_legacy_fragments == 1);

    if(mode == CAPABILITY_NO_CONTEXT) assert(SDL_GL_MakeCurrent(window, context) == 0);
    gpu_gl_shutdown();
    SDL_GL_DeleteContext(context);
}

/*
 * Damon V. Caskey
 * 2026-11-13
 * Exercise capability reports with all profiling disabled from process start.
 */
int main(void)
{
    SDL_Window *window;
    SDL_GLContext context;
    int mode;

    assert(SDL_setenv("OPENBOR_PROFILE", "0", 1) == 0);
    assert(SDL_setenv("OPENBOR_PROFILE_GPU", "0", 1) == 0);
    if(SDL_Init(SDL_INIT_VIDEO) != 0)
    {
        fprintf(stderr, "SKIP: SDL video unavailable: %s\n", SDL_GetError());
        return 77;
    }
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_COMPATIBILITY);
    window = SDL_CreateWindow("OpenBOR GPU capability tests", 0, 0, 64, 64,
                              SDL_WINDOW_OPENGL | SDL_WINDOW_HIDDEN);
    if(!window)
    {
        fprintf(stderr, "SKIP: GL window unavailable: %s\n", SDL_GetError());
        SDL_Quit();
        return 77;
    }
    /* Infrastructure failure is a skip; failures after this working context
     * are actual regressions in one of the isolated capability scenarios. */
    context = SDL_GL_CreateContext(window);
    if(!context || !LoadGLFunctions())
    {
        fprintf(stderr, "SKIP: GL context unavailable: %s\n", SDL_GetError());
        if(context) SDL_GL_DeleteContext(context);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 77;
    }
    SDL_GL_DeleteContext(context);
    for(mode = 0; mode < CAPABILITY_COUNT; ++mode)
    {
        capability_run(window, (capability_case)mode);
    }
    SDL_DestroyWindow(window);
    SDL_Quit();
    puts("GPU capability log tests passed: startup reports, native-only color rules, ignored unavailable effects, retained ordinary draws, profiler opt-out, no draw-time spam.");
    return 0;
}
