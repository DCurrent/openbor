/*
 * OpenBOR - http://www.chronocrash.com
 * -----------------------------------------------------------------------
 * All rights reserved, see LICENSE in OpenBOR root for details.
 *
 * Copyright (c)  OpenBOR Team
 */

/*
	Code for handling 'screen' structures.
	(Memory allocation and copy functions)
	Last update: 10 feb 2003
*/
#include <stdio.h>
#include <limits.h>
#include <string.h>
#include "types.h"
#include "transform.h"
#include "screen.h"
#include "gpu.h"
#include "engine_profile.h"
#include "drawmethod_color.h"

s_screen *allocscreen(int width, int height, int pixelformat)
{
    s_screen *screen;
    int psize;
    width &= (0xFFFFFFFF - 3);
    psize = width * height * pixelbytes[pixelformat];
    if(pixelformat == PIXEL_x8)
    {
        screen = (s_screen *)malloc(sizeof(s_screen) + psize + PAL_BYTES + ANYNUMBER);
    }
    else
    {
        screen = (s_screen *)malloc(sizeof(s_screen) + psize + ANYNUMBER);
    }
    if(screen == NULL)
    {
        return NULL;
    }
    screen->width = width;
    screen->height = height;
    screen->pixelformat = pixelformat;
    screen->magic = screen_magic;
    if(pixelformat == PIXEL_x8)
    {
        screen->palette = ((unsigned char *)screen->data) + width * height * pixelbytes[(int)pixelformat];
    }
    else
    {
        screen->palette = NULL;
    }
    return screen;
}

void freescreen(s_screen **screen)
{
    if((*screen) != NULL)
    {
        /* Release any render target before its CPU identity can be reused. */
        gpu_release_screen(*screen);
        free((*screen));
    }
    (*screen) = NULL;
}

/*
 * Damon V. Caskey
 * 2026-11-12
 * Copy a screen with destination clipping. Profile a declined native copy
 * through synchronization and CPU copying as one inclusive software span.
 */
void copyscreen(s_screen *dest, s_screen *src)
{
    ep_scope software;
    unsigned char *sp, *dp;
    int width = src->width;
    int height = src->height;
    int pixelformat = src->pixelformat;

    if(pixelformat != dest->pixelformat)
    {
        return;
    }

    /* Keep copies between render targets on the GPU when supported. */
    if(gpu_copy_screen(dest, src))
    {
        return;
    }
    software = engine_profile_span_begin();
    engine_profile_count(EP_COUNT_SOFTWARE_COPIES, 1);
    /* Synchronization belongs to this fallback's wall time. A readback, when
     * required, also appears in its own inclusive transfer measurement. */
    gpu_sync_cpu(src);
    gpu_prepare_cpu_write(dest);

    if(height > dest->height)
    {
        height = dest->height;
    }
    if(width > dest->width)
    {
        width = dest->width;
    }

    dp = dest->data;
    sp = src->data;
    // Copy unclipped
    if(dest->width == src->width)
    {
        memcpy(dest->data, src->data, width * height * pixelbytes[(int)pixelformat]);
        engine_profile_span_end(software, EP_SPAN_SOFTWARE);
        return;
    }

    // Copy clipped
    do
    {
        memcpy(dp, sp, width * pixelbytes[(int)pixelformat]);
        sp += src->width * pixelbytes[(int)pixelformat];
        dp += dest->width * pixelbytes[(int)pixelformat];
    }
    while(--height);
    engine_profile_span_end(software, EP_SPAN_SOFTWARE);
}

/*
 * Damon V. Caskey
 * 2026-11-12
 * Clear either resident GPU storage or its CPU shadow. Count and time the
 * software path separately so mixed-renderer costs remain visible.
 */
void clearscreen(s_screen *s)
{
    ep_scope software;
    if(s == NULL)
    {
        return;
    }
    if(gpu_clear_screen(s))
    {
        return;
    }
    software = engine_profile_span_begin();
    engine_profile_count(EP_COUNT_SOFTWARE_CLEARS, 1);
    /* Clearing replaces every pixel, so no previous GPU contents are needed. */
    gpu_mark_cpu_dirty(s);
    memset(s->data, 0, s->width * s->height * pixelbytes[(int)s->pixelformat]);
    engine_profile_span_end(software, EP_SPAN_SOFTWARE);
}

// Screen copy function with offset options. Supports clipping.
void copyscreen_o(s_screen *dest, s_screen *src, int x, int y)
{
    unsigned char *sp, *dp;
    int pixelformat = src->pixelformat;
    int sw = src->width;
    int sh = src->height;
    int dw = dest->width;
    int cw = sw, ch = sh;
    int linew, slinew, dlinew;
    int sox, soy;

    int xmin = useclip ? clipx1 : 0,
        xmax = useclip ? clipx2 : dest->width,
        ymin = useclip ? clipy1 : 0,
        ymax = useclip ? clipy2 : dest->height;

    // Copy anything at all?
    if(x >= xmax)
    {
        return;
    }
    if(sw + x <= xmin)
    {
        return;
    }
    if(y >= ymax)
    {
        return;
    }
    if(sh + y <= ymin)
    {
        return;
    }

    sox = 0;
    soy = 0;

    // Clip?
    if(x < xmin)
    {
        sox = xmin - x;
        cw -= sox;
    }
    if(y < ymin)
    {
        soy = ymin - y;
        ch -= soy;
    }

    if(x + sw > xmax)
    {
        cw -= (x + sw) - xmax;
    }
    if(y + sh > ymax)
    {
        ch -= (y + sh) - ymax;
    }

    if(x < xmin)
    {
        x = xmin;
    }
    if(y < ymin)
    {
        y = ymin;
    }

    if(dest->pixelformat != src->pixelformat)
    {
        return;
    }

    /* Public software entry points must also observe preceding GPU draws. */
    gpu_sync_cpu(src);
    gpu_prepare_cpu_write(dest);
    sp = src->data + (soy * sw + sox) * pixelbytes[(int)pixelformat];
    dp = dest->data + (y * dw + x) * pixelbytes[(int)pixelformat];
    linew = cw * pixelbytes[(int)pixelformat];
    slinew = sw * pixelbytes[(int)pixelformat];
    dlinew = dw * pixelbytes[(int)pixelformat];
    // Copy data
    do
    {
        memcpy(dp, sp, linew);
        sp += slinew;
        dp += dlinew;
    }
    while(--ch);
}

// same as above, with color key
void copyscreen_trans(s_screen *dest, s_screen *src, int x, int y)
{
    unsigned char *sp, *dp;
    int sw = src->width;
    int sh = src->height;
    int dw = dest->width;
    int cw = sw, ch = sh;
    int sox, soy;
    int i;

    int xmin = useclip ? clipx1 : 0,
        xmax = useclip ? clipx2 : dest->width,
        ymin = useclip ? clipy1 : 0,
        ymax = useclip ? clipy2 : dest->height;

    // Copy anything at all?
    if(x >= xmax)
    {
        return;
    }
    if(sw + x <= xmin)
    {
        return;
    }
    if(y >= ymax)
    {
        return;
    }
    if(sh + y <= ymin)
    {
        return;
    }

    sox = 0;
    soy = 0;

    // Clip?
    if(x < xmin)
    {
        sox = xmin - x;
        cw -= sox;
    }
    if(y < ymin)
    {
        soy = ymin - y;
        ch -= soy;
    }

    if(x + sw > xmax)
    {
        cw -= (x + sw) - xmax;
    }
    if(y + sh > ymax)
    {
        ch -= (y + sh) - ymax;
    }

    if(x < xmin)
    {
        x = xmin;
    }
    if(y < ymin)
    {
        y = ymin;
    }

    gpu_sync_cpu(src);
    gpu_prepare_cpu_write(dest);
    sp = src->data + (soy * sw + sox);
    dp = dest->data + (y * dw + x);
    // Copy data
    do
    {
        i = cw - 1;
        do
        {
            if(sp[i] == 0)
            {
                continue;
            }
            dp[i] = sp[i];
        }
        while(i--);
        sp += sw;
        dp += dw;
    }
    while(--ch);
}

//same as above, with remap, work only under 8bit pixel format
void copyscreen_remap(s_screen *dest, s_screen *src, int x, int y, unsigned char *remap)
{
    unsigned char *sp = src->data;
    unsigned char *dp = dest->data;
    int i;
    int sw = src->width;
    int sh = src->height;
    int dw = dest->width;
    int cw = sw, ch = sh;
    int sox, soy;

    int xmin = useclip ? clipx1 : 0,
        xmax = useclip ? clipx2 : dest->width,
        ymin = useclip ? clipy1 : 0,
        ymax = useclip ? clipy2 : dest->height;

    // Copy anything at all?
    if(x >= xmax)
    {
        return;
    }
    if(sw + x <= xmin)
    {
        return;
    }
    if(y >= ymax)
    {
        return;
    }
    if(sh + y <= ymin)
    {
        return;
    }

    sox = 0;
    soy = 0;

    // Clip?
    if(x < xmin)
    {
        sox = xmin - x;
        cw -= sox;
    }
    if(y < ymin)
    {
        soy = ymin - y;
        ch -= soy;
    }

    if(x + sw > xmax)
    {
        cw -= (x + sw) - xmax;
    }
    if(y + sh > ymax)
    {
        ch -= (y + sh) - ymax;
    }

    if(x < xmin)
    {
        x = xmin;
    }
    if(y < ymin)
    {
        y = ymin;
    }

    gpu_sync_cpu(src);
    gpu_prepare_cpu_write(dest);
    sp += (soy * sw + sox);
    dp += (y * dw + x);

    // Copy data
    do
    {
        i = cw - 1;
        do
        {
            if(!sp[i])
            {
                continue;
            }
            dp[i] = remap[sp[i]];
        }
        while(i--);
        sp += sw;
        dp += dw;
    }
    while(--ch);
}

//same as above, with alpha blend
void blendscreen(s_screen *dest, s_screen *src, int x, int y, unsigned char *lut)
{
    unsigned char *sp = src->data;
    unsigned char *dp = dest->data;
    int i;
    int sw = src->width;
    int sh = src->height;
    int dw = dest->width;
    int cw = sw, ch = sh;
    int sox, soy;
    unsigned char *d, *s;

    int xmin = useclip ? clipx1 : 0,
        xmax = useclip ? clipx2 : dest->width,
        ymin = useclip ? clipy1 : 0,
        ymax = useclip ? clipy2 : dest->height;

    // Copy anything at all?
    if(x >= xmax)
    {
        return;
    }
    if(sw + x <= xmin)
    {
        return;
    }
    if(y >= ymax)
    {
        return;
    }
    if(sh + y <= ymin)
    {
        return;
    }

    sox = 0;
    soy = 0;

    // Clip?
    if(x < xmin)
    {
        sox = xmin - x;
        cw -= sox;
    }
    if(y < ymin)
    {
        soy = ymin - y;
        ch -= soy;
    }

    if(x + sw > xmax)
    {
        cw -= (x + sw) - xmax;
    }
    if(y + sh > ymax)
    {
        ch -= (y + sh) - ymax;
    }

    if(x < xmin)
    {
        x = xmin;
    }
    if(y < ymin)
    {
        y = ymin;
    }

    gpu_sync_cpu(src);
    gpu_prepare_cpu_write(dest);
    sp += soy * sw + sox;
    dp += y * dw + x;

    // Copy data
    do
    {
        i = cw;
        do
        {
            d = dp + i - 1;
            s = sp + i - 1;
            if(!(*s))
            {
                continue;
            }
            *d = lut[(*s) << 8 | (*d)];
        }
        while(--i);
        sp += sw;
        dp += dw;
    }
    while(--ch);
}

/*
 * Damon V. Caskey
 * 2026-11-13
 * Select software screen geometry and preserve the existing pixel operations.
 * Explicit affine geometry overrides the stored legacy transform settings.
 */
static void _putscreen(s_screen *dest, s_screen *src, int x, int y, s_drawmethod *drawmethod)
{
    unsigned char *table;
    int alpha, transbg;
    gfx_entry gfx;

    if(!drawmethod || !(drawmethod->config & DRAWMETHOD_CONFIG_ENABLED))
    {
        table = NULL;
        alpha = 0;
        transbg = 0;
    }
    else if(drawmethod->config & DRAWMETHOD_CONFIG_AFFINE)
    {
        /* Opt-in affine takes precedence over legacy shear, rotation and water. */
        gfx.screen = src;
        gfx_draw_affine(dest, &gfx, x, y, 0, 0, drawmethod);
        return;
    }
    else if(drawmethod->water.watermode && drawmethod->water.amplitude)
    {
        gfx.screen = src;
        if(drawmethod->water.watermode == 3)
        {
            gfx_draw_plane(dest, &gfx, x, y, 0, 0, drawmethod);
        }
        else
        {
            gfx_draw_water(dest, &gfx, x, y, 0, 0, drawmethod);
        }
        return ;
    }
    else if(drawmethod->rotate)
    {
        gfx.screen = src;
        gfx_draw_rotate(dest, &gfx, x, y, 0, 0, drawmethod);
        return;
    }
    else if(drawmethod->scalex != 256 || drawmethod->scaley != 256 || drawmethod->shiftx ||
            drawmethod_colors_active(drawmethod))
    {
        /* The generalized writer applies chroma before effects and Substitute
         * after composition, including otherwise untransformed screen copies. */
        gfx.screen = src;
        gfx_draw_scale(dest, &gfx, x, y, 0, 0, drawmethod);
        return;
    }
    else
    {
        table = drawmethod->table;
        alpha = drawmethod->alpha;
        transbg = (drawmethod->config & DRAWMETHOD_CONFIG_BACKGROUND_TRANSPARENCY);
        x -= drawmethod->centerx;
        y -= drawmethod->centery;
    }

    if(!table && alpha <= 0 && !transbg && !usechannel)
    {
        if(dest->pixelformat == src->pixelformat && dest->width == src->width && dest->height == src->height && !x && !y)
        {
            copyscreen(dest, src);
            return;
        }
    }

    if(dest->pixelformat == PIXEL_8)
    {
        if(table)
        {
            copyscreen_remap(dest, src, x, y, drawmethod->table);
        }
        else if(alpha > 0)
        {
            blendscreen(dest, src, x, y, blendtables[drawmethod->alpha - 1]);
        }
        else if(transbg)
        {
            copyscreen_trans(dest, src, x, y);
        }
        else
        {
            copyscreen_o(dest, src, x, y);
        }
    }
    else if(dest->pixelformat == PIXEL_16)
    {
        if(src->pixelformat == PIXEL_x8)
        {
            putscreenx8p16(dest, src, x, y, transbg, (unsigned short *)table, getblendfunction16(alpha));
        }
        else if(src->pixelformat == PIXEL_16)
        {
            blendscreen16(dest, src, x, y, transbg, getblendfunction16(alpha));
        }
    }
    else if(dest->pixelformat == PIXEL_32)
    {
        if(src->pixelformat == PIXEL_x8)
        {
            putscreenx8p32(dest, src, x, y, transbg, (unsigned *)table, getblendfunction32(alpha));
        }
        else if(src->pixelformat == PIXEL_32)
        {
            blendscreen32(dest, src, x, y, transbg, getblendfunction32(alpha));
        }
    }
}

/*
 * Damon V. Caskey
 * 2026-11-12
 * 2026-11-13 - Bound actual repeat positions before narrowing to pixel coordinates.
 * Dispatch a screen draw with its repeats and effects. A software fallback
 * is measured once around the complete operation, including coherence work.
 */
void putscreen(s_screen *dest, s_screen *src, int x, int y, s_drawmethod *drawmethod)
{
    ep_scope software;
    int xrepeat, yrepeat, xspan, yspan, i, j;
    s64 dx, dy;
    s_screen *color_snapshot = NULL;
    size_t color_snapshot_bytes = 0;

    if(drawmethod_colors_active(drawmethod) && !gpu_require_color_target(dest, drawmethod))
    {
        return;
    }
    drawmethod_global_init(drawmethod);

    /* One dispatch includes all repeats; unsupported effects retain their
     * original software path with current source and destination shadows. */
    if(gpu_draw_screen(dest, src, x, y, drawmethod))
    {
        return;
    }
    software = engine_profile_span_begin();
    engine_profile_count(EP_COUNT_SOFTWARE_DRAWS, 1);
    if(drawmethod_colors_active(drawmethod))
    {
        engine_profile_count(EP_COUNT_COLOR_SOFTWARE_DRAWS, 1);
        if(!src || (src->pixelformat != PIXEL_x8 && src->pixelformat != PIXEL_32))
        {
            gpu_color_rules_error("Chroma and Substitute require an indexed or RGB32 source screen.");
            engine_profile_span_end(software, EP_SPAN_SOFTWARE);
            return;
        }
    }
    gpu_sync_cpu(src);
    gpu_prepare_cpu_write(dest);

    if(drawmethod && drawmethod->config & DRAWMETHOD_CONFIG_ENABLED)
    {
        xrepeat = drawmethod->xrepeat;
        yrepeat = drawmethod->yrepeat;
        xspan = drawmethod->xspan;
        yspan = drawmethod->yspan;
    }
    else
    {
        xrepeat = yrepeat = 1;
        xspan = yspan = 0;
    }

    /* Each self-draw samples the source as it existed before that instance.
     * Reuse one allocation across repetitions, refreshing its pixels between
     * them so overlapping instances still observe earlier completed draws. */
    if(drawmethod_colors_active(drawmethod) && src == dest && xrepeat > 0 && yrepeat > 0 &&
       src->width > 0 && src->height > 0)
    {
        /* allocscreen rounds widths to four pixels. Preserve this existing
         * screen's exact stride, including externally created odd widths. */
        if((size_t)src->width <= ((size_t)-1 - sizeof(s_screen)) /
                                sizeof(unsigned) / (size_t)src->height)
        {
            color_snapshot_bytes = (size_t)src->width * (size_t)src->height * sizeof(unsigned);
            color_snapshot = malloc(sizeof(s_screen) + color_snapshot_bytes);
        }
        if(!color_snapshot)
        {
            gpu_color_rules_error("Unable to allocate a software color-rule self-draw snapshot.");
            engine_profile_span_end(software, EP_SPAN_SOFTWARE);
            return;
        }
        color_snapshot->magic = screen_magic;
        color_snapshot->width = src->width;
        color_snapshot->height = src->height;
        color_snapshot->pixelformat = PIXEL_32;
        color_snapshot->palette = NULL;
    }

    /* Calculate only actual instances in wider arithmetic. Unrepresentable
     * coordinates are skipped, and no unused final increment can overflow. */
    for(j = 0; j < yrepeat; j++)
    {
        dy = (s64)y + (s64)j * yspan;
        if(dy < INT_MIN || dy > INT_MAX)
        {
            continue;
        }
        for(i = 0; i < xrepeat; i++)
        {
            dx = (s64)x + (s64)i * xspan;
            if(dx < INT_MIN || dx > INT_MAX)
            {
                continue;
            }
            if(color_snapshot)
            {
                memcpy(color_snapshot->data, src->data, color_snapshot_bytes);
            }
            _putscreen(dest, color_snapshot ? color_snapshot : src, (int)dx, (int)dy, drawmethod);
        }
    }
    if(color_snapshot)
    {
        freescreen(&color_snapshot);
    }
    engine_profile_span_end(software, EP_SPAN_SOFTWARE);
}

// Scale screen
void scalescreen(s_screen *dest, s_screen *src)
{
    int sw, sh;
    int dw, dh;
    int dx, dy;
    unsigned char *sp;
    unsigned char *dp;
    unsigned char *lineptr;
    unsigned int xstep, ystep, xpos, ypos;
    int pixelformat = src->pixelformat;

    if(dest->pixelformat != pixelformat)
    {
        return;
    }

    if(src == NULL || dest == NULL)
    {
        return;
    }
    gpu_sync_cpu(src);
    gpu_prepare_cpu_write(dest);
    sp = src->data;
    dp = dest->data;

    sw = src->width * pixelbytes[(int)pixelformat];
    sh = src->height;
    dw = dest->width * pixelbytes[(int)pixelformat];
    dh = dest->height;

    xstep = (sw << 16) / dw;
    ystep = (sh << 16) / dh;

    ypos = 0;
    for(dy = 0; dy < dh; dy++)
    {
        lineptr = sp + ((ypos >> 16) * sw);
        ypos += ystep;
        xpos = 0;
        for(dx = 0; dx < dw; dx++)
        {
            *dp = lineptr[xpos >> 16];
            ++dp;
            xpos += xstep;
        }
    }
}

/*
 * Zooms in or out on the screen.
 * Parameters:
 *     centerx - x coord of zoom center on unclipped, unscaled screen
 *     centery - y coord of zoom center on unclipped, unscaled screen
 *     scalex - x scale factor
 *     scaley - y scale factor
 */
void zoomscreen(s_screen *dest, s_screen *src, int centerx, int centery, int scalex, int scaley)
{
    s_screen *frame;
    int screenwidth = src->width;
    int screenheight = src->height;
    int width = (screenwidth << 8) / scalex; // width of clipped, unscaled screen
    int height = (screenheight << 8) / scaley; // height of clipped, unscaled screen
    int xmin = (width >> 1) - centerx; // x coord before clipping corresponding to x=0 after clipping
    int ymin = (height >> 1) - centery; // y coord before clipping corresponding to y=0 after clipping
    int pixelformat = dest->pixelformat;

    if(src->pixelformat != pixelformat)
    {
        return;
    }
    if(xmin >= screenwidth || xmin + ((width * scalex) >> 8) < 0)
    {
        return;    // out of left or right border
    }
    if(ymin >= screenheight)
    {
        return;
    }

    frame = allocscreen(width, height, pixelformat); // the part of the screen that will be zoomed
    copyscreen_o(frame, src, xmin, ymin);

    if(pixelbytes[pixelformat] == 1)
    {
        scalescreen(dest, frame);
    }
    else if(pixelbytes[pixelformat] == 2)
    {
        scalescreen16(dest, frame);
    }
    else if(pixelbytes[pixelformat] == 4)
    {
        scalescreen32(dest, frame);
    }

    freescreen(&frame);
}
