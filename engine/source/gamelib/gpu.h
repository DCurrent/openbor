/*
 * OpenBOR - http://www.chronocrash.com
 * All rights reserved, see LICENSE in OpenBOR root for details.
 * Copyright (c) OpenBOR Team
 */
#ifndef OPENBOR_GPU_H
#define OPENBOR_GPU_H

#include "types.h"

/*
 * Rendering dispatch is independent of the window system. Draw/clear/copy
 * callbacks return zero only before modifying their target. The caller can
 * then synchronize the relevant CPU storage and execute software fallback.
 * Successful native writes leave GPU storage authoritative until a CPU reader
 * requests synchronization; presenting or sampling that texture needs no copy.
 *
 * The backend owns this callback table for its entire registered lifetime.
 * Shutdown synchronizes surviving screens and evicts pointer-keyed resources.
 * Unregistering restores CPU-only dispatch while the backend finishes the
 * remaining graphics-context cleanup.
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
} gpu_backend;

/* Ordinary startup diagnostics are independent of optional frame profiling. */
void gpu_log(const char *format, ...);
void gpu_register_backend(const gpu_backend *backend);
void gpu_unregister_backend(void);
int gpu_is_active(void);
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

#endif
