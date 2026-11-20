/*
 * OpenBOR - http://www.chronocrash.com
 * -----------------------------------------------------------------------
 * All rights reserved, see LICENSE in OpenBOR root for details.
 *
 * Copyright (c)  OpenBOR Team
 */

#ifndef VIDEO_H
#define VIDEO_H

#include <stddef.h>
#include "types.h"
#include "gpu.h"
#include "yuv.h"
#if SDL
#include "SDL.h"
#endif

extern int opengl;

int SetVideoMode(int, int);
void FramerateDelay();

// Initialize or release the supported RGB32 presentation mode.
int video_set_mode(s_videomodes);
int video_restore_mode(void);
int video_copy_screen(s_screen*);
void video_clearscreen();
void video_fullscreen_flip();
void video_stretch(int);
void video_set_window_title(const char*);
void video_set_color_correction(int, int);

#if defined(ANDROID) || defined(OPENBOR_ANDROID_SCENE_TEST)
/* Damon V. Caskey - 2026-11-20
 * Android owns the native GLES context. Lifecycle notifications are consumed
 * on the rendering thread; ordinary drawing must first have a usable context. */
void video_shutdown(void);
void video_handle_event(const SDL_Event *event);
int video_ensure_context(void);
#endif

/* Ordered launcher composition uses GPU drawing on every supported platform.
 * Begin precedes the background of each new frame. General native RGB32 screen
 * storage is shared with gameplay on desktop and Android. Presentation samples
 * the resident image without reading its CPU shadow. */
int video_launcher_begin(s_screen *target);
int video_launcher_screen(s_screen *target, s_screen *source, int x, int y);
int video_launcher_box(s_screen *target, int x, int y, int width, int height,
                       unsigned color);
#if defined(ANDROID) || defined(OPENBOR_ANDROID_SCENE_TEST)
/* The Android adapter tracks complete-frame eligibility while forwarding
 * glyphs to the same native scene renderer used by desktop. */
int video_launcher_text(s_screen *target, int x, int y,
                        unsigned foreground, unsigned background, int fill,
                        const unsigned char *glyphs, size_t count,
                        const unsigned char *font_rows, size_t font_glyph_count);
#endif

// for WebM video playback
int video_setup_yuv_overlay(const yuv_video_mode*);
int video_prepare_yuv_frame(const gpu_yuv_source*, e_drawmethod_resample);
int video_capture_yuv_frame(s_screen*, const gpu_yuv_source*, e_drawmethod_resample);
int video_display_yuv_frame(void);

int video_current_refresh_rate();

#endif
