/*
 * Native GL-state cache regressions using the shared renderer fixtures.
 * Pixel assertions accompany diagnostic counts so skipped setup cannot hide
 * stale bindings, uniforms, framebuffer selection or ownership transitions.
 */
#define main unused_state_renderer_test_suite
#include "test_gpu_renderer.c"
#undef main
#include "engine_profile.h"

#define STATE_TEST_REPETITIONS 128

static uint64_t state_profile_time;

/*
 * Damon V. Caskey
 * 2026-11-12
 * Populate a CPU-authored RGB screen while honoring native shadow ownership.
 */
static void state_test_fill(s_screen *screen, unsigned color)
{
    int index;

    /* Synchronize before replacing pixels so later native sampling observes
     * the dirty CPU version instead of an older resident texture. */
    gpu_prepare_cpu_write(screen);
    for(index = 0; index < screen->width * screen->height; ++index)
    {
        ((unsigned *)screen->data)[index] = color;
    }
}

/*
 * Damon V. Caskey
 * 2026-11-12
 * Prove identical warm draws reuse tracked state without changing pixels or
 * creating new image/palette uploads on the repeated native path.
 */
static void test_state_repeated_draw(void)
{
    s_screen *source = test_screen(4, 4, PIXEL_x8);
    s_screen *target = test_screen(8, 8, PIXEL_32);
    const gpu_renderer_stats *stats = gpu_gl_get_stats();
    s_drawmethod method = plainmethod;
    uint64_t changes, reuses;
    unsigned long uploads, draws;
    int iteration, x, y;

    memset(source->data, 1, 16);
    ((unsigned *)source->palette)[1] = 0x00123456;
    method.clipx = method.clipy = 2;
    method.clipw = method.cliph = 4;
    assert(gpu_clear_screen(target));
    assert(gpu_draw_screen(target, source, 2, 2, &method));
    changes = stats->state_changes;
    reuses = stats->state_reuses;
    uploads = stats->uploads;
    draws = stats->draws;

    /* Hold geometry, texture, target and material constant. Every additional
     * operation remains a real draw, but no cached setter should reach GL. */
    for(iteration = 0; iteration < STATE_TEST_REPETITIONS; ++iteration)
    {
        assert(gpu_draw_screen(target, source, 2, 2, &method));
    }
    assert(stats->draws == draws + STATE_TEST_REPETITIONS);
    assert(stats->state_changes == changes);
    assert(stats->state_reuses > reuses);
    assert(stats->uploads == uploads);
    gpu_sync_cpu(target);
    for(y = 0; y < target->height; ++y)
    {
        for(x = 0; x < target->width; ++x)
        {
            unsigned expected = x >= 2 && x < 6 && y >= 2 && y < 6 ? 0x00123456 : 0;
            assert(test_pixel(target, x, y) == expected);
        }
    }
    release_test_screen(target);
    release_test_screen(source);
}

/*
 * Damon V. Caskey
 * 2026-11-12
 * Exercise native maintenance operations that change texture/FBO bindings
 * between otherwise ordinary draws, including resource-name retirement.
 */
static void test_state_resource_mutations(void)
{
    s_screen *first = test_screen(4, 4, PIXEL_32);
    s_screen *second = test_screen(2, 2, PIXEL_32);
    s_screen *indexed = test_screen(4, 1, PIXEL_x8);
    s_screen *scratch = test_screen(7, 5, PIXEL_32);
    s_screen *target = test_screen(16, 12, PIXEL_32);
    unsigned texture;
    int width, height, x, y;

    state_test_fill(first, 0x00112233);
    state_test_fill(second, 0x00445566);
    memset(indexed->data, 1, 4);
    ((unsigned *)indexed->palette)[1] = 0x00778899;
    assert(gpu_clear_screen(target));
    assert(gpu_draw_screen(target, first, 0, 0, &plainmethod));

    /* Reading a different target changes the read framebuffer. The next
     * target draw must restore its FBO even if its material is unchanged. */
    assert(gpu_draw_box(scratch, 0, 0, 7, 5, 0x00abcdef, &plainmethod));
    assert(gpu_draw_screen(target, first, 4, 0, &plainmethod));
    gpu_sync_cpu(scratch);
    assert(gpu_draw_screen(target, first, 4, 0, &plainmethod));
    assert(test_pixel(scratch, 6, 4) == 0x00abcdef);

    /* Dirty-source upload and a separate exposed-screen allocation both
     * edit unit zero. Cached source bindings must reflect those changes. */
    state_test_fill(first, 0x00010203);
    assert(gpu_draw_screen(target, first, 0, 4, &plainmethod));
    assert(gpu_gl_get_screen_texture(second, &texture, &width, &height));
    assert(texture && width == 2 && height == 2);
    assert(gpu_draw_screen(target, first, 4, 4, &plainmethod));

    /* Palette misses use a nonzero texture unit. Retiring an indexed image
     * while it was just sampled must also forget any cached texture name. */
    assert(gpu_draw_screen(target, indexed, 0, 8, &plainmethod));
    ((unsigned *)indexed->palette)[1] = 0x00998877;
    assert(gpu_draw_screen(target, indexed, 4, 8, &plainmethod));
    gpu_release_screen(indexed);
    ((unsigned *)indexed->palette)[1] = 0x0013579b;
    assert(gpu_draw_screen(target, indexed, 8, 8, &plainmethod));

    /* Clearing after a clipped draw must disable scissoring for the whole
     * replacement, then restore normal scene raster state on the next draw. */
    assert(gpu_clear_screen(scratch));
    assert(gpu_draw_box(scratch, 2, 1, 2, 2, 0x002468ac, &plainmethod));
    assert(gpu_draw_screen(target, second, 12, 0, &plainmethod));
    gpu_sync_cpu(scratch);
    for(y = 0; y < scratch->height; ++y)
    {
        for(x = 0; x < scratch->width; ++x)
        {
            unsigned expected = x >= 2 && x < 4 && y >= 1 && y < 3 ? 0x002468ac : 0;
            assert(test_pixel(scratch, x, y) == expected);
        }
    }
    gpu_sync_cpu(target);
    for(y = 0; y < target->height; ++y)
    {
        for(x = 0; x < target->width; ++x)
        {
            unsigned expected = 0;
            if(y < 4 && x < 8)
            {
                expected = 0x00112233;
            }
            if(y >= 4 && y < 8 && x < 8)
            {
                expected = 0x00010203;
            }
            if(y < 2 && x >= 12 && x < 14)
            {
                expected = 0x00445566;
            }
            if(y == 8 && x < 12)
            {
                expected = x < 4 ? 0x00778899 : x < 8 ? 0x00998877 : 0x0013579b;
            }
            assert(test_pixel(target, x, y) == expected);
        }
    }
    /* Retire the current framebuffer and reuse the same CPU owner as a new
     * image. Recycled GL names must not inherit a deleted object's validity. */
    gpu_release_screen(target);
    state_test_fill(target, 0x00818181);
    assert(gpu_draw_screen(target, second, 12, 0, &plainmethod));
    gpu_sync_cpu(target);
    assert(test_pixel(target, 12, 0) == 0x00445566);
    assert(test_pixel(target, 0, 0) == 0x00818181);
    release_test_screen(target);
    release_test_screen(scratch);
    release_test_screen(indexed);
    release_test_screen(second);
    release_test_screen(first);
}

/*
 * Damon V. Caskey
 * 2026-11-12
 * Optionally force complete setup before an operation to provide a control
 * rendering of the same material sequence without retained-state shortcuts.
 */
static void state_test_boundary(int invalidate)
{
    if(invalidate)
    {
        gpu_gl_invalidate_state();
    }
}

/*
 * Damon V. Caskey
 * 2026-11-12
 * Render a deterministic mixed-material command stream with optional cache
 * invalidation before each operation, retaining normal painter ordering.
 */
static void state_test_material_sequence(s_screen *target, int invalidate)
{
    const unsigned char indices[4] = {0, 1, 2, 3};
    const unsigned char opacities[4] = {255, 128, 0, 64};
    s_screen *indexed = test_screen(4, 4, PIXEL_x8);
    s_screen *rgb = test_screen(3, 2, PIXEL_32);
    s_screen *scratch = test_screen(9, 7, PIXEL_32);
    s_sprite *sprite = test_sprite(indices);
    s_sprite *mask = test_sprite(opacities);
    s_drawmethod method;
    unsigned alternate[GPU_PALETTE_COLORS] = {0};
    unsigned char *table = malloc(256 * 256);
    int index, mode;

    assert(table);
    for(index = 0; index < 16; ++index)
    {
        indexed->data[index] = (unsigned char)(index % 4);
    }
    for(index = 0; index < 4; ++index)
    {
        ((unsigned *)indexed->palette)[index] = 0x00123456u + (unsigned)index * 0x000b1721u;
        ((unsigned *)sprite->palette)[index] = 0x00e0a060u - (unsigned)index * 0x00010203u;
        alternate[index] = 0x00987654u + (unsigned)index * 0x00010203u;
    }
    for(index = 0; index < 6; ++index)
    {
        ((unsigned *)rgb->data)[index] = 0x00402010u + (unsigned)index * 0x000b0711u;
    }

    state_test_boundary(invalidate);
    assert(gpu_draw_box(target, 0, 0, target->width, target->height, 0x00214365, &plainmethod));

    /* Vary every shader material family while alternating repeated draws
     * with changes in dimensions, clipping, transparency and texture format. */
    for(mode = 0; mode < 6; ++mode)
    {
        method = plainmethod;
        method.config |= DRAWMETHOD_CONFIG_BACKGROUND_TRANSPARENCY;
        if(mode == 1)
        {
            method.fillcolor = 0x00bada55;
        }
        if(mode == 2)
        {
            method.tintmode = 2;
            method.tintcolor = 0x00557799;
        }
        if(mode == 3)
        {
            method.channelr = 71;
            method.channelg = 129;
            method.channelb = 211;
        }
        if(mode == 4)
        {
            method.alpha = 1;
            method.table = (unsigned char *)alternate;
        }
        if(mode == 5)
        {
            method.clipx = 26;
            method.clipy = 1;
            method.clipw = 2;
            method.cliph = 2;
        }
        state_test_boundary(invalidate);
        assert(gpu_draw_screen(target, indexed, 1 + mode * 5, 1, &method));
        state_test_boundary(invalidate);
        assert(gpu_draw_screen(target, indexed, 1 + mode * 5, 1, &method));
        state_test_boundary(invalidate);
        assert(gpu_draw_screen(target, rgb, 1 + mode * 5, 6, &plainmethod));
    }

    /* Switch between masked and ordinary sprite draws. Destination snapshots
     * and the mask sampler must not leak into the following unmasked draw. */
    sprite->mask = mask;
    method = plainmethod;
    method.alpha = 6;
    method.tintmode = 1;
    method.tintcolor = 0x00ffffff;
    state_test_boundary(invalidate);
    assert(gpu_draw_sprite(target, sprite, 1, 10, &method));
    sprite->mask = NULL;
    state_test_boundary(invalidate);
    assert(gpu_draw_sprite(target, sprite, 6, 10, &plainmethod));

    /* The table presence mask changes independently of material alpha mode.
     * Repeated updates retain the same atlas handle and table allocation. */
    memset(table, 77, 256 * 256);
    blendtables[0] = table;
    gpu_invalidate_tables();
    method = plainmethod;
    method.alpha = 1;
    state_test_boundary(invalidate);
    assert(gpu_draw_screen(target, rgb, 11, 10, &method));
    memset(table, 119, 256 * 256);
    gpu_invalidate_tables();
    state_test_boundary(invalidate);
    assert(gpu_draw_screen(target, rgb, 16, 10, &method));
    blendtables[0] = NULL;
    gpu_invalidate_tables();
    state_test_boundary(invalidate);
    assert(gpu_draw_screen(target, rgb, 21, 10, &method));

    /* Wrapped plane strips and ordinary affine draws select opposite wrap
     * values. Repeat the plane to exercise an unchanged initial wrap value. */
    method = plainmethod;
    method.water.watermode = WATER_MODE_SHEAR;
    method.water.beginsize = 0.75f;
    method.water.endsize = 1.25f;
    method.water.perspective = WATER_PERSPECTIVE_TILE;
    method.water.wavetime = 11;
    state_test_boundary(invalidate);
    assert(gpu_draw_screen(target, indexed, 1, 14, &method));
    state_test_boundary(invalidate);
    assert(gpu_draw_screen(target, indexed, 1, 14, &method));
    state_test_boundary(invalidate);
    assert(gpu_draw_screen(target, indexed, 6, 14, &plainmethod));
    state_test_boundary(invalidate);
    assert(gpu_draw_screen(target, indexed, 11, 14, &method));

    /* Capture-sized targets, CPU synchronization and self-drawing exercise
     * binding changes inside otherwise unchanged native material settings. */
    state_test_boundary(invalidate);
    assert(gpu_draw_box(scratch, 0, 0, 9, 7, 0x003579bd, &plainmethod));
    state_test_boundary(invalidate);
    assert(gpu_draw_screen(scratch, indexed, 2, 1, &plainmethod));
    state_test_boundary(invalidate);
    assert(gpu_draw_screen(target, scratch, 20, 14, &plainmethod));
    gpu_sync_cpu(scratch);
    state_test_boundary(invalidate);
    assert(gpu_draw_screen(target, rgb, 16, 14, &plainmethod));
    method = plainmethod;
    method.clipx = 1;
    method.clipy = 20;
    method.clipw = 30;
    method.cliph = 3;
    state_test_boundary(invalidate);
    assert(gpu_draw_screen(target, target, 1, 20, &method));

    release_test_screen(scratch);
    release_test_screen(rgb);
    release_test_screen(indexed);
    gpu_release_sprite(mask);
    gpu_release_sprite(sprite);
    free(mask);
    free(sprite);
    free(table);
}

/*
 * Damon V. Caskey
 * 2026-11-12
 * Compare a cached command stream against the same operations with mandatory
 * setup at every boundary, including all pixels and untouched margins.
 */
static void test_state_material_control(void)
{
    s_screen *cached = test_screen(32, 24, PIXEL_32);
    s_screen *control = test_screen(32, 24, PIXEL_32);
    int x, y;

    state_test_material_sequence(cached, 0);
    state_test_material_sequence(control, 1);
    gpu_sync_cpu(cached);
    gpu_sync_cpu(control);
    for(y = 0; y < cached->height; ++y)
    {
        for(x = 0; x < cached->width; ++x)
        {
            assert(test_pixel(cached, x, y) == test_pixel(control, x, y));
        }
    }
    /* These independent points ensure neither command stream can silently
     * turn into a blank or uniformly filled control image. */
    assert(test_pixel(cached, 31, 23) == 0x00214365);
    assert(test_pixel(cached, 2, 1) != 0x00214365);
    assert(test_pixel(cached, 11, 10) == 0x004d4d4d);
    assert(test_pixel(cached, 16, 10) == 0x00777777);
    release_test_screen(control);
    release_test_screen(cached);
}

/*
 * Damon V. Caskey
 * 2026-11-12
 * Poison shared GL state as a separate renderer would, without modifying
 * scene texture object parameters or the private scene shader's uniforms.
 */
static void state_test_external_gl(void)
{
    PFNGLBINDFRAMEBUFFERPROC bind_framebuffer = (PFNGLBINDFRAMEBUFFERPROC)SDL_GL_GetProcAddress("glBindFramebuffer");
    PFNGLACTIVETEXTUREPROC active_texture = (PFNGLACTIVETEXTUREPROC)SDL_GL_GetProcAddress("glActiveTexture");
    PFNGLUSEPROGRAMPROC use_program = (PFNGLUSEPROGRAMPROC)SDL_GL_GetProcAddress("glUseProgram");
    int unit;

    if(!bind_framebuffer)
    {
        bind_framebuffer = (PFNGLBINDFRAMEBUFFERPROC)SDL_GL_GetProcAddress("glBindFramebufferEXT");
    }
    assert(bind_framebuffer && active_texture && use_program);
    bind_framebuffer(GL_FRAMEBUFFER, 0);
    use_program(0);
    glViewport(3, 4, 1, 1);
    glScissor(0, 0, 0, 0);
    glEnable(GL_SCISSOR_TEST);
    glEnable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glEnable(GL_ALPHA_TEST);
    glEnable(GL_STENCIL_TEST);
    glEnable(GL_DITHER);
    glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
    for(unit = 0; unit < 5; ++unit)
    {
        active_texture(GL_TEXTURE0 + unit);
        glBindTexture(GL_TEXTURE_2D, 0);
    }
    assert(glGetError() == GL_NO_ERROR);
}

/*
 * Damon V. Caskey
 * 2026-11-12
 * Restore scene state after presenter handoff and explicit cache invalidation,
 * while proving invalidation itself performs no hidden GL state changes.
 */
static void test_state_external_ownership(void)
{
    s_screen *source = test_screen(4, 4, PIXEL_32);
    s_screen *target = test_screen(8, 8, PIXEL_32);
    const gpu_renderer_stats *stats = gpu_gl_get_stats();
    GLint viewport[4], current_program, active_unit;
    uint64_t changes;

    state_test_fill(source, 0x00bada55);
    assert(gpu_clear_screen(target));
    assert(gpu_draw_screen(target, source, 0, 0, &plainmethod));

    /* The standard presenter boundary must invalidate its caller's cached
     * scene assumptions before arbitrary external rendering begins. */
    gpu_gl_prepare_present();
    state_test_external_gl();
    changes = stats->state_changes;
    assert(gpu_draw_screen(target, source, 4, 0, &plainmethod));
    assert(stats->state_changes > changes);

    /* Public external helpers can invalidate without resetting GL. Checking
     * viewport/program/unit proves invalidation is metadata-only work. */
    gpu_gl_invalidate_state();
    state_test_external_gl();
    gpu_gl_invalidate_state();
    glGetIntegerv(GL_VIEWPORT, viewport);
    glGetIntegerv(GL_CURRENT_PROGRAM, &current_program);
    glGetIntegerv(GL_ACTIVE_TEXTURE, &active_unit);
    assert(viewport[0] == 3 && viewport[1] == 4 && viewport[2] == 1 && viewport[3] == 1);
    assert(current_program == 0 && active_unit == GL_TEXTURE4);
    assert(gpu_draw_screen(target, source, 0, 4, &plainmethod));
    assert(gpu_draw_screen(target, source, 4, 4, &plainmethod));
    gpu_sync_cpu(target);
    for(current_program = 0; current_program < 64; ++current_program)
    {
        assert((((unsigned *)target->data)[current_program] & 0x00ffffffu) == 0x00bada55);
    }

    /* Clear has a smaller state contract than a draw. It must restore write
     * masks and disable a foreign scissor without declaring all raster state
     * valid for the scene draw that follows it. */
    gpu_gl_prepare_present();
    state_test_external_gl();
    assert(gpu_clear_screen(target));
    gpu_sync_cpu(target);
    for(current_program = 0; current_program < 64; ++current_program)
    {
        assert((((unsigned *)target->data)[current_program] & 0x00ffffffu) == 0);
    }
    assert(gpu_draw_screen(target, source, 2, 2, &plainmethod));
    gpu_sync_cpu(target);
    assert(test_pixel(target, 3, 3) == 0x00bada55);
    assert(test_pixel(target, 0, 0) == 0);
    release_test_screen(target);
    release_test_screen(source);
}

/*
 * Damon V. Caskey
 * 2026-11-12
 * Recreate the real GL context while CPU asset owners survive, requiring
 * fresh bindings and uniforms even when new GL objects reuse old names.
 */
static void test_state_context_recreation(SDL_Window *window, SDL_GLContext *context)
{
    s_screen *source = test_screen(4, 4, PIXEL_32);
    s_screen *target = test_screen(8, 8, PIXEL_32);
    uint64_t changes;

    state_test_fill(source, 0x00765432);
    assert(gpu_clear_screen(target));
    assert(gpu_draw_screen(target, source, 2, 2, &plainmethod));
    gpu_gl_shutdown();
    assert(test_pixel(target, 2, 2) == 0x00765432);
    SDL_GL_DeleteContext(*context);
    *context = SDL_GL_CreateContext(window);
    assert(*context);
    assert(LoadGLFunctions());
    assert(gpu_gl_init());

    /* Retained source pixels and preserved targets must be uploaded into the
     * replacement context, then become eligible for unchanged-state reuse. */
    assert(gpu_clear_screen(target));
    assert(gpu_draw_screen(target, source, 2, 2, &plainmethod));
    changes = gpu_gl_get_stats()->state_changes;
    assert(gpu_draw_screen(target, source, 2, 2, &plainmethod));
    assert(gpu_gl_get_stats()->state_changes == changes);
    gpu_sync_cpu(target);
    assert(test_pixel(target, 2, 2) == 0x00765432);
    assert(test_pixel(target, 0, 0) == 0);
    release_test_screen(target);
    release_test_screen(source);
}

/*
 * Damon V. Caskey
 * 2026-11-12
 * Supply a deterministic profiler clock while renderer operations execute
 * against the real driver, isolating accounting from host timing noise.
 */
static uint64_t state_test_profile_clock(void)
{
    return state_profile_time;
}

/*
 * Damon V. Caskey
 * 2026-11-12
 * Validate the normal profiler log envelope without writing measurement
 * windows to the isolated native fixture's output stream.
 */
static void state_test_profile_log(const char *line)
{
    assert(line && strncmp(line, "[ENGINE PROFILE]", 16) == 0);
}

/*
 * Damon V. Caskey
 * 2026-11-12
 * Verify real native cached setters reach per-frame counters exactly once,
 * including unchanged draws and the separate draw/clear submission paths.
 */
static void test_state_native_profile_counters(void)
{
    s_screen *source = test_screen(4, 4, PIXEL_32);
    s_screen *target = test_screen(8, 8, PIXEL_32);
    const gpu_renderer_stats *stats = gpu_gl_get_stats();
    const ep_report *report;
    uint64_t frame, changes, reuses, emitted_changes, emitted_reuses;
    int scene, pass, iteration;

    state_test_fill(source, 0x00345678);
    assert(gpu_clear_screen(target));
    assert(gpu_draw_screen(target, source, 0, 0, &plainmethod));
    engine_profile_set_hooks(state_test_profile_clock, state_test_profile_log);
    for(pass = 0; pass < 2; ++pass)
    {
        engine_profile_configure(1, 1, 0);
        state_profile_time = 1000;
        frame = engine_profile_frame_begin(1, &scene, target);
        assert(frame);
        changes = stats->state_changes;
        reuses = stats->state_reuses;
        if(pass == 0)
        {
            /* No resource allocation or material change occurs in this
             * accepted frame. Only reuse requests should reach its counters. */
            for(iteration = 0; iteration < STATE_TEST_REPETITIONS; ++iteration)
            {
                assert(gpu_draw_screen(target, source, 0, 0, &plainmethod));
            }
        }
        else
        {
            /* Geometry changes the scissor, clearing disables it, and the
             * following draw re-enables it without double-counted deltas. */
            assert(gpu_draw_screen(target, source, 1, 1, &plainmethod));
            assert(gpu_clear_screen(target));
            assert(gpu_draw_screen(target, source, 0, 0, &plainmethod));
        }
        emitted_changes = stats->state_changes - changes;
        emitted_reuses = stats->state_reuses - reuses;
        state_profile_time = 2000;
        engine_profile_frame_end(frame);
        state_profile_time = 11000;
        assert(engine_profile_frame_begin(1, &scene, target) == 0);
        report = engine_profile_last_report();
        assert(report->frames == 1);
        assert(report->counts[EP_COUNT_GPU_STATE_CHANGES] == emitted_changes);
        assert(report->counts[EP_COUNT_GPU_STATE_REUSES] == emitted_reuses);
        assert(report->count_max[EP_COUNT_GPU_STATE_CHANGES] == emitted_changes);
        assert(report->count_max[EP_COUNT_GPU_STATE_REUSES] == emitted_reuses);
        assert(emitted_reuses > 0);
        assert(report->counts[EP_COUNT_NATIVE_DRAWS] == (pass ? 2 : STATE_TEST_REPETITIONS));
        assert(report->counts[EP_COUNT_NATIVE_CLEARS] == (unsigned)pass);
        if(pass == 0)
        {
            assert(emitted_changes == 0);
        }
        else
        {
            assert(emitted_changes > 0);
        }
    }

    /* Keep subsequent context tests independent of this explicit collector,
     * and verify final pixels only after the accounting windows have closed. */
    engine_profile_configure(0, 1, 0);
    engine_profile_set_hooks(NULL, NULL);
    gpu_sync_cpu(target);
    assert(test_pixel(target, 1, 1) == 0x00345678);
    assert(test_pixel(target, 4, 4) == 0);
    release_test_screen(target);
    release_test_screen(source);
}

/*
 * Damon V. Caskey
 * 2026-11-12
 * Run cached-state behavior on a real desktop compatibility context, keeping
 * unavailable video infrastructure distinct from assertion failures.
 */
int main(void)
{
    SDL_Window *window;
    SDL_GLContext context;

    if(SDL_Init(SDL_INIT_VIDEO) != 0)
    {
        fprintf(stderr, "SKIP: SDL video unavailable: %s\n", SDL_GetError());
        return 77;
    }
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_COMPATIBILITY);
    window = SDL_CreateWindow("OpenBOR state-cache tests", 0, 0, 64, 64,
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
    assert(gpu_gl_init());
    test_state_repeated_draw();
    test_state_resource_mutations();
    test_state_material_control();
    test_state_external_ownership();
    test_state_native_profile_counters();
    test_state_context_recreation(window, &context);

    /* Complete teardown while the owning context is still current, matching
     * the production lifecycle and catching stale state during preservation. */
    gpu_gl_shutdown();
    SDL_GL_DeleteContext(context);
    SDL_DestroyWindow(window);
    SDL_Quit();
    puts("GPU state-cache tests passed: unchanged draws, maintenance, materials, ownership, native counters, context recreation.");
    return 0;
}
