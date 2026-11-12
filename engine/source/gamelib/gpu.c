/*
 * OpenBOR - http://www.chronocrash.com
 * All rights reserved, see LICENSE in OpenBOR root for details.
 * Copyright (c) OpenBOR Team
 */
#include <stdio.h>
#include <stdlib.h>
#include "gpu.h"
#include "engine_profile.h"

static const gpu_backend *active_backend;

/*
 * Damon V. Caskey
 * 2026-11-12
 * Install a fully initialized renderer. The backend owns its callback table
 * and must preserve it until it has drained and unregistered its resources.
 */
void gpu_register_backend(const gpu_backend *backend)
{
    /* Publish callbacks only after their context, shaders and resources are ready. */
    active_backend = backend;
}

/*
 * Damon V. Caskey
 * 2026-11-12
 * Return drawing to the software path after the backend has synchronized
 * live screens. Context teardown can then finish without accepting new draws.
 */
void gpu_unregister_backend(void)
{
    /* Subsequent drawing entry points will select their existing CPU paths. */
    active_backend = NULL;
}

/*
 * Damon V. Caskey
 * 2026-11-12
 * Report whether native GPU drawing is available in the current context.
 */
int gpu_is_active(void)
{
    /* Individual callbacks still decide whether a particular draw is supported. */
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
    /* An absent renderer or unsupported operation leaves ownership with the caller. */
    if(!active_backend || !active_backend->draw_screen)
    {
        engine_profile_fallback(EP_FALLBACK_NO_BACKEND);
        return 0;
    }

    /*
     * Give the backend the original screen identities so resident targets can
     * be sampled directly. Zero means the destination remains safe for the
     * caller's synchronized software fallback; any nonzero result is success.
     */
    return active_backend->draw_screen(dest, src, x, y, method) != 0;
}

/*
 * Damon V. Caskey
 * 2026-11-12
 * Submit an indexed sprite and its drawmethod to the active GPU renderer.
 */
int gpu_draw_sprite(s_screen *dest, s_sprite *sprite, int x, int y, const s_drawmethod *method)
{
    if(!active_backend || !active_backend->draw_sprite)
    {
        engine_profile_fallback(EP_FALLBACK_NO_BACKEND);
        return 0;
    }

    /* Sprite identity remains intact so the backend can reuse its decoded texture. */
    return active_backend->draw_sprite(dest, sprite, x, y, method) != 0;
}

/*
 * Damon V. Caskey
 * 2026-11-12
 * Submit a filled rectangle, including one-pixel dots, to GPU drawing.
 */
int gpu_draw_box(s_screen *dest, int x, int y, int width, int height, unsigned color, const s_drawmethod *method)
{
    if(!active_backend || !active_backend->draw_box)
    {
        engine_profile_fallback(EP_FALLBACK_NO_BACKEND);
        return 0;
    }

    /* One-pixel and larger rectangles share the same destination/coherence rules. */
    return active_backend->draw_box(dest, x, y, width, height, color, method) != 0;
}

/*
 * Damon V. Caskey
 * 2026-11-12
 * Submit a line when the backend can retain its rasterization contract.
 */
int gpu_draw_line(s_screen *dest, int x1, int y1, int x2, int y2, unsigned color, const s_drawmethod *method)
{
    if(!active_backend || !active_backend->draw_line)
    {
        engine_profile_fallback(EP_FALLBACK_NO_BACKEND);
        return 0;
    }

    /* Keep endpoint and drawmethod information available for exact native coverage. */
    return active_backend->draw_line(dest, x1, y1, x2, y2, color, method) != 0;
}

/*
 * Damon V. Caskey
 * 2026-11-12
 * Clear GPU storage directly instead of downloading and zeroing its shadow.
 */
int gpu_clear_screen(s_screen *screen)
{
    if(!active_backend || !active_backend->clear_screen)
    {
        engine_profile_fallback(EP_FALLBACK_NO_BACKEND);
        return 0;
    }

    /* Clearing the full image allows the backend to skip old-pixel reads. */
    return active_backend->clear_screen(screen) != 0;
}

/*
 * Damon V. Caskey
 * 2026-11-12
 * Copy compatible screens while keeping their pixels in GPU storage.
 */
int gpu_copy_screen(s_screen *dest, s_screen *src)
{
    if(!active_backend || !active_backend->copy_screen)
    {
        engine_profile_fallback(EP_FALLBACK_NO_BACKEND);
        return 0;
    }

    /* Preserve both owner identities so compatible copies stay in GPU storage. */
    return active_backend->copy_screen(dest, src) != 0;
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
    /* Software-only screens already expose the authoritative image to CPU readers. */
    if(!screen || !active_backend || !active_backend->sync_cpu)
    {
        return;
    }

    /*
     * The backend checks which copy is newer and downloads only when needed.
     * Failed synchronization must stop the CPU consumer before it reads or
     * modifies a stale shadow and destroys the last valid image version.
     */
    if(!active_backend->sync_cpu(screen))
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
    /*
     * Record the version change without uploading yet. Full-image replacement
     * can use this directly; partial edits first need gpu_prepare_cpu_write().
     */
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
    /* Preserve pixels outside the CPU writer's region before changing ownership. */
    gpu_sync_cpu(screen);

    /* The next GPU consumer must upload the image produced by this CPU write. */
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
    /* Evict the pointer-keyed cache entry before the allocation can be reused. */
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
    /* Decoded GPU data becomes invalid when the source sprite is replaced or freed. */
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
    /* In-place table edits can retain their pointers, so callers signal them explicitly. */
    if(active_backend && active_backend->invalidate_tables)
    {
        active_backend->invalidate_tables();
    }
}
