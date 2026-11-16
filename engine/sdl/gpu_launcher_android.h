/*
 * OpenBOR - http://www.chronocrash.com
 * Copyright (c) OpenBOR Team. All rights reserved. See LICENSE.
 */
#ifndef OPENBOR_GPU_LAUNCHER_ANDROID_H
#define OPENBOR_GPU_LAUNCHER_ANDROID_H

#include <stddef.h>
#include <stdint.h>
#include "SDL.h"
#include "types.h"

typedef struct gpu_launcher_android gpu_launcher_android;

typedef struct gpu_launcher_android_stats
{
    uint64_t frames, draws, text_draws, text_glyphs;
    uint64_t source_uploads, palette_uploads, font_uploads, rejections;
} gpu_launcher_android_stats;

/*
 * Damon V. Caskey
 * 2026-11-16
 * Own a native SDL BGR888/RGB32 launcher target in its GLES2 context. Commands
 * execute immediately in call order; begin clears opaque black and starts a
 * new frame. Every rejection invalidates the frame until the next begin.
 * Texture returns a borrowed completed command stream only while that frame
 * remains valid. No operation reads back pixels or retries on the CPU.
 */
gpu_launcher_android *gpu_launcher_android_create(SDL_Renderer *renderer, int width, int height);
int gpu_launcher_android_begin(gpu_launcher_android *launcher);
int gpu_launcher_android_screen(gpu_launcher_android *launcher, const s_screen *source, int x, int y);
int gpu_launcher_android_box(gpu_launcher_android *launcher, int x, int y,
                             int width, int height, unsigned color);
int gpu_launcher_android_text(gpu_launcher_android *launcher, int x, int y,
                              unsigned foreground, unsigned background, int fill,
                              const unsigned char *glyphs, size_t count,
                              const unsigned char *font_rows, size_t font_glyph_count);
void gpu_launcher_android_invalidate(gpu_launcher_android *launcher);
SDL_Texture *gpu_launcher_android_texture(gpu_launcher_android *launcher);
const gpu_launcher_android_stats *gpu_launcher_android_get_stats(const gpu_launcher_android *launcher);
void gpu_launcher_android_destroy(gpu_launcher_android *launcher);
/* Device/context loss retires metadata and SDL-owned storage without deleting
 * private raw GL names that may already identify objects in a new context. */
void gpu_launcher_android_abandon(gpu_launcher_android *launcher);

#endif
