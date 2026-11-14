/*
 * Damon V. Caskey
 * 2026-11-14
 * Verify real buffered native draws, stream rollover, resource recovery and
 * external vertex-state ownership without production instrumentation hooks.
 */
#define main unused_submission_renderer_test_suite
#include "test_gpu_renderer.c"
#undef main
#include "gpu_transform.h"

typedef void (APIENTRYP submission_begin_fn)(GLenum);
typedef void (APIENTRYP submission_vertex_fn)(GLfloat, GLfloat);
typedef void (APIENTRYP submission_draw_fn)(GLenum, GLsizei, GLenum, const void *);

typedef enum submission_failure
{
    SUBMISSION_FAIL_NONE,
    SUBMISSION_FAIL_VERTEX_STORAGE,
    SUBMISSION_FAIL_VERTEX_UPDATE,
    SUBMISSION_FAIL_INDEX_STORAGE
} submission_failure;

typedef struct submission_trace
{
    unsigned draws, immediate_calls, vertex_storage, vertex_updates, index_storage;
    uint64_t elements, vertex_bytes;
} submission_trace;

static submission_trace submission_calls;
static submission_failure submission_fail;
static unsigned submission_failures, submission_generated, submission_deleted;
static size_t submission_capacity;
static PFNGLGENBUFFERSPROC submission_real_gen;
static PFNGLDELETEBUFFERSPROC submission_real_delete;
static PFNGLBINDBUFFERPROC submission_real_bind;
static PFNGLBUFFERDATAPROC submission_real_data;
static PFNGLBUFFERSUBDATAPROC submission_real_update;
static PFNGLGETBUFFERPARAMETERIVPROC submission_real_buffer_parameter;
static PFNGLGETVERTEXATTRIBIVPROC submission_real_attribute;
static PFNGLENABLEVERTEXATTRIBARRAYPROC submission_real_enable;
static PFNGLDISABLEVERTEXATTRIBARRAYPROC submission_real_disable;
static PFNGLVERTEXATTRIBPOINTERPROC submission_real_pointer;
static submission_begin_fn submission_real_begin;
static submission_vertex_fn submission_real_vertex, submission_real_texcoord;
static submission_draw_fn submission_real_draw;
void *__real_SDL_GL_GetProcAddress(const char *name);

/*
 * Damon V. Caskey
 * 2026-11-14
 * Track native buffer lifetimes across shutdown and actual context replacement.
 */
static void APIENTRY submission_gen(GLsizei count, GLuint *buffers)
{
    GLsizei index;
    submission_real_gen(count, buffers);
    for(index = 0; index < count; ++index) submission_generated += buffers[index] != 0;
}

/*
 * Damon V. Caskey
 * 2026-11-14
 * Count acquired names retired by the renderer, excluding harmless zero names.
 */
static void APIENTRY submission_delete(GLsizei count, const GLuint *buffers)
{
    GLsizei index;
    for(index = 0; index < count; ++index) submission_deleted += buffers[index] != 0;
    submission_real_delete(count, buffers);
}

/*
 * Damon V. Caskey
 * 2026-11-14
 * Observe allocations and inject a real driver error before storage changes.
 */
static void APIENTRY submission_data(GLenum target, GLsizeiptr size,
                                     const void *data, GLenum usage)
{
    if(target == GL_ARRAY_BUFFER) ++submission_calls.vertex_storage;
    if(target == GL_ELEMENT_ARRAY_BUFFER) ++submission_calls.index_storage;
    if((target == GL_ARRAY_BUFFER && submission_fail == SUBMISSION_FAIL_VERTEX_STORAGE) ||
       (target == GL_ELEMENT_ARRAY_BUFFER && submission_fail == SUBMISSION_FAIL_INDEX_STORAGE))
    {
        submission_fail = SUBMISSION_FAIL_NONE;
        ++submission_failures;
        submission_real_data(target, (GLsizeiptr)-1, NULL, usage);
        return;
    }
    submission_real_data(target, size, data, usage);
    if(target == GL_ARRAY_BUFFER && size > 0)
    {
        submission_capacity = (size_t)size / (4 * sizeof(gpu_vertex));
    }
}

/*
 * Damon V. Caskey
 * 2026-11-14
 * Count uploaded geometry and reject one vertex update through normal GL
 * validation without submitting the rejected payload.
 */
static void APIENTRY submission_update(GLenum target, GLintptr offset,
                                       GLsizeiptr size, const void *data)
{
    if(target == GL_ARRAY_BUFFER) ++submission_calls.vertex_updates;
    if(target == GL_ARRAY_BUFFER && submission_fail == SUBMISSION_FAIL_VERTEX_UPDATE)
    {
        submission_fail = SUBMISSION_FAIL_NONE;
        ++submission_failures;
        submission_real_update(target, offset, (GLsizeiptr)-1, data);
        return;
    }
    /* Retain actual payload size so one batch cannot conceal many row quads. */
    if(target == GL_ARRAY_BUFFER && size > 0)
    {
        submission_calls.vertex_bytes += (uint64_t)size;
    }
    submission_real_update(target, offset, size, data);
}

/*
 * Damon V. Caskey
 * 2026-11-14
 * Detect any native return to immediate-mode geometry while retaining real GL.
 */
static void APIENTRY submission_begin(GLenum mode)
{
    ++submission_calls.immediate_calls;
    submission_real_begin(mode);
}

/*
 * Damon V. Caskey
 * 2026-11-14
 * Count individual position calls independently from draw-batch boundaries.
 */
static void APIENTRY submission_vertex(GLfloat x, GLfloat y)
{
    ++submission_calls.immediate_calls;
    submission_real_vertex(x, y);
}

/*
 * Damon V. Caskey
 * 2026-11-14
 * Count individual texture-coordinate calls independently from positions.
 */
static void APIENTRY submission_texcoord(GLfloat u, GLfloat v)
{
    ++submission_calls.immediate_calls;
    submission_real_texcoord(u, v);
}

/*
 * Damon V. Caskey
 * 2026-11-14
 * Prove each native submission consumes buffered indexed triangles and valid
 * explicit attributes, including nonzero element offsets after stream append.
 */
static void APIENTRY submission_draw(GLenum mode, GLsizei count, GLenum type,
                                     const void *indices)
{
    GLint vertex_buffer, index_buffer, position_buffer, texture_buffer;
    GLint enabled, index_bytes;
    assert(mode == GL_TRIANGLES && type == GL_UNSIGNED_INT && count > 0 && count % 6 == 0);
    glGetIntegerv(GL_ARRAY_BUFFER_BINDING, &vertex_buffer);
    glGetIntegerv(GL_ELEMENT_ARRAY_BUFFER_BINDING, &index_buffer);
    assert(vertex_buffer && index_buffer);
    submission_real_attribute(0, GL_VERTEX_ATTRIB_ARRAY_BUFFER_BINDING, &position_buffer);
    submission_real_attribute(1, GL_VERTEX_ATTRIB_ARRAY_BUFFER_BINDING, &texture_buffer);
    assert(position_buffer == vertex_buffer && texture_buffer == vertex_buffer);
    submission_real_attribute(0, GL_VERTEX_ATTRIB_ARRAY_ENABLED, &enabled);
    assert(enabled);
    submission_real_attribute(1, GL_VERTEX_ATTRIB_ARRAY_ENABLED, &enabled);
    assert(enabled);
    submission_real_buffer_parameter(GL_ELEMENT_ARRAY_BUFFER, GL_BUFFER_SIZE, &index_bytes);
    assert((uintptr_t)indices + (size_t)count * sizeof(GLuint) <= (size_t)index_bytes);
    ++submission_calls.draws;
    submission_calls.elements += (unsigned)count;
    submission_real_draw(mode, count, type, indices);
}

/*
 * Damon V. Caskey
 * 2026-11-14
 * Intercept only submission entry points from the existing SDL loader.
 */
void *__wrap_SDL_GL_GetProcAddress(const char *name)
{
    void *entry = __real_SDL_GL_GetProcAddress(name);
#define SUBMISSION_WRAP(symbol, type, storage, replacement) \
    if(!strcmp(name, symbol)) { storage = (type)entry; return entry ? (void *)replacement : NULL; }
    SUBMISSION_WRAP("glGenBuffers", PFNGLGENBUFFERSPROC, submission_real_gen, submission_gen);
    SUBMISSION_WRAP("glDeleteBuffers", PFNGLDELETEBUFFERSPROC, submission_real_delete, submission_delete);
    SUBMISSION_WRAP("glBufferData", PFNGLBUFFERDATAPROC, submission_real_data, submission_data);
    SUBMISSION_WRAP("glBufferSubData", PFNGLBUFFERSUBDATAPROC, submission_real_update, submission_update);
    SUBMISSION_WRAP("glDrawElements", submission_draw_fn, submission_real_draw, submission_draw);
    SUBMISSION_WRAP("glBegin", submission_begin_fn, submission_real_begin, submission_begin);
    SUBMISSION_WRAP("glVertex2f", submission_vertex_fn, submission_real_vertex, submission_vertex);
    SUBMISSION_WRAP("glTexCoord2f", submission_vertex_fn, submission_real_texcoord, submission_texcoord);
#undef SUBMISSION_WRAP
    return entry;
}

/*
 * Damon V. Caskey
 * 2026-11-14
 * Give each source texel a unique RGB value to expose addressing and seam errors.
 */
static void submission_fill_source(s_screen *source)
{
    int pixel;
    for(pixel = 0; pixel < source->width * source->height; ++pixel)
    {
        ((unsigned *)source->data)[pixel] = 0x00100000u | (unsigned)(pixel + 1);
    }
}

/*
 * Damon V. Caskey
 * 2026-11-14
 * Compare complete native images, including untouched pixels around geometry.
 */
static void submission_verify(s_screen *screen, const unsigned *expected, const char *label)
{
    int pixel;
    gpu_sync_cpu(screen);
    for(pixel = 0; pixel < screen->width * screen->height; ++pixel)
    {
        if(test_pixel(screen, pixel % screen->width, pixel / screen->width) != expected[pixel])
        {
            fprintf(stderr, "%s pixel(%d,%d): expected %06x, actual %06x\n", label,
                    pixel % screen->width, pixel / screen->width, expected[pixel],
                    test_pixel(screen, pixel % screen->width, pixel / screen->width));
            assert(0);
        }
    }
}

/*
 * Damon V. Caskey
 * 2026-11-14
 * Prove safe legacy shear uploads four vertices and submits six real indices
 * while preserving signed row rounding, both flips and the drawmethod pivot.
 */
static void test_submission_shear(void)
{
    s_screen *source = test_screen(7, 9, PIXEL_32);
    s_screen *target = test_screen(32, 24, PIXEL_32);
    unsigned expected[32 * 24];
    const int draw_x = 12, draw_y = 6;
    int sign, flip, sx, sy;
    submission_fill_source(source);

    /* Unit scales give an independent staircase: each source row is shifted
     * from its destination lower edge, then its whole span rounds downward. */
    for(sign = -1; sign <= 1; sign += 2)
    {
        for(flip = 0; flip < 4; ++flip)
        {
            s_drawmethod method = plainmethod;
            unsigned long fallbacks, readbacks;
            int origin_x, origin_y, shift;
            method.shiftx = sign * 128;
            method.centerx = 2; method.centery = 3;
            method.config |= (flip & 1 ? DRAWMETHOD_CONFIG_FLIP_X : 0)
                           | (flip & 2 ? DRAWMETHOD_CONFIG_FLIP_Y : 0);
            origin_x = draw_x + (flip & 1 ? method.centerx - source->width : -method.centerx);
            origin_y = draw_y + (flip & 2 ? method.centery - source->height : -method.centery);
            shift = flip & 2 ? -method.shiftx : method.shiftx;
            memset(expected, 0, sizeof(expected));
            for(sy = 0; sy < source->height; ++sy)
            {
                int y = origin_y + (flip & 2 ? source->height - 1 - sy : sy);
                int left = origin_x + (int)floor((double)(y + 1 - draw_y) * shift / 256.0);
                for(sx = 0; sx < source->width; ++sx)
                {
                    int x = left + (flip & 1 ? source->width - 1 - sx : sx);
                    assert(x >= 0 && x < target->width && y >= 0 && y < target->height);
                    expected[y * target->width + x] =
                        ((unsigned *)source->data)[sy * source->width + sx];
                }
            }

            /* Clear before tracing. Readback follows the assertions so the
             * measured operation consists only of the native image draw. */
            assert(gpu_clear_screen(target));
            fallbacks = gpu_gl_get_stats()->fallback_draws;
            readbacks = gpu_gl_get_stats()->readbacks;
            memset(&submission_calls, 0, sizeof(submission_calls));
            assert(gpu_draw_screen(target, source, draw_x, draw_y, &method));
            assert(submission_calls.draws == 1 && submission_calls.elements == 6);
            assert(submission_calls.vertex_updates == 1);
            assert(submission_calls.vertex_bytes == 4 * sizeof(gpu_vertex));
            assert(submission_calls.vertex_bytes == 64);
            assert(submission_calls.immediate_calls == 0);
            assert(gpu_gl_get_stats()->fallback_draws == fallbacks);
            assert(gpu_gl_get_stats()->readbacks == readbacks);
            submission_verify(target, expected, "single-quad legacy shear");
        }
    }
    release_test_screen(target);
    release_test_screen(source);
}

/*
 * Damon V. Caskey
 * 2026-11-14
 * Submit enough row geometry to roll the streaming VBO while retaining each
 * earlier draw's vertices and reusing the static quad-index buffer.
 */
static void test_submission_stream(void)
{
    const int rows = 193;
    int repeats = (int)(submission_capacity / rows) + 2;
    s_screen *source = test_screen(8, rows, PIXEL_32);
    s_screen *target;
    s_drawmethod method = plainmethod;
    unsigned *expected;
    int draw, x, y;
    assert(repeats > 2 && repeats < 128);
    target = test_screen(repeats * 9 + 1, rows + 4, PIXEL_32);
    expected = calloc((size_t)target->width * target->height, sizeof(*expected));
    assert(expected);
    submission_fill_source(source);
    method.water.watermode = WATER_MODE_SINE;
    method.water.wavelength = 1;
    method.water.amplitude = 1;
    assert(gpu_clear_screen(target));
    memset(&submission_calls, 0, sizeof(submission_calls));

    /* One full wave cycle per row gives every row the same anchored phase.
     * Nonzero amplitude selects row geometry while leaving the expected image
     * unwarped. Different columns expose stale VBO ranges after rollover. */
    for(draw = 0; draw < repeats; ++draw)
    {
        assert(gpu_draw_screen(target, source, 1 + draw * 9, 2, &method));
        for(y = 0; y < rows; ++y)
        {
            for(x = 0; x < source->width; ++x)
            {
                expected[(y + 2) * target->width + 1 + draw * 9 + x] =
                    ((unsigned *)source->data)[y * source->width + x];
            }
        }
    }
    assert(submission_calls.draws == (unsigned)repeats);
    assert(submission_calls.elements == (uint64_t)repeats * rows * 6);
    assert(submission_calls.vertex_updates == (unsigned)repeats);
    assert(submission_calls.vertex_storage > 0);
    assert(submission_calls.index_storage == 0);
    assert(submission_calls.immediate_calls == 0);
    submission_verify(target, expected, "stream rollover");
    free(expected);
    release_test_screen(target);
    release_test_screen(source);
}

/*
 * Damon V. Caskey
 * 2026-11-14
 * Fail EBO growth, VBO orphaning and vertex upload before destination writes,
 * then retry each operation to prove capacity and binding recovery.
 */
static void test_submission_failures(void)
{
    const submission_failure failures[] = {
        SUBMISSION_FAIL_INDEX_STORAGE, SUBMISSION_FAIL_VERTEX_STORAGE,
        SUBMISSION_FAIL_VERTEX_UPDATE
    };
    const unsigned background = 0x00214365u;
    int rows = (int)submission_capacity + 17;
    s_screen *source = test_screen(8, rows, PIXEL_32);
    s_screen *target = test_screen(12, rows + 4, PIXEL_32);
    s_drawmethod method = plainmethod;
    unsigned *expected = malloc((size_t)target->width * target->height * sizeof(*expected));
    unsigned failure, pixel;
    assert(expected);
    submission_fill_source(source);
    method.water.watermode = WATER_MODE_SINE;
    /* The one-pixel wavelength advances exactly 256 phase units per row.
     * Every row keeps the same displacement relative to its anchored origin. */
    method.water.wavelength = 1;
    method.water.amplitude = 1;

    for(failure = 0; failure < sizeof(failures) / sizeof(failures[0]); ++failure)
    {
        unsigned injected = submission_failures;
        unsigned long rejected = gpu_gl_get_stats()->fallback_draws;
        int x, y;
        assert(gpu_draw_box(target, 0, 0, target->width, target->height, background, &plainmethod));
        for(pixel = 0; pixel < (unsigned)(target->width * target->height); ++pixel)
        {
            expected[pixel] = background;
        }
        memset(&submission_calls, 0, sizeof(submission_calls));
        submission_fail = failures[failure];
        assert(!gpu_draw_screen(target, source, 2, 2, &method));
        assert(submission_fail == SUBMISSION_FAIL_NONE && submission_failures == injected + 1);
        assert(submission_calls.draws == 0 && submission_calls.immediate_calls == 0);
        assert(gpu_gl_get_stats()->fallback_draws == rejected + 1);
        assert(glGetError() == GL_NO_ERROR);
        submission_verify(target, expected, "failed buffered draw remains atomic");

        /* Readback above inspects the GPU-authored background. A repaired
         * retry must submit the full stream without relying on stale storage. */
        assert(gpu_draw_screen(target, source, 2, 2, &method));
        assert(submission_calls.draws == 1);
        assert(submission_calls.elements == (uint64_t)rows * 6);
        for(y = 0; y < rows; ++y)
        {
            for(x = 0; x < source->width; ++x)
            {
                expected[(y + 2) * target->width + x + 2] =
                    ((unsigned *)source->data)[y * source->width + x];
            }
        }
        submission_verify(target, expected, "buffered draw retry");
    }
    free(expected);
    release_test_screen(target);
    release_test_screen(source);
}

/*
 * Damon V. Caskey
 * 2026-11-14
 * Restore explicit vertex inputs after an external renderer changes both
 * buffer bindings, attribute pointers and enable state at an ownership boundary.
 */
static void test_submission_external_state(void)
{
    GLuint external[2];
    const GLfloat poisoned[16] = {0};
    s_screen *source = test_screen(4, 4, PIXEL_32);
    s_screen *target = test_screen(4, 4, PIXEL_32);
    GLint binding, enabled;
    submission_fill_source(source);
    assert(gpu_draw_screen(target, source, 0, 0, &plainmethod));
    gpu_gl_prepare_present();
    glGetIntegerv(GL_ARRAY_BUFFER_BINDING, &binding); assert(binding == 0);
    glGetIntegerv(GL_ELEMENT_ARRAY_BUFFER_BINDING, &binding); assert(binding == 0);
    submission_real_attribute(0, GL_VERTEX_ATTRIB_ARRAY_ENABLED, &enabled); assert(!enabled);
    submission_real_attribute(1, GL_VERTEX_ATTRIB_ARRAY_ENABLED, &enabled); assert(!enabled);

    /* Direct real entry points deliberately bypass the native cache and test
     * lifetime counters, matching a separate renderer sharing the context. */
    submission_real_gen(2, external);
    submission_real_bind(GL_ARRAY_BUFFER, external[0]);
    submission_real_data(GL_ARRAY_BUFFER, sizeof(poisoned), poisoned, GL_STATIC_DRAW);
    submission_real_bind(GL_ELEMENT_ARRAY_BUFFER, external[1]);
    submission_real_data(GL_ELEMENT_ARRAY_BUFFER, sizeof(poisoned), poisoned, GL_STATIC_DRAW);
    submission_real_pointer(0, 2, GL_FLOAT, GL_FALSE, 0, NULL);
    submission_real_pointer(1, 2, GL_FLOAT, GL_FALSE, 0, NULL);
    submission_real_enable(0);
    submission_real_disable(1);
    gpu_gl_invalidate_state();
    glGetIntegerv(GL_ARRAY_BUFFER_BINDING, &binding); assert(binding == (GLint)external[0]);
    glGetIntegerv(GL_ELEMENT_ARRAY_BUFFER_BINDING, &binding); assert(binding == (GLint)external[1]);
    assert(glGetError() == GL_NO_ERROR);
    assert(gpu_clear_screen(target));
    assert(gpu_draw_screen(target, source, 0, 0, &plainmethod));
    submission_verify(target, (const unsigned *)source->data, "external vertex state");
    submission_real_delete(2, external);
    release_test_screen(target);
    release_test_screen(source);
}

/*
 * Damon V. Caskey
 * 2026-11-14
 * Run the real native backend in two separate contexts. Submission tracing
 * proves buffered execution; image comparisons prove retained draw semantics.
 */
int main(void)
{
    SDL_Window *window;
    SDL_GLContext context;
    int iteration;
    if(SDL_Init(SDL_INIT_VIDEO) != 0) return 77;
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_COMPATIBILITY);
    window = SDL_CreateWindow("OpenBOR submission tests", 0, 0, 64, 64,
                              SDL_WINDOW_OPENGL | SDL_WINDOW_HIDDEN);
    context = window ? SDL_GL_CreateContext(window) : NULL;
    if(!context)
    {
        if(window) SDL_DestroyWindow(window);
        SDL_Quit();
        return 77;
    }
    for(iteration = 0; iteration < 2; ++iteration)
    {
        /* Driver entry points belong to the current context, just like the
         * renderer's private loader and the resources verified below. */
        submission_real_bind = (PFNGLBINDBUFFERPROC)__real_SDL_GL_GetProcAddress("glBindBuffer");
        submission_real_buffer_parameter = (PFNGLGETBUFFERPARAMETERIVPROC)__real_SDL_GL_GetProcAddress("glGetBufferParameteriv");
        submission_real_attribute = (PFNGLGETVERTEXATTRIBIVPROC)__real_SDL_GL_GetProcAddress("glGetVertexAttribiv");
        submission_real_enable = (PFNGLENABLEVERTEXATTRIBARRAYPROC)__real_SDL_GL_GetProcAddress("glEnableVertexAttribArray");
        submission_real_disable = (PFNGLDISABLEVERTEXATTRIBARRAYPROC)__real_SDL_GL_GetProcAddress("glDisableVertexAttribArray");
        submission_real_pointer = (PFNGLVERTEXATTRIBPOINTERPROC)__real_SDL_GL_GetProcAddress("glVertexAttribPointer");
        assert(submission_real_bind && submission_real_buffer_parameter && submission_real_attribute &&
               submission_real_enable && submission_real_disable && submission_real_pointer);
        assert(LoadGLFunctions() && gpu_gl_init());
        assert(submission_capacity > 0 && submission_generated > submission_deleted);
        if(!iteration)
        {
            test_submission_stream();
            test_submission_failures();
        }
        test_submission_shear();
        test_submission_external_state();
        assert(submission_calls.immediate_calls == 0);
        gpu_gl_shutdown();
        assert(submission_generated == submission_deleted);
        assert(glGetError() == GL_NO_ERROR);
        SDL_GL_DeleteContext(context);
        if(!iteration)
        {
            context = SDL_GL_CreateContext(window);
            assert(context);
        }
    }
    SDL_DestroyWindow(window);
    SDL_Quit();
    printf("GPU buffered submission tests passed: streamed indexed triangles, VBO rollover, "
           "six-index/64-byte shear, three atomic failure/retry cases, "
           "vertex-state restoration and context recreation.\n");
    return 0;
}
