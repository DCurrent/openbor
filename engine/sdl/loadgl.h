/*
 * OpenBOR - http://www.LavaLit.com
 * -----------------------------------------------------------------------
 * All rights reserved, see LICENSE in OpenBOR root for details.
 *
 * Copyright (c) 2004 - 2011 OpenBOR Team
 */

#ifndef LOADGL_H
#define LOADGL_H

#ifdef LOADGL
#undef GL_GLEXT_PROTOTYPES
#else
#define GL_GLEXT_PROTOTYPES
#endif

#ifdef GLES
#include "SDL_opengles.h"
#elif WIN
// SDL_opengl.h in Windows includes <windows.h>, which clashes with several
// definitions in openbor.h :(
#define APIENTRY __stdcall
#include <GL/gl.h>
#include <GL/glext.h>
#else
#include "SDL_opengl.h"
#endif

#ifdef LOADGL
// call this immediately after setting an OpenGL video mode in Linux or Windows
int LoadGLFunctions();

extern PFNGLCREATESHADEROBJECTARBPROC glCreateShaderObjectARB;
extern PFNGLSHADERSOURCEARBPROC glShaderSourceARB;
extern PFNGLCOMPILESHADERARBPROC glCompileShaderARB;
extern PFNGLCREATEPROGRAMOBJECTARBPROC glCreateProgramObjectARB;
extern PFNGLATTACHOBJECTARBPROC glAttachObjectARB;
extern PFNGLLINKPROGRAMARBPROC glLinkProgramARB;
extern PFNGLUSEPROGRAMOBJECTARBPROC glUseProgramObjectARB;
extern PFNGLGETUNIFORMLOCATIONARBPROC glGetUniformLocationARB;
extern PFNGLUNIFORM1IARBPROC glUniform1iARB;
extern PFNGLUNIFORM1FARBPROC glUniform1fARB;
extern PFNGLUNIFORM4FARBPROC glUniform4fARB;

#if WIN
// <GL/gl.h> in Windows only defines prototypes available in OpenGL 1.1, which
// is from 1995!  We define the function prototypes here for functions that
// became core in later revisions of the OpenGL API.  Currently this only
// includes multitexturing, which became core in OpenGL 1.3 (2001).
void APIENTRY glActiveTexture(GLenum texture);
void APIENTRY glMultiTexCoord2f(GLenum texture, GLfloat s, GLfloat t);
#endif // WIN

#endif // LOADGL

#endif // !defined(LOADGL_H)

