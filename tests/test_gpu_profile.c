/*
 * Native profiler/query lifecycle regression tests.
 *
 * Reuse the renderer's image fixtures while keeping its main suite independent.
 * Functional pixel reads synchronize test images; the profiler itself must
 * never force GPU completion to obtain a timestamp sample.
 */
#define main unused_backend_test_suite
#include "test_gpu_renderer.c"
#undef main
#include "engine_profile.h"

static uint64_t profile_test_time_us;
static unsigned profile_test_log_lines;
static int profile_test_scene;

/*
 * Damon V. Caskey
 * 2026-11-12
 *
 * Make CPU accounting deterministic while real GL commands supply independent
 * GPU timestamps. Native durations are tested for attribution, not speed.
 */
static uint64_t profile_test_clock(void)
{
    return profile_test_time_us;
}

/*
 * Damon V. Caskey
 * 2026-11-12
 *
 * Check that native capability and measurement reports use the normal engine
 * log prefix instead of relying on a terminal-only diagnostic stream.
 */
static void profile_test_log(const char *line)
{
    assert(line && strncmp(line, "[ENGINE PROFILE]", 16) == 0);
    ++profile_test_log_lines;
}

/*
 * Damon V. Caskey
 * 2026-11-12
 *
 * Request a current CPU shadow before inspecting a GPU-authored pixel. This
 * is ordinary image-coherence work, independently counted as a readback when
 * needed, rather than a completion wait introduced by timestamp polling.
 */
static unsigned profile_test_read_pixel(s_screen *screen, int x, int y)
{
    gpu_sync_cpu(screen);
    return test_pixel(screen, x, y);
}

/*
 * Damon V. Caskey
 * 2026-11-12
 *
 * Open a single-frame collection window around the next native operation.
 * The target pointer also lets queue hooks distinguish main-screen playback.
 */
static uint64_t profile_test_begin(s_screen *target)
{
    profile_test_time_us = 1000;
    return engine_profile_frame_begin(1, &profile_test_scene, target);
}

/*
 * Damon V. Caskey
 * 2026-11-12
 *
 * Publish one completed frame using its next update entry. The explicit
 * report barrier keeps driver/log timing outside deterministic CPU totals.
 */
static const ep_report *profile_test_finish(uint64_t frame, s_screen *target)
{
    profile_test_time_us = 2000;
    engine_profile_frame_end(frame);
    profile_test_time_us = 11000;
    assert(engine_profile_frame_begin(1, &profile_test_scene, target) == 0);
    assert(engine_profile_last_report()->frames == 1);
    return engine_profile_last_report();
}

/*
 * Damon V. Caskey
 * 2026-11-12
 *
 * Keep native rendering available when the master profiler switch is off.
 * Disabled measurement must allocate no active timestamp provider or samples.
 */
static void test_native_profile_disabled(void)
{
    s_screen *target = test_screen(4, 4, PIXEL_32);

    engine_profile_configure(0, 1, 0);
    assert(SDL_setenv("OPENBOR_PROFILE_GPU", "1", 1) == 0);
    assert(gpu_gl_init());
    assert(!gpu_gl_get_query_stats()->enabled);
    assert(profile_test_begin(target) == 0);
    assert(engine_profile_gpu_begin(EP_GPU_SCENE) == 0);
    assert(gpu_draw_box(target, 0, 0, 4, 4, 0x00123456, &plainmethod));
    assert(profile_test_read_pixel(target, 2, 2) == 0x00123456);
    engine_profile_gpu_poll();
    assert(gpu_gl_get_query_stats()->issued == 0);
    assert(engine_profile_last_report()->frames == 0);
    release_test_screen(target);
    gpu_gl_shutdown();
}

/*
 * Damon V. Caskey
 * 2026-11-12
 *
 * Exercise native counters with GPU timestamps disabled independently. Image
 * uploads, destination snapshots, CPU reads and declined formats remain
 * measurable even on drivers without timer-query support.
 */
static void test_native_profile_counters(void)
{
    s_screen *target = test_screen(4, 4, PIXEL_32);
    s_screen *source = test_screen(2, 2, PIXEL_32);
    s_screen *legacy = test_screen(4, 4, PIXEL_16);
    s_drawmethod blend = plainmethod;
    const ep_report *report;
    uint64_t frame;
    unsigned index;

    engine_profile_configure(1, 1, 0);
    assert(SDL_setenv("OPENBOR_PROFILE_GPU", "0", 1) == 0);
    assert(gpu_gl_init());
    assert(!gpu_gl_get_query_stats()->enabled);
    for(index = 0; index < 4; ++index)
    {
        ((unsigned *)source->data)[index] = 0x00aabbcc;
    }
    frame = profile_test_begin(target);
    assert(frame);
    assert(engine_profile_gpu_begin(EP_GPU_SCENE) == 0);
    assert(gpu_clear_screen(target));
    assert(gpu_draw_box(target, 0, 0, 4, 4, 0x00102030, &plainmethod));
    assert(gpu_draw_screen(target, source, 1, 1, &plainmethod));
    blend.alpha = 6;
    assert(gpu_draw_box(target, 0, 0, 1, 1, 0x00102030, &blend));
    assert(!gpu_clear_screen(legacy));

    /* The first pixel request requires one complete 4x4 RGB32 readback. The
     * second request reuses that CPU shadow and must not add another transfer. */
    assert(profile_test_read_pixel(target, 0, 0) == 0x00102030);
    assert(profile_test_read_pixel(target, 1, 1) == 0x00aabbcc);
    report = profile_test_finish(frame, target);
    assert(report->counts[EP_COUNT_NATIVE_DRAWS] == 3);
    assert(report->counts[EP_COUNT_NATIVE_CLEARS] == 1);
    assert(report->counts[EP_COUNT_NATIVE_QUADS] == 3);
    assert(report->counts[EP_COUNT_NATIVE_BATCHES] == 3);
    assert(report->counts[EP_COUNT_IMAGE_UPLOADS] >= 1);
    assert(report->counts[EP_COUNT_IMAGE_UPLOAD_BYTES] >= 16);
    assert(report->counts[EP_COUNT_DEST_COPIES] == 1);
    assert(report->counts[EP_COUNT_DEST_COPY_BYTES] == 4);
    assert(report->counts[EP_COUNT_READBACKS] == 1);
    assert(report->counts[EP_COUNT_READBACK_BYTES] == 64);
    assert(report->fallbacks[EP_FALLBACK_TARGET_FORMAT] == 1);
    assert(report->counts[EP_COUNT_GPU_QUERY_SKIPPED] == 1);
    assert(report->gpu_samples[EP_GPU_SCENE] == 0);
    release_test_screen(legacy);
    release_test_screen(source);
    release_test_screen(target);
    gpu_gl_shutdown();
}

/*
 * Damon V. Caskey
 * 2026-11-12
 *
 * Verify nested timestamp pairs and a saturated ring against a real driver.
 * Pixel assertions complete actual rendering before polling, so production
 * query code needs no flush, completion wait, or retry loop in this fixture.
 */
static int test_native_profile_queries(void)
{
    s_screen *target = test_screen(4, 4, PIXEL_32);
    const ep_report *report;
    const gpu_query_stats *stats;
    uint64_t frame, scene, capture;
    uint64_t handles[256];
    uint64_t issued, completed, dropped;
    unsigned index;

    engine_profile_configure(1, 1, 0);
    assert(SDL_setenv("OPENBOR_PROFILE_GPU", "1", 1) == 0);
    assert(gpu_gl_init());
    stats = gpu_gl_get_query_stats();
    if(!stats->enabled)
    {
        puts("SKIP: optional native GPU timestamps unavailable; native profiler counters were verified.");
        release_test_screen(target);
        gpu_gl_shutdown();
        return 0;
    }

    frame = profile_test_begin(target);
    scene = engine_profile_gpu_begin(EP_GPU_SCENE);
    capture = engine_profile_gpu_begin(EP_GPU_CAPTURE);
    assert(scene && capture && scene != capture);
    assert(gpu_draw_box(target, 0, 0, 4, 4, 0x00010203, &plainmethod));
    engine_profile_gpu_end(capture);
    engine_profile_gpu_end(scene);
    assert(profile_test_read_pixel(target, 2, 2) == 0x00010203);
    engine_profile_gpu_poll();
    assert(stats->enabled && stats->errors == 0);
    assert(stats->issued == 2 && stats->completed == 2 && stats->pending == 0);
    report = profile_test_finish(frame, target);
    assert(report->gpu_samples[EP_GPU_SCENE] == 1);
    assert(report->gpu_samples[EP_GPU_CAPTURE] == 1);
    assert(report->counts[EP_COUNT_GPU_QUERY_PAIRS] == 2);

    /* Holding every scope open makes saturation deterministic even on a fast
     * GPU. Neither polling nor an extra begin may recycle an unfinished slot. */
    engine_profile_configure(1, 1, 0);
    frame = profile_test_begin(target);
    issued = stats->issued;
    completed = stats->completed;
    dropped = stats->dropped;
    for(index = 0; index < sizeof(handles) / sizeof(handles[0]); ++index)
    {
        handles[index] = engine_profile_gpu_begin(EP_GPU_CAPTURE);
        assert(handles[index]);
    }
    assert(engine_profile_gpu_begin(EP_GPU_CAPTURE) == 0);
    engine_profile_gpu_poll();
    assert(stats->pending == 256);
    assert(stats->issued == issued + 256);
    assert(stats->completed == completed);
    assert(stats->dropped == dropped + 1);
    for(index = 256; index > 0; --index)
    {
        engine_profile_gpu_end(handles[index - 1]);
    }
    assert(gpu_draw_box(target, 0, 0, 4, 4, 0x00445566, &plainmethod));
    assert(profile_test_read_pixel(target, 1, 1) == 0x00445566);
    engine_profile_gpu_poll();
    assert(stats->errors == 0 && stats->pending == 0);
    assert(stats->completed == completed + 256);
    report = profile_test_finish(frame, target);
    assert(report->counts[EP_COUNT_GPU_QUERY_PAIRS] == 256);
    assert(report->counts[EP_COUNT_GPU_QUERY_DROPPED] == 1);
    assert(report->gpu_samples[EP_GPU_CAPTURE] == 256);
    release_test_screen(target);
    gpu_gl_shutdown();
    return 1;
}

/*
 * Damon V. Caskey
 * 2026-11-12
 *
 * Preserve an unrelated GL error across every optional timestamp boundary.
 * Profiling must leave error ownership with the renderer or presenter that
 * caused it, rather than consume it or disable timing because of that error.
 */
static void test_native_profile_error_state(void)
{
    s_screen *target = test_screen(4, 4, PIXEL_32);
    const gpu_query_stats *stats;
    const ep_report *report;
    uint64_t frame, query;

    engine_profile_configure(1, 1, 0);
    assert(gpu_gl_init());
    stats = gpu_gl_get_query_stats();
    assert(stats->enabled);
    frame = profile_test_begin(target);
    assert(frame);
    assert(glGetError() == GL_NO_ERROR);

    /* Inject outside native drawing, then consume the error only in the
     * fixture after checking begin. Valid timestamp commands still execute
     * while the earlier GL error remains available to its actual owner. */
    glEnable((GLenum)0xffffffffu);
    query = engine_profile_gpu_begin(EP_GPU_CAPTURE);
    assert(query && stats->enabled && stats->errors == 0);
    assert(glGetError() == GL_INVALID_ENUM);
    assert(glGetError() == GL_NO_ERROR);

    glEnable((GLenum)0xffffffffu);
    engine_profile_gpu_end(query);
    assert(stats->enabled && stats->errors == 0);
    assert(glGetError() == GL_INVALID_ENUM);
    assert(glGetError() == GL_NO_ERROR);

    /* Ordinary image validation finishes the preceding commands. The third
     * injected error therefore covers both readiness and result reads during
     * one nonblocking poll, without introducing a timer-specific wait. */
    assert(gpu_draw_box(target, 0, 0, 4, 4, 0x00112233, &plainmethod));
    assert(profile_test_read_pixel(target, 2, 2) == 0x00112233);
    assert(glGetError() == GL_NO_ERROR);
    glEnable((GLenum)0xffffffffu);
    engine_profile_gpu_poll();
    assert(stats->enabled && stats->errors == 0);
    assert(stats->pending == 0 && stats->completed == 1);
    assert(glGetError() == GL_INVALID_ENUM);
    assert(glGetError() == GL_NO_ERROR);
    report = profile_test_finish(frame, target);
    assert(report->gpu_samples[EP_GPU_CAPTURE] == 1);
    assert(report->counts[EP_COUNT_GPU_QUERY_PAIRS] == 1);
    release_test_screen(target);
    gpu_gl_shutdown();
}

/*
 * Damon V. Caskey
 * 2026-11-12
 *
 * Retire an open query with its context, then try its stale token against a
 * replacement context. Generation checks must protect newly allocated scopes
 * even when GL recycles object names and the ring reuses the same slot.
 */
static SDL_GLContext test_native_profile_context(SDL_Window *window, SDL_GLContext context)
{
    s_screen *target = test_screen(4, 4, PIXEL_32);
    const gpu_query_stats *stats;
    const ep_report *report;
    uint64_t frame, stale, fresh;

    engine_profile_configure(1, 1, 0);
    assert(gpu_gl_init());
    assert(gpu_gl_get_query_stats()->enabled);
    frame = profile_test_begin(target);
    assert(frame);
    stale = engine_profile_gpu_begin(EP_GPU_CAPTURE);
    assert(stale);
    assert(gpu_draw_box(target, 0, 0, 4, 4, 0x00001122, &plainmethod));
    engine_profile_reset("native fixture context replacement");
    gpu_gl_shutdown();
    assert(gpu_gl_get_query_stats()->abandoned == 1);
    assert(profile_test_read_pixel(target, 2, 2) == 0x00001122);
    SDL_GL_DeleteContext(context);
    context = SDL_GL_CreateContext(window);
    assert(context && LoadGLFunctions());
    assert(gpu_gl_init());
    stats = gpu_gl_get_query_stats();
    assert(stats->enabled);

    frame = profile_test_begin(target);
    fresh = engine_profile_gpu_begin(EP_GPU_CAPTURE);
    assert(fresh && fresh != stale);
    engine_profile_gpu_end(stale);
    assert(gpu_draw_box(target, 0, 0, 4, 4, 0x00334455, &plainmethod));
    assert(profile_test_read_pixel(target, 2, 2) == 0x00334455);
    engine_profile_gpu_poll();
    assert(stats->pending == 1 && stats->completed == 0);

    /* Finish the replacement scope only after proving the stale end left it
     * open. Its result must belong to the new frame rather than the retired one. */
    engine_profile_gpu_end(fresh);
    assert(gpu_draw_box(target, 0, 0, 4, 4, 0x00667788, &plainmethod));
    assert(profile_test_read_pixel(target, 2, 2) == 0x00667788);
    engine_profile_gpu_poll();
    assert(stats->pending == 0 && stats->completed == 1 && stats->errors == 0);
    report = profile_test_finish(frame, target);
    assert(report->gpu_samples[EP_GPU_CAPTURE] == 1);
    assert(report->gpu_samples[EP_GPU_SCENE] == 0);
    release_test_screen(target);
    gpu_gl_shutdown();
    return context;
}

/*
 * Damon V. Caskey
 * 2026-11-12
 *
 * Run real native counters and optional timestamp-query lifecycle checks.
 * Missing graphics infrastructure skips this executable; missing timer-query
 * capability skips only the timestamp-specific assertions.
 */
int main(void)
{
    SDL_Window *window;
    SDL_GLContext context;
    int timestamps_supported;

    engine_profile_set_hooks(profile_test_clock, profile_test_log);
    if(SDL_Init(SDL_INIT_VIDEO) != 0)
    {
        fprintf(stderr, "SKIP: SDL video unavailable: %s\n", SDL_GetError());
        return 77;
    }
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_COMPATIBILITY);
    window = SDL_CreateWindow("OpenBOR GPU profiler tests", 0, 0, 64, 64,
                              SDL_WINDOW_OPENGL | SDL_WINDOW_HIDDEN);
    context = window ? SDL_GL_CreateContext(window) : NULL;
    if(!context)
    {
        fprintf(stderr, "SKIP: GL context unavailable: %s\n", SDL_GetError());
        if(window)
        {
            SDL_DestroyWindow(window);
        }
        SDL_Quit();
        return 77;
    }
    assert(LoadGLFunctions());
    test_native_profile_disabled();
    test_native_profile_counters();
    timestamps_supported = test_native_profile_queries();
    if(timestamps_supported)
    {
        test_native_profile_error_state();
        context = test_native_profile_context(window, context);
    }
    assert(profile_test_log_lines > 0);
    engine_profile_shutdown();
    SDL_GL_DeleteContext(context);
    SDL_DestroyWindow(window);
    SDL_Quit();
    puts("GPU profiler tests passed: counters, byte totals, independent opt-out, nested queries, saturation, error ownership, context tokens.");
    return 0;
}
