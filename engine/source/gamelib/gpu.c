/*
 * OpenBOR - http://www.chronocrash.com
 * All rights reserved, see LICENSE in OpenBOR root for details.
 * Copyright (c) OpenBOR Team
 */
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include "gpu.h"
#include "engine_profile.h"
#ifndef PP_TEST
#include "utils.h"
#endif

static const gpu_backend *active_backend;
static int native_required;
static unsigned declined_operation_warnings;

/*
 * Damon V. Caskey
 * 2026-11-13
 * Keep renderer availability and performance warnings in the ordinary engine
 * log even when profiling is disabled. Callers supply startup diagnostics,
 * rather than adding formatting or file writes to individual drawing calls.
 */
void gpu_log(const char *format, ...)
{
    char message[4096];
    va_list arguments;
    if(!format)
    {
        return;
    }

    /* Bound driver-provided diagnostics and treat their contents as data. */
    va_start(arguments, format);
    vsnprintf(message, sizeof(message), format, arguments);
    va_end(arguments);
#ifndef PP_TEST
    writeToLogFile("[GPU] %s\n", message);
#else
    /* Standalone fixtures capture the same records without the game runtime. */
    fprintf(stdout, "[GPU] %s\n", message);
#endif
}

/*
 * Damon V. Caskey
 * 2026-11-12
 * 2026-11-15 - Select native-only RGB32 dispatch with the registered renderer.
 * Install a fully initialized renderer. The backend owns its callback table
 * and must preserve it until it has drained and unregistered its resources.
 */
void gpu_register_backend(const gpu_backend *backend)
{
    /* Publish callbacks only after their context, shaders and resources are ready. */
    active_backend = backend;
    if(backend)
    {
        gpu_set_native_required(1);
    }
}

/*
 * Damon V. Caskey
 * 2026-11-12
 * 2026-11-15 - Retain native-only policy across context loss and teardown.
 * Retire callbacks after the backend has synchronized live screens. Losing
 * their context must not silently enable CPU rasterization for RGB32 draws.
 */
void gpu_unregister_backend(void)
{
    /* Mode selection, rather than backend lifetime, controls CPU eligibility. */
    active_backend = NULL;
}

/*
 * Damon V. Caskey
 * 2026-11-15
 * Select the scene-rendering policy before attempting native initialization.
 * Only an explicitly unmigrated configuration may select CPU dispatch; failed
 * initialization and unregistering a backend never clear this requirement.
 */
void gpu_set_native_required(int required)
{
    /* Preserve one warning per operation throughout an unchanged selection. */
    if(native_required != (required != 0))
    {
        declined_operation_warnings = 0;
    }
    native_required = required != 0;
}

/*
 * Damon V. Caskey
 * 2026-11-15
 * Decide whether a declined native root operation still requires CPU work.
 * Migrated RGB32 scene targets log once per operation and remain unchanged;
 * indexed image work and explicitly unmigrated modes keep their CPU paths.
 */
int gpu_allow_cpu_operation(const s_screen *screen, gpu_operation operation)
{
    static const char *const names[GPU_OPERATION_COUNT] = {
        "screen draw", "sprite draw", "rectangle draw", "line draw",
        "pixel draw", "screen clear", "screen copy", "scene presentation"
    };
    unsigned warning;

    /* Invalid destinations cannot be rasterized by either implementation. */
    if(!screen)
    {
        return 0;
    }
    if(!native_required || screen->pixelformat != PIXEL_32)
    {
        return 1;
    }

    /* A fixed bit check is the only repeated failure-path work. Keep the
     * ordinary log usable even when optional profiling is disabled. */
    if(operation >= 0 && operation < GPU_OPERATION_COUNT)
    {
        warning = 1u << operation;
        if(!(declined_operation_warnings & warning))
        {
            declined_operation_warnings |= warning;
            gpu_log("WARNING: Native %s unavailable or rejected; RGB32 operation skipped. CPU rendering retry is disabled.",
                    names[operation]);
        }
    }
    return 0;
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
 * return zero; root dispatch applies the selected no-retry rendering policy.
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
     * be sampled directly. Zero preserves the failure result so root dispatch
     * can log it without falsely reporting a completed native draw.
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
