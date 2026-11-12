/*
 * Native resident-palette regressions using a real SDL/OpenGL context.
 * Reuse image/queue fixtures without running the general renderer suite.
 */
#define main unused_palette_renderer_test_suite
#include "test_gpu_renderer.c"
#undef main

#define PALETTE_TEST_HASH_PRIME UINT32_C(16777619)
#define PALETTE_TEST_HASH_OFFSET UINT32_C(2166136261)
#define PALETTE_TEST_ALTERNATIONS 128

/*
 * Damon V. Caskey
 * 2026-11-12
 * Build distinct, reproducible palettes with visible colors at every tested
 * index. Index zero deliberately contains white to test coverage semantics.
 */
static void palette_test_fill(uint32_t palette[GPU_PALETTE_COLORS], uint32_t seed)
{
    unsigned index;

    for(index = 0; index < GPU_PALETTE_COLORS; ++index)
    {
        palette[index] = (seed * UINT32_C(65793) + index * UINT32_C(263171)) & UINT32_C(0x00ffffff);
    }
    palette[0] = UINT32_C(0x00ffffff);
    palette[1] = seed & UINT32_C(0x00ffffff);
}

/*
 * Damon V. Caskey
 * 2026-11-12
 * Reproduce the content hash solely to construct deliberate full-hash
 * collisions. Exact content comparisons, not hash uniqueness, must decide hits.
 */
static uint32_t palette_test_hash(const uint32_t palette[GPU_PALETTE_COLORS])
{
    uint32_t hash = PALETTE_TEST_HASH_OFFSET;
    unsigned index;

    for(index = 0; index < GPU_PALETTE_COLORS; ++index)
    {
        hash = (hash ^ palette[index]) * PALETTE_TEST_HASH_PRIME;
    }
    /* Mirror the production final avalanche. Equal full recurrence values
     * remain equal while upper-channel changes distribute across buckets. */
    hash ^= hash >> 16;
    hash *= UINT32_C(0x85ebca6b);
    hash ^= hash >> 13;
    hash *= UINT32_C(0xc2b2ae35);
    hash ^= hash >> 16;
    return hash;
}

/*
 * Damon V. Caskey
 * 2026-11-12
 * Construct different palette bytes with exactly the same 32-bit content
 * hash by compensating a penultimate-word edit in the final word.
 */
static void palette_test_collision(uint32_t output[GPU_PALETTE_COLORS],
                                   const uint32_t original[GPU_PALETTE_COLORS],
                                   uint32_t difference)
{
    uint32_t prefix = PALETTE_TEST_HASH_OFFSET;
    uint32_t original_step, changed_step;
    unsigned index;

    memcpy(output, original, GPU_PALETTE_BYTES);
    for(index = 0; index < GPU_PALETTE_COLORS - 2; ++index)
    {
        prefix = (prefix ^ original[index]) * PALETTE_TEST_HASH_PRIME;
    }
    output[GPU_PALETTE_COLORS - 2] ^= difference;
    original_step = (prefix ^ original[GPU_PALETTE_COLORS - 2]) * PALETTE_TEST_HASH_PRIME;
    changed_step = (prefix ^ output[GPU_PALETTE_COLORS - 2]) * PALETTE_TEST_HASH_PRIME;
    output[GPU_PALETTE_COLORS - 1] = original[GPU_PALETTE_COLORS - 1] ^ original_step ^ changed_step;
    assert(memcmp(output, original, GPU_PALETTE_BYTES) != 0);
    assert(palette_test_hash(output) == palette_test_hash(original));
}

/*
 * Damon V. Caskey
 * 2026-11-12
 * Select an explicit palette without modifying indexed source pixels. The
 * input may be unaligned because production palettes are byte-addressed.
 */
static void palette_test_draw(s_screen *target, s_screen *source,
                              const void *palette, int x, int y)
{
    s_drawmethod method = plainmethod;

    method.table = (unsigned char *)palette;
    method.config |= DRAWMETHOD_CONFIG_BACKGROUND_TRANSPARENCY;
    assert(gpu_draw_screen(target, source, x, y, &method));
}

/*
 * Damon V. Caskey
 * 2026-11-12
 * Prove repeated A/B/A switching uploads two resident palettes only, preserves
 * previously issued colors, and never reuploads unchanged indexed pixels.
 */
static void test_palette_alternation_and_mutation(void)
{
    const unsigned char indices[4] = {0, 1, 2, 3};
    uint32_t first[GPU_PALETTE_COLORS], second[GPU_PALETTE_COLORS];
    uint32_t same_contents[GPU_PALETTE_COLORS];
    unsigned char unaligned_storage[GPU_PALETTE_BYTES + 1];
    unsigned char *unaligned_palette = unaligned_storage;
    s_screen *source = test_screen(4, 1, PIXEL_x8);
    s_screen *target = test_screen(32, 18, PIXEL_32);
    const gpu_renderer_stats *stats;
    unsigned long image_uploads, palette_uploads;
    unsigned iteration, index;

    palette_test_fill(first, UINT32_C(0x112233));
    palette_test_fill(second, UINT32_C(0x336699));
    memcpy(source->palette, first, GPU_PALETTE_BYTES);
    memcpy(source->data, indices, sizeof(indices));
    assert(gpu_gl_init());
    stats = gpu_gl_get_stats();
    assert(gpu_clear_screen(target));

    /* Warm image residency once. Subsequent differences in total upload
     * counts must consist only of newly encountered palette contents. */
    palette_test_draw(target, source, first, 0, 0);
    assert(stats->palette_uploads == 1 && stats->palette_cache_misses == 1);
    image_uploads = stats->uploads - stats->palette_uploads;
    for(iteration = 0; iteration < PALETTE_TEST_ALTERNATIONS; ++iteration)
    {
        const uint32_t *palette = (iteration & 1) ? second : first;
        palette_test_draw(target, source, palette, (int)(iteration % 8) * 4,
                          (int)(iteration / 8));
    }
    assert(stats->palette_uploads == 2 && stats->palette_cache_misses == 2);
    assert(stats->palette_cache_hits == PALETTE_TEST_ALTERNATIONS - 1);
    assert(stats->palette_cache_evictions == 0);
    assert(stats->uploads - stats->palette_uploads == image_uploads);

    /* Read only after the complete alternating command stream. Earlier
     * draws must retain their selected row even after later palette changes. */
    gpu_sync_cpu(target);
    for(iteration = 0; iteration < PALETTE_TEST_ALTERNATIONS; ++iteration)
    {
        const uint32_t *palette = (iteration & 1) ? second : first;
        for(index = 0; index < 4; ++index)
        {
            unsigned expected = index ? palette[index] & UINT32_C(0x00ffffff) : 0;
            assert(test_pixel(target, (int)(iteration % 8) * 4 + (int)index,
                              (int)(iteration / 8)) == expected);
        }
    }

    /* Pointer identity must not create duplicates. Both a separate aligned
     * copy and byte-offset storage must hit the already resident contents. */
    memcpy(same_contents, first, GPU_PALETTE_BYTES);
    if((uintptr_t)unaligned_palette % sizeof(uint32_t) == 0)
    {
        ++unaligned_palette;
    }
    assert((uintptr_t)unaligned_palette % sizeof(uint32_t) != 0);
    memcpy(unaligned_palette, second, GPU_PALETTE_BYTES);
    palette_uploads = stats->palette_uploads;
    palette_test_draw(target, source, same_contents, 0, 16);
    palette_test_draw(target, source, unaligned_palette, 4, 16);
    assert(stats->palette_uploads == palette_uploads);

    /* Mutating the same palette pointer creates one new content version.
     * Restoring old bytes must select their original resident row again. */
    same_contents[1] = UINT32_C(0x00abcdef);
    palette_test_draw(target, source, same_contents, 8, 16);
    assert(stats->palette_uploads == palette_uploads + 1);
    memcpy(same_contents, first, GPU_PALETTE_BYTES);
    palette_test_draw(target, source, same_contents, 12, 16);
    assert(stats->palette_uploads == palette_uploads + 1);
    assert(stats->uploads - stats->palette_uploads == image_uploads);
    gpu_sync_cpu(target);
    assert(test_pixel(target, 1, 16) == first[1]);
    assert(test_pixel(target, 5, 16) == second[1]);
    assert(test_pixel(target, 9, 16) == UINT32_C(0x00abcdef));
    assert(test_pixel(target, 13, 16) == first[1]);
    release_test_screen(target);
    release_test_screen(source);
    gpu_gl_shutdown();
}

/*
 * Damon V. Caskey
 * 2026-11-12
 * Share palette residency across indexed sprites/screens and explicit
 * overrides while restoring atlas selection after other rendering owners.
 */
static void test_palette_sources_and_state(void)
{
    const unsigned char indices[4] = {1, 2, 3, 4};
    uint32_t first[GPU_PALETTE_COLORS], second[GPU_PALETTE_COLORS];
    s_sprite *sprite = test_sprite(indices);
    s_screen *source = test_screen(4, 1, PIXEL_x8);
    s_screen *rgb = test_screen(4, 1, PIXEL_32);
    s_screen *target = test_screen(4, 8, PIXEL_32);
    s_drawmethod method = plainmethod;
    const gpu_renderer_stats *stats;
    unsigned char *source_palette;
    unsigned index;

    palette_test_fill(first, UINT32_C(0x00123456));
    palette_test_fill(second, UINT32_C(0x00654321));
    memcpy(source->data, indices, sizeof(indices));
    memcpy(source->palette, first, GPU_PALETTE_BYTES);
    memcpy(sprite->palette, second, GPU_PALETTE_BYTES);
    for(index = 0; index < 4; ++index)
    {
        ((unsigned *)rgb->data)[index] = UINT32_C(0x002468ac);
    }
    assert(gpu_gl_init());
    stats = gpu_gl_get_stats();
    assert(gpu_clear_screen(target));

    /* Native source palettes and drawmethod.table use one content cache;
     * selecting an override must leave the original palette unchanged. */
    assert(gpu_draw_screen(target, source, 0, 0, &plainmethod));
    assert(gpu_draw_sprite(target, sprite, 0, 1, &plainmethod));
    method.table = (unsigned char *)first;
    assert(gpu_draw_sprite(target, sprite, 0, 2, &method));
    method.table = (unsigned char *)second;
    assert(gpu_draw_screen(target, source, 0, 3, &method));
    assert(stats->palette_uploads == 2 && stats->palette_cache_misses == 2);
    assert(stats->palette_cache_hits == 2);

    /* Non-indexed work and presenter handoff may replace GL bindings or
     * program state, but must not discard residency or retain the wrong row. */
    assert(gpu_draw_box(target, 0, 4, 4, 1, UINT32_C(0x0013579b), &plainmethod));
    assert(gpu_draw_screen(target, rgb, 0, 5, &plainmethod));
    gpu_gl_prepare_present();
    assert(gpu_draw_screen(target, source, 0, 6, &plainmethod));
    assert(gpu_draw_sprite(target, sprite, 0, 7, &plainmethod));
    assert(stats->palette_uploads == 2 && stats->palette_cache_hits == 4);
    gpu_sync_cpu(target);
    for(index = 0; index < 4; ++index)
    {
        assert(test_pixel(target, (int)index, 0) == first[indices[index]]);
        assert(test_pixel(target, (int)index, 1) == second[indices[index]]);
        assert(test_pixel(target, (int)index, 2) == first[indices[index]]);
        assert(test_pixel(target, (int)index, 3) == second[indices[index]]);
        assert(test_pixel(target, (int)index, 4) == UINT32_C(0x0013579b));
        assert(test_pixel(target, (int)index, 5) == UINT32_C(0x002468ac));
        assert(test_pixel(target, (int)index, 6) == first[indices[index]]);
        assert(test_pixel(target, (int)index, 7) == second[indices[index]]);
    }
    assert(memcmp(source->palette, first, GPU_PALETTE_BYTES) == 0);
    assert(memcmp(sprite->palette, second, GPU_PALETTE_BYTES) == 0);

    /* Missing palette data must decline before cache lookup; it must not
     * accidentally reuse whichever row was selected by the previous draw. */
    source_palette = source->palette;
    source->palette = NULL;
    assert(!gpu_draw_screen(target, source, 0, 0, &plainmethod));
    assert(stats->palette_cache_misses == 2 && stats->palette_cache_hits == 4);
    assert(stats->palette_uploads == 2);
    source->palette = source_palette;
    gpu_release_sprite(sprite);
    free(sprite);
    release_test_screen(target);
    release_test_screen(rgb);
    release_test_screen(source);
    gpu_gl_shutdown();
}

/*
 * Damon V. Caskey
 * 2026-11-12
 * Keep full-hash collisions distinct, exercise the atlas's final row, and
 * remove a cold collision-chain member without losing its hot neighbors.
 */
static void test_palette_collision_and_lru(void)
{
    uint32_t first[GPU_PALETTE_COLORS], second[GPU_PALETTE_COLORS];
    uint32_t third[GPU_PALETTE_COLORS], candidate[GPU_PALETTE_COLORS];
    const unsigned char indices[4] = {254, 255, 1, 0};
    s_screen *source = test_screen(4, 1, PIXEL_x8);
    s_screen *target = test_screen(16, 2, PIXEL_32);
    const gpu_renderer_stats *stats;
    unsigned index, color;
    unsigned long uploads;

    palette_test_fill(first, UINT32_C(0x00102030));
    palette_test_collision(second, first, UINT32_C(0x00000100));
    palette_test_collision(third, first, UINT32_C(0x00010000));
    memcpy(source->data, indices, sizeof(indices));
    assert(gpu_gl_init());
    stats = gpu_gl_get_stats();
    assert(gpu_clear_screen(target));
    palette_test_draw(target, source, first, 0, 0);
    palette_test_draw(target, source, second, 4, 0);
    palette_test_draw(target, source, third, 8, 0);
    assert(stats->palette_cache_misses == 3 && stats->palette_uploads == 3);
    gpu_sync_cpu(target);
    for(color = 0; color < 3; ++color)
    {
        assert(test_pixel(target, (int)color, 0) == (first[indices[color]] & UINT32_C(0x00ffffff)));
        assert(test_pixel(target, 4 + (int)color, 0) == (second[indices[color]] & UINT32_C(0x00ffffff)));
        assert(test_pixel(target, 8 + (int)color, 0) == (third[indices[color]] & UINT32_C(0x00ffffff)));
    }

    /* Fill every remaining slot, including the final atlas row. Fullness
     * alone must not evict an entry until another distinct palette arrives. */
    for(index = 3; index < GPU_PALETTE_CACHE_CAPACITY; ++index)
    {
        palette_test_fill(candidate, UINT32_C(0x00200000) + index);
        palette_test_draw(target, source, candidate, 12, 0);
    }
    assert(stats->palette_uploads == GPU_PALETTE_CACHE_CAPACITY);
    assert(stats->palette_cache_evictions == 0);
    gpu_sync_cpu(target);
    assert(test_pixel(target, 14, 0) == candidate[1]);

    /* Refresh the outer entries in the three-member collision chain. Its
     * middle member is now the least recently used palette in the cache. */
    palette_test_draw(target, source, first, 0, 1);
    palette_test_draw(target, source, third, 8, 1);
    palette_test_fill(candidate, UINT32_C(0x00300000));
    palette_test_draw(target, source, candidate, 12, 1);
    assert(stats->palette_cache_evictions == 1);
    assert(stats->palette_uploads == GPU_PALETTE_CACHE_CAPACITY + 1);
    uploads = stats->palette_uploads;
    palette_test_draw(target, source, first, 0, 1);
    palette_test_draw(target, source, third, 8, 1);
    assert(stats->palette_uploads == uploads);
    palette_test_draw(target, source, second, 4, 1);
    assert(stats->palette_uploads == uploads + 1);
    assert(stats->palette_cache_evictions == 2);
    gpu_sync_cpu(target);
    for(color = 0; color < 3; ++color)
    {
        assert(test_pixel(target, (int)color, 1) == (first[indices[color]] & UINT32_C(0x00ffffff)));
        assert(test_pixel(target, 4 + (int)color, 1) == (second[indices[color]] & UINT32_C(0x00ffffff)));
        assert(test_pixel(target, 8 + (int)color, 1) == (third[indices[color]] & UINT32_C(0x00ffffff)));
    }
    release_test_screen(target);
    release_test_screen(source);
    gpu_gl_shutdown();
}

/*
 * Damon V. Caskey
 * 2026-11-12
 * Replay indexed content into captures before and after palette mutation.
 * GPU-resident RGB captures must retain the colors selected during each draw.
 */
static void test_palette_capture_versions(void)
{
    uint32_t first[GPU_PALETTE_COLORS], second[GPU_PALETTE_COLORS];
    s_screen *source = test_screen(4, 1, PIXEL_x8);
    s_screen *capture = test_screen(4, 1, PIXEL_32);
    s_screen *output = test_screen(4, 3, PIXEL_32);
    const gpu_renderer_stats *stats;
    unsigned index;

    palette_test_fill(first, UINT32_C(0x00123456));
    palette_test_fill(second, UINT32_C(0x00654321));
    memset(source->data, 1, 4);
    memcpy(source->palette, first, GPU_PALETTE_BYTES);
    assert(gpu_gl_init());
    stats = gpu_gl_get_stats();
    assert(gpu_clear_screen(capture));
    assert(gpu_clear_screen(output));
    spriteq_unlock();
    spriteq_clear();
    spriteq_add_screen(0, 0, 1, source, NULL, 0);
    spriteq_draw(capture, 0, 1, 1, 0, 0);
    assert(gpu_draw_screen(output, capture, 0, 0, &plainmethod));

    /* The queue retains the source handle. Palette contents are resolved at
     * playback, while the prior RGB capture has already committed its colors. */
    memcpy(source->palette, second, GPU_PALETTE_BYTES);
    spriteq_draw(capture, 0, 1, 1, 0, 0);
    assert(gpu_draw_screen(output, capture, 0, 1, &plainmethod));
    memcpy(source->palette, first, GPU_PALETTE_BYTES);
    spriteq_draw(capture, 0, 1, 1, 0, 0);
    assert(gpu_draw_screen(output, capture, 0, 2, &plainmethod));
    assert(stats->palette_uploads == 2 && stats->palette_cache_hits == 1);
    assert(stats->readbacks == 0);
    gpu_sync_cpu(output);
    for(index = 0; index < 4; ++index)
    {
        assert(test_pixel(output, (int)index, 0) == first[1]);
        assert(test_pixel(output, (int)index, 1) == second[1]);
        assert(test_pixel(output, (int)index, 2) == first[1]);
    }
    spriteq_clear();
    release_test_screen(output);
    release_test_screen(capture);
    release_test_screen(source);
    gpu_gl_shutdown();
}

/*
 * Damon V. Caskey
 * 2026-11-12
 * Reject a failed palette upload without trusting stale residency metadata.
 * Repeated old contents must be uploaded again after failure invalidation.
 */
static void test_palette_upload_failure(void)
{
    uint32_t first[GPU_PALETTE_COLORS], second[GPU_PALETTE_COLORS];
    s_screen *source = test_screen(4, 1, PIXEL_x8);
    s_screen *target = test_screen(4, 1, PIXEL_32);
    s_drawmethod method = plainmethod;
    const gpu_renderer_stats *stats;
    unsigned long misses, hits, uploads;

    palette_test_fill(first, UINT32_C(0x00112233));
    palette_test_fill(second, UINT32_C(0x00445566));
    memset(source->data, 1, 4);
    assert(gpu_gl_init());
    stats = gpu_gl_get_stats();
    palette_test_draw(target, source, first, 0, 0);
    misses = stats->palette_cache_misses;
    hits = stats->palette_cache_hits;
    uploads = stats->palette_uploads;
    assert(glGetError() == GL_NO_ERROR);

    /* Both images are already resident, so this pending error is consumed
     * by the new palette row's upload check rather than image creation. */
    glEnable((GLenum)0xffffffffu);
    method.table = (unsigned char *)second;
    assert(!gpu_draw_screen(target, source, 0, 0, &method));
    assert(glGetError() == GL_NO_ERROR);
    assert(stats->palette_cache_misses == misses + 1);
    assert(stats->palette_uploads == uploads);
    assert(stats->fallback_draws == 1);

    /* Invalidation discards CPU cache claims even if the failed GL check
     * followed a physically successful upload. Rebuilding is then safe. */
    palette_test_draw(target, source, first, 0, 0);
    assert(stats->palette_cache_misses == misses + 2);
    assert(stats->palette_cache_hits == hits);
    assert(stats->palette_uploads == uploads + 1);
    gpu_sync_cpu(target);
    assert(test_pixel(target, 1, 0) == first[1]);
    palette_test_draw(target, source, second, 0, 0);
    gpu_sync_cpu(target);
    assert(test_pixel(target, 1, 0) == second[1]);
    release_test_screen(target);
    release_test_screen(source);
    gpu_gl_shutdown();
}

/*
 * Damon V. Caskey
 * 2026-11-12
 * Preserve source owners across context recreation while requiring a fresh
 * palette upload into the replacement atlas rather than a stale cache hit.
 */
static SDL_GLContext test_palette_context(SDL_Window *window, SDL_GLContext context)
{
    uint32_t palette[GPU_PALETTE_COLORS];
    s_screen *source = test_screen(4, 1, PIXEL_x8);
    s_screen *target = test_screen(4, 1, PIXEL_32);
    const gpu_renderer_stats *stats;

    palette_test_fill(palette, UINT32_C(0x001234ab));
    memset(source->data, 1, 4);
    assert(gpu_gl_init());
    palette_test_draw(target, source, palette, 0, 0);
    assert(gpu_gl_get_stats()->palette_uploads == 1);
    gpu_gl_shutdown();
    assert(test_pixel(target, 0, 0) == palette[1]);
    SDL_GL_DeleteContext(context);
    context = SDL_GL_CreateContext(window);
    assert(context && LoadGLFunctions());
    assert(gpu_gl_init());
    stats = gpu_gl_get_stats();
    assert(stats->palette_uploads == 0 && stats->palette_cache_hits == 0);
    assert(stats->palette_cache_misses == 0 && stats->palette_cache_evictions == 0);
    palette_test_draw(target, source, palette, 0, 0);
    assert(stats->palette_uploads == 1 && stats->palette_cache_misses == 1);
    assert(stats->palette_cache_hits == 0);
    gpu_sync_cpu(target);
    assert(test_pixel(target, 0, 0) == palette[1]);
    release_test_screen(target);
    release_test_screen(source);
    gpu_gl_shutdown();
    return context;
}

/*
 * Damon V. Caskey
 * 2026-11-12
 * Exercise palette residency, mutation, collisions, bounded eviction, failure
 * recovery and context lifetime against an actual hidden OpenGL renderer.
 */
int main(void)
{
    SDL_Window *window;
    SDL_GLContext context;

    assert(SDL_setenv("OPENBOR_PROFILE", "0", 1) == 0);
    if(SDL_Init(SDL_INIT_VIDEO) != 0)
    {
        fprintf(stderr, "SKIP: SDL video unavailable: %s\n", SDL_GetError());
        return 77;
    }
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_COMPATIBILITY);
    window = SDL_CreateWindow("OpenBOR resident palette tests", 0, 0, 64, 64,
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
    test_palette_alternation_and_mutation();
    test_palette_sources_and_state();
    test_palette_collision_and_lru();
    test_palette_capture_versions();
    test_palette_upload_failure();
    context = test_palette_context(window, context);
    SDL_GL_DeleteContext(context);
    SDL_DestroyWindow(window);
    SDL_Quit();
    puts("GPU palette cache tests passed: reuse, mutation, unaligned content, source overrides, state restoration, full-hash collisions, LRU eviction, captures, failure recovery, context recreation.");
    return 0;
}
