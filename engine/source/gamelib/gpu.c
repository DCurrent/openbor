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
static unsigned declined_operation_warnings;
static unsigned declined_resample_warnings;
static int declined_movie_warning;
static int declined_gif_frame_warning, declined_gif_merge_warning;
static const char *const operation_names[GPU_OPERATION_COUNT] = {
    "screen draw", "sprite draw", "rectangle draw", "line draw",
    "pixel draw", "screen clear", "screen copy", "scene presentation"
};

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
 * 2026-11-20 - Reset bounded failure diagnostics for each registered backend.
 * Install a fully initialized renderer. The backend owns its callback table
 * and must preserve it until it has drained and unregistered its resources.
 */
void gpu_register_backend(const gpu_backend *backend)
{
    /* Publish callbacks only after their context, shaders and resources are ready. */
    active_backend = backend;
    declined_movie_warning = 0;
    declined_gif_frame_warning = declined_gif_merge_warning = 0;
    if(backend)
    {
        declined_operation_warnings = 0;
        declined_resample_warnings = 0;
    }
}

/*
 * Damon V. Caskey
 * 2026-11-12
 * 2026-11-20 - Scene rendering has no software implementation to enable.
 * Retire callbacks after the backend has synchronized live screens. Preserve
 * failure diagnostics across context loss until a new backend is registered.
 */
void gpu_unregister_backend(void)
{
    active_backend = NULL;
}

/*
 * Damon V. Caskey
 * 2026-11-20
 * Report a rejected scene operation without changing image ownership or data.
 * Rendering is GPU-only from process startup through context teardown.
 */
void gpu_report_rejected_operation(const s_screen *screen, gpu_operation operation)
{
    unsigned warning;

    if(!screen || !pixel_format_supported(screen->pixelformat))
    {
        return;
    }

    /* A fixed bit check is the only repeated failure-path work. Keep the
     * ordinary log usable even when optional profiling is disabled. */
    if(operation >= 0 && operation < GPU_OPERATION_COUNT)
    {
        warning = 1u << operation;
        if(!(declined_operation_warnings & warning))
        {
            declined_operation_warnings |= warning;
            gpu_log("WARNING: Native %s unavailable or rejected (target=%u); operation skipped. CPU scene rendering is unavailable.",
                    operation_names[operation], (unsigned)screen->pixelformat);
        }
    }
}

/*
 * Damon V. Caskey
 * 2026-11-20
 * Include the selected filter in bounded image-draw failure diagnostics.
 * Failed draws do not synchronize, alter pixels or substitute another sampler.
 */
void gpu_report_rejected_image_operation(const s_screen *screen, gpu_operation operation,
                                         const s_drawmethod *method)
{
    unsigned warning;
    if(!screen || !pixel_format_supported(screen->pixelformat))
    {
        return;
    }
    if(method && (method->config & DRAWMETHOD_CONFIG_ENABLED) &&
       method->resample != DC_RESAMPLE_NEAREST)
    {
        /* One record per operation survives missing callbacks and backend loss. */
        if(operation >= 0 && operation < GPU_OPERATION_COUNT)
        {
            warning = 1u << operation;
            if(!(declined_resample_warnings & warning))
            {
                declined_resample_warnings |= warning;
                gpu_log("WARNING: GPU resampling for %s unavailable or rejected "
                        "(resample=%d, target=%u); operation skipped. CPU resampling is unavailable.",
                        operation_names[operation], (int)method->resample,
                        (unsigned)screen->pixelformat);
            }
        }
        return;
    }
    gpu_report_rejected_operation(screen, operation);
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
 * 2026-11-15
 * Reject obsolete image formats before any backend callback can change image
 * ownership. Retain the normal format diagnostic for rejected native draws.
 */
static int gpu_screen_format_valid(const s_screen *screen, ep_fallback reason)
{
    if(!screen || !pixel_format_supported(screen->pixelformat))
    {
        engine_profile_fallback(reason);
        return 0;
    }
    return 1;
}

/*
 * Damon V. Caskey
 * 2026-11-20
 * Indexed images are source assets only. Reject other destinations before a
 * backend callback can claim ownership or modify a scene target.
 */
static int gpu_scene_target_valid(const s_screen *screen)
{
    if(!screen || screen->pixelformat != PIXEL_32)
    {
        engine_profile_fallback(EP_FALLBACK_TARGET_FORMAT);
        return 0;
    }
    return 1;
}

/*
 * Damon V. Caskey
 * 2026-11-12
 * 2026-11-15 - Validate both image formats before backend dispatch.
 * Try a screen draw without synchronizing CPU pixels. Unsupported draws
 * return zero; root dispatch logs the failure without changing CPU pixels.
 */
int gpu_draw_screen(s_screen *dest, s_screen *src, int x, int y, const s_drawmethod *method)
{
    if(!gpu_scene_target_valid(dest) ||
       !gpu_screen_format_valid(src, EP_FALLBACK_SOURCE_FORMAT))
    {
        return 0;
    }
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
 * 2026-11-15 - Validate the destination and indexed sprite/mask formats.
 * Submit an indexed sprite and its drawmethod to the active GPU renderer.
 */
int gpu_draw_sprite(s_screen *dest, s_sprite *sprite, int x, int y, const s_drawmethod *method)
{
    if(!gpu_scene_target_valid(dest))
    {
        return 0;
    }
    if(!sprite || !pixel_format_indexed(sprite->pixelformat))
    {
        engine_profile_fallback(EP_FALLBACK_SOURCE_FORMAT);
        return 0;
    }
    if(sprite->mask && !pixel_format_indexed(sprite->mask->pixelformat))
    {
        engine_profile_fallback(EP_FALLBACK_MASK);
        return 0;
    }
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
 * 2026-11-15 - Reject unsupported target formats before backend dispatch.
 * Submit a filled rectangle, including one-pixel dots, to GPU drawing.
 */
int gpu_draw_box(s_screen *dest, int x, int y, int width, int height, unsigned color, const s_drawmethod *method)
{
    if(!gpu_scene_target_valid(dest))
    {
        return 0;
    }
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
 * 2026-11-15 - Reject unsupported target formats before backend dispatch.
 * Submit a line when the backend can retain its rasterization contract.
 */
int gpu_draw_line(s_screen *dest, int x1, int y1, int x2, int y2, unsigned color, const s_drawmethod *method)
{
    if(!gpu_scene_target_valid(dest))
    {
        return 0;
    }
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
 * 2026-11-15 - Reject unsupported target formats before backend dispatch.
 * Clear GPU storage directly instead of downloading and zeroing its shadow.
 */
int gpu_clear_screen(s_screen *screen)
{
    if(!gpu_scene_target_valid(screen))
    {
        return 0;
    }
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
 * 2026-11-15 - Validate both image formats before backend dispatch.
 * Copy compatible screens while keeping their pixels in GPU storage.
 */
int gpu_copy_screen(s_screen *dest, s_screen *src)
{
    if(!gpu_scene_target_valid(dest) ||
       !gpu_screen_format_valid(src, EP_FALLBACK_SOURCE_FORMAT))
    {
        return 0;
    }
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
 * 2026-11-15
 * Compose decoded planes through the native renderer only. Failed movie
 * draws leave the target unchanged and report once per backend lifetime;
 * no CPU conversion, scaling or copy is a substitute for this operation.
 */
int gpu_draw_yuv(s_screen *dest, const gpu_yuv_source *source, const gpu_movie_draw *draw)
{
    if(dest && dest->pixelformat == PIXEL_32 && source && draw &&
       active_backend && active_backend->draw_yuv &&
       active_backend->draw_yuv(dest, source, draw))
    {
        return 1;
    }
    if(!active_backend || !active_backend->draw_yuv)
    {
        engine_profile_fallback(EP_FALLBACK_NO_BACKEND);
    }
    if(!declined_movie_warning)
    {
        declined_movie_warning = 1;
        gpu_log("WARNING: Native movie drawing unavailable or rejected; movie draw skipped. CPU conversion and rendering retry are disabled.");
    }
    return 0;
}

/*
 * Damon V. Caskey
 * 2026-11-15
 * Retire a movie source before its stable owner is reset or reused. Context
 * shutdown already discards these recreatable textures without readback.
 */
void gpu_release_yuv(const void *owner)
{
    if(owner && active_backend && active_backend->release_yuv)
    {
        active_backend->release_yuv(owner);
    }
}

/*
 * Damon V. Caskey
 * 2026-11-20
 * Resolve indexed GIF colors and transparent patches on the native canvas.
 * Failure leaves its logical contents unchanged and has no CPU substitute.
 */
int gpu_compose_indexed(s_screen *dest, const gpu_indexed_frame *frame)
{
    if(dest && dest->pixelformat == PIXEL_32 && frame &&
       active_backend && active_backend->compose_indexed &&
       active_backend->compose_indexed(dest, frame))
    {
        return 1;
    }
    if(!active_backend || !active_backend->compose_indexed)
    {
        engine_profile_fallback(EP_FALLBACK_NO_BACKEND);
    }
    if(!declined_gif_frame_warning)
    {
        declined_gif_frame_warning = 1;
        gpu_log("WARNING: Native GIF frame composition unavailable or rejected; frame unchanged. CPU palette conversion and rendering retry are disabled.");
    }
    return 0;
}

/*
 * Damon V. Caskey
 * 2026-11-20
 * Assemble OpenBOR's three GIF streams without downloading their canvases.
 * All RGB bytes retain the established bitwise OR, including script edits.
 */
int gpu_merge_channels(s_screen *dest, s_screen *red, s_screen *green, s_screen *blue)
{
    if(dest && dest->pixelformat == PIXEL_32 && red && green && blue &&
       active_backend && active_backend->merge_channels &&
       active_backend->merge_channels(dest, red, green, blue))
    {
        return 1;
    }
    if(!active_backend || !active_backend->merge_channels)
    {
        engine_profile_fallback(EP_FALLBACK_NO_BACKEND);
    }
    if(!declined_gif_merge_warning)
    {
        declined_gif_merge_warning = 1;
        gpu_log("WARNING: Native GIF channel composition unavailable or rejected; frame unchanged. CPU merging and rendering retry are disabled.");
    }
    return 0;
}

/*
 * Damon V. Caskey
 * 2026-11-12
 * 2026-11-15 - Ignore unsupported image formats before ownership callbacks.
 * Make the CPU shadow current only when a CPU reader actually needs it.
 * Readback failure cannot safely fall through with stale pixels, so stop
 * explicitly rather than silently corrupting subsequent effects or output.
 */
void gpu_sync_cpu(s_screen *screen)
{
    /* CPU-owned images already expose their authoritative pixels to readers. */
    if(!screen || !pixel_format_supported(screen->pixelformat) ||
       !active_backend || !active_backend->sync_cpu)
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
 * 2026-11-15 - Ignore unsupported image formats before ownership callbacks.
 * Mark a CPU-written screen for upload before its next GPU use. Callers
 * replacing only part of an image must synchronize it before this operation.
 */
void gpu_mark_cpu_dirty(s_screen *screen)
{
    /*
     * Record the version change without uploading yet. Full-image replacement
     * can use this directly; partial edits first need gpu_prepare_cpu_write().
     */
    if(screen && pixel_format_supported(screen->pixelformat) &&
       active_backend && active_backend->mark_cpu_dirty)
    {
        active_backend->mark_cpu_dirty(screen);
    }
}

/*
 * Damon V. Caskey
 * 2026-11-12
 * Preserve GPU-produced contents before an explicit partial image edit, then
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
