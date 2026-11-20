/*
 * Damon V. Caskey
 * 2026-11-15
 * Compare real GPU reconstruction with independent long-double kernels and
 * inverse mappings. Expected pixels never call the production sampler or
 * software renderer. Readback occurs only at explicit verification boundaries.
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
#include "gpu_test_context.h"

/* Exercise queue snapshots without providing any software drawing backend. */
#include "../engine/source/gamelib/spriteq.c"

#define RESAMPLE_SOURCE_PIXELS 256
#define RESAMPLE_TARGET_PIXELS 4096
#define RESAMPLE_BACKGROUND 0x0038597bu
#define RESAMPLE_PI 3.141592653589793238462643383279502884L

int pixelbytes[5] = {1, 1, 0, 0, 4};
int pixelformat = PIXEL_32;
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

typedef struct resample_source
{
    int width, height, indexed, masked, centerx, centery;
    unsigned rgb[RESAMPLE_SOURCE_PIXELS], palette[256];
    unsigned char indices[RESAMPLE_SOURCE_PIXELS], opacity[RESAMPLE_SOURCE_PIXELS];
    s_screen *screen;
    s_sprite *sprite, *mask;
} resample_source;

typedef struct resample_value
{
    long double color[3], coverage, opacity;
} resample_value;

static unsigned resample_images_checked;
static unsigned resample_pixels_checked;
static unsigned resample_maximum_error;

/*
 * Damon V. Caskey
 * 2026-11-15
 * Keep production allocations fatal while linking no game logging subsystem.
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
 * Queue dispatch must be accepted by the native renderer in this executable.
 */
void putpixel(unsigned x, unsigned y, int color, s_screen *screen, s_drawmethod *method)
{
    assert(gpu_draw_box(screen, (int)x, (int)y, 1, 1, (unsigned)color, method));
}

/*
 * Damon V. Caskey
 * 2026-11-15
 * Require native acceptance when the production queue dispatches a sprite.
 */
void putsprite(int x, int y, s_sprite *sprite, s_screen *screen, s_drawmethod *method)
{
    assert(gpu_draw_sprite(screen, sprite, x, y, method));
}

/*
 * Damon V. Caskey
 * 2026-11-15
 * Keep queued screens GPU-resident through the production drawing entry point.
 */
void putscreen(s_screen *dest, s_screen *src, int x, int y, s_drawmethod *method)
{
    assert(gpu_draw_screen(dest, src, x, y, method));
}

/*
 * Damon V. Caskey
 * 2026-11-15
 * Route queued line records through the native renderer without a CPU retry.
 */
void putline(int sx, int sy, int ex, int ey, int color, s_screen *screen, s_drawmethod *method)
{
    assert(gpu_draw_line(screen, sx, sy, ex, ey, (unsigned)color, method));
}

/*
 * Damon V. Caskey
 * 2026-11-15
 * Preserve native rectangle coverage for queue background and overlap records.
 */
void putbox(int x, int y, int width, int height, int color, s_screen *screen, s_drawmethod *method)
{
    assert(gpu_draw_box(screen, x, y, width, height, (unsigned)color, method));
}

/*
 * Damon V. Caskey
 * 2026-11-15
 * Use real screen storage while keeping the expected source data independent.
 */
static s_screen *resample_screen(int width, int height, int format, unsigned initial)
{
    size_t bytes = (size_t)width * height * pixelbytes[format];
    s_screen *screen = calloc(1, sizeof(*screen) + bytes + PAL_BYTES);
    int index;
    assert(screen && width * height <= RESAMPLE_TARGET_PIXELS);
    screen->magic = screen_magic;
    screen->width = width;
    screen->height = height;
    screen->pixelformat = format;
    if(format == PIXEL_x8)
        screen->palette = (unsigned char *)screen + offsetof(s_screen, data) + bytes;
    if(format == PIXEL_32)
        for(index = 0; index < width * height; ++index)
            ((unsigned *)screen->data)[index] = initial;
    return screen;
}

/*
 * Damon V. Caskey
 * 2026-11-15
 * Encode complete byte rows. Zero indices remain meaningful source coverage.
 */
static s_sprite *resample_sprite(int width, int height, const unsigned char *pixels)
{
    size_t payload = (size_t)height * (sizeof(int) + width + 3);
    s_sprite *sprite = calloc(1, sizeof(*sprite) + payload + PAL_BYTES);
    unsigned char *encoded;
    int row;
    assert(sprite && width < 255);
    sprite->magic = sprite_magic;
    sprite->width = sprite->srcwidth = width;
    sprite->height = sprite->srcheight = height;
    sprite->pixelformat = PIXEL_x8;
    sprite->palette = (unsigned char *)sprite + sizeof(*sprite) + payload;
    encoded = (unsigned char *)sprite->data + height * sizeof(int);
    for(row = 0; row < height; ++row)
    {
        sprite->data[row] = (int)(encoded - (unsigned char *)&sprite->data[row]);
        encoded[0] = 0; encoded[1] = (unsigned char)width;
        memcpy(encoded + 2, pixels + row * width, (size_t)width);
        encoded[width + 2] = 0xff;
        encoded += width + 3;
    }
    return sprite;
}

/*
 * Damon V. Caskey
 * 2026-11-15
 * Uploadable artwork and the oracle own separate, identical initial samples.
 */
static void resample_prepare_source(resample_source *source, int sprite)
{
    int count = source->width * source->height;
    assert(count <= RESAMPLE_SOURCE_PIXELS);
    if(sprite)
    {
        assert(source->indexed);
        source->sprite = resample_sprite(source->width, source->height, source->indices);
        source->sprite->centerx = source->centerx;
        source->sprite->centery = source->centery;
        memcpy(source->sprite->palette, source->palette, sizeof(source->palette));
        if(source->masked)
        {
            source->mask = resample_sprite(source->width, source->height, source->opacity);
            source->sprite->mask = source->mask;
        }
    }
    else
    {
        source->screen = resample_screen(source->width, source->height,
                                         source->indexed ? PIXEL_x8 : PIXEL_32, 0);
        memcpy(source->screen->data, source->indexed ? (void *)source->indices : source->rgb,
               (size_t)count * (source->indexed ? 1 : sizeof(unsigned)));
        if(source->indexed)
            memcpy(source->screen->palette, source->palette, sizeof(source->palette));
    }
}

/*
 * Damon V. Caskey
 * 2026-11-15
 * Retire resource identities before freeing their allocations.
 */
static void resample_release_source(resample_source *source)
{
    if(source->screen) { gpu_release_screen(source->screen); free(source->screen); }
    if(source->sprite) { gpu_release_sprite(source->sprite); free(source->sprite); }
    if(source->mask) { gpu_release_sprite(source->mask); free(source->mask); }
    source->screen = NULL; source->sprite = source->mask = NULL;
}

/*
 * Damon V. Caskey
 * 2026-11-15
 * Evict a standalone target before its allocation address can be reused.
 */
static void resample_release_screen(s_screen *screen)
{
    gpu_release_screen(screen);
    free(screen);
}

/*
 * Damon V. Caskey
 * 2026-11-15
 * The general Mitchell-Netravali equation is independent of shader-specific
 * optimized polynomials. Lanczos uses the mathematical sinc product directly.
 */
static long double resample_weight(int mode, long double distance)
{
    long double x = fabsl(distance), b = 0, c = 0, radius;
    if(mode == DC_RESAMPLE_NEAREST) return x < 0.5L ? 1 : 0;
    if(mode == DC_RESAMPLE_BILINEAR) return x < 1 ? 1 - x : 0;
    if(mode == DC_RESAMPLE_LANCZOS_2 || mode == DC_RESAMPLE_LANCZOS_3)
    {
        radius = mode == DC_RESAMPLE_LANCZOS_2 ? 2 : 3;
        if(x == 0) return 1;
        if(x >= radius) return 0;
        return sinl(RESAMPLE_PI * x) * sinl(RESAMPLE_PI * x / radius)
             / (RESAMPLE_PI * RESAMPLE_PI * x * x / radius);
    }
    if(mode == DC_RESAMPLE_BICUBIC_CATMULL_ROM) c = 0.5L;
    else if(mode == DC_RESAMPLE_BICUBIC_MITCHELL) b = c = 1.0L / 3;
    else { assert(mode == DC_RESAMPLE_BICUBIC_BSPLINE); b = 1; }
    if(x < 1)
        return ((12 - 9*b - 6*c)*x*x*x + (-18 + 12*b + 6*c)*x*x + 6 - 2*b) / 6;
    if(x < 2)
        return ((-b - 6*c)*x*x*x + (6*b + 30*c)*x*x + (-12*b - 48*c)*x + 8*b + 24*c) / 6;
    return 0;
}

/*
 * Damon V. Caskey
 * 2026-11-15
 * Source taps clamp to the logical image or wrap each axis for tiled planes.
 */
static int resample_address(int value, int size, int wrapped)
{
    if(wrapped) { value %= size; return value < 0 ? value + size : value; }
    return value < 0 ? 0 : value >= size ? size - 1 : value;
}

/*
 * Damon V. Caskey
 * 2026-11-15
 * Bound signed reconstruction results without reducing intermediate precision.
 */
static long double resample_clamp(long double value, long double minimum, long double maximum)
{
    return value < minimum ? minimum : value > maximum ? maximum : value;
}

/*
 * Damon V. Caskey
 * 2026-11-15
 * Quantize the oracle's final channel to the documented nearest RGB byte.
 */
static unsigned resample_byte(long double value)
{
    return (unsigned)floorl(resample_clamp(value, 0, 255) + 0.5L);
}

/*
 * Damon V. Caskey
 * 2026-11-15
 * Resolve each contributing source texel before reconstruction. Indexed zero
 * is a coverage key independently of its palette color; other indices may
 * legitimately resolve to black. Chroma never examines an interpolated index.
 */
static unsigned resample_source_tap(const resample_source *source, const s_drawmethod *method,
                                    int x, int y, int wrapped, long double *coverage,
                                    long double *opacity)
{
    int location, index, slot;
    unsigned color;
    const unsigned *palette = method->table ? (const unsigned *)method->table : source->palette;
    int keyed = source->sprite != NULL ||
                (method->config & DRAWMETHOD_CONFIG_BACKGROUND_TRANSPARENCY);
    x = resample_address(x, source->width, wrapped);
    y = resample_address(y, source->height, wrapped);
    location = y * source->width + x;
    index = source->indices[location];
    color = (source->indexed ? palette[index] : source->rgb[location]) & 0xffffffu;
    *coverage = keyed && (source->indexed ? index == 0 : color == 0) ? 0 : 1;
    *opacity = source->masked ? source->opacity[location] : 255;
    for(slot = 0; slot < DRAWMETHOD_COLOR_SLOTS; ++slot)
        if((method->colors.chroma_enabled & (1u << slot)) &&
           color == (method->colors.chroma[slot] & 0xffffffu)) *coverage = 0;
    return color;
}

/*
 * Damon V. Caskey
 * 2026-11-15
 * Evaluate the full separable reconstruction in wider arithmetic. Clamping
 * addresses does not discard kernel weights at image edges. Masks enter the
 * weighted coverage before color reconstruction, preventing invisible source
 * colors from bleeding through an interpolated opacity boundary.
 */
static resample_value resample_reconstruct(const resample_source *source,
                                           const s_drawmethod *method,
                                           long double u, long double v, int wrapped)
{
    resample_value result = {{0, 0, 0}, 0, 0};
    int mode = method->resample, x, y, channel;
    int radius = mode == DC_RESAMPLE_BILINEAR ? 1 : mode == DC_RESAMPLE_LANCZOS_3 ? 3 : 2;
    long double weights = 0, covered = 0;
    if(mode == DC_RESAMPLE_NEAREST)
    {
        unsigned color = resample_source_tap(source, method, (int)floorl(u + 0.5L),
                                              (int)floorl(v + 0.5L), wrapped,
                                              &result.coverage, &result.opacity);
        for(channel = 0; channel < 3; ++channel) result.color[channel] = (color >> (channel*8)) & 255u;
        return result;
    }
    for(y = (int)ceill(v - radius); y <= (int)floorl(v + radius); ++y)
    {
        long double wy = resample_weight(mode, v-y);
        for(x = (int)ceill(u - radius); x <= (int)floorl(u + radius); ++x)
        {
            long double coverage, opacity, weight = wy * resample_weight(mode, u-x);
            unsigned color;
            if(weight == 0) continue;
            color = resample_source_tap(source, method, x, y, wrapped, &coverage, &opacity);
            if(source->masked) coverage *= opacity / 255;
            weights += weight;
            covered += weight * coverage;
            for(channel = 0; channel < 3; ++channel)
                result.color[channel] += weight * coverage * ((color >> (channel*8)) & 255u);
        }
    }
    assert(fabsl(weights) > 0.000001L);
    result.coverage = resample_clamp(covered / weights, 0, 1);
    if(result.coverage <= 0.000001L)
    {
        result.coverage = 0;
        return result;
    }
    for(channel = 0; channel < 3; ++channel)
        result.color[channel] = resample_byte(result.color[channel] / covered);
    result.opacity = result.coverage * 255;
    return result;
}

/*
 * Damon V. Caskey
 * 2026-11-15
 * Integer equations and table addresses are the documented RGB material
 * contract. This oracle does not call the shader's reference helper routines.
 */
static unsigned resample_blend(int mode, unsigned source, unsigned destination)
{
    unsigned selector, other, value;
    if(mode < 1 || mode > MAX_BLENDINGS) return source;
    if(blendtables[mode-1]) return blendtables[mode-1][source * 256u + destination];
    switch(mode)
    {
        case 1: return 255u - (255u-source)*(255u-destination)/255u;
        case 2: return source*destination/256u;
        case 3:
        case 4:
            selector = mode == 3 ? destination : source;
            other = mode == 3 ? source : destination;
            return selector < 128 ? 2u*selector*other/256u
                                 : 255u-(511u-2u*selector)*(255u-other)/255u;
        case 5:
            value = destination*256u/(256u-source);
            return value > 255 ? 255 : value;
        default: return (source+destination)/2u;
    }
}

/*
 * Damon V. Caskey
 * 2026-11-15
 * Reconstructed color enters existing material arithmetic as RGB bytes.
 * Mask opacity retains the 255 complement and 256 divisor. Other materials
 * composite by reconstructed coverage before final byte substitution.
 */
static unsigned resample_material(const resample_source *source, const s_drawmethod *method,
                                  resample_value sample, unsigned background)
{
    const unsigned opacity[3] = {method->channelr, method->channelg, method->channelb};
    int channel, slot, substitute = 1;
    unsigned result = 0;
    if(sample.coverage <= 0) return background;
    if(source->masked) substitute = sample.opacity > 0;
    else if(method->alpha == 6 && !method->tintmode)
        substitute = method->channelr || method->channelg || method->channelb;
    for(channel = 0; channel < 3; ++channel)
    {
        unsigned destination = (background >> (channel*8)) & 255u;
        unsigned color = method->fillcolor ? (method->fillcolor >> (channel*8)) & 255u
                                           : resample_byte(sample.color[channel]);
        long double value;
        if(source->masked)
            value = floorl((color*sample.opacity + destination*(255-sample.opacity))/256);
        else
        {
            if(method->tintmode)
            {
                color = resample_blend((int)method->tintmode,
                                      (method->tintcolor >> (channel*8)) & 255u, color);
                value = resample_blend(method->alpha, color, destination);
            }
            else if(method->alpha == 6 &&
                    (method->channelr < 255 || method->channelg < 255 || method->channelb < 255))
                value = (color*opacity[channel] + destination*(255u-opacity[channel])) / 256u;
            else
                value = resample_blend(method->alpha, color, destination);
            value = destination + sample.coverage*(value-destination);
        }
        result |= resample_byte(value) << (channel*8);
    }
    if(substitute)
        for(slot = 0; slot < DRAWMETHOD_COLOR_SLOTS; ++slot)
            if((method->colors.substitute_enabled & (1u << slot)) &&
               result == (method->colors.substitute[slot].match & 0xffffffu))
                return method->colors.substitute[slot].replacement & 0xffffffu;
    return result;
}

/*
 * Damon V. Caskey
 * 2026-11-15
 * Solve the published source-to-destination affine transform at each pixel
 * center. Rotation preserves the engine's angle/flip convention while all
 * filtered paths use source-edge coordinates before the final half-texel shift.
 */
static int resample_position(const resample_source *source, const s_screen *target,
                              const s_drawmethod *method, int draw_x, int draw_y,
                              int x, int y, long double *u, long double *v, int *wrapped)
{
    long double sx = method->scalex / 256.0L, sy = method->scaley / 256.0L;
    long double cx = source->centerx + method->centerx, cy = source->centery + method->centery;
    long double px = x + 0.5L - draw_x, py = y + 0.5L - draw_y;
    int source_extent = 1;
    int flipx = (method->config & DRAWMETHOD_CONFIG_FLIP_X) != 0;
    int flipy = (method->config & DRAWMETHOD_CONFIG_FLIP_Y) != 0;
    *wrapped = 0;
    if(method->clipw > 0 && method->cliph > 0 &&
       (x < method->clipx || x >= method->clipx + method->clipw ||
        y < method->clipy || y >= method->clipy + method->cliph)) return 0;
    if(method->config & DRAWMETHOD_CONFIG_AFFINE)
    {
        long double a = method->affine.m00, b = method->affine.m01;
        long double c = method->affine.m10, d = method->affine.m11, determinant;
        if(flipx) { a = -a; c = -c; }
        if(flipy) { b = -b; d = -d; }
        determinant = a*d-b*c;
        assert(determinant != 0);
        px -= method->affine.tx; py -= method->affine.ty;
        *u = cx + (d*px-b*py)/determinant;
        *v = cy + (a*py-c*px)/determinant;
    }
    else if(method->water.watermode)
    {
        int left = draw_x - (int)cx, top = draw_y - (int)cy;
        int row = y-top;
        if(row < 0 || row >= source->height) return 0;
        if(method->water.watermode == WATER_MODE_SHEAR)
        {
            float size = method->water.beginsize, source_y = 0;
            float increment = (method->water.endsize-size)/source->height;
            float step = 0, ystep = 0;
            int cursor;
            if(x < left || x >= left+source->width) return 0;
            for(cursor = 0; cursor <= row; ++cursor)
            {
                step = 1.0f / size;
                ystep = method->water.perspective == WATER_PERSPECTIVE_TILE ? step :
                        method->water.perspective == WATER_PERSPECTIVE_STRETCH ?
                        step*source->height/source->width : 1.0f;
                if(cursor != row) { source_y += ystep; size += increment; }
            }
            *u = method->water.wavetime - left + source->width
               + (x+0.5L-target->width/2.0L)*(long double)step;
            *v = (long double)source_y + 0.5L*ystep;
            *wrapped = 1;
        }
        else
        {
            int initial = method->water.wavetime % 256, phase, anchor, displacement;
            if(initial < 0) initial += 256;
            phase = (int)floorl(fmodl(initial + row*(256.0L/method->water.wavelength), 256));
            anchor = method->water.watermode == WATER_MODE_SINE ?
                (int)floorl((1+sinl(initial*2*RESAMPLE_PI/256))*method->water.amplitude+0.5L)
                : method->water.amplitude;
            displacement = (int)floorl((1+sinl(phase*2*RESAMPLE_PI/256))*method->water.amplitude+0.5L)-anchor;
            *u = x-left-displacement+0.5L;
            *v = row+0.5L;
        }
    }
    else if(method->rotate != 0)
    {
        int degrees = (int)fmodl(method->rotate, 360);
        long double sine, cosine, angle;
        if(flipx) sx = -sx; else degrees = -degrees;
        if(flipy) { sy = -sy; degrees = -degrees; }
        angle = degrees*RESAMPLE_PI/180;
        sine = sinl(angle); cosine = cosl(angle);
        if(degrees % 90 == 0) { sine = roundl(sine); cosine = roundl(cosine); }
        if(method->resample == DC_RESAMPLE_NEAREST) { px -= 0.5L; py -= 0.5L; }
        *u = cx + px*cosine/sx - py*sine/sy;
        *v = cy + px*sine/sx + py*cosine/sy;
    }
    else
    {
        long double width = source->width*method->scalex;
        long double height = source->height*method->scaley;
        long double dy = draw_y*256.0L + (flipy ? cy*method->scaley-height : -cy*method->scaley);
        long double dx = draw_x*256.0L + (flipx ? cx*method->scalex-width : -cx*method->scalex);
        long double shift = flipy ? -method->shiftx : method->shiftx;
        long double xmin = 0, xmax = target->width*256.0L;
        long double ymin = 0, ymax = target->height*256.0L;
        long double begin, end, row, rows, right, count, original_dx, sampling_dx;
        if(method->clipw > 0 && method->cliph > 0)
        {
            xmin = fmaxl(xmin, method->clipx*256.0L);
            xmax = fminl(xmax, (method->clipx+method->clipw)*256.0L);
            ymin = fmaxl(ymin, method->clipy*256.0L);
            ymax = fminl(ymax, (method->clipy+method->cliph)*256.0L);
        }
        dx += floorl((dy+height-draw_y*256.0L)*shift/256);
        original_dx = dx;
        begin = fmaxl(dy, ymin); end = fminl(dy+height, ymax);
        rows = floorl((end-begin)/256);
        row = floorl(end/256)-1-y;
        if(row < 0 || row >= rows) return 0;
        if(dy+height > ymax) dx -= floorl(shift*(dy+height-end)/256);
        dx -= row*shift;
        begin = fmaxl(dx, xmin); end = fminl(dx+width, xmax);
        right = floorl(end/256); count = floorl((end-begin)/256);
        if(x < right-count || x >= right) return 0;
        /* Clipping changes retained coverage, but must not rephase the
         * continuous inverse map relative to its original row recurrence. */
        sampling_dx = original_dx - (floorl((dy+height)/256)-1-y)*shift;
        *u = (flipx ? source->width : 0) + (x+0.5L-sampling_dx/256)*(flipx ? -1 : 1)/sx;
        *v = (flipy ? source->height : 0) + (y+0.5L-dy/256)*(flipy ? -1 : 1)/sy;
        source_extent = 0;
    }
    if(source_extent && !*wrapped && (*u < 0 || *u >= source->width || *v < 0 || *v >= source->height)) return 0;
    *u -= 0.5L; *v -= 0.5L;
    return 1;
}

/*
 * Damon V. Caskey
 * 2026-11-15
 * Generate a complete image including untouched borders and explicit clips.
 */
static void resample_expected(const resample_source *source, const s_screen *target,
                              const s_drawmethod *method, int draw_x, int draw_y,
                              const unsigned *background, unsigned *expected)
{
    int x, y;
    for(y = 0; y < target->height; ++y)
        for(x = 0; x < target->width; ++x)
        {
            int index = y*target->width+x, wrapped;
            long double u, v;
            expected[index] = background ? background[index] : RESAMPLE_BACKGROUND;
            if(resample_position(source, target, method, draw_x, draw_y, x, y, &u, &v, &wrapped))
                expected[index] = resample_material(source, method,
                    resample_reconstruct(source, method, u, v, wrapped), expected[index]);
        }
}

/*
 * Damon V. Caskey
 * 2026-11-15
 * Compare RGB bytes only. One-byte differences bound float reconstruction and
 * final quantization; nearest and exact material fixtures demand exact output.
 */
static void resample_verify(s_screen *target, const unsigned *expected, int tolerance,
                            const char *label, int mode)
{
    int index, channel, failures = 0;
    gpu_sync_cpu(target);
    for(index = 0; index < target->width*target->height; ++index)
    {
        unsigned actual = ((unsigned *)target->data)[index] & 0xffffffu;
        int mismatch = 0;
        for(channel = 0; channel < 3; ++channel)
        {
            unsigned a = (actual >> (channel*8)) & 255u;
            unsigned b = (expected[index] >> (channel*8)) & 255u;
            unsigned error = a > b ? a-b : b-a;
            if(error > resample_maximum_error) resample_maximum_error = error;
            if(error > (unsigned)tolerance) mismatch = 1;
        }
        if(mismatch && failures++ < 8)
            fprintf(stderr, "%s filter %d at (%d,%d): expected %06x, actual %06x\n",
                    label, mode, index%target->width, index/target->width, expected[index], actual);
    }
    assert(failures == 0);
    ++resample_images_checked;
    resample_pixels_checked += (unsigned)(target->width*target->height);
}

/*
 * Damon V. Caskey
 * 2026-11-15
 * Each comparison proves both pixels and ownership: the draw may not read back,
 * update the CPU shadow, or enter the software fallback path.
 */
static void resample_compare(const resample_source *source, s_screen *target,
                             const s_drawmethod *method, int x, int y,
                             const char *label, int tolerance)
{
    unsigned expected[RESAMPLE_TARGET_PIXELS], shadow[RESAMPLE_TARGET_PIXELS];
    gpu_renderer_stats before;
    assert(gpu_draw_box(target, 0, 0, target->width, target->height, RESAMPLE_BACKGROUND, &plainmethod));
    resample_expected(source, target, method, x, y, NULL, expected);
    memcpy(shadow, target->data, (size_t)target->width*target->height*sizeof(unsigned));
    before = *gpu_gl_get_stats();
    if(source->sprite) assert(gpu_draw_sprite(target, source->sprite, x, y, method));
    else assert(gpu_draw_screen(target, source->screen, x, y, method));
    assert(gpu_gl_get_stats()->readbacks == before.readbacks);
    assert(gpu_gl_get_stats()->fallback_draws == before.fallback_draws);
    assert(memcmp(shadow, target->data, (size_t)target->width*target->height*sizeof(unsigned)) == 0);
    resample_verify(target, expected, tolerance, label, method->resample);
}

/*
 * Damon V. Caskey
 * 2026-11-15
 * High-frequency independent channels distinguish kernels and make filtering
 * numeric palette indices produce obviously incorrect colors.
 */
static void resample_pattern(resample_source *source, int indexed)
{
    int x, y;
    memset(source, 0, sizeof(*source));
    source->width = 7; source->height = 5; source->indexed = indexed;
    for(y = 0; y < source->height; ++y)
        for(x = 0; x < source->width; ++x)
        {
            int index = y*source->width+x;
            unsigned color = (unsigned)((19+83*x+17*y)%256)
                           | (unsigned)((233+23*x+71*y)%256) << 8
                           | (unsigned)((91+139*x+43*y)%256) << 16;
            source->rgb[index] = source->palette[index+1] = color;
            source->indices[index] = (unsigned char)(index+1);
        }
}

/*
 * Damon V. Caskey
 * 2026-11-15
 * Exercise every formula during enlargement and reduction, indexed palette
 * lookup, reflected pivots, rotation, affine transforms, clipping and shear.
 */
static void resample_test_kernels(void)
{
    resample_source source;
    s_screen *target = resample_screen(37, 29, PIXEL_32, 0x00abcdef);
    int indexed, mode, pose;
    assert(DC_RESAMPLE_MOVIE == DC_RESAMPLE_BILINEAR);
    assert(plainmethod.resample == DC_RESAMPLE_NEAREST);
    for(indexed = 0; indexed < 2; ++indexed)
    {
        resample_pattern(&source, indexed);
        resample_prepare_source(&source, 0);
        for(mode = DC_RESAMPLE_NEAREST; mode <= DC_RESAMPLE_LANCZOS_3; ++mode)
            for(pose = 0; pose < (mode ? 9 : 2); ++pose)
            {
                s_drawmethod method = plainmethod;
                method.resample = (e_drawmethod_resample)mode;
                method.centerx = 2; method.centery = 1;
                method.scalex = pose == 0 ? 256 : pose == 1 ? 512 : 384;
                method.scaley = pose == 0 ? 256 : pose == 1 ? 512 : 320;
                if(pose == 2) { method.scalex = 192; method.scaley = 160; }
                if(pose == 3 || pose == 4) method.config |= DRAWMETHOD_CONFIG_FLIP_X;
                if(pose == 4) method.config |= DRAWMETHOD_CONFIG_FLIP_Y;
                if(pose == 5) method.rotate = 29;
                if(pose == 6)
                {
                    method.config |= DRAWMETHOD_CONFIG_AFFINE | DRAWMETHOD_CONFIG_FLIP_Y;
                    method.affine = (affine_transform){1.5f, -0.375f, 0.25f, 1.25f, 0.3125f, -0.1875f};
                }
                if(pose == 7 || pose == 8)
                {
                    method.shiftx = pose == 7 ? 96 : -128;
                    method.scaley = 384;
                    if(pose == 8) method.config |= DRAWMETHOD_CONFIG_FLIP_X | DRAWMETHOD_CONFIG_FLIP_Y;
                }
                if(pose >= 3)
                {
                    method.clipx = 8; method.clipy = 7;
                    method.clipw = 13; method.cliph = 13;
                }
                resample_compare(&source, target, &method, 14, 13,
                                 indexed ? "indexed reconstruction and transform" : "RGB reconstruction and transform",
                                 mode ? 1 : 0);
            }
        resample_release_source(&source);
    }
    resample_release_screen(target);
}

/*
 * Damon V. Caskey
 * 2026-11-15
 * Wave identity sampling and each wrapped plane perspective use the shared
 * reconstruction rules. Negative scroll and clipping expose wrap-phase errors.
 */
static void resample_test_rows(void)
{
    resample_source source;
    s_screen *target = resample_screen(23, 17, PIXEL_32, 0);
    int mode, effect;
    resample_pattern(&source, 1);
    resample_prepare_source(&source, 0);
    for(mode = DC_RESAMPLE_BILINEAR; mode <= DC_RESAMPLE_LANCZOS_3; ++mode)
        for(effect = 0; effect < 5; ++effect)
        {
            s_drawmethod method = plainmethod;
            method.resample = (e_drawmethod_resample)mode;
            method.centerx = 1; method.centery = 1;
            method.clipx = 5; method.clipy = 6; method.clipw = 10; method.cliph = 4;
            if(effect < 2)
            {
                method.water.watermode = (e_water_mode)(effect+1);
                method.water.amplitude = 3;
                method.water.wavelength = 8;
                method.water.wavetime = 37;
            }
            else
            {
                method.water.watermode = WATER_MODE_SHEAR;
                method.water.beginsize = 0.75f; method.water.endsize = 1.25f;
                method.water.perspective = (e_water_perspective)(effect-2);
                method.water.wavetime = -11;
            }
            resample_compare(&source, target, &method, 8, 6, "water and wrapped planes", 1);
        }
    resample_release_source(&source);
    resample_release_screen(target);
}

/*
 * Damon V. Caskey
 * 2026-11-15
 * Integral plane scrolling must not round away the fractional filter phase.
 * Compare both signs with the independent continuous formula and an equivalent
 * scroll reduced by whole texture periods. Two-million-texel scrolls also
 * exercise native row geometry beyond the compact sampler's precision range.
 */
static void resample_test_plane_scroll(void)
{
    static const int scrolls[] = {9999, -9999, 1000000, -1000000, 2000000, -2000000};
    static const int modes[] = {DC_RESAMPLE_BILINEAR, DC_RESAMPLE_LANCZOS_3};
    resample_source source;
    s_screen *target = resample_screen(23, 17, PIXEL_32, 0);
    unsigned periodic[RESAMPLE_TARGET_PIXELS];
    int mode, scroll, index, channel;
    resample_pattern(&source, 1);
    resample_prepare_source(&source, 0);
    for(mode = 0; mode < (int)(sizeof(modes)/sizeof(modes[0])); ++mode)
        for(scroll = 0; scroll < (int)(sizeof(scrolls)/sizeof(scrolls[0])); ++scroll)
        {
            s_drawmethod method = plainmethod;
            char label[80];
            method.resample = (e_drawmethod_resample)modes[mode];
            method.centerx = method.centery = 1;
            method.water.watermode = WATER_MODE_SHEAR;
            method.water.beginsize = 0.7f; method.water.endsize = 1.1f;
            method.water.perspective = WATER_PERSPECTIVE_TILE;
            method.water.wavetime = scrolls[scroll] % source.width;
            if(method.water.wavetime < 0) method.water.wavetime += source.width;
            resample_compare(&source, target, &method, 8, 6, "plane scroll modulo texture width", 1);
            for(index = 0; index < target->width*target->height; ++index)
                periodic[index] = ((unsigned *)target->data)[index] & 0xffffffu;

            method.water.wavetime = scrolls[scroll];
            snprintf(label, sizeof(label), "continuous plane scroll %d", scrolls[scroll]);
            resample_compare(&source, target, &method, 8, 6, label, 1);
            for(index = 0; index < target->width*target->height; ++index)
                for(channel = 0; channel < 3; ++channel)
                {
                    int actual = (((unsigned *)target->data)[index] >> (channel*8)) & 255u;
                    int repeated = (periodic[index] >> (channel*8)) & 255u;
                    if(abs(actual-repeated) > 1)
                    {
                        fprintf(stderr, "%s filter %d at (%d,%d): periodic channel %d, actual %d\n",
                                label, modes[mode], index%target->width, index/target->width, repeated, actual);
                        assert(abs(actual-repeated) <= 1);
                    }
                }
        }
    resample_release_source(&source);
    resample_release_screen(target);
}

/*
 * Damon V. Caskey
 * 2026-11-15
 * Combine reconstructed coverage with palette remaps, source keys, fill,
 * tint, channel opacity and destination blends. Transparent colored samples
 * and valid black palette entries specifically guard against color fringes.
 */
static void resample_test_materials(void)
{
    static const unsigned char indices[12] = {0,1,2,3, 3,2,0,1, 1,3,2,0};
    static const unsigned char opacities[12] = {255,128,0,64, 0,255,64,128, 128,64,255,0};
    resample_source source;
    s_screen *target = resample_screen(17, 13, PIXEL_32, 0);
    unsigned remap[256] = {0};
    int mode, policy, masked;
    memset(&source, 0, sizeof(source));
    source.width = 4; source.height = 3; source.indexed = 1;
    source.palette[0] = 0xffffff;
    source.palette[1] = 0;
    source.palette[2] = 0x20c040;
    source.palette[3] = 0xe040a0;
    remap[0] = 0xffffff; remap[1] = 0; remap[2] = 0xd03090; remap[3] = 0x18b8e8;
    memcpy(source.indices, indices, sizeof(indices));
    memcpy(source.opacity, opacities, sizeof(opacities));
    for(masked = 0; masked < 2; ++masked)
    {
        source.masked = masked;
        source.centerx = masked ? 1 : 0;
        source.centery = masked ? 1 : 0;
        resample_prepare_source(&source, masked);
        for(mode = DC_RESAMPLE_BILINEAR; mode <= DC_RESAMPLE_LANCZOS_3; ++mode)
            for(policy = 0; policy < 6; ++policy)
            {
                s_drawmethod method = plainmethod;
                method.resample = (e_drawmethod_resample)mode;
                method.scalex = 512; method.scaley = 512;
                if(policy >= 1) method.config |= DRAWMETHOD_CONFIG_BACKGROUND_TRANSPARENCY;
                if(policy == 2)
                {
                    method.table = (unsigned char *)remap;
                    method.tintmode = 2; method.tintcolor = 0x92c7ef;
                    method.alpha = 6;
                }
                if(policy == 3)
                {
                    method.colors.chroma_enabled = 1u << 2;
                    method.colors.chroma[2] = source.palette[2];
                    method.fillcolor = 0x71319d;
                }
                if(policy == 4)
                {
                    method.alpha = 6;
                    method.channelr = 17; method.channelg = 128; method.channelb = 251;
                }
                if(policy == 5)
                {
                    method.table = (unsigned char *)remap;
                    method.colors.chroma_enabled = 1;
                    method.colors.chroma[0] = remap[3];
                    method.alpha = 6; method.tintmode = 1; method.tintcolor = 0xffffff;
                    method.channelr = method.channelg = method.channelb = 0;
                }
                resample_compare(&source, target, &method, 6, 4,
                                 masked ? "filtered sprite mask precedence" : "filtered palette and materials", 1);
            }
        resample_release_source(&source);
    }
    resample_release_screen(target);
}

/*
 * Damon V. Caskey
 * 2026-11-15
 * RGB zero and colored chroma keys contribute no color to neighboring samples.
 * Black remains visible when zero-key coverage is disabled, and fill replaces
 * only source color after the independently reconstructed coverage decision.
 */
static void resample_test_rgb_keys(void)
{
    resample_source source;
    s_screen *target = resample_screen(15, 9, PIXEL_32, 0);
    int mode, policy;
    memset(&source, 0, sizeof(source));
    source.width = 4; source.height = 2;
    source.rgb[0] = 0; source.rgb[1] = 0xff0000; source.rgb[2] = 0x00c040; source.rgb[3] = 0;
    source.rgb[4] = 0xff0000; source.rgb[5] = 0; source.rgb[6] = 0; source.rgb[7] = 0x00c040;
    resample_prepare_source(&source, 0);
    for(mode = DC_RESAMPLE_BILINEAR; mode <= DC_RESAMPLE_LANCZOS_3; ++mode)
        for(policy = 0; policy < 4; ++policy)
        {
            s_drawmethod method = plainmethod;
            method.resample = (e_drawmethod_resample)mode;
            method.scalex = method.scaley = 512;
            if(policy >= 1) method.config |= DRAWMETHOD_CONFIG_BACKGROUND_TRANSPARENCY;
            if(policy >= 2)
            {
                method.colors.chroma_enabled = 1;
                method.colors.chroma[0] = 0xff0000;
            }
            if(policy == 3) method.fillcolor = 0x9251cf;
            resample_compare(&source, target, &method, 3, 2, "RGB keys before reconstruction and fill", 1);
        }
    resample_release_source(&source);
    resample_release_screen(target);
}

/*
 * Damon V. Caskey
 * 2026-11-15
 * Known bilinear quarter-phase results make final Substitute exact and prove
 * that custom byte tables receive reconstructed RGB bytes in the right order.
 */
static void resample_test_substitute_and_tables(void)
{
    resample_source source;
    s_screen *target = resample_screen(13, 5, PIXEL_32, 0);
    s_drawmethod method = plainmethod;
    unsigned background[RESAMPLE_TARGET_PIXELS], expected[RESAMPLE_TARGET_PIXELS];
    unsigned char *table = malloc(256u*256u);
    unsigned index, src, dst;
    memset(&source, 0, sizeof(source));
    source.width = 4; source.height = 1;
    source.rgb[0] = source.rgb[2] = 0x202020;
    source.rgb[1] = source.rgb[3] = 0xa0a0a0;
    resample_prepare_source(&source, 0);
    for(index = 0; index < (unsigned)(target->width*target->height); ++index) background[index] = 0x808080;
    method.resample = DC_RESAMPLE_BILINEAR;
    method.scalex = 512;
    method.alpha = 6;
    method.colors.substitute_enabled = 3;
    method.colors.substitute[0] = (s_color_substitute){0x606060, 0x010203};
    method.colors.substitute[1] = (s_color_substitute){0x010203, 0xfefdfc};
    resample_expected(&source, target, &method, 3, 2, background, expected);
    assert(expected[2*target->width+4] == 0x010203);
    assert(gpu_draw_box(target, 0, 0, target->width, target->height, 0x808080, &plainmethod));
    assert(gpu_draw_screen(target, source.screen, 3, 2, &method));
    resample_verify(target, expected, 0, "Substitute after interpolation and blend without cascade", method.resample);

    assert(table);
    for(src = 0; src < 256; ++src)
        for(dst = 0; dst < 256; ++dst) table[src*256+dst] = (unsigned char)((3*src+dst+7)%256);
    blendtables[1] = table;
    gpu_invalidate_tables();
    method.alpha = 2; method.colors.substitute_enabled = 0;
    resample_compare(&source, target, &method, 3, 2, "filtered custom blend-table addressing", 0);
    blendtables[1] = NULL;
    gpu_invalidate_tables();
    free(table);
    resample_release_source(&source);
    resample_release_screen(target);
}

/*
 * Damon V. Caskey
 * 2026-11-15
 * Fractional bottom clipping must not change a surviving source sample's
 * continuous phase. The oracle keeps the original shear recurrence separate
 * from the retained legacy clipping coverage.
 */
static void resample_test_clipped_phase(void)
{
    resample_source source;
    s_screen *target = resample_screen(24, 15, PIXEL_32, 0);
    int flip, shift, clipped;
    resample_pattern(&source, 0);
    resample_prepare_source(&source, 0);
    for(flip = 0; flip < 4; ++flip)
        for(shift = -1; shift <= 1; ++shift)
            for(clipped = 0; clipped < 2; ++clipped)
            {
                s_drawmethod method = plainmethod;
                method.resample = DC_RESAMPLE_BICUBIC_CATMULL_ROM;
                method.scalex = 352; method.scaley = 352;
                method.shiftx = shift*73;
                method.centerx = 2; method.centery = 1;
                if(flip & 1) method.config |= DRAWMETHOD_CONFIG_FLIP_X;
                if(flip & 2) method.config |= DRAWMETHOD_CONFIG_FLIP_Y;
                if(clipped)
                {
                    method.clipx = 8; method.clipy = 4; method.clipw = 10; method.cliph = 8;
                }
                resample_compare(&source, target, &method, 12, 10, "fractional clipped phase and reflected pivots", 1);
            }
    resample_release_source(&source);
    resample_release_screen(target);
}

/*
 * Damon V. Caskey
 * 2026-11-15
 * Queued filters are captured by value. A GPU-authored source whose CPU shadow
 * is deliberately stale must feed filtered captures without any readback.
 */
static void resample_test_queue_and_gpu_source(void)
{
    resample_source source;
    s_screen *target = resample_screen(19, 15, PIXEL_32, 0);
    s_drawmethod method = plainmethod, captured;
    unsigned expected[RESAMPLE_TARGET_PIXELS];
    unsigned long readbacks, uploads;
    int x, y;
    resample_pattern(&source, 0);
    source.screen = resample_screen(source.width, source.height, PIXEL_32, 0);
    readbacks = gpu_gl_get_stats()->readbacks;
    for(y = 0; y < source.height; ++y)
        for(x = 0; x < source.width; ++x)
            assert(gpu_draw_box(source.screen, x, y, 1, 1, source.rgb[y*source.width+x], &plainmethod));
    for(x = 0; x < source.width*source.height; ++x) assert(((unsigned *)source.screen->data)[x] == 0);
    method.resample = DC_RESAMPLE_BICUBIC_MITCHELL;
    method.scalex = method.scaley = 512;
    captured = method;
    spriteq_unlock(); spriteq_clear();
    spriteq_add_screen(2, 2, 7, source.screen, &method, 0);
    method.resample = DC_RESAMPLE_NEAREST;
    method.scalex = 256;
    assert(gpu_draw_box(target, 0, 0, target->width, target->height, RESAMPLE_BACKGROUND, &plainmethod));
    uploads = gpu_gl_get_stats()->uploads;
    spriteq_draw(target, 0, 7, 7, 0, 0);
    assert(gpu_gl_get_stats()->readbacks == readbacks);
    assert(gpu_gl_get_stats()->uploads == uploads);
    for(x = 0; x < source.width*source.height; ++x) assert(((unsigned *)source.screen->data)[x] == 0);
    resample_expected(&source, target, &captured, 2, 2, NULL, expected);
    resample_verify(target, expected, 1, "queued filter snapshot and GPU-authored source", captured.resample);
    spriteq_clear();
    resample_release_source(&source);
    resample_release_screen(target);
}

/*
 * Damon V. Caskey
 * 2026-11-15
 * An overlapping filtered self-copy must gather every tap from one completed
 * source snapshot. Reading pixels already overwritten by the current draw or
 * copying only the destination footprint would corrupt this expectation.
 */
static void resample_test_self_copy(void)
{
    resample_source source;
    unsigned expected[RESAMPLE_TARGET_PIXELS];
    int mode, x, y;
    resample_pattern(&source, 0);
    source.rgb[8] = source.rgb[23] = 0;
    source.screen = resample_screen(source.width, source.height, PIXEL_32, 0);
    for(mode = DC_RESAMPLE_BILINEAR; mode <= DC_RESAMPLE_LANCZOS_3; ++mode)
    {
        s_drawmethod method = plainmethod;
        gpu_renderer_stats before;
        for(y = 0; y < source.height; ++y)
            for(x = 0; x < source.width; ++x)
                assert(gpu_draw_box(source.screen, x, y, 1, 1, source.rgb[y*source.width+x], &plainmethod));
        method.resample = (e_drawmethod_resample)mode;
        method.config |= DRAWMETHOD_CONFIG_BACKGROUND_TRANSPARENCY;
        method.scalex = 384; method.scaley = 320;
        resample_expected(&source, source.screen, &method, 1, 0, source.rgb, expected);
        before = *gpu_gl_get_stats();
        assert(gpu_draw_screen(source.screen, source.screen, 1, 0, &method));
        assert(gpu_gl_get_stats()->readbacks == before.readbacks);
        assert(gpu_gl_get_stats()->uploads == before.uploads);
        assert(gpu_gl_get_stats()->destination_copies == before.destination_copies+1);
        assert(gpu_gl_get_stats()->fallback_draws == before.fallback_draws);
        resample_verify(source.screen, expected, 1, "overlapping filtered self-copy", mode);
    }
    resample_release_source(&source);
}

/*
 * Damon V. Caskey
 * 2026-11-15
 * Filter changes must invalidate material state without changing resident
 * source images. Repeating one mode may reuse state; switching back to nearest
 * must restore its exact sampling behavior.
 */
static void resample_test_state_switches(void)
{
    const int modes[] = {DC_RESAMPLE_BILINEAR, DC_RESAMPLE_BICUBIC_MITCHELL,
                         DC_RESAMPLE_LANCZOS_3, DC_RESAMPLE_NEAREST,
                         DC_RESAMPLE_BILINEAR, DC_RESAMPLE_BILINEAR};
    resample_source source;
    s_screen *target = resample_screen(19, 15, PIXEL_32, 0);
    unsigned long uploads = 0;
    int index;
    resample_pattern(&source, 0);
    resample_prepare_source(&source, 0);
    for(index = 0; index < (int)(sizeof(modes)/sizeof(modes[0])); ++index)
    {
        s_drawmethod method = plainmethod;
        method.resample = (e_drawmethod_resample)modes[index];
        method.scalex = method.scaley = 512;
        resample_compare(&source, target, &method, 2, 2, "sampler state switches", index == 3 ? 0 : 1);
        if(index == 0) uploads = gpu_gl_get_stats()->uploads;
        else assert(gpu_gl_get_stats()->uploads == uploads);
    }
    resample_release_source(&source);
    resample_release_screen(target);
}

/*
 * Damon V. Caskey
 * 2026-11-15
 * Distinguish unavailable graphics infrastructure from a failed rendering
 * assertion. This executable links no CPU pixel renderer or software fallback.
 */
int main(void)
{
    SDL_Window *window;
    SDL_GLContext context;
    assert(sizeof(void *) == 8);
    if(SDL_Init(SDL_INIT_VIDEO) != 0) return 77;
    gpu_test_context_attributes();
    window = SDL_CreateWindow("OpenBOR reconstruction tests", 0, 0, 64, 64,
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
    resample_test_kernels();
    resample_test_rows();
    resample_test_plane_scroll();
    resample_test_materials();
    resample_test_rgb_keys();
    resample_test_substitute_and_tables();
    resample_test_clipped_phase();
    resample_test_queue_and_gpu_source();
    resample_test_self_copy();
    resample_test_state_switches();
    assert(gpu_gl_get_stats()->fallback_draws == 0);
    gpu_gl_shutdown();
    SDL_GL_DeleteContext(context);
    SDL_DestroyWindow(window);
    SDL_Quit();
    printf("GPU reconstruction tests passed: %u complete images, %u RGB pixels, maximum byte error %u; native draws without implicit readback.\n",
           resample_images_checked, resample_pixels_checked, resample_maximum_error);
    return 0;
}
