/*
 * Damon V. Caskey
 * 2026-11-20
 * Small, test-only RGB sampling oracles. These evaluate each destination
 * sample directly without production rasterizer state, backend callbacks,
 * material processing, sprites, or GPU transform helpers. They deliberately
 * cannot substitute for a scene renderer.
 */
#ifndef OPENBOR_TEST_GPU_TRANSFORM_REFERENCE_H
#define OPENBOR_TEST_GPU_TRANSFORM_REFERENCE_H

enum
{
    REFERENCE_FIXED_ONE = 256,
    REFERENCE_FIXED_RECIPROCAL = REFERENCE_FIXED_ONE * REFERENCE_FIXED_ONE,
    REFERENCE_WAVE_PERIOD = 256
};

/*
 * Damon V. Caskey
 * 2026-11-20
 * Mathematical floor division defines signed fixed-point rounding without
 * relying on implementation-defined right shifts of negative integers.
 */
static int64_t reference_fixed_floor(int64_t value)
{
    int64_t quotient = value / REFERENCE_FIXED_ONE;
    return quotient - (value < 0 && value % REFERENCE_FIXED_ONE != 0);
}

/*
 * Damon V. Caskey
 * 2026-11-20
 * Invert the documented fixed-point row lattice at one destination sample.
 * Integer reciprocal truncation and a clipped right/bottom edge determine
 * sample phase; direct arithmetic avoids the renderer's incremental loops.
 */
static unsigned canonical_scale_pixel(const s_screen *source, const s_screen *target,
                                      const s_drawmethod *method, int x, int y,
                                      int dest_x, int dest_y)
{
    const int64_t unit = REFERENCE_FIXED_ONE;
    int64_t left = 0, top = 0, right = target->width * unit, bottom = target->height * unit;
    int64_t width = (int64_t)source->width * method->scalex;
    int64_t height = (int64_t)source->height * method->scaley;
    int64_t pivot_x = (int64_t)method->centerx * method->scalex;
    int64_t pivot_y = (int64_t)method->centery * method->scaley;
    int flip_x = (method->config & DRAWMETHOD_CONFIG_FLIP_X) != 0;
    int flip_y = (method->config & DRAWMETHOD_CONFIG_FLIP_Y) != 0;
    int64_t shift = flip_y ? -method->shiftx : method->shiftx;
    int64_t origin_x = x * unit + (flip_x ? pivot_x - width : -pivot_x);
    int64_t origin_y = y * unit + (flip_y ? pivot_y - height : -pivot_y);
    int64_t end_y, begin_y, trim_y, row, row_x, end_x, begin_x, column;
    int64_t step_x, step_y, source_x, source_y;

    if(method->scalex <= 0 || method->scaley <= 0) return 0;
    if(method->clipw > 0 && method->cliph > 0)
    {
        if(method->clipx * unit > left) left = method->clipx * unit;
        if(method->clipy * unit > top) top = method->clipy * unit;
        if((method->clipx + method->clipw) * unit < right)
            right = (method->clipx + method->clipw) * unit;
        if((method->clipy + method->cliph) * unit < bottom)
            bottom = (method->clipy + method->cliph) * unit;
    }
    end_y = origin_y + height < bottom ? origin_y + height : bottom;
    begin_y = origin_y > top ? origin_y : top;
    trim_y = origin_y + height - end_y;
    row = reference_fixed_floor(end_y - unit) - dest_y;
    if(row < 0 || end_y - (row + 1) * unit < begin_y) return 0;

    origin_x += reference_fixed_floor((origin_y + height - y * unit) * shift);
    row_x = origin_x - reference_fixed_floor(shift * trim_y) - row * shift;
    end_x = row_x + width < right ? row_x + width : right;
    begin_x = row_x > left ? row_x : left;
    column = reference_fixed_floor(end_x) - 1 - dest_x;
    if(column < 0 || end_x - (column + 1) * unit < begin_x) return 0;

    step_x = REFERENCE_FIXED_RECIPROCAL / method->scalex * (flip_x ? 1 : -1);
    step_y = REFERENCE_FIXED_RECIPROCAL / method->scaley * (flip_y ? 1 : -1);
    source_x = (flip_x ? 0 : source->width * unit + step_x)
        + reference_fixed_floor(step_x * (row_x + width - end_x)) + column * step_x;
    source_y = (flip_y ? 0 : source->height * unit + step_y)
        + reference_fixed_floor(step_y * trim_y) + row * step_y;
    source_x = reference_fixed_floor(source_x);
    source_y = reference_fixed_floor(source_y);
    if(source_x < 0 || source_y < 0 || source_x >= source->width || source_y >= source->height)
        return 0;
    return ((const unsigned *)source->data)[source_y * source->width + source_x] & 0x00ffffffu;
}

/*
 * Damon V. Caskey
 * 2026-11-20
 * Evaluate the quantized sine formula independently of the shader and retired
 * lookup table. Long-double phase arithmetic supplies the mathematical row
 * reference; the calling fixture bounds accepted GPU drift to one X pixel.
 */
static unsigned canonical_water_pixel(const s_screen *source, const s_drawmethod *method,
                                      int x, int y, int dest_x, int dest_y)
{
    const long double tau = 6.283185307179586476925286766559L;
    int source_y = dest_y - y + method->centery;
    int initial = method->water.wavetime % REFERENCE_WAVE_PERIOD;
    int amplitude = method->water.amplitude, offset, displacement, source_x;
    long double phase;
    float sine_factor;

    if(source_y < 0 || source_y >= source->height) return 0;
    if(method->clipw > 0 && method->cliph > 0 &&
       (dest_x < method->clipx || dest_x >= method->clipx + method->clipw ||
        dest_y < method->clipy || dest_y >= method->clipy + method->cliph)) return 0;
    if(initial < 0) initial += REFERENCE_WAVE_PERIOD;
    sine_factor = (float)(1.0L + sinl(tau * initial / REFERENCE_WAVE_PERIOD));
    offset = method->water.watermode == WATER_MODE_SINE ? (int)(sine_factor * amplitude + 0.5) : amplitude;
    phase = fmodl(initial + source_y * (long double)REFERENCE_WAVE_PERIOD / method->water.wavelength,
                  REFERENCE_WAVE_PERIOD);
    if(phase < 0) phase += REFERENCE_WAVE_PERIOD;
    sine_factor = (float)(1.0L + sinl(tau * (int)phase / REFERENCE_WAVE_PERIOD));
    displacement = (int)(sine_factor * amplitude + 0.5) - offset;
    source_x = dest_x - x + method->centerx - displacement;
    if(source_x < 0 || source_x >= source->width) return 0;
    return ((const unsigned *)source->data)[source_y * source->width + source_x] & 0x00ffffffu;
}

/*
 * Damon V. Caskey
 * 2026-11-20
 * Define the existing integer endpoint-clipping convention in isolation.
 * Both endpoints use the original slope, including its integer truncation.
 */
static int reference_clip_endpoint(int *x, int *y, int dx, int dy, int width, int height)
{
    if(*x < 0) { *y -= *x * dy / dx; *x = 0; }
    if(*y < 0) { *x -= *y * dx / dy; *y = 0; }
    if(*x >= width) { *y -= (*x - width) * dy / dx; *x = width - 1; }
    if(*y >= height) { *x -= (*y - height) * dx / dy; *y = height - 1; }
    return *x >= 0 && *x < width && *y >= 0 && *y < height;
}

/*
 * Damon V. Caskey
 * 2026-11-20
 * Enumerate only opaque test-line coverage using direct integer interpolation
 * instead of the production renderer's Bresenham accumulator or draw calls.
 * Endpoint inclusion and the historical half-step tie direction are explicit.
 */
static void reference_line(s_screen *target, int x1, int y1, int x2, int y2, unsigned color)
{
    int dx = x2 - x1, dy = y2 - y1, major, minor, step, x_sign, y_sign;
    if((x1 < 0 && x2 < 0) || (y1 < 0 && y2 < 0) ||
       (x1 >= target->width && x2 >= target->width) ||
       (y1 >= target->height && y2 >= target->height)) return;
    if(!reference_clip_endpoint(&x1, &y1, dx, dy, target->width, target->height) ||
       !reference_clip_endpoint(&x2, &y2, dx, dy, target->width, target->height)) return;
    dx = abs(x2 - x1); dy = abs(y2 - y1);
    x_sign = x2 < x1 ? -1 : 1;
    y_sign = y2 < y1 ? -1 : 1;
    major = dx > dy ? dx : dy;
    minor = dx > dy ? dy : dx;
    for(step = 0; step <= major; ++step)
    {
        int offset = step == 0 ? 0 : (step * minor + major - major / 2) / major;
        int x = x1 + x_sign * (dx > dy ? step : offset);
        int y = y1 + y_sign * (dx > dy ? offset : step);
        if(step == major) { x = x2; y = y2; }
        assert(x >= 0 && x < target->width && y >= 0 && y < target->height);
        ((unsigned *)target->data)[y * target->width + x] = color;
    }
}

#endif
