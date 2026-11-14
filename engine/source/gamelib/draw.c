/*
 * OpenBOR - http://www.chronocrash.com
 * -----------------------------------------------------------------------
 * All rights reserved, see LICENSE in OpenBOR root for details.
 *
 * Copyright (c)  OpenBOR Team
 */

// Primitive drawing functions. Should really be done in ASM...
// Last update: 24-jun-2002

#include <stdlib.h>
#include "types.h"
#include "draw.h"
#include "gpu.h"
#include "engine_profile.h"
#include "drawmethod_color.h"

#ifndef		NULL
#define		NULL	 ((void*)0)
#endif


#define		abso(x)		(x<0?-x:x)


// Not a particularly fast line function, but it works, and clips!
void line(int sx, int sy, int ex, int ey, int colour, s_screen *screen, int alpha)
{

    int diffx, diffy;
    int absdiffx, absdiffy;
    int xdir, ydir;
    int thres;
    int d;
    unsigned char *lut;

    // Some off-screen lines may slip through this test!
    if(sx < 0 && ex < 0)
    {
        return;
    }
    if(sy < 0 && ey < 0)
    {
        return;
    }
    if(sx >= screen->width && ex >= screen->width)
    {
        return;
    }
    if(sy >= screen->height && ey >= screen->height)
    {
        return;
    }


    // Check clipping and calculate new coords if necessary

    diffx = ex - sx;
    diffy = ey - sy;

    if(sx < 0)
    {
        sy -= (sx * diffy / diffx);
        sx = 0;
    }
    if(sy < 0)
    {
        sx -= (sy * diffx / diffy);
        sy = 0;
    }
    if(sx >= screen->width)
    {
        sy -= ((sx - screen->width) * diffy / diffx);
        sx = screen->width - 1;
    }
    if(sy >= screen->height)
    {
        sx -= ((sy - screen->height) * diffx / diffy);
        sy = screen->height - 1;
    }

    if(ex < 0)
    {
        ey -= (ex * diffy / diffx);
        ex = 0;
    }
    if(ey < 0)
    {
        ex -= (ey * diffx / diffy);
        ey = 0;
    }
    if(ex >= screen->width)
    {
        ey -= ((ex - screen->width) * diffy / diffx);
        ex = screen->width - 1;
    }
    if(ey >= screen->height)
    {
        ex -= ((ey - screen->height) * diffx / diffy);
        ey = screen->height - 1;
    }


    // Second test: the lines that passed test 1 won't pass this time!
    if(sx < 0 || ex < 0)
    {
        return;
    }
    if(sy < 0 || ey < 0)
    {
        return;
    }
    if(sx >= screen->width || ex >= screen->width)
    {
        return;
    }
    if(sy >= screen->height || ey >= screen->height)
    {
        return;
    }


    // Recalculate directions
    diffx = ex - sx;
    diffy = ey - sy;

    absdiffx = abso(diffx);
    absdiffy = abso(diffy);

    sy *= screen->width;
    ey *= screen->width;

    gpu_prepare_cpu_write(screen);
    lut = alpha > 0 ? blendtables[alpha - 1] : NULL;

    if(lut)
    {
        lut += (colour << 8);
    }

    if(absdiffx > absdiffy)
    {
        // Draw a flat line
        thres = absdiffx >> 1;
        xdir = 1;
        if(diffx < 0)
        {
            xdir = -xdir;
        }
        ydir = screen->width;
        if(diffy < 0)
        {
            ydir = -ydir;
        }
        while(sx != ex)
        {
            d = sx + sy;
            screen->data[d] = (lut && screen->data[d]) ? (lut[screen->data[d]]) : colour;
            sx += xdir;
            if((thres -= absdiffy) <= 0)
            {
                sy += ydir;
                thres += absdiffx;
            }
        }
        d = ex + ey;
        screen->data[d] = (lut && screen->data[d]) ? (lut[screen->data[d]]) : colour;
        return;
    }

    // Draw a high line
    thres = absdiffy >> 1;
    xdir = 1;
    if(diffx < 0)
    {
        xdir = -1;
    }
    ydir = screen->width;
    if(diffy < 0)
    {
        ydir = -ydir;
    }
    while(sy != ey)
    {
        d = sx + sy;
        screen->data[d] = (lut && screen->data[d]) ? (lut[screen->data[d]]) : colour;;
        sy += ydir;
        if((thres -= absdiffx) <= 0)
        {
            sx += xdir;
            thres += absdiffy;
        }
    }
    d = ex + ey;
    screen->data[d] = (lut && screen->data[d]) ? (lut[screen->data[d]]) : colour;
}





void drawbox(int x, int y, int width, int height, int colour, s_screen *screen, int alpha)
{
    unsigned char *cp;
    unsigned char *lut;

    if(width <= 0)
    {
        return;
    }
    if(height <= 0)
    {
        return;
    }
    if(screen == NULL)
    {
        return;
    }

    if(x < 0)
    {
        if((width += x) <= 0)
        {
            return;
        }
        x = 0;
    }
    else if(x >= screen->width)
    {
        return;
    }
    if(y < 0)
    {
        if((height += y) <= 0)
        {
            return;
        }
        y = 0;
    }
    else if(y >= screen->height)
    {
        return;
    }
    if(x + width > screen->width)
    {
        width = screen->width - x;
    }
    if(y + height > screen->height)
    {
        height = screen->height - y;
    }

    gpu_prepare_cpu_write(screen);
    cp = (unsigned char *)screen->data + y * screen->width + x;
    lut = alpha > 0 ? blendtables[alpha - 1] : NULL;
    if(lut)
    {
        lut += (colour << 8);
    }
    while(--height >= 0)
    {
        for(x = 0; x < width; x++)
        {
            *cp = (lut && *cp) ? (lut[((int)(*cp)) & 0xFF]) : colour;
            ++cp;
        }
        cp += screen->width - width;
    }
}



// Putpixel used by circle function
void _putpixel(int x, int y, int colour, s_screen *screen, int alpha)
{
    int pixind;
    unsigned char *lut;
    if((unsigned)x >= (unsigned)screen->width || (unsigned)y >= (unsigned)screen->height)
    {
        return;
    }
    gpu_prepare_cpu_write(screen);
    pixind = x + y * screen->width;
    lut = alpha > 0 ? blendtables[alpha - 1] + (colour << 8) : NULL;
    screen->data[pixind] = (lut && screen->data[pixind]) ? (lut[(int)(screen->data[pixind]) & 0xFF]) : colour;
}




// Code to draw a circle.
// I ripped this, not sure how it works...
// It seems it divides the circle into 8 parts, which are drawn
// simultaneously.
// Not much optimization, though, since every pixel is clipped
// separately.

void circle(int x, int y, int rad, int col, s_screen *screen, int alpha)
{
    int cx = 0;				// 'Circle X'
    int cy = rad;				// 'Circle Y'
    int df = 1 - rad;
    int d_e = 3;
    int d_se = -2 * rad + 5;

    do
    {
        _putpixel(x + cx, y + cy, col, screen, alpha);
        if(cx)
        {
            _putpixel(x - cx, y + cy, col, screen, alpha);
        }
        if(cy)
        {
            _putpixel(x + cx, y - cy, col, screen, alpha);
        }
        if(cx && cy)
        {
            _putpixel(x - cx, y - cy, col, screen, alpha);
        }

        if(cx != cy)
        {
            _putpixel(x + cy, y + cx, col, screen, alpha);
            if(cx)
            {
                _putpixel(x + cy, y - cx, col, screen, alpha);
            }
            if(cy)
            {
                _putpixel(x - cy, y + cx, col, screen, alpha);
            }
            if(cx && cy)
            {
                _putpixel(x - cy, y - cx, col, screen, alpha);
            }
        }

        if(df < 0)
        {
            df   += d_e;
            d_e  += 2;
            d_se += 2;
        }
        else
        {
            df   += d_se;
            d_e  += 2;
            d_se += 4;
            --cy;
        }
        ++cx;
    }
    while(cx <= cy);
}



static int draw_init( s_drawmethod *drawmethod)
{
    int alpha = 0;
    drawmethod_global_init(drawmethod);

    if (drawmethod && drawmethod->config & DRAWMETHOD_CONFIG_ENABLED)
    {
        alpha = drawmethod->alpha;
    }

    return alpha;
}

//======================== root methods ==================================

/*
 * Damon V. Caskey
 * 2026-11-14
 * Bound opt-in primitive writes by the target and explicit drawmethod clip.
 * Wider arithmetic prevents a distant clip rectangle from wrapping into the
 * visible image. The existing primitive paths remain untouched when disabled.
 */
static int color_primitive_bounds(const s_screen *screen, const s_drawmethod *method,
                                  int *left, int *top, int *right, int *bottom)
{
    s64 l = 0, t = 0, r = screen->width, b = screen->height;

    /* A nonpositive clip size retains the normal unclipped drawmethod rule. */
    if(method->clipw > 0 && method->cliph > 0)
    {
        if(method->clipx > l) l = method->clipx;
        if(method->clipy > t) t = method->clipy;
        if((s64)method->clipx + method->clipw < r) r = (s64)method->clipx + method->clipw;
        if((s64)method->clipy + method->cliph < b) b = (s64)method->clipy + method->cliph;
    }
    if(l >= r || t >= b)
    {
        return 0;
    }
    *left = (int)l;
    *top = (int)t;
    *right = (int)r;
    *bottom = (int)b;
    return 1;
}

/*
 * Damon V. Caskey
 * 2026-11-14
 * Composite one covered RGB32 primitive pixel before applying Substitute.
 * Fully zero channel opacity retains the old byte blend without repairing
 * its result, so the new option cannot add color to transparent coverage.
 */
static void color_primitive_pixel(unsigned *destination, unsigned source,
                                  blend32fp blend, const s_drawmethod *method)
{
    unsigned result = blend ? blend(source, *destination & 0x00ffffffu) : source;
    int contributes = method->tintmode != 0 || method->alpha != 6 ||
                      method->channelr || method->channelg || method->channelb;

    /* Existing blend helpers already quantize their result to channel bytes. */
    result &= 0x00ffffffu;
    *destination = contributes ? drawmethod_substitute_color(method, result) : result;
}

/*
 * Damon V. Caskey
 * 2026-11-14
 * Draw a clipped solid rectangle through the RGB32 color-rule contract.
 * Primitive chroma compares the supplied color, not a dummy source texel or
 * the image-only fillcolor field. No separate cleanup pass touches holes.
 */
static void color_primitive_box(int x, int y, int width, int height, unsigned color,
                                s_screen *screen, const s_drawmethod *method, int alpha)
{
    int left, top, right, bottom, row, column;
    s64 r = (s64)x + width, b = (s64)y + height;
    blend32fp blend;

    color &= 0x00ffffffu;
    if(width <= 0 || height <= 0 || drawmethod_chroma_matches(method, color) ||
       !color_primitive_bounds(screen, method, &left, &top, &right, &bottom))
    {
        return;
    }
    if(x > left) left = x;
    if(y > top) top = y;
    if(r < right) right = (int)r;
    if(b < bottom) bottom = (int)b;
    if(left >= right || top >= bottom)
    {
        return;
    }

    /* Resolve tint and blend once, then visit only the covered rectangle. */
    blend = getblendfunction32(alpha);
    for(row = top; row < bottom; ++row)
    {
        unsigned *destination = (unsigned *)screen->data + (size_t)row * screen->width + left;
        for(column = left; column < right; ++column, ++destination)
        {
            color_primitive_pixel(destination, color, blend, method);
        }
    }
}

/*
 * Damon V. Caskey
 * 2026-11-14
 * Clip line endpoints with the same edge order and integer rounding as the
 * native renderer. Retaining that order preserves its covered pixels before
 * an explicit drawmethod clip is applied during the bounded raster walk.
 */
static int color_primitive_clip_line(int *x1, int *y1, int *x2, int *y2, int width, int height)
{
    s64 sx = *x1, sy = *y1, ex = *x2, ey = *y2;
    s64 dx = ex - sx, dy = ey - sy;
    if((sx < 0 && ex < 0) || (sy < 0 && ey < 0) ||
       (sx >= width && ex >= width) || (sy >= height && ey >= height))
    {
        return 0;
    }

    /* Each edge rejects a parallel line before division. */
    if(sx < 0)
    {
        if(!dx) return 0;
        sy -= sx * dy / dx;
        sx = 0;
    }
    if(sy < 0)
    {
        if(!dy) return 0;
        sx -= sy * dx / dy;
        sy = 0;
    }
    if(sx >= width)
    {
        if(!dx) return 0;
        sy -= (sx - width) * dy / dx;
        sx = width - 1;
    }
    if(sy >= height)
    {
        if(!dy) return 0;
        sx -= (sy - height) * dx / dy;
        sy = height - 1;
    }
    if(ex < 0)
    {
        if(!dx) return 0;
        ey -= ex * dy / dx;
        ex = 0;
    }
    if(ey < 0)
    {
        if(!dy) return 0;
        ex -= ey * dx / dy;
        ey = 0;
    }
    if(ex >= width)
    {
        if(!dx) return 0;
        ey -= (ex - width) * dy / dx;
        ex = width - 1;
    }
    if(ey >= height)
    {
        if(!dy) return 0;
        ex -= (ey - height) * dx / dy;
        ey = height - 1;
    }
    if(sx < 0 || sy < 0 || ex < 0 || ey < 0 ||
       sx >= width || ex >= width || sy >= height || ey >= height)
    {
        return 0;
    }
    *x1 = (int)sx;
    *y1 = (int)sy;
    *x2 = (int)ex;
    *y2 = (int)ey;
    return 1;
}

/*
 * Damon V. Caskey
 * 2026-11-14
 * Preserve integer line coverage while applying color rules at each covered
 * pixel. Endpoint inclusion and threshold ties match the native unit quads.
 */
static void color_primitive_line(int sx, int sy, int ex, int ey, unsigned color,
                                 s_screen *screen, const s_drawmethod *method, int alpha)
{
    int left, top, right, bottom, dx, dy, step_x, step_y, threshold;
    blend32fp blend;

    color &= 0x00ffffffu;
    if(drawmethod_chroma_matches(method, color) ||
       !color_primitive_bounds(screen, method, &left, &top, &right, &bottom) ||
       !color_primitive_clip_line(&sx, &sy, &ex, &ey, screen->width, screen->height))
    {
        return;
    }
    blend = getblendfunction32(alpha);
    dx = abs(ex - sx);
    dy = abs(ey - sy);
    step_x = ex < sx ? -1 : 1;
    step_y = ey < sy ? -1 : 1;
    threshold = (dx > dy ? dx : dy) >> 1;

    /* Test the drawmethod clip after viewport clipping so it cannot change
     * the line's raster origin or shift interior pixels at a clip boundary. */
    for(;;)
    {
        /* The legacy dominant-axis walk stops before its endpoint. A final
         * threshold tie may advance the other axis, so append the authored
         * endpoint explicitly rather than continuing from that extra step. */
        if((dx > dy && sx == ex) || (dx <= dy && sy == ey))
        {
            sx = ex;
            sy = ey;
        }
        if(sx >= left && sx < right && sy >= top && sy < bottom)
        {
            unsigned *destination = (unsigned *)screen->data + (size_t)sy * screen->width + sx;
            color_primitive_pixel(destination, color, blend, method);
        }
        if(sx == ex && sy == ey) break;
        if(dx > dy)
        {
            sx += step_x;
            if((threshold -= dy) <= 0)
            {
                sy += step_y;
                threshold += dx;
            }
        }
        else
        {
            sy += step_y;
            if((threshold -= dx) <= 0)
            {
                sx += step_x;
                threshold += dy;
            }
        }
    }
}

/*
 * Damon V. Caskey
 * 2026-11-12
 * Draw a filled rectangle through the native or format-specific CPU path.
 * Time software fallback from synchronization through completed rasterization.
 */
void putbox(int x, int y, int width, int height, int colour, s_screen *screen, s_drawmethod *drawmethod)
{
    int alpha = draw_init(drawmethod);
    ep_scope software;

    if(drawmethod_colors_active(drawmethod) && !gpu_require_color_target(screen, drawmethod))
    {
        return;
    }

    if(gpu_draw_box(screen, x, y, width, height, (unsigned)colour, drawmethod))
    {
        return;
    }
    software = engine_profile_span_begin();
    engine_profile_count(EP_COUNT_SOFTWARE_DRAWS, 1);
    gpu_prepare_cpu_write(screen);

    if(drawmethod_colors_active(drawmethod))
    {
        engine_profile_count(EP_COUNT_COLOR_SOFTWARE_DRAWS, 1);
        color_primitive_box(x, y, width, height, (unsigned)colour, screen, drawmethod, alpha);
        engine_profile_span_end(software, EP_SPAN_SOFTWARE);
        return;
    }

    switch(screen->pixelformat)
    {
    case PIXEL_8:
        drawbox(x, y, width, height, colour, screen, alpha);
        break;
    case PIXEL_16:
        drawbox16(x, y, width, height, colour, screen, alpha);
        break;
    case PIXEL_32:
        drawbox32(x, y, width, height, colour, screen, alpha);
        break;
    }
    engine_profile_span_end(software, EP_SPAN_SOFTWARE);
}

/*
 * Damon V. Caskey
 * 2026-11-12
 * Draw a line while exposing the complete CPU fallback cost. The lower-level
 * pixel loops remain inside this one timed software dispatch.
 */
void putline(int sx, int sy, int ex, int ey, int colour, s_screen *screen, s_drawmethod *drawmethod)
{
    int alpha = draw_init(drawmethod);
    ep_scope software;

    if(drawmethod_colors_active(drawmethod) && !gpu_require_color_target(screen, drawmethod))
    {
        return;
    }

    if(gpu_draw_line(screen, sx, sy, ex, ey, (unsigned)colour, drawmethod))
    {
        return;
    }
    software = engine_profile_span_begin();
    engine_profile_count(EP_COUNT_SOFTWARE_DRAWS, 1);
    gpu_prepare_cpu_write(screen);

    if(drawmethod_colors_active(drawmethod))
    {
        engine_profile_count(EP_COUNT_COLOR_SOFTWARE_DRAWS, 1);
        color_primitive_line(sx, sy, ex, ey, (unsigned)colour, screen, drawmethod, alpha);
        engine_profile_span_end(software, EP_SPAN_SOFTWARE);
        return;
    }

    switch(screen->pixelformat)
    {
    case PIXEL_8:
        line(sx, sy, ex, ey, colour, screen, alpha);
        break;
    case PIXEL_16:
        line16(sx, sy, ex, ey, colour, screen, alpha);
        break;
    case PIXEL_32:
        line32(sx, sy, ex, ey, colour, screen, alpha);
        break;
    }
    engine_profile_span_end(software, EP_SPAN_SOFTWARE);
}

/*
 * Damon V. Caskey
 * 2026-11-12
 * Draw one in-bounds pixel and measure any software fallback. Clipped pixels
 * leave both renderers untouched and do not count as software dispatches.
 */
void putpixel(unsigned x, unsigned y, int colour, s_screen *screen, s_drawmethod *drawmethod)
{
    int alpha = draw_init(drawmethod);
    ep_scope software;

    if(drawmethod_colors_active(drawmethod) && !gpu_require_color_target(screen, drawmethod))
    {
        return;
    }

    /* Reject unsigned off-screen coordinates before the rectangle API's
     * signed conversion. Single pixels share its native drawing operation. */
    if(x >= (unsigned)screen->width || y >= (unsigned)screen->height)
    {
        return;
    }
    if(gpu_draw_box(screen, (int)x, (int)y, 1, 1, (unsigned)colour, drawmethod))
    {
        return;
    }
    software = engine_profile_span_begin();
    engine_profile_count(EP_COUNT_SOFTWARE_DRAWS, 1);
    gpu_prepare_cpu_write(screen);
    if(drawmethod_colors_active(drawmethod))
    {
        engine_profile_count(EP_COUNT_COLOR_SOFTWARE_DRAWS, 1);
        color_primitive_box((int)x, (int)y, 1, 1, (unsigned)colour, screen, drawmethod, alpha);
        engine_profile_span_end(software, EP_SPAN_SOFTWARE);
        return;
    }
    switch(screen->pixelformat)
    {
    case PIXEL_8:
        _putpixel(x, y, colour, screen, alpha);
        break;
    case PIXEL_16:
        _putpixel16(x, y, colour, screen, alpha);
        break;
    case PIXEL_32:
        _putpixel32(x, y, colour, screen, alpha);
        break;
    }
    engine_profile_span_end(software, EP_SPAN_SOFTWARE);
}
