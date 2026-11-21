/*
 * OpenBOR - http://www.chronocrash.com
 * Copyright (c) OpenBOR Team. All rights reserved. See LICENSE.
 */
#include "video_settings.h"
#include <math.h>

/*
 * Damon V. Caskey
 * 2026-11-20
 * Migrate only display controls. Legacy desktop OpenGL ignored hwfilter and
 * always used High Quality; SDL and Android used its nearest/bilinear value.
 * Keep unrelated fields, final-filter IDs, and the on-disk structure intact.
 * Validate scale before any floating-to-integer window-size conversion.
 */
void video_settings_normalize(s_savedata *settings, int android)
{
#if SDL
    int default_filter = android ? VIDEO_SCALING_NEAREST : VIDEO_SCALING_HIGH_QUALITY;
    if(!settings) return;

    if(settings->usegl != VIDEO_SETTINGS_NATIVE)
    {
        if(!android && settings->usegl != 0)
            settings->hwfilter = VIDEO_SCALING_HIGH_QUALITY;
        else
            settings->hwfilter = settings->hwfilter == VIDEO_SCALING_BILINEAR ?
                VIDEO_SCALING_BILINEAR : VIDEO_SCALING_NEAREST;
    }
    else if(settings->hwfilter < 0 || settings->hwfilter >= VIDEO_SCALING_COUNT)
    {
        settings->hwfilter = default_filter;
    }
    settings->usegl = VIDEO_SETTINGS_NATIVE;

    if(!isfinite(settings->hwscale))
        settings->hwscale = android ? VIDEO_SCALE_AUTOMATIC : 1.0f;
    else if(android && settings->hwscale <= VIDEO_SCALE_AUTOMATIC)
        settings->hwscale = VIDEO_SCALE_AUTOMATIC;
    else if(settings->hwscale < VIDEO_SCALE_MIN)
        settings->hwscale = VIDEO_SCALE_MIN;
    else if(settings->hwscale > VIDEO_SCALE_MAX)
        settings->hwscale = VIDEO_SCALE_MAX;
#else
    (void)settings;
    (void)android;
#endif
}

/* Labels describe final scaling; translation belongs to the calling UI. */
const char *video_scaling_filter_name(int filter)
{
    switch(filter)
    {
    case VIDEO_SCALING_BILINEAR: return "Bilinear";
    case VIDEO_SCALING_NEAREST: return "Nearest";
    case VIDEO_SCALING_HIGH_QUALITY: return "High Quality";
    default: return "Unknown";
    }
}
