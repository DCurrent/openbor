/*
 * Native GPU shutdown regression for colliding resource-cache identities.
 * Build using the command in test_gpu_renderer.c, replacing that test filename
 * with this one. Add -fsanitize=address -fno-omit-frame-pointer for the lifetime
 * check. Run with SDL_VIDEODRIVER=offscreen. Mesa validates correctness here,
 * not hardware performance. Exit status 77 means no GL context was available.
 *
 * Reuse the native test allocation/dispatch fixtures without running its suite.
 */
#define main unused_gpu_renderer_test_suite
#include "test_gpu_renderer.c"
#undef main

#define SHUTDOWN_TEST_BUCKETS 1024
#define SHUTDOWN_TEST_CANDIDATES 2048

/*
 * Damon V. Caskey
 * 2026-11-12
 * Match the renderer's identity hash so this test deliberately constructs a
 * multi-node bucket rather than depending on incidental allocator behavior.
 */
static size_t shutdown_test_bucket(const void *owner)
{
    uintptr_t value = (uintptr_t)owner;

    /* Keep this structural test aligned if GPU_CACHE_BUCKETS/hash changes. */
    return ((value >> 4) ^ (value >> 14)) & (SHUTDOWN_TEST_BUCKETS - 1);
}

/*
 * Damon V. Caskey
 * 2026-11-12
 * Create two live GPU-owned screens sharing one cache bucket, then destroy
 * the backend while both CPU shadows are stale. Each node must be read back
 * before deletion without following a previously freed bucket head.
 */
int main(void)
{
    s_screen *candidates[SHUTDOWN_TEST_CANDIDATES] = {0};
    s_screen *buckets[SHUTDOWN_TEST_BUCKETS] = {0};
    s_screen *first = NULL, *second = NULL;
    SDL_Window *window;
    SDL_GLContext context;
    int count = 0, i;

    /* Separate unavailable display infrastructure from a rendering failure. */
    if(SDL_Init(SDL_INIT_VIDEO) != 0)
    {
        fprintf(stderr, "SKIP: SDL video unavailable: %s\n", SDL_GetError());
        return 77;
    }
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK,
                        SDL_GL_CONTEXT_PROFILE_COMPATIBILITY);
    window = SDL_CreateWindow("OpenBOR GPU shutdown tests", 0, 0, 64, 64,
                              SDL_WINDOW_OPENGL | SDL_WINDOW_HIDDEN);
    context = window ? SDL_GL_CreateContext(window) : NULL;
    if(!context)
    {
        fprintf(stderr, "SKIP: GL context unavailable: %s\n", SDL_GetError());
        if(window) SDL_DestroyWindow(window);
        SDL_Quit();
        return 77;
    }
    assert(LoadGLFunctions());
    assert(gpu_gl_init());

    /* Only the colliding pair become GPU resources; other candidates remain
     * small CPU allocations and establish a deterministic pigeonhole bound. */
    while(count < SHUTDOWN_TEST_CANDIDATES && !second)
    {
        size_t bucket;
        s_screen *screen = test_screen(4, 4, PIXEL_32);
        candidates[count++] = screen;
        bucket = shutdown_test_bucket(screen);
        if(buckets[bucket])
        {
            first = buckets[bucket];
            second = screen;
        }
        else
        {
            buckets[bucket] = screen;
        }
    }
    assert(first && second && first != second);
    assert(shutdown_test_bucket(first) == shutdown_test_bucket(second));

    /* Both queue-independent writes remain on GPU. No explicit sync is
     * permitted before shutdown, otherwise the broken chain could be hidden. */
    assert(gpu_clear_screen(first));
    assert(gpu_draw_box(first, 0, 0, 4, 4, 0x123456, &plainmethod));
    assert(gpu_clear_screen(second));
    assert(gpu_draw_box(second, 0, 0, 4, 4, 0xABCDEF, &plainmethod));
    assert(((unsigned *)first->data)[0] == 0);
    assert(((unsigned *)second->data)[0] == 0);
    assert(gpu_gl_get_stats()->readbacks == 0);

    /* Shutdown itself must resolve both versions while traversing the bucket. */
    gpu_gl_shutdown();
    assert(!gpu_is_active());
    assert(gpu_gl_get_stats()->readbacks == 2);
    for(i = 0; i < 16; ++i)
    {
        assert(((unsigned *)first->data)[i] == 0x123456);
        assert(((unsigned *)second->data)[i] == 0xABCDEF);
    }

    /* Cached identities are gone, so releasing every CPU owner is harmless. */
    for(i = 0; i < count; ++i) release_test_screen(candidates[i]);
    SDL_GL_DeleteContext(context);
    SDL_DestroyWindow(window);
    SDL_Quit();
    puts("GPU shutdown collision test passed: two stale shadows synchronized safely.");
    return 0;
}
