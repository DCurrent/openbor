/*
 * OpenBOR - http://www.chronocrash.com
 * -----------------------------------------------------------------------
 * All rights reserved, see LICENSE in OpenBOR root for details.
 *
 * Copyright (c)  OpenBOR Team
 */

#include "globals.h"
#include "types.h"
#include "sprite.h"
#include "gpu.h"

enum
{
    SPRITE_RLE_END_OF_LINE = 0xFF
};

/*
 * Damon V. Caskey
 * 2026-11-20
 * Submit indexed artwork, masks and repetitions exclusively to the GPU.
 * Source indices remain asset data; the scene destination is always RGB32.
 * Rejection leaves the CPU shadow and its ownership unchanged.
 */
void putsprite(int x, int y, s_sprite *sprite, s_screen *screen, s_drawmethod *drawmethod)
{
    if(!screen || !sprite || screen->pixelformat != PIXEL_32 ||
       !pixel_format_indexed(sprite->pixelformat) ||
       (sprite->mask && !pixel_format_indexed(sprite->mask->pixelformat)))
    {
        return;
    }

    if(!gpu_draw_sprite(screen, sprite, x, y, drawmethod))
    {
        gpu_report_rejected_image_operation(screen, GPU_OPERATION_SPRITE, drawmethod);
    }
}

/*
 * Damon V. Caskey
 * 2026-11-20
 * Read a source-artwork index for explicit script image access. This decodes
 * one RLE row without drawing to a scene or changing GPU resource ownership.
 * Preserve the public char result used by the script pixel-reading API.
 */
char sprite_get_pixel(s_sprite *sprite, int x, int y)
{
    int *linetab;
    int lx = 0, count;
    unsigned char *data;

    if(!sprite || !pixel_format_indexed(sprite->pixelformat) ||
       x < 0 || x >= sprite->width || y < 0 || y >= sprite->height)
    {
        return TRANSPARENT_IDX;
    }

    linetab = ((int *)sprite->data) + y;
    data = ((unsigned char *)linetab) + (*linetab);

    while(1)
    {
        count = *data++;
        if(count == SPRITE_RLE_END_OF_LINE || lx + count > x)
        {
            return TRANSPARENT_IDX;
        }
        lx += count;
        count = *data++;
        if(!count)
        {
            continue;
        }
        if(lx + count > x)
        {
            return data[x - lx];
        }
        lx += count;
        data += count;
    }
}

/*
 * Damon V. Caskey
 * 2026-11-15
 * Encode indexed artwork and alpha masks into byte RLE sprites. Invalid
 * pixel formats must not become mislabeled sprite data. NULL for dest
 * requests the encoded storage size without writing a sprite.
 */
unsigned encodesprite(
    int centerx, int centery,
    s_bitmap *bitmap, s_sprite *dest
)
{
    int x, x0, y, w, h, xoffset, origwidth;
    unsigned char *data;
    int *linetab;
    unsigned char *src;
    int pb = PAL_BYTES, extrab;

    if(!bitmap || !pixel_format_indexed(bitmap->pixelformat))
    {
        return 0;
    }
    src = bitmap->data;

    if(dest)
    {
        /* Encoding can replace a previously uploaded sprite at this address. */
        gpu_release_sprite(dest);
        dest->magic = sprite_magic;
    }

    if(bitmap->clipped_width <= 0 || bitmap->clipped_height <= 0)
    {
        // Image is empty (or bad), create an empty sprite
        if(dest)
        {
            //dest->is_flip_of = NULL;
            dest->centerx = 0;
            dest->centery = 0;
            dest->width = 0;
            dest->height = 0;
            dest->pixelformat = bitmap->pixelformat;
            dest->mask = NULL;
            dest->palette = NULL;
        }
        return sizeof(s_sprite);
    }

    w = bitmap->clipped_width;
    h = bitmap->clipped_height;
    xoffset = bitmap->clipped_x_offset;
    origwidth = bitmap->width;

    if(dest)
    {
        //dest->is_flip_of = NULL;
        dest->centerx = centerx;
        dest->centery = centery;
        dest->width = w;
        dest->height = h;
        dest->pixelformat = bitmap->pixelformat;
        dest->mask = NULL;
    }
    linetab = (int *)(dest->data);
    data = (unsigned char *)(linetab + h);

    src += bitmap->clipped_y_offset * origwidth;
    src += xoffset;

    for(y = 0; y < h; y++, src += origwidth)
    {
        if(dest)
        {
            linetab[y] = ((size_t)data) - ((size_t)(linetab + y));
        }
        x = 0;
        for(;;)
        {
            // search for the first visible pixel
            x0 = x;
            for(; (x < w) && ((x - x0) < 0xFE); x++)
            {
                if(src[x])
                {
                    break;
                }
            }
            // handle EOL
            if(x >= w)
            {
                if(dest)
                {
                    *data = 0xFF;
                }
                data++;
                break;
            }
            // encode clearcount
            if(dest)
            {
                *data = x - x0;
            }
            data++;
            // if we're still not visible, encode a null visible count and continue
            if(!src[x])
            {
                if(dest)
                {
                    *data = 0;
                }
                data++;
                continue;
            }
            // search for the first invisible pixel
            x0 = x;
            for(; (x < w) && ((x - x0) < 0xFF); x++)
            {
                if(!src[x])
                {
                    break;
                }
            }
            // encode viscount and visible pixels
            if(dest)
            {
                *data++ = x - x0;
                memcpy(data, src + x0, x - x0);
                data += x - x0;
            }
            else
            {
                data += 1 + (x - x0);
            }
        }
    }

    if(!bitmap->palette)
    {
        pb = extrab = 0;
    }
    else
    {
        extrab = ((size_t)data) - ((size_t)dest);
        extrab %= 4;
        extrab = 4 - extrab;
        extrab %= 4;
    }

    //point palette to the last byte of the pixel data
    if(dest)
    {
        if(bitmap->palette) // if the bitmap contains palette, copy it
        {
            dest->palette = ((unsigned char *)data) + extrab ;
            memcpy(dest->palette, bitmap->palette, pb);
        }
        else
        {
            dest->palette = NULL;
        }
    }
    return ((size_t)data) - ((size_t)dest) + extrab + pb + ANYNUMBER;
}

/////////////////////////////////////////////////////////////////////////////

unsigned fakey_encodesprite(s_bitmap *bitmap)
{
    return encodesprite(0, 0, bitmap, NULL);
}

/////////////////////////////////////////////////////////////////////////////

