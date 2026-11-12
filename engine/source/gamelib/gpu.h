/*
 * OpenBOR - http://www.chronocrash.com
 * All rights reserved, see LICENSE in OpenBOR root for details.
 * Copyright (c) OpenBOR Team
 */
#ifndef OPENBOR_GPU_H
#define OPENBOR_GPU_H

#include "types.h"

/*
 * Rendering dispatch is independent of the window system. A backend returns
 * zero only BEFORE modifying a target, allowing the existing software path
 * to run against synchronized CPU storage. Screens retain their CPU shadow
 * during migration; it is not kept current after every GPU draw.
 */
typedef struct gpu_backend
{
    int (*draw_screen)(s_screen *, s_screen *, int, int, const s_drawmethod *);
    int (*draw_sprite)(s_screen *, s_sprite *, int, int, const s_drawmethod *);
    int (*draw_box)(s_screen *, int, int, int, int, unsigned, const s_drawmethod *);
    int (*draw_line)(s_screen *, int, int, int, int, unsigned, const s_drawmethod *);
    int (*clear_screen)(s_screen *);
    int (*copy_screen)(s_screen *, s_screen *);
    int (*sync_cpu)(s_screen *);
    void (*mark_cpu_dirty)(s_screen *);
    void (*release_screen)(s_screen *);
    void (*release_sprite)(s_sprite *);
    void (*invalidate_tables)(void);
} gpu_backend;

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
