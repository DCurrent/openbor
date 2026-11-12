/*
 * Compare native GPU transforms with the production software rasterizer.
 *
 * Build with the same SDL/OpenGL flags as test_gpu_renderer.c, linking:
 *   tests/test_gpu_transform_gl.c engine/source/gamelib/gpu.c
 *   engine/source/gamelib/gpu_shader.c engine/source/gamelib/gpu_transform.c
 *   engine/source/gamelib/transform.c engine/source/gamelib/pixelformat.c
 *   engine/source/gamelib/draw32.c engine/source/omath.c
 *   engine/sdl/gpu_renderer.c engine/sdl/loadgl.c
 * Use -ffunction-sections -fdata-sections -Wl,--gc-sections and -lm.
 * Run with SDL_VIDEODRIVER=offscreen when a headless Mesa driver is available.
 */

#include <assert.h>
#include <math.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <SDL.h>
#include "gpu.h"
#include "gpu_renderer.h"
#include "loadgl.h"
#include "transform.h"
#include "draw.h"

const s_drawmethod plainmethod = {
    .config = DRAWMETHOD_CONFIG_ENABLED,
    .alpha = -1, .remap = -1,
    .scalex = 256, .scaley = 256,
    .xrepeat = 1, .yrepeat = 1,
    .channelr = 255, .channelg = 255, .channelb = 255,
    .object_type = OBJECT_TYPE_DRAWMETHOD
};

static unsigned plane_drift_corrections;
static unsigned rotation_bounds_corrections;
static unsigned rotation_sample_corrections;
static unsigned compared_draws;
static unsigned compared_lines;

/*
 * Damon V. Caskey
 * 2026-11-12
 *
 * Independently evaluate rotation's existing angle/flip convention without
 * legacy coefficient truncation or its inconsistent nonuniform-scale bounds.
 */
static unsigned canonical_rotation_pixel(const s_screen *source, const s_drawmethod *method,
                                         int x, int y, int dest_x, int dest_y)
{
    double zx = method->scalex / 256.0, zy = method->scaley / 256.0;
    int degrees = (int)fmod(method->rotate, 360.0f), source_x, source_y;
    double angle, sine, cosine, u, v, nearest;
    if(method->config & DRAWMETHOD_CONFIG_FLIP_X) zx = -zx;
    else degrees = -degrees;
    if(method->config & DRAWMETHOD_CONFIG_FLIP_Y) { zy = -zy; degrees = -degrees; }
    angle = degrees * (3.14159265358979323846 / 180.0);
    sine = sin(angle); cosine = cos(angle);
    if(degrees % 90 == 0) { sine = round(sine); cosine = round(cosine); }
    u = method->centerx + (dest_x-x)*cosine/zx - (dest_y-y)*sine/zy;
    v = method->centery + (dest_x-x)*sine/zx + (dest_y-y)*cosine/zy;
    nearest = floor(u + 0.5);
    if(fabs(u-nearest) < 0.0001) u = nearest;
    nearest = floor(v + 0.5);
    if(fabs(v-nearest) < 0.0001) v = nearest;
    source_x = (int)floor(u); source_y = (int)floor(v);
    if(source_x < 0 || source_x >= source->width || source_y < 0 || source_y >= source->height)
        return 0;
    return ((const unsigned *)source->data)[source_y*source->width + source_x] & 0x00ffffffu;
}

/*
 * Damon V. Caskey
 * 2026-11-12
 *
 * Independently evaluate the intended plane row mapping. Preserve vertical
 * recurrence, but evaluate horizontal position directly instead of repeatedly
 * adding floats. Snap only the documented 0.0001-texel integer-boundary band.
 */
static unsigned canonical_plane_pixel(const s_screen *source, const s_screen *target,
                                      const s_drawmethod *method, int x, int y,
                                      int dest_x, int dest_y)
{
    int origin_x = x - method->centerx, origin_y = y - method->centery;
    int left = origin_x < 0 ? 0 : origin_x;
    int right = origin_x + source->width;
    int row = dest_y - origin_y, i, source_x, source_y;
    float size = method->water.beginsize, sy = 0.0f;
    float factor = (method->water.endsize - size) / source->height;
    float step, start;
    double position, nearest;
    if(right > target->width) right = target->width;
    if(dest_x < left || dest_x >= right || row < 0 || row >= source->height) return 0;
    for(i = 0; i < row; ++i, size += factor)
    {
        step = 1.0f / size;
        sy += method->water.perspective == WATER_PERSPECTIVE_TILE ? step
            : method->water.perspective == WATER_PERSPECTIVE_STRETCH
              ? step * source->height / source->width : 1.0f;
    }
    step = 1.0f / size;
    start = method->water.wavetime - origin_x + source->width
          - (target->width / 2.0f - left) * step;
    position = (double)start + (dest_x - left) * (double)step;
    nearest = floor(position + 0.5);
    if(fabs(position - nearest) < 0.0001) position = nearest;
    source_x = (int)position % source->width;
    source_y = (int)sy % source->height;
    if(source_x < 0) source_x += source->width;
    if(source_y < 0) source_y += source->height;
    return ((const unsigned *)source->data)[source_y * source->width + source_x] & 0x00ffffffu;
}

/*
 * Damon V. Caskey
 * 2026-11-12
 *
 * Retain the engine allocation contract in this standalone test executable.
 */
void *checkAlloc(void *pointer, size_t size, const char *function, const char *file, int line)
{
    (void)size; (void)function; (void)file; (void)line;
    assert(pointer);
    return pointer;
}

/*
 * Damon V. Caskey
 * 2026-11-12
 *
 * Allocate a small RGB source or target with the real engine's memory layout.
 */
static s_screen *make_screen(int width, int height)
{
    s_screen *screen = calloc(1, sizeof(*screen) + (size_t)width * height * 4);
    assert(screen);
    screen->magic = screen_magic;
    screen->width = width;
    screen->height = height;
    screen->pixelformat = PIXEL_32;
    return screen;
}

/*
 * Damon V. Caskey
 * 2026-11-12
 *
 * Compare one real software draw with the GPU draw, printing exact context on
 * failure. Rotation may reveal valid pixels formerly cut off by its old bounds.
 */
static void compare_draw(s_screen *source, s_screen *reference, s_screen *native,
                         s_drawmethod *method, int family, int x, int y)
{
    gfx_entry entry;
    int pixel, differences = 0;
    entry.screen = source;
    gpu_prepare_cpu_write(reference);
    memset(reference->data, 0, (size_t)reference->width * reference->height * 4);
    assert(gpu_clear_screen(native));
    drawmethod_global_init(method);
    if(family == 0) gfx_draw_scale(reference, &entry, x, y, 0, 0, method);
    else if(family == 1) gfx_draw_water(reference, &entry, x, y, 0, 0, method);
    else if(family == 2) gfx_draw_plane(reference, &entry, x, y, 0, 0, method);
    else gfx_draw_rotate(reference, &entry, x, y, 0, 0, method);

    assert(gpu_draw_screen(native, source, x, y, method));
    gpu_sync_cpu(native);
    for(pixel = 0; pixel < reference->width * reference->height; ++pixel)
    {
        unsigned expected = ((unsigned *)reference->data)[pixel] & 0x00ffffffu;
        unsigned actual = ((unsigned *)native->data)[pixel] & 0x00ffffffu;
        if(family == 2)
        {
            unsigned canonical = canonical_plane_pixel(source, reference, method, x, y,
                                 pixel % reference->width, pixel / reference->width);
            if(expected != canonical) ++plane_drift_corrections;
            expected = canonical;
        }
        else if(family == 3)
        {
            unsigned canonical = canonical_rotation_pixel(source, method, x, y,
                                 pixel % reference->width, pixel / reference->width);
            if(expected != canonical)
            {
                if(expected == 0) ++rotation_bounds_corrections;
                else ++rotation_sample_corrections;
            }
            expected = canonical;
        }
        if(expected != actual)
        {
            if(differences < 5)
                fprintf(stderr, "family%d at(%d,%d) scale(%d,%d) shift%d flip%d pivot(%d,%d) "
                        "clip(%d,%d,%d,%d) rotate%.1f pixel(%d,%d) expected%06x actual%06x\n",
                        family, x, y, method->scalex, method->scaley, method->shiftx,
                        method->config, method->centerx, method->centery,
                        method->clipx, method->clipy, method->clipw, method->cliph, method->rotate,
                        pixel % reference->width, pixel / reference->width, expected, actual);
            ++differences;
        }
    }
    assert(differences == 0);
    ++compared_draws;
}

/*
 * Damon V. Caskey
 * 2026-11-12
 *
 * Exercise all flips, varied fixed-point scales/shears, and both target and
 * drawmethod clipping against the actual legacy pixel loop.
 */
static void test_scale(s_screen *source, s_screen *reference, s_screen *native)
{
    const int scales[] = {64, 128, 192, 256, 384, 512, 640};
    const int shears[] = {0, 128, -96};
    int scale, shear, flip, location;
    for(scale = 0; scale < 7; ++scale)
        for(shear = 0; shear < 3; ++shear)
            for(flip = 0; flip < 4; ++flip)
                for(location = 0; location < 4; ++location)
                {
                    s_drawmethod method = plainmethod;
                    method.scalex = scales[scale];
                    method.scaley = scales[(scale + 3) % 7];
                    method.shiftx = shears[shear];
                    method.centerx = 3; method.centery = 2;
                    method.config |= (flip & 1 ? DRAWMETHOD_CONFIG_FLIP_X : 0)
                                   | (flip & 2 ? DRAWMETHOD_CONFIG_FLIP_Y : 0);
                    if(location == 3)
                    {
                        method.clipx = 4; method.clipy = 5;
                        method.clipw = 10; method.cliph = 9;
                    }
                    compare_draw(source, reference, native, &method, 0,
                                 location == 0 ? 24 : location == 1 ? 1 : location == 2 ? 60 : 8,
                                 location == 0 ? 24 : location == 1 ? 1 : location == 2 ? 62 : 8);
                }
}

/*
 * Damon V. Caskey
 * 2026-11-12
 *
 * Verify actual sine-table water output, including cropped source rows.
 */
static void test_water(s_screen *source, s_screen *reference, s_screen *native)
{
    int mode, location, time;
    for(mode = 1; mode <= 2; ++mode)
        for(location = 0; location < 3; ++location)
            for(time = -12; time <= 37; time += 49)
            {
                s_drawmethod method = plainmethod;
                method.water.watermode = (e_water_mode)mode;
                method.water.amplitude = 3;
                method.water.wavelength = 10.5f;
                method.water.wavetime = time;
                compare_draw(source, reference, native, &method, 1,
                             location == 0 ? 20 : location == 1 ? -4 : 60,
                             location == 0 ? 20 : location == 1 ? -3 : 60);
            }
}

/*
 * Damon V. Caskey
 * 2026-11-12
 *
 * Verify the existing plane recurrence in tile/stretch/none perspective modes.
 */
static void test_plane(s_screen *source, s_screen *reference, s_screen *native)
{
    int perspective, location, sizes;
    for(perspective = 0; perspective <= 2; ++perspective)
        for(location = 0; location < 3; ++location)
            for(sizes = 0; sizes < 3; ++sizes)
            {
                s_drawmethod method = plainmethod;
                method.water.watermode = WATER_MODE_SHEAR;
                method.water.beginsize = sizes == 0 ? 1.0f : sizes == 1 ? 0.25f : 3.0f;
                method.water.endsize = sizes == 0 ? 1.0f : sizes == 1 ? 1.75f : 0.25f;
                method.water.perspective = (e_water_perspective)perspective;
                method.water.wavetime = 11;
                compare_draw(source, reference, native, &method, 2,
                             location == 0 ? 20 : location == 1 ? -4 : 60,
                             location == 0 ? 20 : location == 1 ? -3 : 60);
            }
}

/*
 * Damon V. Caskey
 * 2026-11-12
 *
 * Check rotation's established inverse-map orientation with each flip state.
 */
static void test_rotate(s_screen *source, s_screen *reference, s_screen *native)
{
    const int angles[] = {90, 180, 270, 33};
    int angle, flip;
    for(angle = 0; angle < 4; ++angle)
        for(flip = 0; flip < 4; ++flip)
        {
            s_drawmethod method = plainmethod;
            method.rotate = angles[angle];
            method.centerx = 6; method.centery = 4;
            method.scalex = 384; method.scaley = 192;
            method.config |= (flip & 1 ? DRAWMETHOD_CONFIG_FLIP_X : 0)
                           | (flip & 2 ? DRAWMETHOD_CONFIG_FLIP_Y : 0);
            compare_draw(source, reference, native, &method, 3, 28, 28);
        }
}

/*
 * Damon V. Caskey
 * 2026-11-12
 *
 * Compare production Bresenham coverage across octants, both endpoint orders,
 * degenerate points, horizontal/vertical lines, and offscreen clipping. New
 * drawmethod scissoring is independently applied to the legacy reference.
 */
static void test_lines(s_screen *reference, s_screen *native)
{
    const int deltas[] = {-80, -40, -20, -1, 0, 1, 20, 40, 80};
    int dx, dy, direction, clipped, pixel;
    for(dx = 0; dx < 9; ++dx)
        for(dy = 0; dy < 9; ++dy)
            for(direction = 0; direction < 2; ++direction)
                for(clipped = 0; clipped < 2; ++clipped)
                {
                    s_drawmethod method = plainmethod;
                    int sx = direction ? 32 + deltas[dx] : 32;
                    int sy = direction ? 32 + deltas[dy] : 32;
                    int ex = direction ? 32 : 32 + deltas[dx];
                    int ey = direction ? 32 : 32 + deltas[dy];
                    if(clipped)
                    {
                        method.clipx = 16; method.clipy = 16;
                        method.clipw = 32; method.cliph = 32;
                    }
                    gpu_prepare_cpu_write(reference);
                    memset(reference->data, 0, (size_t)reference->width * reference->height * 4);
                    assert(gpu_clear_screen(native));
                    drawmethod_global_init(&method);
                    line32(sx, sy, ex, ey, 0x00abcdefu, reference, 0);
                    assert(gpu_draw_line(native, sx, sy, ex, ey, 0x00abcdefu, &method));
                    gpu_sync_cpu(native);
                    for(pixel = 0; pixel < reference->width * reference->height; ++pixel)
                    {
                        int x = pixel % reference->width, y = pixel / reference->width;
                        unsigned expected = ((unsigned *)reference->data)[pixel] & 0x00ffffffu;
                        unsigned actual = ((unsigned *)native->data)[pixel] & 0x00ffffffu;
                        if(clipped && (x < 16 || x >= 48 || y < 16 || y >= 48)) expected = 0;
                        if(expected != actual)
                        {
                            fprintf(stderr, "line(%d,%d)-(%d,%d) clip%d pixel(%d,%d) "
                                    "expected%06x actual%06x\n", sx, sy, ex, ey, clipped,
                                    x, y, expected, actual);
                            assert(0);
                        }
                    }
                    ++compared_lines;
                }
}

/*
 * Damon V. Caskey
 * 2026-11-12
 *
 * Run production-CPU-versus-native-GPU transform comparisons on a real context.
 */
int main(void)
{
    SDL_Window *window;
    SDL_GLContext context;
    s_screen *source, *reference, *native;
    int pixel;
    if(SDL_Init(SDL_INIT_VIDEO) != 0) return 77;
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_COMPATIBILITY);
    window = SDL_CreateWindow("OpenBOR transform tests", 0, 0, 64, 64, SDL_WINDOW_OPENGL | SDL_WINDOW_HIDDEN);
    context = window ? SDL_GL_CreateContext(window) : NULL;
    if(!context)
    {
        if(window) SDL_DestroyWindow(window);
        SDL_Quit();
        return 77;
    }
    assert(LoadGLFunctions());
    assert(gpu_gl_init());
    pixelformat = PIXEL_32;
    source = make_screen(16, 12);
    reference = make_screen(64, 64);
    native = make_screen(64, 64);
    for(pixel = 0; pixel < source->width * source->height; ++pixel)
        ((unsigned *)source->data)[pixel] = 0x00100000u | (unsigned)(pixel + 1);
    test_scale(source, reference, native);
    test_water(source, reference, native);
    test_plane(source, reference, native);
    test_rotate(source, reference, native);
    test_lines(reference, native);
    gpu_release_screen(source); gpu_release_screen(reference); gpu_release_screen(native);
    free(source); free(reference); free(native);
    gpu_gl_shutdown();
    SDL_GL_DeleteContext(context); SDL_DestroyWindow(window); SDL_Quit();
    printf("GPU transform pixel tests passed: %u draws, %u lines; corrected legacy pixels: "
           "plane boundaries=%u, rotation coverage=%u, rotation sampling=%u.\n",
           compared_draws, compared_lines, plane_drift_corrections,
           rotation_bounds_corrections, rotation_sample_corrections);
    return 0;
}
