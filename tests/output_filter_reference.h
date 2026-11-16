/*
 * Damon V. Caskey
 * 2026-11-16
 * Independent RGB8 output-filter reference shared by desktop and GLES tests.
 * This test-only arithmetic is not linked into any production renderer.
 */
#ifndef OPENBOR_OUTPUT_FILTER_REFERENCE_H
#define OPENBOR_OUTPUT_FILTER_REFERENCE_H

#include <assert.h>
#include "gfx.h"

/*
 * Damon V. Caskey
 * 2026-11-16
 * Model the logical black border independently of GL texture wrap state.
 */
static unsigned output_filter_sample(const unsigned *source, int width, int height,
                                     int x, int y)
{
    return x < 0 || x >= width || y < 0 || y >= height ? 0 :
           source[y * width + x] & 0x00ffffffu;
}

/*
 * Damon V. Caskey
 * 2026-11-16
 * Truncate each half separately, preserving the original integer kernel while
 * retaining all eight bits of every color channel.
 */
static unsigned output_filter_half(unsigned first, unsigned second)
{
    unsigned result = 0;
    int shift;
    for(shift = 0; shift < 24; shift += 8)
        result |= ((((first >> shift) & 255) / 2 +
                    ((second >> shift) & 255) / 2) << shift);
    return result;
}

/*
 * Damon V. Caskey
 * 2026-11-16
 * Calculate one output pixel from named source neighbors and integer channel
 * operations. Dot-mask entries select red, green and blue by ordinary bits.
 */
static unsigned output_filter_expected(const unsigned *source, int width, int height,
                                       int filter, int x, int y)
{
    static const unsigned dot_channels[16] = {
        2, 1, 4, 0, 7, 0, 7, 0, 4, 0, 2, 1, 7, 0, 7, 0
    };
    unsigned center, right, above, below, left, result = 0;
    int sx = x / 2, sy = y / 2, shift;
    if(filter == BLITTER_NONE)
        return output_filter_sample(source, width, height, x, y);
    center = output_filter_sample(source, width, height, sx, sy);
    right = output_filter_sample(source, width, height, sx + 1, sy);
    switch(filter)
    {
    case BLITTER_BILINEAR:
        if(!(x & 1) && !(y & 1)) return center;
        if(!(y & 1)) return output_filter_half(center, right);
        below = output_filter_sample(source, width, height, sx, sy + 1);
        if(!(x & 1)) return output_filter_half(center, below);
        return output_filter_half(output_filter_half(center, right),
                                  output_filter_half(below, output_filter_sample(
                                      source, width, height, sx + 1, sy + 1)));
    case BLITTER_ADMAME2X:
        left = output_filter_sample(source, width, height, sx - 1, sy);
        above = output_filter_sample(source, width, height, sx, sy > 0 ? sy - 1 : sy);
        below = output_filter_sample(source, width, height, sx, sy + 1 < height ? sy + 1 : sy);
        if(above == below || left == right) return center;
        if(!(y & 1)) return (x & 1) ? (right == above ? right : center) :
                                                   (left == above ? left : center);
        return (x & 1) ? (right == below ? right : center) :
                        (left == below ? left : center);
    case BLITTER_SCANLINES:
        return (y & 1) ? 0 : center;
    case BLITTER_SCANLINESTV:
        center = (x & 1) ? output_filter_half(center, right) : center;
        if(!(y & 1)) return center;
        for(shift = 0; shift < 24; shift += 8)
            result |= ((((center >> shift) & 255) / 4 * 3) << shift);
        return result;
    case BLITTER_TV2X:
        if(!(y & 1)) return center;
        for(shift = 0; shift < 24; shift += 8)
            result |= ((((center >> shift) & 255) * 7 / 8) << shift);
        return result;
    case BLITTER_DOTMATRIX:
        for(shift = 0; shift < 24; shift += 8)
        {
            unsigned component = (center >> shift) & 255;
            if(dot_channels[(y % 4) * 4 + x % 4] & (1u << (shift / 8)))
                component -= component / 4;
            result |= component << shift;
        }
        return result;
    default:
        assert(!"invalid oracle filter");
        return 0;
    }
}

#endif
