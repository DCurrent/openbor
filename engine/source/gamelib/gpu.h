/*
 * OpenBOR - http://www.chronocrash.com
 * All rights reserved, see LICENSE in OpenBOR root for details.
 * Copyright (c) OpenBOR Team
 */
#ifndef OPENBOR_GPU_H
#define OPENBOR_GPU_H

#include "types.h"
#include <stdint.h>

/*
 * Damon V. Caskey
 * 2026-11-15
 * Describe tightly packed, even-sized 8-bit YUV420 decoder planes without
 * retaining their CPU addresses. The stable owner identifies one source;
 * generation changes whenever its decoded pixels change, including seeks.
 */
typedef struct gpu_yuv_source
{
    const void *owner;
    uint64_t generation;
    int width, height;
    const unsigned char *y, *u, *v;
} gpu_yuv_source;

/*
 * Damon V. Caskey
 * 2026-11-15
 * One opaque movie rectangle and its sampling policy. Correction follows
 * the established movie polynomial, with neutral zero and range [-1, 1].
 * Screen composition normally leaves correction to final presentation.
 */
typedef struct gpu_movie_draw
{
    int x, y, width, height;
    e_drawmethod_resample resample;
    int black_filter;
    float gamma, brightness;
} gpu_movie_draw;

/*
 * Rendering dispatch is independent of the window system. Draw/clear/copy
 * callbacks return zero only before modifying their target. RGB32 callers
 * selected for native rendering log and stop after failure; they never retry
 * through the CPU rasterizer. Unmigrated rendering configurations retain
 * explicit CPU dispatch and synchronized pixel access.
 * Explicit non-nearest image filters require GPU drawing in every configuration;
 * unsupported filtered draws log and stop before any CPU approximation.
 * Successful native writes leave GPU storage authoritative until a CPU reader
 * requests synchronization; presenting or sampling that texture needs no copy.
 *
 * The backend owns this callback table for its entire registered lifetime.
 * Shutdown synchronizes surviving screens and evicts pointer-keyed resources.
 * Unregistering does not change the selected rendering policy. Native-only
 * dispatch stays selected across failed initialization or context teardown.
 */
typedef struct gpu_backend
{
    /* Drawing callbacks accept the original owners and their requested draw state. */
    int (*draw_screen)(s_screen *, s_screen *, int, int, const s_drawmethod *);
    int (*draw_sprite)(s_screen *, s_sprite *, int, int, const s_drawmethod *);
    int (*draw_box)(s_screen *, int, int, int, int, unsigned, const s_drawmethod *);
    int (*draw_line)(s_screen *, int, int, int, int, unsigned, const s_drawmethod *);
    int (*clear_screen)(s_screen *);
    int (*copy_screen)(s_screen *, s_screen *);

    /* CPU readers synchronize; CPU writers mark the shadow as the newest version. */
    int (*sync_cpu)(s_screen *);
    void (*mark_cpu_dirty)(s_screen *);

    /* Free/re-encode paths evict identities before their source memory is reused. */
    void (*release_screen)(s_screen *);
    void (*release_sprite)(s_sprite *);

    /* Table edits can retain an address while changing all of its lookup values. */
    void (*invalidate_tables)(void);

    /* Movie planes stay resident across screen and fullscreen draws. */
    int (*draw_yuv)(s_screen *, const gpu_yuv_source *, const gpu_movie_draw *);
    void (*release_yuv)(const void *owner);
} gpu_backend;

/* Root dispatch uses separate warning bits so a failed operation logs once,
 * without adding formatting or file writes to subsequent draw failures. */
typedef enum gpu_operation
{
    GPU_OPERATION_SCREEN,
    GPU_OPERATION_SPRITE,
    GPU_OPERATION_BOX,
    GPU_OPERATION_LINE,
    GPU_OPERATION_PIXEL,
    GPU_OPERATION_CLEAR,
    GPU_OPERATION_COPY,
    GPU_OPERATION_PRESENT,
    GPU_OPERATION_COUNT
} gpu_operation;

/* Ordinary startup diagnostics are independent of optional frame profiling. */
void gpu_log(const char *format, ...);
void gpu_register_backend(const gpu_backend *backend);
void gpu_unregister_backend(void);
int gpu_is_active(void);
void gpu_set_native_required(int required);
int gpu_allow_cpu_operation(const s_screen *screen, gpu_operation operation);
int gpu_allow_cpu_image_operation(const s_screen *screen, gpu_operation operation,
                                  const s_drawmethod *method);
int gpu_draw_screen(s_screen *dest, s_screen *src, int x, int y, const s_drawmethod *method);
int gpu_draw_sprite(s_screen *dest, s_sprite *sprite, int x, int y, const s_drawmethod *method);
int gpu_draw_box(s_screen *dest, int x, int y, int width, int height, unsigned color, const s_drawmethod *method);
int gpu_draw_line(s_screen *dest, int x1, int y1, int x2, int y2, unsigned color, const s_drawmethod *method);
int gpu_clear_screen(s_screen *screen);
int gpu_copy_screen(s_screen *dest, s_screen *src);
void gpu_sync_cpu(s_screen *screen);
void gpu_mark_cpu_dirty(s_screen *screen);
void gpu_prepare_cpu_write(s_screen *screen);
void gpu_release_screen(s_screen *screen);
void gpu_release_sprite(s_sprite *sprite);
void gpu_invalidate_tables(void);
int gpu_draw_yuv(s_screen *dest, const gpu_yuv_source *source, const gpu_movie_draw *draw);
void gpu_release_yuv(const void *owner);

#endif
