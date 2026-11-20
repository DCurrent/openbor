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
