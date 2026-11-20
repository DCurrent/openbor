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
 * Damon V. Caskey
 * 2026-11-20
 * One decoded GIF patch in tightly packed, top-to-bottom index order.
 * Palette holds 256 packed RGB32 colors. Transparency index -1 disables
 * keying; otherwise that exact source index preserves the canvas.
 * Addresses are borrowed only for this call, and never retained by the GPU.
 */
typedef struct gpu_indexed_frame
{
    const unsigned char *indices;
    const unsigned char *palette;
    int width, height, x, y, transparent;
} gpu_indexed_frame;

enum
{
    GPU_INDEXED_OPAQUE = -1,
    GPU_INDEXED_MAX_INDEX = 255
};

/*
 * Rendering dispatch is independent of the window system. Draw/clear/copy
 * callbacks return zero only before modifying their target. Scene drawing,
 * clearing and copying require RGB32 destinations and have no CPU renderer.
 * Callers log and stop after failure. Indexed source-asset maintenance and
 * explicit image access retain CPU data without providing scene rasterization.
 * Every image filter requires GPU drawing; rejected draws never substitute a
 * different sampler.
 * Successful native writes leave GPU storage authoritative until a CPU reader
 * requests synchronization; presenting or sampling that texture needs no copy.
 *
 * The backend owns this callback table for its entire registered lifetime.
 * Shutdown synchronizes surviving screens and evicts pointer-keyed resources.
 * Failed initialization and context teardown leave scene drawing unavailable.
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

    /* GIF patches retain their RGB32 canvas; RGB streams merge bytewise OR. */
    int (*compose_indexed)(s_screen *, const gpu_indexed_frame *);
    int (*merge_channels)(s_screen *, s_screen *, s_screen *, s_screen *);
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
void gpu_report_rejected_operation(const s_screen *screen, gpu_operation operation);
void gpu_report_rejected_image_operation(const s_screen *screen, gpu_operation operation,
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
int gpu_compose_indexed(s_screen *dest, const gpu_indexed_frame *frame);
int gpu_merge_channels(s_screen *dest, s_screen *red, s_screen *green, s_screen *blue);

#endif
