/*
 * OpenBOR - http://www.chronocrash.com
 * -----------------------------------------------------------------------
 * All rights reserved, see LICENSE in OpenBOR root for details.
 *
 * Copyright (c)  OpenBOR Team
 */

#include <stdlib.h>
#include <string.h>
#include "screen.h"
#include "gpu.h"

/*
 * Damon V. Caskey
 * 2026-11-15
 * Allocate RGB32 screens or indexed artwork without accepting obsolete
 * color formats or indexing the stride table with an invalid identity.
 */
s_screen *allocscreen(int width, int height, int pixelformat)
{
    s_screen *screen;
    int psize;

    if(!pixel_format_supported(pixelformat))
    {
        return NULL;
    }
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
 * 2026-11-20
 * Copy RGB32 render targets entirely on the GPU. Indexed buffers are source
 * artwork, including cached backgrounds, and retain their plain byte-copy
 * operation. That asset operation has no blending, scaling or scene writes.
 * Destination clipping and the existing separate palette ownership remain.
 */
void copyscreen(s_screen *dest, s_screen *src)
{
    int width, height, row;

    if(!dest || !src || !pixel_format_supported(dest->pixelformat) ||
       dest->pixelformat != src->pixelformat || dest == src)
    {
        return;
    }

    if(pixel_format_indexed(dest->pixelformat))
    {
        width = src->width < dest->width ? src->width : dest->width;
        height = src->height < dest->height ? src->height : dest->height;
        if(width <= 0 || height <= 0)
        {
            return;
        }

        /* Indexed artwork remains CPU-owned. Invalidate any uploaded source
         * texture before editing its bytes; no render-target readback is needed. */
        gpu_mark_cpu_dirty(dest);
        if(dest->width == src->width)
        {
            memcpy(dest->data, src->data, (size_t)width * height);
        }
        else
        {
            for(row = 0; row < height; ++row)
            {
                memcpy(dest->data + (size_t)row * dest->width,
                       src->data + (size_t)row * src->width, (size_t)width);
            }
        }
        return;
    }

    if(!gpu_copy_screen(dest, src))
    {
        gpu_report_rejected_operation(dest, GPU_OPERATION_COPY);
    }
}

/*
 * Damon V. Caskey
 * 2026-11-20
 * Clear RGB32 targets through the GPU with logging on failure. Resetting an
 * indexed source image only changes its owned index bytes; it is asset
 * maintenance and cannot become a scene-rendering fallback.
 */
void clearscreen(s_screen *screen)
{
    if(!screen || !pixel_format_supported(screen->pixelformat))
    {
        return;
    }

    if(pixel_format_indexed(screen->pixelformat))
    {
        if(screen->width <= 0 || screen->height <= 0)
        {
            return;
        }
        gpu_mark_cpu_dirty(screen);
        memset(screen->data, 0, (size_t)screen->width * screen->height);
        return;
    }

    if(!gpu_clear_screen(screen))
    {
        gpu_report_rejected_operation(screen, GPU_OPERATION_CLEAR);
    }
}

/*
 * Damon V. Caskey
 * 2026-11-20
 * Submit the complete screen draw to native rendering, including repeats,
 * clipping, transforms and sampling. Rejected draws only report the failure;
 * no CPU renderer or mutable pixel-writer state participates in dispatch.
 */
void putscreen(s_screen *dest, s_screen *src, int x, int y, s_drawmethod *drawmethod)
{
    if(!dest || !src || !pixel_format_supported(dest->pixelformat) ||
       !pixel_format_supported(src->pixelformat))
    {
        return;
    }

    if(!gpu_draw_screen(dest, src, x, y, drawmethod))
    {
        gpu_report_rejected_image_operation(dest, GPU_OPERATION_SCREEN, drawmethod);
    }
}
