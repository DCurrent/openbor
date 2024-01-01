/*
 * OpenBOR - http://www.chronocrash.com
 * -----------------------------------------------------------------------
 * All rights reserved, see LICENSE in OpenBOR root for details.
 *
 * Copyright (c)  OpenBOR Team
 */

// Simple bitmap code. Not fast, but useful nonetheless...
// 25-jan-2003

#include <stdio.h>
#include <string.h>
#include <stddef.h>
#include "globals.h"
#include "types.h"

#define		TRANS_INDEX		0x00

s_bitmap *allocbitmap(int width, int height, int format)
{
    s_bitmap *b;
    ptrdiff_t psize, extrab;
    if(width * height == 0)
    {
        return NULL;
    }
    psize = width * height * pixelbytes[(int)format];
    extrab = (4 - psize % 4) % 4;
    if(format == PIXEL_x8)
    {
        b = (s_bitmap *)malloc(sizeof(s_bitmap) + psize + extrab + PAL_BYTES);
    }
    else
    {
        b = (s_bitmap *)malloc(sizeof(s_bitmap) + psize);
    }
    if(b)
    {
        b->width = width;
        b->height = height;
        b->pixelformat = format;
        b->magic = bitmap_magic;
        b->clipped_x_offset = 0;
        b->clipped_y_offset = 0;
        b->clipped_width = width;
        b->clipped_height = height;
        if(format == PIXEL_x8)
        {
            b->palette = ((unsigned char *)b->data) + psize + extrab;
        }
        else
        {
            b->palette = NULL;
        }
    }
    return b;
}

void freebitmap(s_bitmap *bitmap)
{
    if(bitmap != NULL)
    {
        free(bitmap);
        bitmap = NULL;
    }
}

// Sluggish getbitmap function. Should be redone in ASM.
void getbitmap(int x, int y, int width, int height, s_bitmap *bitmap, s_screen *screen)
{

    int s, d;
    //int i;
    int j;

    // Clip width and height
    if(x < 0)
    {
        width += x;
        x = 0;
    }
    if(y < 0)
    {
        height += y;
        y = 0;
    }
    if(x + width > screen->width)
    {
        width = screen->width - x;
    }
    if(y + height > screen->height)
    {
        height = screen->height - y;
    }
    if(width <= 0 || height <= 0)
    {
        bitmap->width = 0;
        bitmap->height = 0;
        return;
    }

    bitmap->width = bitmap->clipped_width = width;
    bitmap->height = bitmap->clipped_height = height;
    bitmap->clipped_x_offset = 0;
    bitmap->clipped_y_offset = 0;

    d = 0;
    for(j = 0; j < height; j++)
    {
        s = x + (y + j) * screen->width;
        memcpy(((char *)bitmap->data) + d, ((char *)screen->data) + s, width);
        d += width;
        /*
        for(i=0; i<width; i++){
        	bitmap->data[d] = screen->data[s];
        	++d;
        	++s;
        }
        */
    }
}



// Clipped putbitmap. Slow.
void putbitmap(int x, int y, s_bitmap *bitmap, s_screen *screen)
{
    int skipleft = 0;
    int skiptop = 0;
    int width = bitmap->width;
    int height = bitmap->height;
    int s, d;
    int i;

    // Clip width and height
    if(x < 0)
    {
        skipleft = -x;
        width -= skipleft;
        x = 0;
    }
    if(y < 0)
    {
        skiptop = -y;
        height -= skiptop;
        y = 0;
    }
    if(x + width > screen->width)
    {
        width = screen->width - x;
    }
    if(y + height > screen->height)
    {
        height = screen->height - y;
    }
    if(width <= 0 || height <= 0)
    {
        return;
    }

    d = (y * screen->width) + x;

    do
    {
        s = skiptop * bitmap->width + skipleft;
        ++skiptop;
        for(i = 0; i < width; i++)
        {
            screen->data[d] = bitmap->data[s];
            ++d;
            ++s;
        }
        d += screen->width;
        d -= width;
    }
    while(--height);
}




// Flip horizontally
void flipbitmap(s_bitmap *bitmap)
{
    int x, xo, y;
    unsigned char t;
    int xsize = bitmap->width;
    int ysize = bitmap->height;

    for(y = 0; y < ysize; y++)
    {
        for(x = 0, xo = xsize - 1; x < xsize / 2; x++, xo--)
        {
            t = bitmap->data[y * xsize + x];
            bitmap->data[y * xsize + x] = bitmap->data[y * xsize + xo];
            bitmap->data[y * xsize + xo] = t;
        }
    }
}





// Clipbitmap: cuts off transparent edges to optimize a bitmap.
void clipbitmap(s_bitmap *bitmap, int *clip_left, int *clip_right, int *clip_top, int *clip_bottom)
{

    int x, y;
    int clip, clear;
    int xsize = bitmap->width;
    int ysize = bitmap->height;
    int fullwidth = bitmap->width;
    int top_clipmove = 0;
    int left_clipmove = 0;
    int bottom_clipmove = 0;
    int right_clipmove = 0;



    // Determine size of empty top
    clip = 0;
    for(y = 0; y < ysize; y++)
    {
        clear = 1;
        for(x = 0; x < xsize && clear; x++)
        {
            if(bitmap->data[y * xsize + x] != TRANS_INDEX)
            {
                clear = 0;
            }
        }
        if(clear)
        {
            ++clip;
        }
        else
        {
            break;
        }
    }

    if(clip)
    {
        // "Cut off" empty top
        ysize -= clip;
        if(ysize < 1)
        {
            // If nothing is left of the bitmap, return...
            if(clip_left)
            {
                *clip_left = 0;
            }
            if(clip_right)
            {
                *clip_right = 0;
            }
            if(clip_top)
            {
                *clip_top = 0;
            }
            if(clip_bottom)
            {
                *clip_bottom = 0;
            }
            bitmap->clipped_width = 0;
            bitmap->clipped_height = 0;
            return;
        }
        bitmap->clipped_y_offset = clip;
        top_clipmove = clip;
    }



    // Determine size of empty bottom
    clip = 0;
    for(y = bitmap->height - 1; y >= top_clipmove; y--)
    {
        clear = 1;
        for(x = 0; x < xsize && clear; x++)
        {
            if(bitmap->data[y * xsize + x] != TRANS_INDEX)
            {
                clear = 0;
            }
        }
        if(clear)
        {
            ++clip;
        }
        else
        {
            break;
        }
    }

    // "Cut off" empty bottom
    ysize -= clip;
    bitmap->clipped_height = ysize;
    bottom_clipmove = clip;


    // Determine size of empty left side
    clip = 2000000000;
    for(y = top_clipmove; y < ysize + top_clipmove; y++)
    {
        clear = 0;
        for(x = 0; x < xsize; x++)
        {
            if(bitmap->data[y * xsize + x] != TRANS_INDEX)
            {
                break;
            }
            ++clear;
        }
        if(clear < clip)
        {
            clip = clear;
        }
    }

    // "Cut off" empty pixels on the left side
    if(clip)
    {
        xsize -= clip;
        bitmap->clipped_width = xsize;
        bitmap->clipped_x_offset = clip;
        left_clipmove = clip;
    }

    // Determine size of empty right side
    clip = 2000000000;
    for(y = top_clipmove; y < ysize + top_clipmove; y++)
    {
        clear = 0;
        for(x = fullwidth - 1; x >= left_clipmove; x--)
        {
            if(bitmap->data[y * fullwidth + x] != TRANS_INDEX)
            {
                break;
            }
            ++clear;
        }
        if(clear < clip)
        {
            clip = clear;
        }
    }

    // "Cut off" empty pixels on the right side
    if(clip)
    {
        xsize -= clip;
        bitmap->clipped_width = xsize;
        right_clipmove = clip;
    }

    if(clip_left)
    {
        *clip_left = left_clipmove;
    }
    if(clip_right)
    {
        *clip_right = right_clipmove;
    }
    if(clip_top)
    {
        *clip_top = top_clipmove;
    }
    if(clip_bottom)
    {
        *clip_bottom = bottom_clipmove;
    }
}



