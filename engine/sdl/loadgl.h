/*
 * OpenBOR - http://www.chronocrash.com
 * -----------------------------------------------------------------------
 * All rights reserved, see LICENSE in OpenBOR root for details.
 *
 * Copyright (c)  OpenBOR Team
 */

#ifndef LOADGL_H
#define LOADGL_H

#ifdef LOADGL
#undef GL_GLEXT_PROTOTYPES
#else
#define GL_GLEXT_PROTOTYPES
#endif

#if WIN
// SDL_opengl.h in Windows includes <windows.h>, which clashes with several
// definitions in openbor.h :(
#define APIENTRY __stdcall
#define WINGDIAPI
#include <GL/gl.h>
#include <GL/glext.h>
#else
#include "SDL_opengl.h"
#endif

#ifdef LOADGL
// Resolve required entry points immediately after creating an OpenGL context.
int LoadGLFunctions();

/* Core GL shader handles are GLuint on every supported platform. Keeping
 * their function signatures separate from ARB handles is required on macOS. */
extern PFNGLCREATESHADERPROC ptr_glCreateShader;
extern PFNGLSHADERSOURCEPROC ptr_glShaderSource;
extern PFNGLCOMPILESHADERPROC ptr_glCompileShader;
extern PFNGLGETSHADERIVPROC ptr_glGetShaderiv;
extern PFNGLGETSHADERINFOLOGPROC ptr_glGetShaderInfoLog;
extern PFNGLDELETESHADERPROC ptr_glDeleteShader;
extern PFNGLCREATEPROGRAMPROC ptr_glCreateProgram;
extern PFNGLATTACHSHADERPROC ptr_glAttachShader;
extern PFNGLLINKPROGRAMPROC ptr_glLinkProgram;
extern PFNGLGETPROGRAMIVPROC ptr_glGetProgramiv;
extern PFNGLGETPROGRAMINFOLOGPROC ptr_glGetProgramInfoLog;
extern PFNGLUSEPROGRAMPROC ptr_glUseProgram;
extern PFNGLGETUNIFORMLOCATIONPROC ptr_glGetUniformLocation;
extern PFNGLUNIFORM1IPROC ptr_glUniform1i;
extern PFNGLUNIFORM2FPROC ptr_glUniform2f;
extern PFNGLUNIFORM1FPROC ptr_glUniform1f;

#if WIN
// <GL/gl.h> in Windows only defines prototypes available in OpenGL 1.1, which
// is from 1995!  We define the function prototypes here for functions that
// became core in later revisions of the OpenGL API.  Currently this only
// includes multitexturing, which became core in OpenGL 1.3 (2001).
void APIENTRY glActiveTexture(GLenum texture);
void APIENTRY glMultiTexCoord2f(GLenum texture, GLfloat s, GLfloat t);
#endif // WIN

#else

#define ptr_glCreateShader glCreateShader
#define ptr_glShaderSource glShaderSource
#define ptr_glCompileShader glCompileShader
#define ptr_glGetShaderiv glGetShaderiv
#define ptr_glGetShaderInfoLog glGetShaderInfoLog
#define ptr_glDeleteShader glDeleteShader
#define ptr_glCreateProgram glCreateProgram
#define ptr_glAttachShader glAttachShader
#define ptr_glLinkProgram glLinkProgram
#define ptr_glGetProgramiv glGetProgramiv
#define ptr_glGetProgramInfoLog glGetProgramInfoLog
#define ptr_glUseProgram glUseProgram
#define ptr_glGetUniformLocation glGetUniformLocation
#define ptr_glUniform1i glUniform1i
#define ptr_glUniform2f glUniform2f
#define ptr_glUniform1f glUniform1f

#endif // LOADGL

#endif // !defined(LOADGL_H)

