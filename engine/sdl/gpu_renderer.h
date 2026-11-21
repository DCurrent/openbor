/*
 * OpenBOR - http://www.chronocrash.com
 * All rights reserved, see LICENSE in OpenBOR root for details.
 * Copyright (c) OpenBOR Team
 */
#ifndef OPENBOR_GPU_RENDERER_H
#define OPENBOR_GPU_RENDERER_H

#include "types.h"
#include "gpu.h"
#include <stddef.h>
#include <stdint.h>

/* Each resident atlas row owns a complete immutable palette snapshot. The
 * fixed limit bounds both GPU storage and the matching CPU content cache. */
#define GPU_PALETTE_COLORS 256
#define GPU_PALETTE_CACHE_CAPACITY 1024
#define GPU_PALETTE_BYTES (GPU_PALETTE_COLORS * sizeof(uint32_t))

/* Logical image row zero is texture row zero, matching the old upload path. */
int gpu_gl_init(void);
void gpu_gl_shutdown(void);
/* Android installs its render-thread lifecycle guard before using shared
 * screens. Suspension preserves newer GPU contents while the context exists.
 * Abandonment makes no GL calls and returns zero if authoritative pixels were
 * lost; that failure must stop recovery rather than expose stale CPU shadows. */
void gpu_gl_set_context_guard(int (*guard)(void));
int gpu_gl_suspend(void);
int gpu_gl_abandon(void);
/* Evict a private, reconstructible target after context loss with no GL calls.
 * Its CPU owner may survive recovery, but its discarded contents are invalid
 * until completely reconstructed and must not be presented before that redraw.
 * Surviving script screens use the preservation/abandonment contract above. */
void gpu_gl_discard_screen(s_screen *screen);
/* Successful replacement changes this value even if SDL reuses its handle. */
uint64_t gpu_gl_get_context_generation(void);
/* Borrowed textures remain renderer-owned. External consumers must restore
 * their NEAREST/CLAMP sampling policy before native drawing resumes. */
int gpu_gl_get_screen_texture(s_screen *screen, unsigned *texture, int *width, int *height);
/* Filter 0 borrows the original screen; filters 1..6 render into one reusable
 * exact 2x RGBA8 texture. The next filtered call may overwrite that texture.
 * Failure leaves caller outputs unchanged and never selects a CPU path. */
int gpu_gl_get_filtered_screen_texture(s_screen *screen, int filter,
                                       unsigned *texture, int *width, int *height);
/* Damon V. Caskey - 2026-11-20
 * Draw an opaque source into exact destination bounds with nearest sampling.
 * Indexed sources retain their palette, including opaque index zero. The
 * native quad preserves top-left sample phase without requiring affine support.
 * Preview-only boundary stabilization can choose an adjacent source texel for
 * extreme ratios; clamped source edges always retain opaque destination coverage. */
int gpu_gl_draw_screen_scaled(s_screen *target, s_screen *source,
                              int x, int y, int width, int height);
/* Pre-mapped launcher glyphs use an immutable 160-glyph, 5x10 bitmap font.
 * Text clips only to the RGB32 screen and submits one ordered native batch.
 * Empty/clipped runs succeed without upload; rejected runs have no CPU retry. */
int gpu_gl_draw_launcher_text(s_screen *screen, int x, int y,
                              unsigned foreground, unsigned background, int fill,
                              const unsigned char *glyphs, size_t count,
                              const unsigned char *font_rows, size_t font_glyph_count);
/* Restore presentation state only within the current owning context. This
 * never invokes the platform recovery guard after a texture was borrowed. */
void gpu_gl_prepare_present(void);
/* Movie presentation retains the native scene backend and source planes. */
int gpu_gl_present_yuv(const gpu_yuv_source *source, const gpu_movie_draw *draw,
                       int viewport_width, int viewport_height);
/* Call before another context owner changes tracked GL state. This forgets
 * cached assumptions without querying the driver or changing GL state. */
void gpu_gl_invalidate_state(void);

/* Internal diagnostics used by regression tests, not script-visible state. */
typedef struct gpu_renderer_stats
{
    unsigned long draws;
    unsigned long uploads;
    unsigned long readbacks;
    unsigned long destination_copies;
    unsigned long fallback_draws;
    unsigned long palette_cache_hits;
    unsigned long palette_cache_misses;
    unsigned long palette_cache_evictions;
    unsigned long palette_uploads;
    /* High-frequency setup totals need 64 bits on Windows LLP64 as well. */
    uint64_t state_changes;
    uint64_t state_reuses;
    /* Repeated affine instances emitted from their validated cached data. */
    uint64_t affine_preparation_reuses;
    /* Curves use reusable native meshes and a separate perspective stream. */
    uint64_t curve_draws, curve_mesh_builds, curve_mesh_reuses;
    uint64_t curve_depth_upload_bytes;
    /* Row effects distinguish actual one-quad draws from cached CPU recurrence
     * preparation and its small, separately accounted GPU data uploads. */
    uint64_t water_draws, plane_draws;
    uint64_t plane_map_builds, plane_map_reuses;
    uint64_t plane_map_uploads, plane_map_upload_bytes, plane_rows_prepared;
    /* One upload counts a complete three-plane frame, not each GL transfer. */
    uint64_t movie_uploads, movie_draws, movie_cache_hits, movie_resources;
    /* Indexed patches upload only their visible bytes; RGB canvases and
     * three-stream assembly stay resident until explicit CPU access. */
    uint64_t gif_frame_uploads, gif_frame_draws, gif_merge_draws, gif_frame_resources;
    /* Filter targets are private GPU-only storage; resources is 0 or 1. */
    uint64_t output_filter_draws, output_filter_allocations, output_filter_resources;
    /* Launcher font storage is shared by every text run in this context.
     * The glyph count records submitted visible cells, including blank cells. */
    uint64_t launcher_text_draws, launcher_text_glyphs;
    uint64_t launcher_font_uploads, launcher_font_resources;
} gpu_renderer_stats;
const gpu_renderer_stats *gpu_gl_get_stats(void);

/* Optional timer diagnostics retain context-local lifecycle totals. Pending
 * includes open scopes and ended pairs awaiting asynchronous availability. */
typedef struct gpu_query_stats
{
    int supported;
    int enabled;
    unsigned pending;
    uint64_t issued;
    uint64_t completed;
    uint64_t dropped;
    uint64_t deferred;
    uint64_t errors;
    uint64_t abandoned;
} gpu_query_stats;
const gpu_query_stats *gpu_gl_get_query_stats(void);

#endif
