/*
 * OpenBOR - http://www.chronocrash.com
 * Copyright (c) OpenBOR Team. All rights reserved. See LICENSE.
 */
#ifndef OPENBOR_VIDEO_SETTINGS_H
#define OPENBOR_VIDEO_SETTINGS_H

#include "savedata.h"

/*
 * Damon V. Caskey
 * 2026-11-20
 * Final display sampling is independent of scene output effects and of each
 * object's drawmethod resampling. Preserve the old nearest/bilinear values.
 */
typedef enum e_video_scaling_filter
{
    VIDEO_SCALING_BILINEAR = 0,
    VIDEO_SCALING_NEAREST = 1,
    VIDEO_SCALING_HIGH_QUALITY = 2,
    VIDEO_SCALING_COUNT
} e_video_scaling_filter;

/* The former backend-preference slot marks the new sampling semantics without
 * changing the packed settings layout. Old values are 0 (SDL) and 1 (OpenGL).
 * Both remain readable; a migrated file keeps its explicit sampling choice. */
#define VIDEO_SETTINGS_NATIVE 2

#define VIDEO_SCALE_AUTOMATIC 0.0f
#define VIDEO_SCALE_MIN 0.25f
#define VIDEO_SCALE_MAX 8.0f
#define VIDEO_SCALE_STEP 0.25f

#if ANDROID
#define VIDEO_SETTINGS_ANDROID 1
#else
#define VIDEO_SETTINGS_ANDROID 0
#endif

/* Stable names keep menu navigation and actions independent of row numbers. */
typedef enum e_video_option
{
    VIDEO_OPTION_BRIGHTNESS,
    VIDEO_OPTION_GAMMA,
    VIDEO_OPTION_WINDOW_OFFSET,
    VIDEO_OPTION_DISPLAY_MODE,
    VIDEO_OPTION_WINDOW_SCALE,
    VIDEO_OPTION_SCALING_FILTER,
    VIDEO_OPTION_FINAL_FILTER,
    VIDEO_OPTION_FPS_LIMIT,
    VIDEO_OPTION_FULLSCREEN_TYPE,
    VIDEO_OPTION_BACK,
    VIDEO_OPTION_COUNT
} e_video_option;

void video_settings_normalize(s_savedata *settings, int android);
const char *video_scaling_filter_name(int filter);

#endif
