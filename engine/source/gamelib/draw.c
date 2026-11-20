/*
 * OpenBOR - http://www.chronocrash.com
 * -----------------------------------------------------------------------
 * All rights reserved, see LICENSE in OpenBOR root for details.
 *
 * Copyright (c)  OpenBOR Team
 */

#include "types.h"
#include "draw.h"
#include "gpu.h"

enum
{
    DRAW_PIXEL_SIZE = 1
};

/*
 * Damon V. Caskey
 * 2026-11-20
 * Submit primitive drawing exclusively to the GPU. Rejected operations log
 * through the common bounded diagnostic without reading or writing CPU pixels.
 */
void putbox(int x, int y, int width, int height, int colour, s_screen *screen, s_drawmethod *drawmethod)
{
    if(!screen || screen->pixelformat != PIXEL_32)
    {
        return;
    }

    if(!gpu_draw_box(screen, x, y, width, height, (unsigned)colour, drawmethod))
    {
        gpu_report_rejected_operation(screen, GPU_OPERATION_BOX);
    }
}

void putline(int sx, int sy, int ex, int ey, int colour, s_screen *screen, s_drawmethod *drawmethod)
{
    if(!screen || screen->pixelformat != PIXEL_32)
    {
        return;
    }

    if(!gpu_draw_line(screen, sx, sy, ex, ey, (unsigned)colour, drawmethod))
    {
        gpu_report_rejected_operation(screen, GPU_OPERATION_LINE);
    }
}

/*
 * Damon V. Caskey
 * 2026-11-20
 * Single pixels share the native rectangle material. Reject unsigned off-screen
 * coordinates before converting them to the signed rectangle API coordinates.
 */
void putpixel(unsigned x, unsigned y, int colour, s_screen *screen, s_drawmethod *drawmethod)
{
    if(!screen || screen->pixelformat != PIXEL_32)
    {
        return;
    }

    if(x >= (unsigned)screen->width || y >= (unsigned)screen->height)
    {
        return;
    }
    if(!gpu_draw_box(screen, (int)x, (int)y, DRAW_PIXEL_SIZE, DRAW_PIXEL_SIZE,
                     (unsigned)colour, drawmethod))
    {
        gpu_report_rejected_operation(screen, GPU_OPERATION_PIXEL);
    }
}
