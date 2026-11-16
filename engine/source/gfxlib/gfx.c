/*
 * OpenBOR - http://www.chronocrash.com
 * -----------------------------------------------------------------------
 * All rights reserved, see LICENSE in OpenBOR root for details.
 *
 * Copyright (c)  OpenBOR Team
 */

#include "gfx.h"

/*
 * Damon V. Caskey
 * 2026-11-16
 * Keep persisted output-filter selectors independent of the GPU backend.
 * Every enabled effect processes RGB32 textures; selector zero bypasses it.
 */
char *const GfxBlitterNames[BLITTER_MAX] =
{
    "Disabled",
    "Bilinear",
    "Advance Mame2x",
    "Scanlines",
    "Scanlines TV",
    "TV 2x",
    "Dot Matrix"
};
