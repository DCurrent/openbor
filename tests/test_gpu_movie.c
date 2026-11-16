/*
 * Damon V. Caskey
 * 2026-11-15
 * Exercise native movie composition against independent long-double YUV and
 * reconstruction equations. No CPU movie converter or pixel renderer is linked.
 * Readback occurs only at explicit verification and context-retirement points.
 */
#include <assert.h>
#include <limits.h>
#include <math.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <SDL.h>
#include "gpu.h"
#include "gpu_renderer.h"
#include "loadgl.h"

#define MOVIE_SOURCE_PIXELS 256
#define MOVIE_TARGET_PIXELS 4096
#define MOVIE_BACKGROUND 0x0038597bu
#define MOVIE_PI 3.141592653589793238462643383279502884L

int pixelbytes[5] = {1, 1, 0, 0, 4};
int pixelformat = PIXEL_32;
int useclip, clipx1, clipx2, clipy1, clipy2;
unsigned char *blendtables[MAX_BLENDINGS];
const s_drawmethod plainmethod = {
    .config = DRAWMETHOD_CONFIG_ENABLED,
    .alpha = -1, .remap = -1,
    .scalex = 256, .scaley = 256,
    .xrepeat = 1, .yrepeat = 1,
    .channelr = 255, .channelg = 255, .channelb = 255,
    .affine = {1, 0, 0, 1, 0, 0},
    .object_type = OBJECT_TYPE_DRAWMETHOD
};

typedef struct movie_fixture
{
    gpu_yuv_source source;
    unsigned char y[MOVIE_SOURCE_PIXELS];
    unsigned char u[MOVIE_SOURCE_PIXELS / 4];
    unsigned char v[MOVIE_SOURCE_PIXELS / 4];
} movie_fixture;

static unsigned movie_images_checked;
static unsigned movie_pixels_checked;
static unsigned movie_maximum_error;

/*
 * Damon V. Caskey
 * 2026-11-15
 * Preserve fatal allocation checks without linking the game's logging layer.
 */
void *checkAlloc(void *pointer, size_t size, const char *function, const char *file, int line)
{
    (void)size; (void)function; (void)file; (void)line;
    assert(pointer);
    return pointer;
}

/*
 * Damon V. Caskey
 * 2026-11-15
 * Keep a recognizable CPU shadow while rendering the destination on the GPU.
 */
static s_screen *movie_screen(int width, int height)
{
    s_screen *screen;
    assert(width > 0 && height > 0 && width * height <= MOVIE_TARGET_PIXELS);
    screen = calloc(1, sizeof(*screen) + (size_t)width * height * sizeof(unsigned));
    assert(screen);
    screen->magic = screen_magic;
    screen->width = width;
    screen->height = height;
    screen->pixelformat = PIXEL_32;
    return screen;
}

/*
 * Damon V. Caskey
 * 2026-11-15
 * Evict screen identities before their storage can be reused.
 */
static void movie_release_screen(s_screen *screen)
{
    gpu_release_screen(screen);
    free(screen);
}

/*
 * Damon V. Caskey
 * 2026-11-15
 * Produce distinct luma/chroma rows and asymmetric chroma channels. Small
 * even widths deliberately include plane rows that are not four-byte aligned.
 */
static void movie_pattern(movie_fixture *fixture, int width, int height, int seed)
{
    int x, y;
    assert(width >= 2 && height >= 2 && !(width & 1) && !(height & 1));
    assert(width * height <= MOVIE_SOURCE_PIXELS);
    memset(fixture, 0, sizeof(*fixture));
    fixture->source.owner = fixture;
    fixture->source.generation = 1;
    fixture->source.width = width;
    fixture->source.height = height;
    fixture->source.y = fixture->y;
    fixture->source.u = fixture->u;
    fixture->source.v = fixture->v;
    for(y = 0; y < height; ++y)
        for(x = 0; x < width; ++x)
            fixture->y[y * width + x] = (unsigned char)(17 + 37*x + 29*y + seed);
    for(y = 0; y < height / 2; ++y)
        for(x = 0; x < width / 2; ++x)
        {
            fixture->u[y * (width / 2) + x] = (unsigned char)(31 + 61*x + 23*y + seed);
            fixture->v[y * (width / 2) + x] = (unsigned char)(239 - 17*x - 53*y - seed);
        }
}

/*
 * Damon V. Caskey
 * 2026-11-15
 * Build uniform planes for exact color, clipping and correction-order cases.
 */
static void movie_constant(movie_fixture *fixture, unsigned y, unsigned u, unsigned v)
{
    movie_pattern(fixture, 2, 2, 0);
    memset(fixture->y, (int)y, sizeof(fixture->y));
    memset(fixture->u, (int)u, sizeof(fixture->u));
    memset(fixture->v, (int)v, sizeof(fixture->v));
}

/*
 * Damon V. Caskey
 * 2026-11-15
 * Derive cubic filters from Mitchell-Netravali's general equation, and
 * Lanczos from sinc products, independently of optimized shader polynomials.
 */
static long double movie_weight(int mode, long double distance)
{
    long double x = fabsl(distance), b = 0, c = 0, radius;
    if(mode == DC_RESAMPLE_BILINEAR) return x < 1 ? 1 - x : 0;
    if(mode == DC_RESAMPLE_LANCZOS_2 || mode == DC_RESAMPLE_LANCZOS_3)
    {
        radius = mode == DC_RESAMPLE_LANCZOS_2 ? 2 : 3;
        if(x == 0) return 1;
        if(x >= radius) return 0;
        return sinl(MOVIE_PI*x) * sinl(MOVIE_PI*x/radius)
             / (MOVIE_PI*MOVIE_PI*x*x/radius);
    }
    if(mode == DC_RESAMPLE_BICUBIC_CATMULL_ROM) c = 0.5L;
    else if(mode == DC_RESAMPLE_BICUBIC_MITCHELL) b = c = 1.0L/3;
    else { assert(mode == DC_RESAMPLE_BICUBIC_BSPLINE); b = 1; }
    if(x < 1)
        return ((12-9*b-6*c)*x*x*x + (-18+12*b+6*c)*x*x + 6-2*b)/6;
    if(x < 2)
        return ((-b-6*c)*x*x*x + (6*b+30*c)*x*x + (-12*b-48*c)*x + 8*b+24*c)/6;
    return 0;
}

/*
 * Damon V. Caskey
 * 2026-11-15
 * Clamp each independent reconstruction tap to its own plane's edge.
 */
static unsigned movie_plane_byte(const unsigned char *plane, int width, int height, int x, int y)
{
    if(x < 0) x = 0;
    if(y < 0) y = 0;
    if(x >= width) x = width - 1;
    if(y >= height) y = height - 1;
    return plane[y * width + x];
}

/*
 * Damon V. Caskey
 * 2026-11-15
 * Reconstruct each plane at the common normalized UV using its own sample
 * centers. Retain negative ringing and overshoot until final color correction.
 */
static long double movie_sample_plane(const unsigned char *plane, int width, int height,
                                      int mode, long double u, long double v)
{
    long double px = u*width - 0.5L, py = v*height - 0.5L;
    long double sum = 0, weight_sum = 0;
    int x, y, radius = mode == DC_RESAMPLE_LANCZOS_3 ? 3 : 2;
    int first_x = (int)floorl(px), first_y = (int)floorl(py);
    if(mode == DC_RESAMPLE_NEAREST)
        return movie_plane_byte(plane, width, height,
                                 (int)floorl(u*width), (int)floorl(v*height))/255.0L;
    if(mode == DC_RESAMPLE_BILINEAR) radius = 1;
    for(y = first_y-radius+1; y <= first_y+radius; ++y)
        for(x = first_x-radius+1; x <= first_x+radius; ++x)
        {
            long double weight = movie_weight(mode, px-x) * movie_weight(mode, py-y);
            sum += movie_plane_byte(plane, width, height, x, y)*weight;
            weight_sum += weight;
        }
    assert(fabsl(weight_sum) > 0.000001L);
    return sum/(255.0L*weight_sum);
}

/*
 * Damon V. Caskey
 * 2026-11-15
 * Preserve the legacy movie matrix and polynomial correction order. The
 * framebuffer clamps only after gamma then brightness, never at RGB import.
 */
static unsigned movie_oracle_pixel(const gpu_yuv_source *source, const gpu_movie_draw *draw,
                                   long double u, long double v)
{
    long double y = movie_sample_plane(source->y, source->width, source->height,
                                        draw->resample, u, v) - 0.0625L;
    long double cb = movie_sample_plane(source->u, source->width/2, source->height/2,
                                         draw->resample, u, v) - 0.5L;
    long double cr = movie_sample_plane(source->v, source->width/2, source->height/2,
                                         draw->resample, u, v) - 0.5L;
    long double rgb[3] = {1.164L*y + 1.596L*cr,
                          1.164L*y - 0.391L*cb - 0.813L*cr,
                          1.164L*y + 2.018L*cb};
    unsigned result = 0;
    int channel;
    for(channel = 0; channel < 3; ++channel)
    {
        long double value = rgb[channel];
        long double gamma = draw->gamma, brightness = draw->brightness;
        if(gamma > 0) value = 1-(1-value)*(1-value*gamma);
        else value = value*(1-(1-value)*-gamma);
        if(brightness > 0) value = value*(1-brightness)+brightness;
        else value = value*(1+brightness);
        if(value < 0) value = 0;
        if(value > 1) value = 1;
        result |= (unsigned)floorl(value*255+0.5L) << (channel*8);
    }
    if(draw->black_filter && !result) result = 0x00010101;
    return result;
}

/*
 * Damon V. Caskey
 * 2026-11-15
 * Resolve coverage with wide signed arithmetic so extreme offsets remain
 * distinguishable from source rescaling or integer overflow.
 */
static void movie_expected(const gpu_yuv_source *source, const gpu_movie_draw *draw,
                            int width, int height, unsigned *expected)
{
    int x, y;
    for(y = 0; y < height; ++y)
        for(x = 0; x < width; ++x)
        {
            int64_t relative_x = (int64_t)x - draw->x;
            int64_t relative_y = (int64_t)y - draw->y;
            unsigned pixel = MOVIE_BACKGROUND;
            if(relative_x >= 0 && relative_y >= 0 &&
               relative_x < draw->width && relative_y < draw->height)
                pixel = movie_oracle_pixel(source, draw,
                    (relative_x+0.5L)/draw->width, (relative_y+0.5L)/draw->height);
            expected[y*width+x] = pixel;
        }
}

/*
 * Damon V. Caskey
 * 2026-11-15
 * Invert the historical fullscreen projection independently. Its integer
 * endpoints span viewport-1, and source rows run downward against GL window Y.
 */
static void movie_expected_window(const gpu_yuv_source *source, const gpu_movie_draw *draw,
                                   int width, int height, unsigned *expected)
{
    int x, y;
    for(y = 0; y < height; ++y)
        for(x = 0; x < width; ++x)
        {
            long double world_x = (x+0.5L)*(width > 1 ? width-1 : width)/width;
            long double world_y = (y+0.5L)*(height > 1 ? height-1 : height)/height;
            long double extent_x = draw->width-(width > 1 ? 1 : 0);
            long double extent_y = draw->height-(height > 1 ? 1 : 0);
            long double u = (world_x-draw->x)/extent_x;
            long double v = (world_y-draw->y)/extent_y;
            unsigned pixel = 0;
            if(extent_x > 0 && extent_y > 0 && u >= 0 && u < 1 && v >= 0 && v < 1)
                pixel = movie_oracle_pixel(source, draw, u, 1-v);
            expected[y*width+x] = pixel;
        }
}

/*
 * Damon V. Caskey
 * 2026-11-15
 * Compare complete images, reporting the first useful coordinate and channel
 * rather than accepting agreement at a handful of convenient sample points.
 */
static void movie_verify_pixels(const unsigned *pixels, const unsigned *expected,
                                 int width, int height, int inverted, const char *label, int mode)
{
    int x, y, channel;
    /* The preserved GL_LINEAR movie sampler may quantize its interpolation
     * weights. The YUV matrix can amplify rounded plane samples to three RGB
     * levels; manually reconstructed kernels retain one-byte tolerance. */
    unsigned tolerance = mode == DC_RESAMPLE_BILINEAR ? 3u : 1u;
    for(y = 0; y < height; ++y)
        for(x = 0; x < width; ++x)
        {
            unsigned actual = pixels[(inverted ? height-1-y : y)*width+x];
            unsigned wanted = expected[y*width+x];
            for(channel = 0; channel < 3; ++channel)
            {
                int got = (actual >> (channel*8)) & 255;
                int want = (wanted >> (channel*8)) & 255;
                unsigned error = (unsigned)abs(got-want);
                if(error > movie_maximum_error) movie_maximum_error = error;
                if(error > tolerance)
                {
                    fprintf(stderr, "%s filter=%d (%d,%d) channel=%d expected=%d actual=%d\n",
                            label, mode, x, y, channel, want, got);
                    assert(error <= tolerance);
                }
            }
        }
    ++movie_images_checked;
    movie_pixels_checked += (unsigned)(width*height);
}

/*
 * Damon V. Caskey
 * 2026-11-15
 * Synchronize only when a test explicitly consumes the completed image.
 */
static void movie_verify_screen(s_screen *screen, const unsigned *expected, const char *label, int mode)
{
    gpu_sync_cpu(screen);
    movie_verify_pixels((const unsigned *)screen->data, expected,
                         screen->width, screen->height, 0, label, mode);
}

/*
 * Damon V. Caskey
 * 2026-11-15
 * Render over GPU-authored pixels and require the operation to leave the
 * CPU shadow alone until the explicit verification boundary.
 */
static void movie_compare(s_screen *target, const gpu_yuv_source *source,
                           const gpu_movie_draw *draw, const char *label)
{
    unsigned expected[MOVIE_TARGET_PIXELS], shadow[MOVIE_TARGET_PIXELS];
    char description[192];
    gpu_renderer_stats before;
    size_t bytes = (size_t)target->width*target->height*sizeof(unsigned);
    movie_expected(source, draw, target->width, target->height, expected);
    assert(gpu_draw_box(target, 0, 0, target->width, target->height, MOVIE_BACKGROUND, &plainmethod));
    memcpy(shadow, target->data, bytes);
    before = *gpu_gl_get_stats();
    assert(gpu_draw_yuv(target, source, draw));
    assert(gpu_gl_get_stats()->readbacks == before.readbacks);
    assert(gpu_gl_get_stats()->fallback_draws == before.fallback_draws);
    assert(!memcmp(shadow, target->data, bytes));
    snprintf(description, sizeof(description), "%s rect=(%d,%d,%d,%d)",
             label, draw->x, draw->y, draw->width, draw->height);
    movie_verify_screen(target, expected, description, draw->resample);
}

/*
 * Damon V. Caskey
 * 2026-11-15
 * Exercise every static kernel at native, enlarged, reduced, thin and clipped
 * sizes. Source filtering remains independent of global sprite clipping.
 */
static void movie_test_reconstruction(void)
{
    const int rectangles[][4] = {
        {2, 3, 6, 10}, {3, 1, 19, 27}, {5, 3, 3, 4}, {2, 1, 1, 17},
        {-4, -3, 23, 17}, {31, 23, 19, 17}, {-8, 7, 18, 9},
        {INT_MIN+3, 0, INT_MAX, 8}, {-INT_MAX+20, -2, INT_MAX, 6}
    };
    movie_fixture fixture;
    s_screen *target = movie_screen(37, 29);
    int mode, index;
    movie_pattern(&fixture, 6, 10, 0);
    useclip = 1; clipx1 = 12; clipy1 = 12; clipx2 = 15; clipy2 = 15;
    for(mode = DC_RESAMPLE_NEAREST; mode <= DC_RESAMPLE_LANCZOS_3; ++mode)
        for(index = 0; index < (int)(sizeof(rectangles)/sizeof(rectangles[0])); ++index)
        {
            gpu_movie_draw draw = {rectangles[index][0], rectangles[index][1],
                rectangles[index][2], rectangles[index][3], (e_drawmethod_resample)mode, 0, 0, 0};
            movie_compare(target, &fixture.source, &draw, "independent YUV plane reconstruction");
        }
    useclip = 0;
    gpu_release_yuv(fixture.source.owner);
    movie_release_screen(target);
}

/*
 * Damon V. Caskey
 * 2026-11-15
 * Pin legacy offsets, asymmetric U/V, out-of-gamut correction and filtering
 * before clipping. Uniform cases include exact expected values independent
 * of the oracle implementation itself.
 */
static void movie_test_color(void)
{
    const struct {unsigned y, u, v, expected; float gamma, brightness;} cases[] = {
        {16, 128, 128, 0x00010001, 0, 0},
        {235, 128, 128, 0x00fffeff, 0, 0},
        {235, 16, 240, 0x0038f681, 1, 0},
        {16, 16, 16, 0x0000a400, 0, 0.25f},
        {16, 16, 16, 0x00c6477c, -1, 0},
        {128, 32, 224, 0, 0.5f, -0.25f},
        {128, 224, 32, 0, -0.5f, 0.5f}
    };
    movie_fixture fixture;
    s_screen *target = movie_screen(13, 11);
    int index, mode;
    for(index = 0; index < (int)(sizeof(cases)/sizeof(cases[0])); ++index)
    {
        movie_constant(&fixture, cases[index].y, cases[index].u, cases[index].v);
        fixture.source.generation = (uint64_t)index+1;
        for(mode = DC_RESAMPLE_NEAREST; mode <= DC_RESAMPLE_LANCZOS_3; ++mode)
        {
            gpu_movie_draw draw = {1, 1, 11, 9, (e_drawmethod_resample)mode, 0,
                                   cases[index].gamma, cases[index].brightness};
            if(index < 5) assert(movie_oracle_pixel(&fixture.source, &draw, 0.5L, 0.5L) == cases[index].expected);
            movie_compare(target, &fixture.source, &draw, "legacy matrix and correction order");
        }
    }
    gpu_release_yuv(fixture.source.owner);
    movie_pattern(&fixture, 2, 2, 0);
    fixture.y[0] = fixture.y[2] = 0;
    fixture.y[1] = fixture.y[3] = 235;
    fixture.u[0] = fixture.v[0] = 128;
    for(mode = DC_RESAMPLE_BILINEAR; mode <= DC_RESAMPLE_LANCZOS_3; ++mode)
    {
        gpu_movie_draw draw = {0, 0, 13, 11, (e_drawmethod_resample)mode, 0, 0, 0};
        movie_compare(target, &fixture.source, &draw, "YUV filtering before RGB clipping");
    }
    gpu_release_yuv(fixture.source.owner);
    movie_release_screen(target);
}

/*
 * Damon V. Caskey
 * 2026-11-15
 * Movie composition always overwrites black. Optional protection changes only
 * exact quantized black for subsequent ordinary keyed screen drawing.
 */
static void movie_test_black(void)
{
    movie_fixture fixture;
    s_screen *target = movie_screen(7, 5), *composite = movie_screen(7, 5);
    gpu_movie_draw draw = {1, 1, 5, 3, DC_RESAMPLE_MOVIE, 0, 0, 0};
    s_drawmethod keyed = plainmethod;
    int filter, index;
    movie_constant(&fixture, 0, 128, 128);
    keyed.config |= DRAWMETHOD_CONFIG_BACKGROUND_TRANSPARENCY;
    for(filter = 0; filter <= 1; ++filter)
    {
        draw.black_filter = filter;
        movie_compare(target, &fixture.source, &draw, "opaque black and black protection");
        assert(((unsigned *)target->data)[2*7+3] == (filter ? 0x00010101u : 0));
        assert(gpu_draw_box(composite, 0, 0, 7, 5, 0x00aabbcc, &plainmethod));
        assert(gpu_draw_screen(composite, target, 0, 0, &keyed));
        gpu_sync_cpu(composite);
        assert(((unsigned *)composite->data)[2*7+3] == (filter ? 0x00010101u : 0x00aabbccu));
    }
    /* Valid zero components must not trigger whole-pixel black protection. */
    for(index = 0; index < 2; ++index)
    {
        movie_constant(&fixture, index ? 16 : 128, index ? 16 : 240, index ? 16 : 16);
        fixture.source.generation = (uint64_t)index+2;
        draw.black_filter = 1;
        movie_compare(target, &fixture.source, &draw, "partial-zero color protection");
    }
    gpu_release_yuv(fixture.source.owner);
    movie_release_screen(target);
    movie_release_screen(composite);
}

/*
 * Damon V. Caskey
 * 2026-11-15
 * Dirty GL unpack state must not change tightly packed plane uploads. Widths
 * two, six and ten make chroma row alignment errors visible immediately.
 */
static void movie_test_upload_state(void)
{
    const int widths[] = {2, 6, 10};
    movie_fixture fixture;
    s_screen *target = movie_screen(19, 13);
    gpu_movie_draw draw = {0, 0, 19, 13, DC_RESAMPLE_MOVIE, 0, 0, 0};
    int index;
    for(index = 0; index < 3; ++index)
    {
        unsigned expected[MOVIE_TARGET_PIXELS];
        gpu_renderer_stats before;
        movie_pattern(&fixture, widths[index], 8, 17*index);
        fixture.source.generation = (uint64_t)index+1;
        assert(gpu_draw_box(target, 0, 0, 19, 13, MOVIE_BACKGROUND, &plainmethod));
        movie_expected(&fixture.source, &draw, 19, 13, expected);
        before = *gpu_gl_get_stats();
        glPixelStorei(GL_UNPACK_ALIGNMENT, 8);
        glPixelStorei(GL_UNPACK_ROW_LENGTH, 31);
        glPixelStorei(GL_UNPACK_SKIP_ROWS, 2);
        glPixelStorei(GL_UNPACK_SKIP_PIXELS, 3);
        assert(gpu_draw_yuv(target, &fixture.source, &draw));
        assert(gpu_gl_get_stats()->readbacks == before.readbacks);
        movie_verify_screen(target, expected, "packed YUV upload state", draw.resample);
    }
    gpu_release_yuv(fixture.source.owner);
    movie_release_screen(target);
}

/*
 * Damon V. Caskey
 * 2026-11-15
 * Stable source identity and generation reuse one plane upload across targets,
 * while new generations, dimensions and retired owner addresses invalidate it.
 */
static void movie_test_cache(void)
{
    movie_fixture first, second;
    s_screen *target = movie_screen(17, 13), *other = movie_screen(11, 9);
    gpu_movie_draw draw = {0, 0, 17, 13, DC_RESAMPLE_MOVIE, 0, 0, 0};
    gpu_renderer_stats before;
    unsigned old_expected[MOVIE_TARGET_PIXELS];
    uint64_t uploads, resources;
    movie_pattern(&first, 6, 8, 1);
    movie_pattern(&second, 10, 6, 95);
    before = *gpu_gl_get_stats();
    movie_compare(target, &first.source, &draw, "first channel upload");
    assert(gpu_gl_get_stats()->movie_uploads == before.movie_uploads+1);
    assert(gpu_gl_get_stats()->movie_resources == before.movie_resources+1);
    uploads = gpu_gl_get_stats()->movie_uploads;
    movie_expected(&first.source, &draw, target->width, target->height, old_expected);
    memset(first.y, 255, sizeof(first.y));
    memset(first.u, 0, sizeof(first.u));
    memset(first.v, 0, sizeof(first.v));
    before = *gpu_gl_get_stats();
    assert(gpu_draw_yuv(target, &first.source, &draw));
    assert(gpu_gl_get_stats()->movie_uploads == uploads);
    assert(gpu_gl_get_stats()->movie_cache_hits == before.movie_cache_hits+1);
    movie_verify_screen(target, old_expected, "generation owns decoded pixels", draw.resample);
    ++first.source.generation;
    movie_compare(target, &first.source, &draw, "new decoded frame generation");
    assert(gpu_gl_get_stats()->movie_uploads == uploads+1);
    uploads = gpu_gl_get_stats()->movie_uploads;
    movie_compare(other, &first.source, &draw, "same channel different target");
    assert(gpu_gl_get_stats()->movie_uploads == uploads);
    movie_compare(target, &second.source, &draw, "second channel source separation");
    assert(gpu_gl_get_stats()->movie_uploads == uploads+1);
    movie_compare(target, &first.source, &draw, "interleaved first channel");
    assert(gpu_gl_get_stats()->movie_uploads == uploads+1);
    /* Size is part of the cache contract even when a caller repeats a token. */
    first.source.width = 4; first.source.height = 4;
    movie_compare(target, &first.source, &draw, "dimension invalidation");
    assert(gpu_gl_get_stats()->movie_uploads == uploads+2);
    resources = gpu_gl_get_stats()->movie_resources;
    gpu_release_yuv(first.source.owner);
    assert(gpu_gl_get_stats()->movie_resources == resources-1);
    gpu_release_yuv(first.source.owner);
    assert(gpu_gl_get_stats()->movie_resources == resources-1);
    movie_pattern(&first, 4, 4, 137);
    movie_compare(target, &first.source, &draw, "released owner address reuse");
    assert(gpu_gl_get_stats()->movie_uploads == uploads+3);
    assert(gpu_gl_get_stats()->movie_resources == resources);
    gpu_release_yuv(first.source.owner);
    gpu_release_yuv(second.source.owner);
    movie_release_screen(target);
    movie_release_screen(other);
}

/*
 * Damon V. Caskey
 * 2026-11-15
 * Fully clipped and invalid requests cannot upload a movie, acquire a target,
 * change pixels, or reach an unavailable software converter.
 */
static void movie_test_rejection(void)
{
    const int offsets[][2] = {{INT_MIN, 0}, {INT_MAX, 0}, {0, INT_MIN}, {0, INT_MAX}, {-8, -8}};
    movie_fixture fixture;
    s_screen *target = movie_screen(9, 7);
    gpu_movie_draw draw = {0, 0, 8, 6, DC_RESAMPLE_MOVIE, 0, 0, 0};
    gpu_renderer_stats before;
    int index;
    movie_pattern(&fixture, 6, 8, 9);
    before = *gpu_gl_get_stats();
    for(index = 0; index < (int)(sizeof(offsets)/sizeof(offsets[0])); ++index)
    {
        draw.x = offsets[index][0]; draw.y = offsets[index][1];
        assert(gpu_draw_yuv(target, &fixture.source, &draw));
    }
    assert(gpu_gl_get_stats()->movie_uploads == before.movie_uploads);
    assert(gpu_gl_get_stats()->movie_draws == before.movie_draws);
    assert(gpu_gl_get_stats()->movie_resources == before.movie_resources);
    assert(gpu_gl_get_stats()->uploads == before.uploads);
    assert(gpu_gl_get_stats()->readbacks == before.readbacks);
    draw.x = draw.y = 0;
    for(index = 0; index < 15; ++index)
    {
        gpu_yuv_source invalid = fixture.source;
        gpu_movie_draw invalid_draw = draw;
        if(index == 0) invalid.owner = NULL;
        if(index == 1) invalid.y = NULL;
        if(index == 2) invalid.u = NULL;
        if(index == 3) invalid.v = NULL;
        if(index == 4) invalid.width = 3;
        if(index == 5) invalid.height = 0;
        if(index == 6) invalid_draw.width = 0;
        if(index == 7) invalid_draw.resample = (e_drawmethod_resample)-1;
        if(index == 8) invalid_draw.gamma = NAN;
        if(index == 9) invalid_draw.brightness = INFINITY;
        if(index == 10) invalid.width = INT_MAX-1;
        if(index == 11) invalid_draw.resample = DC_RESAMPLE_END;
        if(index == 12) invalid_draw.height = -1;
        if(index == 13) invalid_draw.gamma = -1.01f;
        if(index == 14) invalid_draw.brightness = 1.01f;
        assert(!gpu_draw_yuv(target, &invalid, &invalid_draw));
    }
    assert(gpu_gl_get_stats()->movie_uploads == before.movie_uploads);
    assert(gpu_gl_get_stats()->movie_draws == before.movie_draws);
    assert(gpu_gl_get_stats()->movie_resources == before.movie_resources);
    assert(gpu_gl_get_stats()->uploads == before.uploads);
    assert(gpu_gl_get_stats()->readbacks == before.readbacks);
    for(index = 0; index < 9*7; ++index) assert(((unsigned *)target->data)[index] == 0);
    movie_release_screen(target);
}

/*
 * Damon V. Caskey
 * 2026-11-15
 * 2026-11-16 - Use core shader handles consistently on macOS and other targets.
 * Retain an independent copy of the old hardware shader for same-driver
 * comparisons. This reference has no production movie sampling code, source
 * cache, shared vertex buffer, clipping helper or RGB conversion intermediate.
 */
static void movie_legacy_reference(const gpu_yuv_source *source, const gpu_movie_draw *draw,
                                    int width, int height, int window_geometry, unsigned *pixels)
{
    static const char *fragment_source =
        "uniform sampler2D tex;\n"
        "uniform sampler2D utex;\n"
        "uniform sampler2D vtex;\n"
        "uniform float gamma;\n"
        "uniform float brightness;\n"
        "vec3 applyCorrection(vec3 color) {\n"
        "  if(gamma > 0.0) color = 1.0-((1.0-color)*(1.0-color*gamma));\n"
        "  else color = color*(1.0-(1.0-color)*-gamma);\n"
        "  if(brightness > 0.0) color = mix(color,vec3(1.0),brightness);\n"
        "  else color = mix(color,vec3(0.0),-brightness);\n"
        "  return color;\n"
        "}\n"
        "void main() {\n"
        "  vec3 yuv;\n"
        "  yuv.x = texture2D(tex,gl_TexCoord[0].xy).x;\n"
        "  yuv.y = texture2D(utex,gl_TexCoord[0].xy).x;\n"
        "  yuv.z = texture2D(vtex,gl_TexCoord[0].xy).x;\n"
        "  yuv += vec3(-.0625,-.5,-.5);\n"
        "  vec3 rgb;\n"
        "  rgb.x = dot(yuv,vec3(1.164,0,1.596));\n"
        "  rgb.y = dot(yuv,vec3(1.164,-.391,-.813));\n"
        "  rgb.z = dot(yuv,vec3(1.164,2.018,0));\n"
        "  gl_FragColor = vec4(applyCorrection(rgb),1.0);\n"
        "}\n";
    PFNGLDELETEPROGRAMPROC delete_program = SDL_GL_GetProcAddress("glDeleteProgram");
    void (APIENTRY *read_buffer)(GLenum) = SDL_GL_GetProcAddress("glReadBuffer");
    GLuint fragment, program;
    GLuint textures[3];
    const unsigned char *planes[] = {source->y, source->u, source->v};
    GLint ready;
    int plane;
    float top_uv = window_geometry ? 1.0f : 0.0f;
    float bottom_uv = 1.0f-top_uv;
    int right = draw->x+draw->width-(window_geometry && width > 1 ? 1 : 0);
    int top = draw->y+draw->height-(window_geometry && height > 1 ? 1 : 0);
    assert(delete_program && read_buffer);
    gpu_gl_prepare_present();
    gpu_gl_invalidate_state();
    fragment = ptr_glCreateShader(GL_FRAGMENT_SHADER);
    ptr_glShaderSource(fragment, 1, &fragment_source, NULL);
    ptr_glCompileShader(fragment);
    ptr_glGetShaderiv(fragment, GL_COMPILE_STATUS, &ready);
    assert(ready);
    program = ptr_glCreateProgram();
    ptr_glAttachShader(program, fragment);
    ptr_glLinkProgram(program);
    ptr_glGetProgramiv(program, GL_LINK_STATUS, &ready);
    assert(ready);
    ptr_glUseProgram(program);
    ptr_glUniform1i(ptr_glGetUniformLocation(program, "tex"), 0);
    ptr_glUniform1i(ptr_glGetUniformLocation(program, "utex"), 1);
    ptr_glUniform1i(ptr_glGetUniformLocation(program, "vtex"), 2);
    ptr_glUniform1f(ptr_glGetUniformLocation(program, "gamma"), draw->gamma);
    ptr_glUniform1f(ptr_glGetUniformLocation(program, "brightness"), draw->brightness);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
    glPixelStorei(GL_UNPACK_SKIP_ROWS, 0);
    glPixelStorei(GL_UNPACK_SKIP_PIXELS, 0);
    glGenTextures(3, textures);
    for(plane = 0; plane < 3; ++plane)
    {
        glActiveTexture(GL_TEXTURE0+plane);
        glBindTexture(GL_TEXTURE_2D, textures[plane]);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, source->width/(plane ? 2 : 1),
                      source->height/(plane ? 2 : 1), 0, GL_RED, GL_UNSIGNED_BYTE, planes[plane]);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    }
    glViewport(0, 0, width, height);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0, width-(window_geometry && width > 1 ? 1 : 0),
              0, height-(window_geometry && height > 1 ? 1 : 0), -1, 1);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glDisable(GL_BLEND); glDisable(GL_DEPTH_TEST); glDisable(GL_SCISSOR_TEST);
    glDisable(GL_DITHER); glDisable(GL_CULL_FACE); glDisable(GL_ALPHA_TEST);
    if(window_geometry) glClearColor(0, 0, 0, 1);
    else glClearColor((MOVIE_BACKGROUND & 255)/255.0f,
                      ((MOVIE_BACKGROUND >> 8) & 255)/255.0f,
                      ((MOVIE_BACKGROUND >> 16) & 255)/255.0f, 1);
    glClear(GL_COLOR_BUFFER_BIT);
    glActiveTexture(GL_TEXTURE0);
    glBegin(GL_QUADS);
    glMultiTexCoord2f(GL_TEXTURE0, 0, top_uv); glVertex2i(draw->x, draw->y);
    glMultiTexCoord2f(GL_TEXTURE0, 1, top_uv); glVertex2i(right, draw->y);
    glMultiTexCoord2f(GL_TEXTURE0, 1, bottom_uv); glVertex2i(right, top);
    glMultiTexCoord2f(GL_TEXTURE0, 0, bottom_uv); glVertex2i(draw->x, top);
    glEnd();
    read_buffer(GL_BACK);
    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    glPixelStorei(GL_PACK_ROW_LENGTH, 0);
    glPixelStorei(GL_PACK_SKIP_ROWS, 0);
    glPixelStorei(GL_PACK_SKIP_PIXELS, 0);
    glReadPixels(0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
    assert(glGetError() == GL_NO_ERROR);
    ptr_glUseProgram(0);
    glDeleteTextures(3, textures);
    delete_program(program);
    ptr_glDeleteShader(fragment);
    gpu_gl_invalidate_state();
}

/*
 * Damon V. Caskey
 * 2026-11-15
 * Compare bilinear output directly with the preserved old shader on the same
 * driver. This separates hardware interpolation precision from a phase or
 * conversion-order regression while covering both window and screen geometry.
 */
static void movie_test_legacy_equivalence(void)
{
    const int rectangles[][4] = {{3, 1, 19, 27}, {-4, -3, 23, 17}, {0, 0, 37, 29}};
    const float corrections[][2] = {{0, 0}, {1, 0}, {-1, 0}, {0.5f, 0.25f}, {0, -0.5f}};
    movie_fixture fixture;
    s_screen *target = movie_screen(37, 29);
    unsigned legacy[MOVIE_TARGET_PIXELS], native[MOVIE_TARGET_PIXELS];
    void (APIENTRY *read_buffer)(GLenum) = SDL_GL_GetProcAddress("glReadBuffer");
    int rectangle, correction;
    movie_pattern(&fixture, 6, 10, 0);
    assert(read_buffer);
    for(rectangle = 0; rectangle < 3; ++rectangle)
        for(correction = 0; correction < 5; ++correction)
        {
            gpu_movie_draw draw = {rectangles[rectangle][0], rectangles[rectangle][1],
                rectangles[rectangle][2], rectangles[rectangle][3], DC_RESAMPLE_MOVIE, 0,
                corrections[correction][0], corrections[correction][1]};
            movie_legacy_reference(&fixture.source, &draw, 37, 29, 0, legacy);
            assert(gpu_draw_box(target, 0, 0, 37, 29, MOVIE_BACKGROUND, &plainmethod));
            assert(gpu_draw_yuv(target, &fixture.source, &draw));
            gpu_sync_cpu(target);
            /* Pass nearest here to require the tighter one-byte comparison,
             * not the analytic bilinear allowance for driver interpolation. */
            movie_verify_pixels((const unsigned *)target->data, legacy, 37, 29, 0,
                                 "old hardware shader screen equivalence", DC_RESAMPLE_NEAREST);
            movie_legacy_reference(&fixture.source, &draw, 37, 29, 1, legacy);
            assert(gpu_gl_present_yuv(&fixture.source, &draw, 37, 29));
            read_buffer(GL_BACK);
            glReadPixels(0, 0, 37, 29, GL_RGBA, GL_UNSIGNED_BYTE, native);
            movie_verify_pixels(native, legacy, 37, 29, 0,
                                 "old hardware shader fullscreen equivalence", DC_RESAMPLE_NEAREST);
        }
    gpu_release_yuv(fixture.source.owner);
    movie_release_screen(target);
}

/*
 * Damon V. Caskey
 * 2026-11-15
 * Alternate direct window rendering and native targets without stale program,
 * framebuffer, viewport, clipping or sampler state. Both paths share uploads.
 */
static void movie_test_window(void)
{
    movie_fixture fixture;
    s_screen *target = movie_screen(19, 13);
    gpu_movie_draw draw = {3, 5, 23, 17, DC_RESAMPLE_LANCZOS_3, 0, 0.5f, -0.25f};
    unsigned pixels[MOVIE_TARGET_PIXELS], expected[MOVIE_TARGET_PIXELS];
    gpu_renderer_stats before;
    void (APIENTRY *read_buffer)(GLenum) = SDL_GL_GetProcAddress("glReadBuffer");
    movie_pattern(&fixture, 6, 10, 35);
    movie_compare(target, &fixture.source, &draw, "native draw before window");
    before = *gpu_gl_get_stats();
    /* Invalidate the external state deliberately rather than querying it. */
    gpu_gl_invalidate_state();
    glViewport(1, 2, 3, 4);
    glEnable(GL_SCISSOR_TEST);
    glScissor(0, 0, 1, 1);
    glEnable(GL_BLEND);
    assert(gpu_gl_present_yuv(&fixture.source, &draw, 64, 64));
    assert(gpu_is_active());
    assert(gpu_gl_get_stats()->movie_uploads == before.movie_uploads);
    assert(gpu_gl_get_stats()->readbacks == before.readbacks);
    assert(read_buffer);
    read_buffer(GL_BACK);
    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    glPixelStorei(GL_PACK_ROW_LENGTH, 0);
    glPixelStorei(GL_PACK_SKIP_ROWS, 0);
    glPixelStorei(GL_PACK_SKIP_PIXELS, 0);
    glReadPixels(0, 0, 64, 64, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
    movie_expected_window(&fixture.source, &draw, 64, 64, expected);
    movie_verify_pixels(pixels, expected, 64, 64, 0, "direct window movie and black borders", draw.resample);
    draw.x = 1; draw.y = 2; draw.width = 15; draw.height = 9;
    draw.resample = DC_RESAMPLE_NEAREST; draw.gamma = draw.brightness = 0;
    movie_compare(target, &fixture.source, &draw, "native draw after window");
    assert(gpu_draw_box(target, 0, 0, 1, 1, 0x00314253, &plainmethod));
    gpu_sync_cpu(target);
    assert(((unsigned *)target->data)[0] == 0x00314253);
    assert(gpu_gl_get_stats()->movie_uploads == before.movie_uploads);
    gpu_release_yuv(fixture.source.owner);
    movie_release_screen(target);
}

/*
 * Damon V. Caskey
 * 2026-11-15
 * One-pixel window axes have no legacy viewport-1 span. Their explicit pixel
 * edges must still render every resampler without division by zero or errors.
 */
static void movie_test_tiny_window(void)
{
    const int sizes[][2] = {{1, 1}, {1, 7}, {9, 1}};
    movie_fixture fixture;
    unsigned pixels[MOVIE_TARGET_PIXELS], expected[MOVIE_TARGET_PIXELS];
    void (APIENTRY *read_buffer)(GLenum) = SDL_GL_GetProcAddress("glReadBuffer");
    int index, mode;
    assert(read_buffer);
    movie_pattern(&fixture, 6, 10, 15);
    for(index = 0; index < 3; ++index)
        for(mode = DC_RESAMPLE_NEAREST; mode <= DC_RESAMPLE_LANCZOS_3; ++mode)
        {
            int width = sizes[index][0], height = sizes[index][1];
            gpu_movie_draw draw = {0, 0, width, height, (e_drawmethod_resample)mode, 0, 0, 0};
            movie_expected_window(&fixture.source, &draw, width, height, expected);
            assert(gpu_gl_present_yuv(&fixture.source, &draw, width, height));
            read_buffer(GL_BACK);
            glReadPixels(0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
            assert(glGetError() == GL_NO_ERROR);
            movie_verify_pixels(pixels, expected, width, height, 0, "one-pixel viewport axis", mode);
        }
    gpu_release_yuv(fixture.source.owner);
}

/*
 * Damon V. Caskey
 * 2026-11-15
 * Real context replacement retires plane resources, preserves live RGB32
 * screens, and permits retained decoded frames to upload again. Backend loss
 * never revives a CPU conversion path or mutates the destination shadow.
 */
static void movie_test_context(SDL_Window *window, SDL_GLContext *context)
{
    movie_fixture fixture;
    s_screen *target = movie_screen(17, 13);
    gpu_movie_draw draw = {0, 0, 17, 13, DC_RESAMPLE_MOVIE, 0, 0, 0};
    unsigned expected[MOVIE_TARGET_PIXELS];
    gpu_renderer_stats before;
    movie_pattern(&fixture, 6, 8, 51);
    movie_expected(&fixture.source, &draw, 17, 13, expected);
    assert(gpu_draw_yuv(target, &fixture.source, &draw));
    assert(((unsigned *)target->data)[0] == 0);
    before = *gpu_gl_get_stats();
    gpu_gl_shutdown();
    assert(!gpu_is_active());
    assert(gpu_gl_get_stats()->movie_resources == 0);
    assert(gpu_gl_get_stats()->readbacks == before.readbacks+1);
    movie_verify_pixels((const unsigned *)target->data, expected, 17, 13, 0,
                         "context retirement preserves live movie screen", draw.resample);
    before = *gpu_gl_get_stats();
    assert(!gpu_draw_yuv(target, &fixture.source, &draw));
    assert(gpu_gl_get_stats()->readbacks == before.readbacks);
    assert(!gpu_allow_cpu_operation(target, GPU_OPERATION_SCREEN));
    movie_verify_pixels((const unsigned *)target->data, expected, 17, 13, 0,
                         "backend loss leaves captured pixels intact", draw.resample);
    gpu_release_yuv(fixture.source.owner);
    SDL_GL_DeleteContext(*context);
    *context = SDL_GL_CreateContext(window);
    assert(*context);
    assert(LoadGLFunctions());
    assert(gpu_gl_init());
    gpu_set_native_required(1);
    before = *gpu_gl_get_stats();
    movie_compare(target, &fixture.source, &draw, "retained decoder frame after context recreation");
    assert(gpu_gl_get_stats()->movie_uploads == before.movie_uploads+1);
    assert(gpu_gl_get_stats()->movie_resources == before.movie_resources+1);
    gpu_release_yuv(fixture.source.owner);
    movie_release_screen(target);
}

/*
 * Damon V. Caskey
 * 2026-11-15
 * Require a real OpenGL implementation while distinguishing missing platform
 * infrastructure from test failures. This executable has no software fallback.
 */
int main(void)
{
    SDL_Window *window;
    SDL_GLContext context;
    assert(sizeof(void *) == 8);
    if(SDL_Init(SDL_INIT_VIDEO) != 0) return 77;
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_COMPATIBILITY);
    window = SDL_CreateWindow("OpenBOR GPU movie tests", 0, 0, 64, 64,
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
    movie_test_reconstruction();
    movie_test_color();
    movie_test_black();
    movie_test_upload_state();
    movie_test_cache();
    movie_test_rejection();
    movie_test_legacy_equivalence();
    movie_test_window();
    movie_test_tiny_window();
    movie_test_context(window, &context);
    assert(gpu_gl_get_stats()->movie_resources == 0);
    assert(gpu_gl_get_stats()->fallback_draws == 0);
    gpu_gl_shutdown();
    SDL_GL_DeleteContext(context);
    SDL_DestroyWindow(window);
    SDL_Quit();
    printf("GPU movie tests passed: %u complete images, %u RGB pixels, maximum byte error %u; cached native YUV draws without implicit readback.\n",
           movie_images_checked, movie_pixels_checked, movie_maximum_error);
    return 0;
}
