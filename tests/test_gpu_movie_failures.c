/*
 * Damon V. Caskey
 * 2026-11-15
 * Inject real GL plane-upload failures into the production loader and prove
 * movie rejection preserves target pixels, cache generations and scene state.
 */
#define main movie_complete_suite_main
#include "test_gpu_movie.c"
#undef main

extern void (APIENTRY *ptr_glTexSubImage2D)(GLenum, GLint, GLint, GLint, GLsizei,
                                           GLsizei, GLenum, GLenum, const GLvoid *);
static void (APIENTRY *movie_real_upload)(GLenum, GLint, GLint, GLint, GLsizei,
                                          GLsizei, GLenum, GLenum, const GLvoid *);
static unsigned movie_failure_plane, movie_failure_uploads;

/*
 * Damon V. Caskey
 * 2026-11-15
 * Fail one selected tightly packed movie-plane transfer with an actual GL
 * error, after any preceding planes have already changed their source texture.
 */
static void APIENTRY movie_fail_upload(GLenum target, GLint level, GLint x, GLint y,
                                       GLsizei width, GLsizei height,
                                       GLenum format, GLenum type, const GLvoid *pixels)
{
#if defined(OPENBOR_GLES)
    if(format == GL_RED)
#else
    if(format == GL_LUMINANCE)
#endif
    {
        ++movie_failure_uploads;
        if(movie_failure_plane && movie_failure_uploads == movie_failure_plane)
        {
            movie_real_upload((GLenum)0xffffffffu, level, x, y, width, height,
                              format, type, pixels);
            return;
        }
    }
    movie_real_upload(target, level, x, y, width, height, format, type, pixels);
}

/*
 * Damon V. Caskey
 * 2026-11-15
 * Arm an upload interceptor without replacing the renderer's private entry
 * points or relying on an artificial return value that the real API lacks.
 */
static void movie_failure_arm(unsigned plane)
{
    movie_failure_plane = plane;
    movie_failure_uploads = 0;
    ptr_glTexSubImage2D = movie_fail_upload;
}

/*
 * Damon V. Caskey
 * 2026-11-15
 * A failed second-plane update must preserve the destination and invalidate
 * every partially changed cached generation, including an older reused token.
 */
static void movie_test_partial_upload_failure(void)
{
    movie_fixture fixture;
    s_screen *target = movie_screen(17, 13);
    gpu_movie_draw draw = {0, 0, 17, 13, DC_RESAMPLE_NEAREST, 0, 0, 0};
    unsigned expected[MOVIE_TARGET_PIXELS], shadow[MOVIE_TARGET_PIXELS];
    gpu_renderer_stats before;
    size_t bytes = (size_t)target->width * target->height * sizeof(unsigned);
    movie_pattern(&fixture, 6, 8, 27);
    fixture.source.generation = 1;
    movie_expected(&fixture.source, &draw, 17, 13, expected);
    assert(gpu_draw_yuv(target, &fixture.source, &draw));
    memcpy(shadow, target->data, bytes);

    movie_pattern(&fixture, 6, 8, 151);
    fixture.source.generation = 2;
    before = *gpu_gl_get_stats();
    movie_failure_arm(2);
    assert(!gpu_draw_yuv(target, &fixture.source, &draw));
    assert(movie_failure_uploads == 2);
    assert(glGetError() == GL_NO_ERROR);
    assert(gpu_gl_get_stats()->movie_uploads == before.movie_uploads);
    assert(gpu_gl_get_stats()->movie_draws == before.movie_draws);
    assert(gpu_gl_get_stats()->movie_cache_hits == before.movie_cache_hits);
    assert(gpu_gl_get_stats()->readbacks == before.readbacks);
    assert(!memcmp(shadow, target->data, bytes));
    movie_verify_screen(target, expected, "rejected partial upload preserves GPU target", draw.resample);

    movie_failure_arm(0);
    before = *gpu_gl_get_stats();
    assert(gpu_draw_yuv(target, &fixture.source, &draw));
    assert(movie_failure_uploads == 3);
    assert(gpu_gl_get_stats()->movie_uploads == before.movie_uploads + 1);
    movie_expected(&fixture.source, &draw, 17, 13, expected);
    movie_verify_screen(target, expected, "retry replaces every failed plane", draw.resample);

    /* A failed newer generation has overwritten its luma already. Returning
     * to the last successful token must upload, not sample that mixed cache. */
    movie_pattern(&fixture, 6, 8, 79);
    fixture.source.generation = 3;
    movie_failure_arm(2);
    assert(!gpu_draw_yuv(target, &fixture.source, &draw));
    movie_pattern(&fixture, 6, 8, 151);
    fixture.source.generation = 2;
    movie_failure_arm(0);
    before = *gpu_gl_get_stats();
    assert(gpu_draw_yuv(target, &fixture.source, &draw));
    assert(movie_failure_uploads == 3);
    assert(gpu_gl_get_stats()->movie_cache_hits == before.movie_cache_hits);
    movie_verify_screen(target, expected, "older token after failed update reuploads all planes", draw.resample);

    /* Regular scene drawing still restores its own state after rejection. */
    assert(gpu_draw_box(target, 0, 0, 1, 1, 0x005a1c93, &plainmethod));
    gpu_sync_cpu(target);
    assert(((unsigned *)target->data)[0] == 0x005a1c93);
    gpu_release_yuv(fixture.source.owner);
    movie_release_screen(target);
}

/*
 * Damon V. Caskey
 * 2026-11-15
 * First-upload failure leaves the new cache entry unpublished and reusable.
 * Releasing its owner must then evict the same address before a fresh source.
 */
static void movie_test_initial_upload_failure(void)
{
    movie_fixture fixture;
    s_screen *target = movie_screen(11, 9);
    gpu_movie_draw draw = {1, 1, 8, 6, DC_RESAMPLE_NEAREST, 0, 0, 0};
    gpu_renderer_stats before;
    int pixel;
    movie_pattern(&fixture, 6, 8, 91);
    assert(gpu_draw_box(target, 0, 0, 11, 9, MOVIE_BACKGROUND, &plainmethod));
    before = *gpu_gl_get_stats();
    movie_failure_arm(1);
    assert(!gpu_draw_yuv(target, &fixture.source, &draw));
    assert(movie_failure_uploads == 1);
    assert(gpu_gl_get_stats()->movie_uploads == before.movie_uploads);
    assert(gpu_gl_get_stats()->movie_draws == before.movie_draws);
    assert(gpu_gl_get_stats()->movie_resources == before.movie_resources + 1);
    gpu_sync_cpu(target);
    for(pixel = 0; pixel < 11 * 9; ++pixel)
        assert(((unsigned *)target->data)[pixel] == MOVIE_BACKGROUND);
    movie_failure_arm(0);
    movie_compare(target, &fixture.source, &draw, "initial failed upload retries all planes");
    assert(movie_failure_uploads == 3);
    gpu_release_yuv(fixture.source.owner);
    assert(gpu_gl_get_stats()->movie_resources == before.movie_resources);
    movie_pattern(&fixture, 6, 8, 13);
    movie_failure_arm(0);
    movie_compare(target, &fixture.source, &draw, "released failed owner address is fresh");
    assert(movie_failure_uploads == 3);
    gpu_release_yuv(fixture.source.owner);
    movie_release_screen(target);
}

/*
 * Damon V. Caskey
 * 2026-11-15
 * Source upload must succeed before fullscreen presentation clears any border
 * or movie pixel. Compare the back buffer directly around a rejected update.
 */
static void movie_test_window_upload_failure(void)
{
    movie_fixture fixture;
    gpu_movie_draw draw = {3, 5, 23, 17, DC_RESAMPLE_MOVIE, 0, 0, 0};
    unsigned before_pixels[64 * 64], after_pixels[64 * 64];
    void (APIENTRY *read_buffer)(GLenum) = SDL_GL_GetProcAddress("glReadBuffer");
    gpu_renderer_stats before;
    movie_pattern(&fixture, 6, 10, 55);
    movie_failure_arm(0);
    assert(gpu_gl_present_yuv(&fixture.source, &draw, 64, 64));
    assert(read_buffer);
    read_buffer(GL_BACK);
    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    glPixelStorei(GL_PACK_ROW_LENGTH, 0);
    glPixelStorei(GL_PACK_SKIP_ROWS, 0);
    glPixelStorei(GL_PACK_SKIP_PIXELS, 0);
    glReadPixels(0, 0, 64, 64, GL_RGBA, GL_UNSIGNED_BYTE, before_pixels);
    movie_pattern(&fixture, 6, 10, 155);
    fixture.source.generation = 2;
    before = *gpu_gl_get_stats();
    movie_failure_arm(2);
    assert(!gpu_gl_present_yuv(&fixture.source, &draw, 64, 64));
    assert(movie_failure_uploads == 2);
    assert(gpu_gl_get_stats()->movie_uploads == before.movie_uploads);
    assert(gpu_gl_get_stats()->movie_draws == before.movie_draws);
    gpu_gl_prepare_present();
    glReadPixels(0, 0, 64, 64, GL_RGBA, GL_UNSIGNED_BYTE, after_pixels);
    assert(!memcmp(before_pixels, after_pixels, sizeof(before_pixels)));
    assert(glGetError() == GL_NO_ERROR);
    gpu_release_yuv(fixture.source.owner);
}

/*
 * Damon V. Caskey
 * 2026-11-15
 * Run real-driver failure and recovery checks independently of numeric image
 * tolerances, and restore the production loader before context destruction.
 */
int main(void)
{
    SDL_Window *window;
    SDL_GLContext context;
    if(SDL_Init(SDL_INIT_VIDEO) != 0) return 77;
    gpu_test_context_attributes();
    window = SDL_CreateWindow("OpenBOR movie failure tests", 0, 0, 64, 64,
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
    movie_real_upload = ptr_glTexSubImage2D;
    assert(movie_real_upload);
    movie_test_partial_upload_failure();
    movie_test_initial_upload_failure();
    movie_test_window_upload_failure();
    ptr_glTexSubImage2D = movie_real_upload;
    assert(gpu_gl_get_stats()->movie_resources == 0);
    gpu_gl_shutdown();
    SDL_GL_DeleteContext(context);
    SDL_DestroyWindow(window);
    SDL_Quit();
    puts("GPU movie failure tests passed: initial/partial uploads, generation rollback, owner reuse, scene recovery and unchanged window on rejection.");
    return 0;
}
