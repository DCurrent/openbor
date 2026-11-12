/*
 * OpenBOR - http://www.chronocrash.com
 * All rights reserved, see LICENSE in OpenBOR root for details.
 * Copyright (c) OpenBOR Team
 */
#include <stdio.h>
#include <stdlib.h>
#include "gpu.h"

static const gpu_backend *active_backend;

/*
 * Damon V. Caskey
 * 2026-11-12
 * Install a fully initialized renderer. The backend owns its callback table
 * and must preserve it until it has drained and unregistered its resources.
 */
void gpu_register_backend(const gpu_backend *backend)
{
    active_backend = backend;
}

/*
 * Damon V. Caskey
 * 2026-11-12
 * Return drawing to the software path after the backend has synchronized
 * live screens and released its graphics-context resources.
 */
void gpu_unregister_backend(void)
{
    active_backend = NULL;
}

/*
 * Damon V. Caskey
 * 2026-11-12
 * Report whether native GPU drawing is available in the current context.
 */
int gpu_is_active(void)
{
    return active_backend != NULL;
}

/*
 * Damon V. Caskey
 * 2026-11-12
 * Try a screen draw without synchronizing CPU pixels. Unsupported draws
 * return zero and are completed by the caller's existing software path.
 */
int gpu_draw_screen(s_screen *dest, s_screen *src, int x, int y, const s_drawmethod *method)
{
    return active_backend && active_backend->draw_screen &&
        active_backend->draw_screen(dest, src, x, y, method);
}

/*
 * Damon V. Caskey
 * 2026-11-12
 * Submit an indexed sprite and its drawmethod to the active GPU renderer.
 */
int gpu_draw_sprite(s_screen *dest, s_sprite *sprite, int x, int y, const s_drawmethod *method)
{
    return active_backend && active_backend->draw_sprite &&
        active_backend->draw_sprite(dest, sprite, x, y, method);
}

/*
 * Damon V. Caskey
 * 2026-11-12
 * Submit a filled rectangle, including one-pixel dots, to GPU drawing.
 */
int gpu_draw_box(s_screen *dest, int x, int y, int width, int height, unsigned color, const s_drawmethod *method)
{
    return active_backend && active_backend->draw_box &&
        active_backend->draw_box(dest, x, y, width, height, color, method);
}

/*
 * Damon V. Caskey
 * 2026-11-12
 * Submit a line when the backend can retain its rasterization contract.
 */
int gpu_draw_line(s_screen *dest, int x1, int y1, int x2, int y2, unsigned color, const s_drawmethod *method)
{
    return active_backend && active_backend->draw_line &&
        active_backend->draw_line(dest, x1, y1, x2, y2, color, method);
}

/*
 * Damon V. Caskey
 * 2026-11-12
 * Clear GPU storage directly instead of downloading and zeroing its shadow.
 */
int gpu_clear_screen(s_screen *screen)
{
    return active_backend && active_backend->clear_screen &&
        active_backend->clear_screen(screen);
}

/*
 * Damon V. Caskey
 * 2026-11-12
 * Copy compatible screens while keeping their pixels in GPU storage.
 */
int gpu_copy_screen(s_screen *dest, s_screen *src)
{
    return active_backend && active_backend->copy_screen &&
        active_backend->copy_screen(dest, src);
}

/*
 * Damon V. Caskey
 * 2026-11-12
 * Make the CPU shadow current only when a CPU reader actually needs it.
 * Readback failure cannot safely fall through with stale pixels, so stop
 * explicitly rather than silently corrupting subsequent effects or output.
 */
void gpu_sync_cpu(s_screen *screen)
{
    if(screen && active_backend && active_backend->sync_cpu &&
       !active_backend->sync_cpu(screen))
    {
        fprintf(stderr, "GPU renderer: unable to synchronize screen pixels.\n");
        abort();
    }
}

/*
 * Damon V. Caskey
 * 2026-11-12
 * Mark a CPU-written screen for upload before its next GPU use. Callers
 * replacing only part of an image must synchronize it before this operation.
 */
void gpu_mark_cpu_dirty(s_screen *screen)
{
    if(screen && active_backend && active_backend->mark_cpu_dirty)
    {
        active_backend->mark_cpu_dirty(screen);
    }
}

/*
 * Damon V. Caskey
 * 2026-11-12
 * Preserve GPU-produced contents before a partial software write, then
 * transfer ownership of the new image version to its CPU shadow.
 */
void gpu_prepare_cpu_write(s_screen *screen)
{
    gpu_sync_cpu(screen);
    gpu_mark_cpu_dirty(screen);
}

/*
 * Damon V. Caskey
 * 2026-11-12
 * Forget a screen before its CPU allocation can be freed or reused. The
 * backend handles GPU object lifetime without reading pixels being discarded.
 */
void gpu_release_screen(s_screen *screen)
{
    if(screen && active_backend && active_backend->release_screen)
    {
        active_backend->release_screen(screen);
    }
}

/*
 * Damon V. Caskey
 * 2026-11-12
 * Evict decoded sprite storage before a sprite is freed or re-encoded.
 */
void gpu_release_sprite(s_sprite *sprite)
{
    if(sprite && active_backend && active_backend->release_sprite)
    {
        active_backend->release_sprite(sprite);
    }
}

/*
 * Damon V. Caskey
 * 2026-11-12
 * Invalidate GPU copies after the engine replaces its blend lookup tables.
 */
void gpu_invalidate_tables(void)
{
    if(active_backend && active_backend->invalidate_tables)
    {
        active_backend->invalidate_tables();
    }
}
