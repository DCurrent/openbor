/*
 * OpenBOR - http://www.chronocrash.com
 * Copyright (c) OpenBOR Team. All rights reserved. See LICENSE.
 */
#ifndef OPENBOR_GPU_OUTPUT_FILTER_ANDROID_H
#define OPENBOR_GPU_OUTPUT_FILTER_ANDROID_H

#include "SDL.h"

typedef struct gpu_output_filter_android gpu_output_filter_android;

/*
 * Damon V. Caskey
 * 2026-11-16
 * Keep Android output filtering on GLES2 while SDL retains its current scene,
 * movie and touch-control presentation. Source and result use SDL BGR888,
 * which stores one RGB32 pixel as red, green, blue and an unused byte;
 * the adapter owns its doubled result until destruction or the next apply.
 * Creation requires SDL's opengles2 renderer and a selected filter from 1..6.
 * Failed creation or application logs and returns NULL without a CPU retry.
 */
gpu_output_filter_android *gpu_output_filter_android_create(
    SDL_Renderer *renderer, int width, int height, int filter);
SDL_Texture *gpu_output_filter_android_apply(
    gpu_output_filter_android *filter, SDL_Texture *source);
void gpu_output_filter_android_destroy(gpu_output_filter_android *filter);

#endif
