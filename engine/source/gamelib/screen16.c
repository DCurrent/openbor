/*
 * OpenBOR - http://www.chronocrash.com
 * -----------------------------------------------------------------------
 * All rights reserved, see LICENSE in OpenBOR root for details.
 *
 * Copyright (c) 2004 - 2011 OpenBOR Team
 */

#include "globals.h"
#include "types.h"

//with remap, work only under 8bit pixel format
void putscreenx8p16(s_screen *dest, s_screen *src, int x, int y, int key, unsigned short *remap, unsigned short(*blendfp)(unsigned short, unsigned short))
{
    unsigned char *sp = src->data;
    unsigned short *dp = (unsigned short *)dest->data;
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


    if(!remap)
    {
        remap = (unsigned short *)src->palette;
    }

    if(!remap)
    {
        return;
    }

    if(blendfp)
    {
        if(key)
        {
            // blend
            do
            {
                i = cw - 1;
                do
                {
                    if(!sp[i])
                    {
                        continue;
                    }
                    dp[i] = blendfp(remap[sp[i]], dp[i]);
                }
                while(i--);
                sp += sw;
                dp += dw;
            }
            while(--ch);
        }
        else
        {
            // blend
            do
            {
                i = cw - 1;
                do
                {
                    dp[i] = blendfp(remap[sp[i]], dp[i]);
                }
                while(i--);
                sp += sw;
                dp += dw;
            }
            while(--ch);
        }
    }
    else
    {
        if(key)
        {
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
        else
        {
            // Copy data
            do
            {
                //u16pcpy(dp, sp, remap, cw);
                i = cw - 1;
                do
                {
                    dp[i] = remap[sp[i]];
                }
                while(i--);
                sp += sw;
                dp += dw;
            }
            while(--ch);
        }
    }
}


void blendscreen16(s_screen *dest, s_screen *src, int x, int y, int key, u16(*blendfp)(u16, u16))
{
    u16 *sp = (u16 *)src->data;
    u16 *dp = (u16 *)dest->data;
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

    if(blendfp)
    {
        if(key)
        {
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
                    dp[i] = blendfp(sp[i], dp[i]);
                }
                while(i--);
                sp += sw;
                dp += dw;
            }
            while(--ch);
        }
        else
        {
            // Copy data
            do
            {
                i = cw - 1;
                do
                {
                    dp[i] = blendfp(sp[i], dp[i]);
                }
                while(i--);
                sp += sw;
                dp += dw;
            }
            while(--ch);
        }
    }
    else
    {
        if(key)
        {
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
        else
        {
            // Copy data
            do
            {
                memcpy(dp, sp, cw << 1);
                sp += sw;
                dp += dw;
            }
            while(--ch);
        }
    }
}

// Scale screen
void scalescreen16(s_screen *dest, s_screen *src)
{
    int sw, sh;
    int dw, dh;
    int dx, dy;
    u16 *sp;
    u16 *dp;
    u16 *lineptr;
    unsigned int xstep, ystep, xpos, ypos;
    int pixelformat = src->pixelformat;

    //if(dest->pixelformat!=pixelformat || pixelformat!=PIXEL_16) return;
    if(dest->pixelformat != pixelformat)
    {
        return;
    }

    if(src == NULL || dest == NULL)
    {
        return;
    }
    sp = (u16 *)src->data;
    dp = (u16 *)dest->data;

    sw = src->width;
    sh = src->height;
    dw = dest->width;
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

