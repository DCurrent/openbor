/*
 * OpenBOR - http://www.chronocrash.com
 * -----------------------------------------------------------------------
 * All rights reserved, see LICENSE in OpenBOR root for details.
 *
 * Copyright (c) 2004 - 2014 OpenBOR Team
 */

/*
	Code for handling 'screen' structures.
	(Memory allocation and copy functions)
	Last update: 10 feb 2003
*/
#include <stdio.h>
#include <string.h>
#include "types.h"
#include "transform.h"
#include "screen.h"

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
        free((*screen));
    }
    (*screen) = NULL;
}

// Screen copy func. Supports clipping.
void copyscreen(s_screen *dest, s_screen *src)
{
    unsigned char *sp, *dp;
    int width = src->width;
    int height = src->height;
    int pixelformat = src->pixelformat;

    if(pixelformat != dest->pixelformat)
    {
        return;
    }

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
}

void clearscreen(s_screen *s)
{
    if(s == NULL)
    {
        return;
    }
    memset(s->data, 0, s->width * s->height * pixelbytes[(int)s->pixelformat]);
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


static void _putscreen(s_screen *dest, s_screen *src, int x, int y, s_drawmethod *drawmethod)
{
    unsigned char *table;
    int alpha, transbg;
    gfx_entry gfx;

    if(!drawmethod || drawmethod->flag == 0)
    {
        table = NULL;
        alpha = 0;
        transbg = 0;
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
    else if(drawmethod->scalex != 256 || drawmethod->scaley != 256 || drawmethod->shiftx)
    {
        gfx.screen = src;
        gfx_draw_scale(dest, &gfx, x, y, 0, 0, drawmethod);
        return;
    }
    else
    {
        table = drawmethod->table;
        alpha = drawmethod->alpha;
        transbg = drawmethod->transbg;
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

void putscreen(s_screen *dest, s_screen *src, int x, int y, s_drawmethod *drawmethod)
{
    int xrepeat, yrepeat, xspan, yspan, i, j, dx, dy;

    drawmethod_global_init(drawmethod);

    if(drawmethod && drawmethod->flag)
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

    for(j = 0, dy = y; j < yrepeat; j++, dy += yspan)
    {
        for(i = 0, dx = x; i < xrepeat; i++, dx += xspan)
        {
            _putscreen(dest, src, dx, dy, drawmethod);
        }
    }

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
