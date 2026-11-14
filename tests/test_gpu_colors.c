/*
 * Damon V. Caskey
 * 2026-11-14
 * Check native color effects against independent complete-image expectations,
 * then verify actual execution counters, state reuse and resource transfers.
 */
#include <SDL.h>
#include "gpu_renderer.h"
#include "loadgl.h"
#include "engine_profile.h"
#define COLOR_GPU_TEST 1
#include "test_drawmethod_colors.c"

static uint64_t color_profile_time;

/*
 * Damon V. Caskey
 * 2026-11-14
 * Give the execution counters a deterministic completed collection window.
 */
static uint64_t colors_profile_clock(void)
{
    return color_profile_time;
}

/*
 * Damon V. Caskey
 * 2026-11-14
 * Counter assertions consume the report directly without printing its table.
 */
static void colors_profile_log(const char *line)
{
    assert(line);
}

/*
 * Damon V. Caskey
 * 2026-11-14
 * Repeating source-only policies must reuse uniforms without extra snapshots.
 */
static void colors_test_native_cache(void)
{
    s_screen *source = colors_screen(4, 1, PIXEL_32, 0);
    s_screen *target = colors_screen(12, 3, PIXEL_32, COLOR_BACKGROUND);
    s_drawmethod method = plainmethod;
    const gpu_renderer_stats *stats = gpu_gl_get_stats();
    unsigned expected[COLOR_EXPECTED_PIXELS];
    unsigned long copies, readbacks, uploads, draws;
    uint64_t changes, reuses;
    int repetition, x;
    colors_expected(expected, target, COLOR_BACKGROUND);
    assert(drawmethod_set_substitute(&method, 0, 0, 0x010101));
    copies = stats->destination_copies;
    readbacks = stats->readbacks;
    assert(gpu_draw_screen(target, source, 0, 1, &method));
    changes = stats->state_changes; reuses = stats->state_reuses;
    uploads = stats->uploads; draws = stats->draws;
    for(repetition = 0; repetition < 32; ++repetition)
    {
        assert(gpu_draw_screen(target, source, 0, 1, &method));
    }
    assert(stats->draws == draws + 32);
    assert(stats->state_changes == changes && stats->state_reuses > reuses);
    assert(stats->uploads == uploads);
    assert(stats->destination_copies == copies && stats->readbacks == readbacks);

    /* Changing one slot and then disabling it cannot inherit old uniforms. */
    assert(drawmethod_set_substitute(&method, 0, 0, 0x020202));
    assert(gpu_draw_screen(target, source, 4, 1, &method));
    assert(drawmethod_set_substitute_enabled(&method, 0));
    assert(gpu_draw_screen(target, source, 8, 1, &method));
    for(x = 0; x < 4; ++x) expected[12 + x] = 0x010101;
    for(x = 4; x < 8; ++x) expected[12 + x] = 0x020202;
    for(x = 8; x < 12; ++x) expected[12 + x] = 0;
    assert(stats->destination_copies == copies && stats->readbacks == readbacks);
    colors_verify(target, expected, "native color state reuse and disabling");
    colors_release_screen(target);
    colors_release_screen(source);
}

/*
 * Damon V. Caskey
 * 2026-11-14
 * Separate unavailable graphics infrastructure from a failed native contract.
 */
int main(void)
{
    SDL_Window *window;
    SDL_GLContext context;
    const ep_report *report;
    uint64_t frame;
    assert(sizeof(void *) == 8);
    pixelformat = PIXEL_32;
    if(SDL_Init(SDL_INIT_VIDEO) != 0) return 77;
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_COMPATIBILITY);
    window = SDL_CreateWindow("OpenBOR color tests", 0, 0, 64, 64,
                              SDL_WINDOW_OPENGL | SDL_WINDOW_HIDDEN);
    context = window ? SDL_GL_CreateContext(window) : NULL;
    if(!context)
    {
        if(window) SDL_DestroyWindow(window);
        SDL_Quit();
        return 77;
    }
    assert(LoadGLFunctions());
    assert(gpu_gl_init());
    engine_profile_set_hooks(colors_profile_clock, colors_profile_log);
    engine_profile_configure(1, 1, 0);
    color_profile_time = 1000;
    frame = engine_profile_frame_begin(1, &window, NULL);
    assert(frame);
    colors_run_suite();
    colors_test_native_cache();
    color_profile_time = 2000;
    engine_profile_frame_end(frame);
    color_profile_time = 11000;
    assert(engine_profile_frame_begin(1, &window, NULL) == 0);
    report = engine_profile_last_report();
    assert(report->frames == 1);
    assert(report->counts[EP_COUNT_NATIVE_CHROMA_DRAWS] > 0);
    assert(report->counts[EP_COUNT_NATIVE_SUBSTITUTE_DRAWS] > 0);
    assert(report->counts[EP_COUNT_SOFTWARE_DRAWS] == 0);
    assert(gpu_gl_get_stats()->draws > 0);
    assert(gpu_gl_get_stats()->fallback_draws == 0);
    gpu_gl_shutdown();
    SDL_GL_DeleteContext(context);
    SDL_DestroyWindow(window);
    SDL_Quit();
    printf("GPU color tests passed: %u native complete images; active color counters, no software draws or fallback.\n",
           color_images_checked);
    return 0;
}
