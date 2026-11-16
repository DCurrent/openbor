/*
 * OpenBOR - http://www.chronocrash.com
 * -----------------------------------------------------------------------
 * All rights reserved, see LICENSE in OpenBOR root for details.
 *
 * Copyright (c)  OpenBOR Team
 */

#include "videocommon.h"
#include "savedata.h"
#include "gpu.h"
#include <limits.h>
#include <stddef.h>

static s_videosurface videoSurface;

/*
 * Damon V. Caskey
 * 2026-11-16
 * Preserve authored RGB32 dimensions and apply the selected window scale.
 * Output filters own GPU-only 2x targets and never change scene color format.
 */
s_videomodes setupPreBlitProcessing(s_videomodes videomodes)
{
    /* Reject invalid scene formats without allocating conversion surfaces. */
    if(videomodes.pixel != 4)
    {
        gpu_log("ERROR: Presentation requires an RGB32 scene.");
        videomodes.pixel = 0;
        return videomodes;
    }
    videomodes.hScale = savedata.hwscale;
    videomodes.vScale = savedata.hwscale;
    return videomodes;
}

/*
 * Damon V. Caskey
 * 2026-11-16
 * Expose the RGB32 source for Android's remaining CPU-scene upload. Output
 * filtering runs after that upload on the GPU; desktop presents resident
 * textures directly and never requests this CPU view.
 */
s_videosurface *getVideoSurface(s_screen *src)
{
    /* Engine rows are tightly packed; validate their byte stride first. */
    if(!src || src->pixelformat != PIXEL_32 || src->width <= 0 ||
       src->height <= 0 || src->width > INT_MAX / 4)
    {
        return NULL;
    }
    gpu_sync_cpu(src);
    videoSurface.width = src->width;
    videoSurface.height = src->height;
    videoSurface.pitch = src->width * 4;
    videoSurface.data = src->data;
    return &videoSurface;
}
