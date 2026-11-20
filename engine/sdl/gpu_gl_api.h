/*
 * OpenBOR - http://www.chronocrash.com
 * Copyright (c) OpenBOR Team. All rights reserved. See LICENSE.
 */
#ifndef OPENBOR_GPU_GL_API_H
#define OPENBOR_GPU_GL_API_H

/*
 * Damon V. Caskey
 * 2026-11-20
 * Select the native scene API without changing its rendering contracts.
 * Android requires GLES 3.0; the host GLES fixtures select the same code with
 * OPENBOR_GLES. Instrumented host tests may retain the desktop loader's call
 * interception declarations while executing an actual GLES 3 context.
 */
#if defined(ANDROID) && !defined(OPENBOR_GLES)
#define OPENBOR_GLES 1
#endif

#if defined(OPENBOR_GLES) && (defined(ANDROID) || !defined(LOADGL))
#include <GLES3/gl3.h>
#ifndef APIENTRY
#define APIENTRY GL_APIENTRY
#endif
#ifndef APIENTRYP
#define APIENTRYP GL_APIENTRYP
#endif
#else
#include "loadgl.h"
#endif

/* Timestamp profiling is optional; GLES drawing does not depend on this
 * desktop timer-query enum or on any corresponding timer entry point. */
#ifndef GL_QUERY_COUNTER_BITS
#define GL_QUERY_COUNTER_BITS 0x8864
#endif

#endif
