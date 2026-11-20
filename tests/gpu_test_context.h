/*
 * Damon V. Caskey
 * 2026-11-20
 * Run the same independent renderer fixtures against desktop GL and the
 * Android GLES profile. Context selection never substitutes a CPU renderer.
 */
#ifndef OPENBOR_GPU_TEST_CONTEXT_H
#define OPENBOR_GPU_TEST_CONTEXT_H

#include <SDL.h>
#include <stdio.h>
#include <string.h>
#include "gpu_gl_api.h"

/*
 * Damon V. Caskey
 * 2026-11-20
 * Select the production API before window/context creation. The GLES suite
 * requires the ES profile explicitly, so a desktop context cannot pass it.
 */
static inline void gpu_test_context_attributes(void)
{
#if defined(OPENBOR_GLES)
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
#else
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_COMPATIBILITY);
#endif
}

#if defined(OPENBOR_GLES)
/*
 * Damon V. Caskey
 * 2026-11-20
 * GLES entry points are linked through the same API as Android. The native
 * backend still validates its dynamically loaded functions during gpu_gl_init.
 * Verify the created context itself at every replacement boundary as well.
 */
static inline int gpu_test_load_functions(void)
{
    const char *version;
    int profile = 0, major = 0;
#if defined(LOADGL)
    /* Host fault fixtures intercept the existing core-API forwarding layer.
     * Legacy symbols may be resolved but GLES tests never execute them. */
    if(!LoadGLFunctions()) return 0;
#endif
    version = (const char *)glGetString(GL_VERSION);
    if(SDL_GL_GetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, &profile) < 0 ||
       SDL_GL_GetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, &major) < 0 ||
       profile != SDL_GL_CONTEXT_PROFILE_ES || major < 3 || !version ||
       !strstr(version, "OpenGL ES 3."))
    {
        fprintf(stderr, "Required OpenGL ES 3 context unavailable: %s\n",
                version ? version : SDL_GetError());
        return 0;
    }
    return 1;
}
#define LoadGLFunctions gpu_test_load_functions
#endif

#endif
