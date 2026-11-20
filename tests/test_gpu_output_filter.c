/*
 * Damon V. Caskey
 * 2026-11-16
 * Compare complete GPU output-filter images with independent RGB8 arithmetic.
 * Expected pixels never use a production shader or retired packed CPU filter.
 * The shared native fixture supplies real screen ownership and GL setup only.
 */
#define main unused_output_filter_backend_suite
#include "test_gpu_renderer.c"
#undef main
#include "output_filter_reference.h"

static unsigned output_filter_images_checked;
static unsigned output_filter_pixels_checked;
static int output_filter_reject_framebuffer;
static unsigned output_filter_rejected_framebuffers;
static PFNGLCHECKFRAMEBUFFERSTATUSPROC output_filter_real_framebuffer_status;
void *__real_SDL_GL_GetProcAddress(const char *name);

/*
 * Damon V. Caskey
 * 2026-11-16
 * Fail one real allocation boundary without adding production test hooks.
 */
static GLenum APIENTRY output_filter_framebuffer_status(GLenum target)
{
    if(output_filter_reject_framebuffer)
    {
        ++output_filter_rejected_framebuffers;
        return GL_FRAMEBUFFER_UNSUPPORTED;
    }
    return output_filter_real_framebuffer_status(target);
}

/*
 * Damon V. Caskey
 * 2026-11-16
 * Retain every native GL operation except the explicitly armed failure.
 */
void *__wrap_SDL_GL_GetProcAddress(const char *name)
{
    void *address = __real_SDL_GL_GetProcAddress(name);
    if(!strcmp(name, "glCheckFramebufferStatus") ||
       !strcmp(name, "glCheckFramebufferStatusEXT"))
    {
        output_filter_real_framebuffer_status = (PFNGLCHECKFRAMEBUFFERSTATUSPROC)address;
        return address ? (void *)output_filter_framebuffer_status : NULL;
    }
    return address;
}

/*
 * Damon V. Caskey
 * 2026-11-16
 * Inspect a borrowed result only at the test's explicit readback boundary.
 * Texture row zero is logical row zero, so no presentation flip is applied.
 */
static void output_filter_read_texture(unsigned texture, int width, int height,
                                       unsigned *pixels)
{
    PFNGLGENFRAMEBUFFERSPROC generate = SDL_GL_GetProcAddress("glGenFramebuffers");
    PFNGLBINDFRAMEBUFFERPROC bind = SDL_GL_GetProcAddress("glBindFramebuffer");
    PFNGLFRAMEBUFFERTEXTURE2DPROC attach = SDL_GL_GetProcAddress("glFramebufferTexture2D");
    PFNGLCHECKFRAMEBUFFERSTATUSPROC status = SDL_GL_GetProcAddress("glCheckFramebufferStatus");
    PFNGLDELETEFRAMEBUFFERSPROC destroy = SDL_GL_GetProcAddress("glDeleteFramebuffers");
    GLuint framebuffer;
    assert(generate && bind && attach && status && destroy);
    gpu_gl_invalidate_state();
    generate(1, &framebuffer);
    assert(framebuffer);
    bind(GL_FRAMEBUFFER, framebuffer);
    attach(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture, 0);
    assert(status(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE);
    glReadPixels(0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
    bind(GL_FRAMEBUFFER, 0);
    destroy(1, &framebuffer);
    gpu_gl_invalidate_state();
}

/*
 * Damon V. Caskey
 * 2026-11-16
 * Check complete images with exact byte expectations and unchanged source
 * transfer counters. GPU-authored input needs neither upload nor readback.
 */
static unsigned output_filter_compare(s_screen *screen, const unsigned *expected,
                                      int filter, const char *label)
{
    gpu_renderer_stats before = *gpu_gl_get_stats();
    unsigned texture, source_texture, *pixels;
    int width, height, source_width, source_height, x, y;
    assert(gpu_gl_get_filtered_screen_texture(screen, filter, &texture, &width, &height));
    assert(width == screen->width * (filter ? 2 : 1));
    assert(height == screen->height * (filter ? 2 : 1));
    assert(gpu_gl_get_stats()->readbacks == before.readbacks);
    assert(gpu_gl_get_stats()->uploads == before.uploads);
    assert(gpu_gl_get_stats()->fallback_draws == before.fallback_draws);
    assert(gpu_gl_get_stats()->output_filter_draws == before.output_filter_draws + !!filter);
    assert(gpu_gl_get_screen_texture(screen, &source_texture, &source_width, &source_height));
    assert(source_width == screen->width && source_height == screen->height);
    assert(filter ? texture != source_texture : texture == source_texture);
    if(!filter)
    {
        assert(gpu_gl_get_stats()->output_filter_allocations == before.output_filter_allocations);
        assert(gpu_gl_get_stats()->output_filter_resources == before.output_filter_resources);
    }
    pixels = malloc((size_t)width * height * sizeof(*pixels));
    assert(pixels);
    output_filter_read_texture(texture, width, height, pixels);
    for(y = 0; y < height; ++y)
        for(x = 0; x < width; ++x)
        {
            unsigned wanted = output_filter_expected(expected, screen->width,
                                                      screen->height, filter, x, y);
            unsigned actual = pixels[y * width + x] & 0x00ffffffu;
            if(actual != wanted)
            {
                fprintf(stderr, "%s filter %d at (%d,%d), source %dx%d: got %06x, expected %06x\n",
                        label, filter, x, y, screen->width, screen->height, actual, wanted);
                assert(actual == wanted);
            }
            if(filter) assert((pixels[y * width + x] >> 24) == 255);
        }
    free(pixels);
    ++output_filter_images_checked;
    output_filter_pixels_checked += (unsigned)(width * height);
    return texture;
}

/*
 * Damon V. Caskey
 * 2026-11-16
 * Render independently owned expected data into a scene whose CPU shadow stays
 * zero. Odd channels expose any accidental 565 conversion or rounded average.
 */
static void output_filter_draw_pattern(s_screen *screen, unsigned *expected, int pattern)
{
    int x, y;
    for(y = 0; y < screen->height; ++y)
        for(x = 0; x < screen->width; ++x)
        {
            unsigned color;
            switch(pattern)
            {
            case 0: color = 0x00abcdefu; break;
            case 1:
                color = ((unsigned)(x + 31 * y) & 255) |
                        (((unsigned)(255 - x + 17 * y) & 255) << 8) |
                        (((unsigned)(3 * x + 41 * y) & 255) << 16);
                break;
            case 2:
                color = ((x + y) & 1) ? 0x000103ffu : 0x00fe8100u;
                break;
            default:
                color = x == y || x + y == screen->width - 1 ? 0x00e3a107u :
                        (x % 3 == 1 && y % 3 == 1 ? 0x00112f5du : 0);
                break;
            }
            expected[y * screen->width + x] = color;
            assert(gpu_draw_box(screen, x, y, 1, 1, color, &plainmethod));
        }
}

/*
 * Damon V. Caskey
 * 2026-11-16
 * Cover every subpixel phase, tiny edges, non-multiple widths, all RGB8 values
 * and Scale2x equality branches without reading scene data back to the CPU.
 */
static void output_filter_test_images(void)
{
    static const int dimensions[][2] = {{1, 1}, {1, 5}, {5, 1}, {3, 5}, {7, 9}, {16, 16}, {256, 1}};
    size_t size;
    int pattern, filter, index;
    for(size = 0; size < sizeof(dimensions) / sizeof(dimensions[0]); ++size)
    {
        s_screen *screen = test_screen(dimensions[size][0], dimensions[size][1], PIXEL_32);
        unsigned *expected = malloc((size_t)screen->width * screen->height * sizeof(*expected));
        assert(expected);
        for(pattern = 0; pattern < 4; ++pattern)
        {
            output_filter_draw_pattern(screen, expected, pattern);
            for(filter = 0; filter < BLITTER_MAX; ++filter)
                output_filter_compare(screen, expected, filter, "complete RGB8 image");
        }
        for(index = 0; index < screen->width * screen->height; ++index)
            assert(((unsigned *)screen->data)[index] == 0);
        gpu_sync_cpu(screen);
        for(index = 0; index < screen->width * screen->height; ++index)
            assert(test_pixel(screen, index % screen->width, index / screen->width) == expected[index]);
        free(expected);
        release_test_screen(screen);
    }
}

/*
 * Damon V. Caskey
 * 2026-11-16
 * Distinguish the actual lower neighbor from the historical Scale2x row bug,
 * including branches whose neighboring colors differ only in low RGB bits.
 */
static void output_filter_test_scale2x_neighbors(void)
{
    s_screen *screen = test_screen(3, 3, PIXEL_32);
    const unsigned colors[] = {0x00112233u, 0x00112234u, 0x00112333u};
    unsigned expected[9];
    int above, below, left, right, index;
    for(above = 0; above < 3; ++above)
        for(below = 0; below < 3; ++below)
            for(left = 0; left < 3; ++left)
                for(right = 0; right < 3; ++right)
                {
                    for(index = 0; index < 9; ++index) expected[index] = 0x007f0183u;
                    expected[1] = colors[above]; expected[7] = colors[below];
                    expected[3] = colors[left]; expected[5] = colors[right];
                    for(index = 0; index < 9; ++index)
                        assert(gpu_draw_box(screen, index % 3, index / 3, 1, 1,
                                            expected[index], &plainmethod));
                    output_filter_compare(screen, expected, BLITTER_ADMAME2X, "Scale2x lower neighbor");
                }
    release_test_screen(screen);
}

/*
 * Damon V. Caskey
 * 2026-11-16
 * Reuse one resident target across modes and redraws while retaining correct
 * pixels after explicit CPU edits and repeated GPU-only source mutations.
 */
static void output_filter_test_state_and_mutation(void)
{
    static const int filters[] = {1, 6, 2, 4, 3, 5, 0, 6, 6};
    s_screen *screen = test_screen(7, 5, PIXEL_32);
    unsigned expected[35], texture = 0, previous = 0;
    gpu_renderer_stats before;
    size_t mode;
    int width, height;
    output_filter_draw_pattern(screen, expected, 1);
    for(mode = 0; mode < sizeof(filters) / sizeof(filters[0]); ++mode)
    {
        before = *gpu_gl_get_stats();
        texture = output_filter_compare(screen, expected, filters[mode], "filter switch");
        if(mode)
        {
            assert(gpu_gl_get_stats()->output_filter_allocations == before.output_filter_allocations);
            if(filters[mode]) assert(texture == previous);
        }
        if(filters[mode]) previous = texture;
        expected[17] = 0x00010203u * (unsigned)(mode + 1);
        assert(gpu_draw_box(screen, 3, 2, 1, 1, expected[17], &plainmethod));
        assert(test_pixel(screen, 3, 2) == 0);
    }
    output_filter_compare(screen, expected, BLITTER_BILINEAR, "GPU mutation invalidates result");
    gpu_prepare_cpu_write(screen);
    ((unsigned *)screen->data)[9] = expected[9] = 0x00fefdffu;
    gpu_mark_cpu_dirty(screen);
    before = *gpu_gl_get_stats();
    assert(gpu_gl_get_screen_texture(screen, &texture, &width, &height));
    assert(gpu_gl_get_stats()->uploads == before.uploads + 1);
    assert(gpu_gl_get_stats()->readbacks == before.readbacks);
    output_filter_compare(screen, expected, BLITTER_SCANLINESTV, "explicit CPU edit");
    assert(gpu_draw_box(screen, 0, 0, 1, 1, 0x00090705u, &plainmethod));
    expected[0] = 0x00090705u;
    output_filter_compare(screen, expected, BLITTER_DOTMATRIX, "native state after CPU edit");
    release_test_screen(screen);
}

/*
 * Damon V. Caskey
 * 2026-11-16
 * Reject invalid requests before uploads or integer overflow, and preserve
 * caller outputs on failure. Runtime FBO failure cannot revive CPU filtering.
 */
static void output_filter_test_rejections(void)
{
    const int filters[] = {-1, BLITTER_MAX, INT_MAX};
    s_screen *screen = test_screen(11, 7, PIXEL_32);
    s_screen *oversized;
    unsigned expected[77], texture = 123456;
    int width = -11, height = -7, maximum, index, source_width, source_height;
    gpu_renderer_stats before;
    glGetIntegerv(GL_MAX_TEXTURE_SIZE, &maximum);
    assert(maximum > 0 && maximum < INT_MAX);
    output_filter_draw_pattern(screen, expected, 1);
    before = *gpu_gl_get_stats();
    for(index = 0; index < (int)(sizeof(filters) / sizeof(filters[0])); ++index)
    {
        assert(!gpu_gl_get_filtered_screen_texture(screen, filters[index], &texture, &width, &height));
        assert(texture == 123456 && width == -11 && height == -7);
    }
    assert(!gpu_gl_get_filtered_screen_texture(NULL, 1, &texture, &width, &height));
    oversized = test_screen(3, 3, PIXEL_8);
    assert(!gpu_gl_get_filtered_screen_texture(oversized, 1, &texture, &width, &height));
    assert(texture == 123456 && width == -11 && height == -7);
    release_test_screen(oversized);
    assert(gpu_gl_get_stats()->uploads == before.uploads);
    assert(gpu_gl_get_stats()->readbacks == before.readbacks);
    for(index = 0; index < 2; ++index)
    {
        oversized = test_screen(index ? 1 : maximum / 2 + 1,
                                index ? maximum / 2 + 1 : 1, PIXEL_32);
        assert(!gpu_gl_get_filtered_screen_texture(oversized, BLITTER_BILINEAR,
                                                    &texture, &width, &height));
        assert(texture == 123456 && width == -11 && height == -7);
        assert(gpu_gl_get_stats()->uploads == before.uploads);
        assert(gpu_gl_get_stats()->readbacks == before.readbacks);
        release_test_screen(oversized);
    }
    screen->width = INT_MAX;
    assert(!gpu_gl_get_filtered_screen_texture(screen, 1, &texture, &width, &height));
    screen->width = 11;
    assert(texture == 123456 && width == -11 && height == -7);
    assert(gpu_gl_get_screen_texture(screen, &texture, &source_width, &source_height));
    texture = 123456;
    before = *gpu_gl_get_stats();
    output_filter_reject_framebuffer = 1;
    assert(!gpu_gl_get_filtered_screen_texture(screen, BLITTER_TV2X, &texture, &width, &height));
    output_filter_reject_framebuffer = 0;
    assert(output_filter_rejected_framebuffers > 0);
    assert(texture == 123456 && width == -11 && height == -7);
    assert(gpu_is_active());
    assert(!gpu_allow_cpu_operation(screen, GPU_OPERATION_SCREEN));
    assert(gpu_gl_get_stats()->uploads == before.uploads);
    assert(gpu_gl_get_stats()->readbacks == before.readbacks);
    /* The historical counter records rejected native operations even when
     * their CPU retry is disabled. No pixel transfer accompanies rejection. */
    assert(gpu_gl_get_stats()->fallback_draws == before.fallback_draws + 1);
    assert(gpu_gl_get_stats()->output_filter_draws == before.output_filter_draws);
    output_filter_compare(screen, expected, BLITTER_TV2X, "allocation recovery");
    release_test_screen(screen);
}

/*
 * Damon V. Caskey
 * 2026-11-16
 * Recreate the actual context with a surviving source, checking resource
 * retirement and lazy reconstruction without stale GL object identities.
 */
static void output_filter_test_context(SDL_Window *window, SDL_GLContext *context)
{
    s_screen *screen = test_screen(5, 3, PIXEL_32);
    unsigned expected[15], texture;
    int index, width, height;
    output_filter_draw_pattern(screen, expected, 2);
    output_filter_compare(screen, expected, BLITTER_DOTMATRIX, "before context replacement");
    assert(gpu_gl_get_stats()->output_filter_resources == 1);
    gpu_gl_shutdown();
    assert(!gpu_is_active());
    assert(gpu_gl_get_stats()->output_filter_resources == 0);
    for(index = 0; index < 15; ++index)
        assert(test_pixel(screen, index % 5, index / 5) == expected[index]);
    texture = 123456; width = -5; height = -3;
    assert(!gpu_gl_get_filtered_screen_texture(screen, 1, &texture, &width, &height));
    assert(texture == 123456 && width == -5 && height == -3);
    SDL_GL_DeleteContext(*context);
    *context = SDL_GL_CreateContext(window);
    assert(*context);
    assert(LoadGLFunctions());
    assert(gpu_gl_init());
    gpu_set_native_required(1);
    assert(gpu_gl_get_stats()->output_filter_resources == 0);
    assert(gpu_gl_get_screen_texture(screen, &texture, &width, &height));
    output_filter_compare(screen, expected, BLITTER_BILINEAR, "new context");
    expected[6] = 0x00fffdfbu;
    assert(gpu_draw_box(screen, 1, 1, 1, 1, expected[6], &plainmethod));
    output_filter_compare(screen, expected, BLITTER_SCANLINESTV, "new context mutation");
    release_test_screen(screen);
}

/*
 * Damon V. Caskey
 * 2026-11-16
 * Link only native rendering and explicit verification readbacks. Unavailable
 * graphics infrastructure is distinct from shader or pixel assertion failure.
 */
int main(void)
{
    SDL_Window *window;
    SDL_GLContext context;
    assert(sizeof(void *) == 8);
    if(SDL_Init(SDL_INIT_VIDEO) != 0) return 77;
    gpu_test_context_attributes();
    window = SDL_CreateWindow("OpenBOR GPU output-filter tests", 0, 0, 64, 64,
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
    gpu_set_native_required(1);
    assert(gpu_gl_get_stats()->output_filter_resources == 0);
    output_filter_test_images();
    output_filter_test_scale2x_neighbors();
    output_filter_test_state_and_mutation();
    output_filter_test_rejections();
    output_filter_test_context(window, &context);
    assert(gpu_gl_get_stats()->fallback_draws == 0);
    gpu_gl_shutdown();
    SDL_GL_DeleteContext(context);
    SDL_DestroyWindow(window);
    SDL_Quit();
    printf("GPU output-filter tests passed: %u exact RGB8 images, %u pixels; six filters, bypass, borders, mutation, allocation failure and context recreation.\n",
           output_filter_images_checked, output_filter_pixels_checked);
    return 0;
}
