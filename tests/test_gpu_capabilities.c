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
    CAPABILITY_MISSING_BUFFER_ENTRY,
    CAPABILITY_BUFFER_OBJECT_ALLOCATION,
    CAPABILITY_BUFFER_STORAGE_ALLOCATION,
    CAPABILITY_SHEAR_COMPILE,
    CAPABILITY_SHEAR_LINK,
    CAPABILITY_SHEAR_ONLY_UNIFORMS,
    CAPABILITY_COLOR_SHEAR_UNIFORMS,
    CAPABILITY_COUNT
} capability_case;

static const char *const capability_names[CAPABILITY_COUNT] =
{
    "healthy", "low uniforms", "invalid limit query", "low texture units", "small textures",
    "missing entry point", "affine compile failure", "affine link failure",
    "all shader compilation fails", "incomplete framebuffer",
    "disabled by setting", "no current context", "color compile failure",
    "color link failure", "combined shader compile failure",
    "affine-only uniform capacity", "color-only uniform capacity",
    "missing buffer entry point", "buffer object allocation failure",
    "buffer storage allocation failure", "shear compile failure",
    "shear link failure", "shear-only uniform capacity",
    "color and shear uniform capacity"
};
static capability_case capability_mode;
static unsigned capability_injected_compiles, capability_injected_links;
static unsigned capability_affine_fragments, capability_legacy_fragments, capability_color_fragments;
static unsigned capability_shear_fragments, capability_last_fragment_variant;
static unsigned capability_draw_elements, capability_elements;
typedef void (APIENTRYP capability_draw_fn)(GLenum, GLsizei, GLenum, const void *);
static capability_draw_fn capability_real_draw_elements;
static void (APIENTRY *capability_real_get_integer)(GLenum, GLint *);
static PFNGLSHADERSOURCEPROC capability_real_shader_source;
static PFNGLCHECKFRAMEBUFFERSTATUSPROC capability_real_framebuffer_status;
static PFNGLGENBUFFERSPROC capability_real_gen_buffers;
static PFNGLDELETEBUFFERSPROC capability_real_delete_buffers;
static PFNGLBUFFERDATAPROC capability_real_buffer_data;
static unsigned capability_buffer_generations, capability_buffer_failures;
static unsigned capability_buffer_count, capability_buffer_deletions;
static GLuint capability_buffers[2];
void *__real_SDL_GL_GetProcAddress(const char *name);

/*
 * Damon V. Caskey
 * 2026-11-13
 * 2026-11-14 - Cover independent color and affine resource thresholds.
 * 2026-11-14 - Check shear budgets independently of retained material features.
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
    else if(capability_mode == CAPABILITY_SHEAR_ONLY_UNIFORMS && name == GL_MAX_FRAGMENT_UNIFORM_COMPONENTS)
    {
        *value = 96;
    }
    else if(capability_mode == CAPABILITY_COLOR_SHEAR_UNIFORMS && name == GL_MAX_FRAGMENT_UNIFORM_COMPONENTS)
    {
        *value = 192;
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
 * 2026-11-14 - Reject optional shear without losing affine or color features.
 * Exercise genuine compiler/linker failures for the selected shader variant.
 */
static void APIENTRY capability_shader_source(GLuint shader, GLsizei count,
                                              const GLchar *const *strings,
                                              const GLint *lengths)
{
    PFNGLGETSHADERIVPROC get_shader =
        (PFNGLGETSHADERIVPROC)__real_SDL_GL_GetProcAddress("glGetShaderiv");
    GLint type;
    int part, affine = 0, legacy = 0, color = 0, shear = 0;

    /* The source define identifies the optional implementation. Valid legacy
     * retries still compile and link on the real driver. */
    get_shader(shader, GL_SHADER_TYPE, &type);
    for(part = 0; part < count; ++part)
    {
        if(strstr(strings[part], "#define GPU_AFFINE 1")) affine = 1;
        if(strstr(strings[part], "#define GPU_AFFINE 0")) legacy = 1;
        if(strstr(strings[part], "#define GPU_COLOR_RULES 1")) color = 1;
        if(strstr(strings[part], "#define GPU_SHEAR 1")) shear = 1;
    }
    if(type == GL_FRAGMENT_SHADER)
    {
        capability_affine_fragments += affine;
        capability_legacy_fragments += legacy;
        capability_color_fragments += color;
        capability_shear_fragments += shear;
        capability_last_fragment_variant = (unsigned)(affine | (color << 1) | (shear << 2));
        if(capability_mode == CAPABILITY_ALL_COMPILE ||
           (capability_mode == CAPABILITY_AFFINE_COMPILE && affine) ||
           (capability_mode == CAPABILITY_COLOR_COMPILE && color) ||
           (capability_mode == CAPABILITY_COMBINED_COMPILE && affine && color) ||
           (capability_mode == CAPABILITY_SHEAR_COMPILE && shear))
        {
            const GLchar *invalid = "#version 120\n#error injected_capability_compile_failure\n";
            ++capability_injected_compiles;
            capability_real_shader_source(shader, 1, &invalid, NULL);
            return;
        }
        if((capability_mode == CAPABILITY_AFFINE_LINK && affine) ||
           (capability_mode == CAPABILITY_COLOR_LINK && color) ||
           (capability_mode == CAPABILITY_SHEAR_LINK && shear))
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
 * 2026-11-14
 * Reject the second startup buffer name after one successful allocation.
 * Retain acquired names so failure cleanup must release partial resources.
 */
static void APIENTRY capability_gen_buffers(GLsizei count, GLuint *buffers)
{
    GLsizei index;
    ++capability_buffer_generations;
    if(capability_mode == CAPABILITY_BUFFER_OBJECT_ALLOCATION &&
       capability_buffer_generations == 2)
    {
        for(index = 0; index < count; ++index) buffers[index] = 0;
        ++capability_buffer_failures;
        return;
    }
    capability_real_gen_buffers(count, buffers);
    if(capability_mode == CAPABILITY_BUFFER_OBJECT_ALLOCATION ||
       capability_mode == CAPABILITY_BUFFER_STORAGE_ALLOCATION)
    {
        /* These startup cases acquire at most the vertex and index objects. */
        for(index = 0; index < count; ++index)
        {
            if(buffers[index])
            {
                assert(capability_buffer_count < 2);
                capability_buffers[capability_buffer_count++] = buffers[index];
            }
        }
    }
}

/*
 * Damon V. Caskey
 * 2026-11-14
 * Observe explicit cleanup before the failed context itself is destroyed.
 */
static void APIENTRY capability_delete_buffers(GLsizei count, const GLuint *buffers)
{
    GLsizei index;
    unsigned acquired;
    for(index = 0; index < count; ++index)
    {
        for(acquired = 0; acquired < capability_buffer_count; ++acquired)
        {
            if(buffers[index] && buffers[index] == capability_buffers[acquired])
            {
                capability_buffers[acquired] = 0;
                ++capability_buffer_deletions;
            }
        }
    }
    capability_real_delete_buffers(count, buffers);
}

/*
 * Damon V. Caskey
 * 2026-11-14
 * Fail index storage after vertex storage succeeds, leaving a real GL error
 * that initialization must consume before returning to ordinary presentation.
 */
static void APIENTRY capability_buffer_data(GLenum target, GLsizeiptr size,
                                            const void *data, GLenum usage)
{
    if(capability_mode == CAPABILITY_BUFFER_STORAGE_ALLOCATION &&
       target == GL_ELEMENT_ARRAY_BUFFER)
    {
        ++capability_buffer_failures;
        capability_real_buffer_data((GLenum)0xffffffffu, size, data, usage);
        return;
    }
    capability_real_buffer_data(target, size, data, usage);
}

/*
 * Damon V. Caskey
 * 2026-11-14
 * Distinguish one shader-sampled shear quad from retained CPU row geometry.
 * Keep the ordinary indexed submission active for the pixel comparison.
 */
static void APIENTRY capability_submit_elements(GLenum mode, GLsizei count,
                                                GLenum type, const void *indices)
{
    assert(mode == GL_TRIANGLES && type == GL_UNSIGNED_INT && count > 0);
    ++capability_draw_elements;
    capability_elements += (unsigned)count;
    capability_real_draw_elements(mode, count, type, indices);
}

/*
 * Damon V. Caskey
 * 2026-11-13
 * 2026-11-14 - Restrict required buffer functions and startup allocations.
 * 2026-11-14 - Observe native shear geometry without production test hooks.
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
    if(capability_mode == CAPABILITY_MISSING_BUFFER_ENTRY && !strcmp(name, "glBufferData"))
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
    if(!strcmp(name, "glGenBuffers"))
    {
        capability_real_gen_buffers = (PFNGLGENBUFFERSPROC)entry;
        return entry ? (void *)capability_gen_buffers : NULL;
    }
    if(!strcmp(name, "glDeleteBuffers"))
    {
        capability_real_delete_buffers = (PFNGLDELETEBUFFERSPROC)entry;
        return entry ? (void *)capability_delete_buffers : NULL;
    }
    if(!strcmp(name, "glBufferData"))
    {
        capability_real_buffer_data = (PFNGLBUFFERDATAPROC)entry;
        return entry ? (void *)capability_buffer_data : NULL;
    }
    if(!strcmp(name, "glDrawElements"))
    {
        capability_real_draw_elements = (capability_draw_fn)entry;
        return entry ? (void *)capability_submit_elements : NULL;
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
 * 2026-11-14
 * Preserve legacy row snapping and source colors in every native variant.
 * Missing optional shear keeps native pixels while retaining three CPU rows.
 */
static void capability_check_shear(int shear_expected)
{
    static const unsigned colors[] =
    {
        0x00112335, 0x0047596b,
        0x007d8fa1, 0x00b3c5d7,
        0x00e91b2d, 0x003f5163
    };
    const unsigned background = 0x0013579b;
    s_screen *source = test_screen(2, 3, PIXEL_32);
    s_screen *target = test_screen(8, 6, PIXEL_32);
    s_drawmethod method = plainmethod;
    gpu_renderer_stats before;
    int x, y;

    memcpy(source->data, colors, sizeof(colors));
    method.shiftx = 256;
    assert(gpu_draw_box(target, 0, 0, 8, 6, background, &plainmethod));
    before = *gpu_gl_get_stats();
    capability_draw_elements = capability_elements = 0;
    assert(gpu_draw_screen(target, source, 1, 1, &method));
    assert(gpu_gl_get_stats()->draws == before.draws + 1);
    assert(gpu_gl_get_stats()->fallback_draws == before.fallback_draws);
    assert(gpu_gl_get_stats()->readbacks == before.readbacks);
    assert(capability_draw_elements == 1);
    assert(capability_elements == (shear_expected ? 6u : 18u));
    gpu_sync_cpu(target);
    /* Legacy traversal shifts the top source row by one pixel, then advances
     * right by one pixel per row. Check untouched pixels as well as content. */
    for(y = 0; y < target->height; ++y)
    {
        for(x = 0; x < target->width; ++x)
        {
            unsigned expected = background;
            if(y >= 1 && y <= 3 && x >= y + 1 && x <= y + 2)
            {
                expected = colors[(y - 1) * 2 + x - y - 1];
            }
            assert(test_pixel(target, x, y) == expected);
        }
    }
    assert(glGetError() == GL_NO_ERROR);
    release_test_screen(target);
    release_test_screen(source);
}

/*
 * Damon V. Caskey
 * 2026-11-13
 * 2026-11-14 - Verify unavailable color rules leave ordinary native draws intact.
 * 2026-11-14 - Require buffered startup success or explicit coherent fallback.
 * 2026-11-14 - Retain independent features and native pixels without shear.
 * Check startup reporting before drawing and prohibit repeated frame warnings.
 */
static void capability_run(SDL_Window *window, capability_case mode)
{
    SDL_GLContext context;
    FILE *capture;
    int saved_stdout, native_expected, affine_expected, color_expected, shear_expected, result, index;
    gpu_renderer_stats before_init;
    long startup_bytes, final_bytes;
    char log[32768];

    capability_mode = mode;
    capability_injected_compiles = capability_injected_links = 0;
    capability_affine_fragments = capability_legacy_fragments = capability_color_fragments = 0;
    capability_shear_fragments = capability_last_fragment_variant = 0;
    capability_draw_elements = capability_elements = 0;
    capability_buffer_generations = capability_buffer_failures = 0;
    capability_buffer_count = capability_buffer_deletions = 0;
    memset(capability_buffers, 0, sizeof(capability_buffers));
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
    before_init = *gpu_gl_get_stats();
    result = gpu_gl_init();
    assert(!engine_profile_enabled());
    assert(fflush(stdout) == 0);
    startup_bytes = ftell(capture);
    assert(startup_bytes > 0);

    native_expected = mode == CAPABILITY_HEALTHY || mode == CAPABILITY_LOW_UNIFORMS ||
                      mode == CAPABILITY_AFFINE_COMPILE || mode == CAPABILITY_AFFINE_LINK ||
                      mode == CAPABILITY_COLOR_COMPILE || mode == CAPABILITY_COLOR_LINK ||
                      mode == CAPABILITY_COMBINED_COMPILE || mode == CAPABILITY_AFFINE_ONLY_UNIFORMS ||
                      mode == CAPABILITY_COLOR_ONLY_UNIFORMS || mode == CAPABILITY_SHEAR_COMPILE ||
                      mode == CAPABILITY_SHEAR_LINK || mode == CAPABILITY_SHEAR_ONLY_UNIFORMS ||
                      mode == CAPABILITY_COLOR_SHEAR_UNIFORMS;
    affine_expected = mode == CAPABILITY_HEALTHY || mode == CAPABILITY_COLOR_COMPILE ||
                      mode == CAPABILITY_COLOR_LINK || mode == CAPABILITY_COMBINED_COMPILE ||
                      mode == CAPABILITY_AFFINE_ONLY_UNIFORMS || mode == CAPABILITY_SHEAR_COMPILE ||
                      mode == CAPABILITY_SHEAR_LINK;
    color_expected = mode == CAPABILITY_HEALTHY || mode == CAPABILITY_AFFINE_COMPILE ||
                     mode == CAPABILITY_AFFINE_LINK || mode == CAPABILITY_COLOR_ONLY_UNIFORMS ||
                     mode == CAPABILITY_SHEAR_COMPILE || mode == CAPABILITY_SHEAR_LINK ||
                     mode == CAPABILITY_COLOR_SHEAR_UNIFORMS;
    shear_expected = mode == CAPABILITY_HEALTHY || mode == CAPABILITY_AFFINE_COMPILE ||
                     mode == CAPABILITY_AFFINE_LINK || mode == CAPABILITY_COLOR_COMPILE ||
                     mode == CAPABILITY_COLOR_LINK || mode == CAPABILITY_COMBINED_COMPILE ||
                     mode == CAPABILITY_SHEAR_ONLY_UNIFORMS || mode == CAPABILITY_COLOR_SHEAR_UNIFORMS;
    assert(result == native_expected && gpu_is_active() == native_expected);
    if(mode == CAPABILITY_INVALID_QUERY) assert(glGetError() == GL_NO_ERROR);
    if(mode == CAPABILITY_MISSING_BUFFER_ENTRY ||
       mode == CAPABILITY_BUFFER_OBJECT_ALLOCATION ||
       mode == CAPABILITY_BUFFER_STORAGE_ALLOCATION)
    {
        /* Failed initialization must not draw, touch scene pixels, leak its
         * acquired buffers, or leave a driver error for the existing backend. */
        assert(gpu_gl_get_stats()->draws == before_init.draws);
        assert(gpu_gl_get_stats()->uploads == before_init.uploads);
        assert(gpu_gl_get_stats()->readbacks == before_init.readbacks);
        assert(capability_buffer_deletions == capability_buffer_count);
        assert(glGetError() == GL_NO_ERROR);
    }
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
        assert(capability_last_fragment_variant ==
               (unsigned)(affine_expected | (color_expected << 1) | (shear_expected << 2)));
        capability_check_shear(shear_expected);
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
    if(native_expected)
    {
        capability_expect(log, "Vertex/index buffers:");
        capability_expect(log, "Buffered submission:");
        capability_expect(log, "NATIVE - indexed triangles");
        capability_expect(log, "Legacy shear setup:");
        capability_expect(log, shear_expected ?
            "NATIVE - shader sampling, one quad" : "CPU - row geometry; native pixels");
        if(!shear_expected)
            capability_expect(log, "WARNING: Legacy shear shader unavailable; retaining CPU row geometry with native pixel rendering.");
        else
            assert(!strstr(log, "WARNING: Legacy shear shader unavailable;"));
    }
    else
    {
        capability_expect(log, "Legacy shear setup:");
        capability_expect(log, "UNAVAILABLE - software renderer");
    }
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
    if(mode != CAPABILITY_HEALTHY && mode != CAPABILITY_SHEAR_COMPILE &&
       mode != CAPABILITY_SHEAR_LINK)
    {
        capability_expect(log, "WARNING:");
        capability_expect(log, "CPU");
        capability_expect(log, "Significant slowdown is possible.");
    }
    if(capability_injected_compiles || capability_injected_links)
    {
        capability_expect(log, "If native affine is unavailable:   CPU software rendering");
        capability_expect(log, "If color rules are unavailable:    Ignored; no software implementation.");
        capability_expect(log, "Legacy shear requested by retry:");
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
        assert(capability_color_fragments == 1 && capability_shear_fragments == 1);
    }
    if(mode == CAPABILITY_LOW_UNIFORMS)
    {
        capability_expect(log, "fragment uniform components");
        capability_expect(log, "64");
        capability_expect(log, "256");
        assert(capability_affine_fragments == 0 && capability_color_fragments == 0);
        assert(capability_shear_fragments == 0);
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
    if(mode == CAPABILITY_MISSING_BUFFER_ENTRY)
    {
        capability_expect(log, "glBufferData");
        assert(capability_buffer_generations == 0 && capability_buffer_failures == 0);
    }
    if(mode == CAPABILITY_BUFFER_OBJECT_ALLOCATION ||
       mode == CAPABILITY_BUFFER_STORAGE_ALLOCATION)
    {
        capability_expect(log, "WARNING: Native vertex/index buffer allocation failed.");
        assert(capability_buffer_generations == 2 && capability_buffer_failures == 1);
        assert(capability_buffer_count ==
               (mode == CAPABILITY_BUFFER_OBJECT_ALLOCATION ? 1u : 2u));
    }
    if(mode == CAPABILITY_INVALID_QUERY)
        capability_expect(log, "GPU limits could not be verified");
    if(mode == CAPABILITY_AFFINE_COMPILE || mode == CAPABILITY_ALL_COMPILE)
    {
        capability_expect(log, "shader compilation failed");
        capability_expect(log, "injected_capability_compile_failure");
        assert(capability_injected_compiles == (mode == CAPABILITY_ALL_COMPILE ? 8u : 4u));
        if(mode == CAPABILITY_ALL_COMPILE)
        {
            assert(capability_affine_fragments == 4 && capability_legacy_fragments == 4);
            assert(capability_color_fragments == 4 && capability_shear_fragments == 4);
        }
    }
    if(mode == CAPABILITY_AFFINE_LINK)
    {
        capability_expect(log, "shader link failed");
        assert(capability_injected_links == 4);
    }
    if(mode == CAPABILITY_AFFINE_COMPILE || mode == CAPABILITY_AFFINE_LINK)
    {
        capability_expect(log, "affine");
        assert(capability_affine_fragments == 4 && capability_color_fragments == 3);
        assert(capability_shear_fragments == 3);
    }
    if(mode == CAPABILITY_COLOR_COMPILE || mode == CAPABILITY_COMBINED_COMPILE)
    {
        capability_expect(log, "shader compilation failed");
        assert(capability_injected_compiles == 2);
        assert(capability_affine_fragments == 3 && capability_color_fragments == 2);
        assert(capability_shear_fragments == 2);
    }
    if(mode == CAPABILITY_COLOR_LINK)
    {
        capability_expect(log, "shader link failed");
        assert(capability_injected_links == 2);
        assert(capability_affine_fragments == 3 && capability_color_fragments == 2);
        assert(capability_shear_fragments == 2);
    }
    if(mode == CAPABILITY_AFFINE_ONLY_UNIFORMS)
    {
        assert(capability_affine_fragments == 1 && capability_color_fragments == 0);
        assert(capability_shear_fragments == 0);
    }
    if(mode == CAPABILITY_COLOR_ONLY_UNIFORMS)
    {
        assert(capability_affine_fragments == 0 && capability_color_fragments == 1);
        assert(capability_shear_fragments == 0);
    }
    if(mode == CAPABILITY_SHEAR_ONLY_UNIFORMS)
    {
        assert(capability_affine_fragments == 0 && capability_color_fragments == 0);
        assert(capability_shear_fragments == 1);
    }
    if(mode == CAPABILITY_COLOR_SHEAR_UNIFORMS)
    {
        assert(capability_affine_fragments == 0 && capability_color_fragments == 1);
        assert(capability_shear_fragments == 1);
    }
    if(mode == CAPABILITY_SHEAR_COMPILE || mode == CAPABILITY_SHEAR_LINK)
    {
        capability_expect(log, mode == CAPABILITY_SHEAR_COMPILE ?
            "shader compilation failed" : "shader link failed");
        assert(capability_injected_compiles == (mode == CAPABILITY_SHEAR_COMPILE ? 1u : 0u));
        assert(capability_injected_links == (mode == CAPABILITY_SHEAR_LINK ? 1u : 0u));
        assert(capability_affine_fragments == 2 && capability_color_fragments == 2);
        assert(capability_shear_fragments == 1 && capability_legacy_fragments == 0);
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
 * 2026-11-14 - Include optional shear failures and separate uniform budgets.
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
    puts("GPU capability log tests passed: 24 startup scenarios, optional shear and retained row geometry, buffered submission and allocation cleanup, native-only color rules, ignored unavailable effects, retained ordinary draws, profiler opt-out, no draw-time spam.");
    return 0;
}
