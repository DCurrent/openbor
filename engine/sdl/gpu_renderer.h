/*
 * OpenBOR - http://www.chronocrash.com
 * All rights reserved, see LICENSE in OpenBOR root for details.
 * Copyright (c) OpenBOR Team
 */
#ifndef OPENBOR_GPU_RENDERER_H
#define OPENBOR_GPU_RENDERER_H

#include "types.h"

/* Logical image row zero is texture row zero, matching the old upload path. */
int gpu_gl_init(void);
void gpu_gl_shutdown(void);
int gpu_gl_get_screen_texture(s_screen *screen, unsigned *texture, int *width, int *height);
void gpu_gl_prepare_present(void);

/* Internal diagnostics used by regression tests, not script-visible state. */
typedef struct gpu_renderer_stats
{
    unsigned long draws;
    unsigned long uploads;
    unsigned long readbacks;
    unsigned long destination_copies;
    unsigned long fallback_draws;
} gpu_renderer_stats;
const gpu_renderer_stats *gpu_gl_get_stats(void);

#endif
