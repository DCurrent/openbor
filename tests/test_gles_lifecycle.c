/*
 * Damon V. Caskey
 * 2026-11-20
 * Exercise shared screen preservation and identity retirement with a real
 * GLES3 context. Expected pixels are authored constants, never CPU rendering.
 */
#define main unused_lifecycle_renderer_suite
#include "test_gpu_renderer.c"
#undef main

static int lifecycle_ready = 1;
static int lifecycle_reinitialize;
static unsigned lifecycle_deletions;
static GLuint lifecycle_watched_texture;
static unsigned lifecycle_watched_deletions;
static void (APIENTRY *lifecycle_real_delete)(GLsizei, const GLuint *);
void *__real_SDL_GL_GetProcAddress(const char *name);

/* Damon V. Caskey - 2026-11-20: emulate a platform-owned suspension boundary. */
static int lifecycle_guard(void)
{
    if(lifecycle_reinitialize)
    {
        lifecycle_reinitialize = 0;
        assert(gpu_gl_abandon());
        assert(gpu_gl_init());
    }
    return lifecycle_ready;
}

/* Damon V. Caskey - 2026-11-20: observe actual driver resource retirement. */
static void APIENTRY lifecycle_delete(GLsizei count, const GLuint *textures)
{
    GLsizei index;
    lifecycle_deletions += (unsigned)count;
    for(index = 0; index < count; ++index)
        if(lifecycle_watched_texture && textures[index] == lifecycle_watched_texture)
            ++lifecycle_watched_deletions;
    lifecycle_real_delete(count, textures);
}

/* Damon V. Caskey - 2026-11-20: intercept only the existing core forwarding API. */
void *__wrap_SDL_GL_GetProcAddress(const char *name)
{
    void *address = __real_SDL_GL_GetProcAddress(name);
    if(!strcmp(name, "glDeleteTextures"))
    {
        lifecycle_real_delete = address;
        return address ? (void *)lifecycle_delete : NULL;
    }
    return address;
}

/*
 * Damon V. Caskey
 * 2026-11-20
 * Discard owners immediately while drawing is suspended, but defer driver
 * deletion until the owning context is usable. Freed CPU addresses must never
 * remain in the preservation walk or become required for deferred cleanup.
 */
static void lifecycle_retire_suspended(s_screen *survivor)
{
    unsigned char y[4] = {128, 128, 128, 128}, u[1] = {128}, v[1] = {128};
    int owner;
    gpu_yuv_source source = {&owner, 1, 2, 2, y, u, v};
    gpu_movie_draw movie = {0, 0, 4, 4, DC_RESAMPLE_NEAREST, 0, 0, 0};
    unsigned deleted;
    int iteration;
    for(iteration = 0; iteration < 16; ++iteration)
    {
        s_screen *transient = test_screen(4, 4, PIXEL_32);
        gpu_renderer_stats before;
        assert(gpu_draw_box(transient, 0, 0, 4, 4, 0x735119u, &plainmethod));
        assert(gpu_draw_yuv(transient, &source, &movie));
        before = *gpu_gl_get_stats();
        deleted = lifecycle_deletions;
        lifecycle_ready = 0;
        assert(!gpu_draw_box(survivor, 0, 0, 4, 4, 0, &plainmethod));
        assert(gpu_gl_get_stats()->readbacks == before.readbacks);
        gpu_release_yuv(source.owner);
        release_test_screen(transient);
        assert(lifecycle_deletions == deleted);
        assert(gpu_gl_get_stats()->movie_resources == 0);
        lifecycle_ready = 1;
        assert(gpu_draw_box(survivor, 0, 0, 4, 4, 0xA14D27u, &plainmethod));
        assert(lifecycle_deletions >= deleted + 4);
        assert(gpu_gl_get_stats()->readbacks == before.readbacks);
    }
}

/*
 * Damon V. Caskey
 * 2026-11-20
 * Read only GPU-newer screens at suspension, then recreate from their current
 * CPU shadows after the original context is gone. No ordinary frame reads back.
 */
static void lifecycle_preserve(SDL_Window *window, SDL_GLContext *context,
                                s_screen *screen)
{
    unsigned long reads;
    unsigned deleted;
    unsigned pixel;
    s_screen *copy;
    reads = gpu_gl_get_stats()->readbacks;
    lifecycle_ready = 0;
    assert(gpu_gl_suspend());
    assert(gpu_gl_get_stats()->readbacks == reads + 1);
    /* A checkpointed CPU read is valid even while drawing is suspended. */
    gpu_sync_cpu(screen);
    assert(gpu_gl_get_stats()->readbacks == reads + 1);
    for(pixel = 0; pixel < 16; ++pixel)
        assert(((unsigned *)screen->data)[pixel] == 0xA14D27u);
    assert(gpu_gl_suspend());
    assert(gpu_gl_get_stats()->readbacks == reads + 1);
    deleted = lifecycle_deletions;
    SDL_GL_DeleteContext(*context);
    *context = SDL_GL_CreateContext(window);
    assert(*context && LoadGLFunctions());
    assert(gpu_gl_abandon());
    assert(lifecycle_deletions == deleted);
    assert(!gpu_is_active());
    lifecycle_ready = 1;
    assert(gpu_gl_init());
    gpu_gl_set_context_guard(lifecycle_guard);
    copy = test_screen(4, 4, PIXEL_32);
    reads = gpu_gl_get_stats()->readbacks;
    assert(gpu_draw_screen(copy, screen, 0, 0, &plainmethod));
    assert(gpu_gl_get_stats()->readbacks == reads);
    gpu_sync_cpu(copy);
    for(pixel = 0; pixel < 16; ++pixel)
        assert(((unsigned *)copy->data)[pixel] == 0xA14D27u);
    release_test_screen(copy);
}

/*
 * Damon V. Caskey
 * 2026-11-20
 * SDL/EGL may reuse a context handle. Retiring an owner across a new renderer
 * generation must abandon its detached names even if that handle is equal.
 * A disposable private target also must not count as lost script contents.
 */
static void lifecycle_generation(s_screen *survivor)
{
    s_screen *transient = test_screen(4, 4, PIXEL_32);
    unsigned deleted;
    int width, height;
    uint64_t generation = gpu_gl_get_context_generation();
    assert(gpu_draw_box(transient, 0, 0, 4, 4, 0x57319bu, &plainmethod));
    assert(gpu_gl_get_screen_texture(transient, &lifecycle_watched_texture, &width, &height));
    lifecycle_watched_deletions = 0;
    lifecycle_reinitialize = 1;
    release_test_screen(transient);
    assert(gpu_gl_get_context_generation() > generation);
    assert(lifecycle_watched_deletions == 0);
    lifecycle_watched_texture = 0;
    assert(gpu_draw_screen(survivor, survivor, 0, 0, &plainmethod));
    gpu_sync_cpu(survivor);

    transient = test_screen(4, 4, PIXEL_32);
    assert(gpu_draw_box(transient, 0, 0, 4, 4, 0x794da7u, &plainmethod));
    deleted = lifecycle_deletions;
    gpu_gl_discard_screen(transient);
    release_test_screen(transient);
    assert(lifecycle_deletions == deleted);
    assert(gpu_gl_abandon());
    assert(lifecycle_deletions == deleted);
    assert(gpu_gl_init());
    gpu_gl_set_context_guard(lifecycle_guard);
}

/*
 * Damon V. Caskey
 * 2026-11-20
 * Teardown must drain a blocked owner's deferred allocation even when no live
 * owner remains to trigger normal draw-time retirement.
 */
static void lifecycle_shutdown_retired(void)
{
    s_screen *transient = test_screen(4, 4, PIXEL_32);
    unsigned deleted;
    assert(gpu_draw_box(transient, 0, 0, 4, 4, 0x12379bu, &plainmethod));
    deleted = lifecycle_deletions;
    lifecycle_ready = 0;
    release_test_screen(transient);
    assert(lifecycle_deletions == deleted);
    gpu_gl_shutdown();
    /* The deferred target plus the three shared immutable textures. */
    assert(lifecycle_deletions == deleted + 4);
    lifecycle_ready = 1;
    assert(gpu_gl_init());
    gpu_gl_set_context_guard(lifecycle_guard);
}

/*
 * Damon V. Caskey
 * 2026-11-20
 * Losing authoritative pixels must be reported; stale CPU storage cannot be
 * described as a restored frame. The platform fixture checks fatal handling.
 */
static void lifecycle_reject_lost(SDL_Window *window, SDL_GLContext *context,
                                   s_screen *screen)
{
    unsigned deleted, pixel;
    unsigned long reads = gpu_gl_get_stats()->readbacks;
    assert(gpu_draw_box(screen, 0, 0, 4, 4, 0xE39B51u, &plainmethod));
    assert(gpu_gl_get_stats()->readbacks == reads);
    deleted = lifecycle_deletions;
    SDL_GL_DeleteContext(*context);
    *context = SDL_GL_CreateContext(window);
    assert(*context && LoadGLFunctions());
    assert(!gpu_gl_abandon());
    assert(lifecycle_deletions == deleted);
    assert(gpu_gl_get_stats()->readbacks == reads);
    assert(!gpu_is_active());
    for(pixel = 0; pixel < 16; ++pixel)
        assert(((unsigned *)screen->data)[pixel] == 0xA14D27u);
}

int main(void)
{
    SDL_Window *window;
    SDL_GLContext context;
    s_screen *screen;
    if(SDL_Init(SDL_INIT_VIDEO) != 0) return 77;
    gpu_test_context_attributes();
    window = SDL_CreateWindow("OpenBOR GLES lifecycle", 0, 0, 32, 32,
                               SDL_WINDOW_OPENGL | SDL_WINDOW_HIDDEN);
    context = window ? SDL_GL_CreateContext(window) : NULL;
    if(!context)
    {
        if(window) SDL_DestroyWindow(window);
        SDL_Quit();
        return 77;
    }
    assert(LoadGLFunctions() && gpu_gl_init());
    gpu_gl_set_context_guard(lifecycle_guard);
    screen = test_screen(4, 4, PIXEL_32);
    lifecycle_retire_suspended(screen);
    lifecycle_preserve(window, &context, screen);
    lifecycle_generation(screen);
    lifecycle_shutdown_retired();
    lifecycle_reject_lost(window, &context, screen);
    gpu_gl_set_context_guard(NULL);
    release_test_screen(screen);
    SDL_GL_DeleteContext(context);
    SDL_DestroyWindow(window);
    SDL_Quit();
    puts("Android GLES3 lifecycle passed: suspended owner retirement, one-time preservation, restored pixels and lost-content refusal without stale GL deletion.");
    return 0;
}
