/*
 * OpenBOR - http://www.chronocrash.com
 * -----------------------------------------------------------------------
 * All rights reserved, see LICENSE in OpenBOR root for details.
 *
 * Copyright (c)  OpenBOR Team
 */

/*
 * Dynamic OpenGL function loader.  Allows the program to run using SDL software
 * blitting when there is no OpenGL implementation available.  This is the case
 * by default in some Linux distributions and on some very old Windows computers.
 *
 * Date: August 3, 2010
 * Update, September 17, 2010
 */

#ifdef OPENGL

#include "sdlport.h"
#include "loadgl.h"

#ifndef APIENTRY
#define APIENTRY
#endif

void (APIENTRY *ptr_glViewport)(GLint x, GLint y, GLsizei width, GLsizei height);
void (APIENTRY *ptr_glMatrixMode)(GLenum mode);
void (APIENTRY *ptr_glLoadIdentity)(void);

void (APIENTRY *ptr_glGenTextures)(GLsizei n, GLuint *textures);
void (APIENTRY *ptr_glDeleteTextures)(GLsizei n, const GLuint *textures);
void (APIENTRY *ptr_glBindTexture)(GLenum target, GLuint texture);
void (APIENTRY *ptr_glTexImage2D)(GLenum target, GLint level, GLint internalFormat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const GLvoid *pixels);
void (APIENTRY *ptr_glTexSubImage2D)(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid *pixels);

void (APIENTRY *ptr_glClear)(GLbitfield mask);
void (APIENTRY *ptr_glClearColor)(GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha);
void (APIENTRY *ptr_glColor4f)(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha);
void (APIENTRY *ptr_glBlendFunc)(GLenum sfactor, GLenum dfactor);
void (APIENTRY *ptr_glTexEnvi)(GLenum, GLenum, GLint);
void (APIENTRY *ptr_glTexEnvfv)(GLenum, GLenum, const GLfloat*);

void (APIENTRY *ptr_glEnable)(GLenum cap);
void (APIENTRY *ptr_glDisable)(GLenum cap);

void (APIENTRY *ptr_glGetIntegerv)(GLenum pname, GLint* params);
const GLubyte* (APIENTRY *ptr_glGetString)(GLenum name);
GLenum (APIENTRY *ptr_glGetError)(void);

void (APIENTRY *ptr_glOrtho)(GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble near_val, GLdouble far_val);
void (APIENTRY *ptr_glTexParameteri)(GLenum target, GLenum pname, GLint param);
void (APIENTRY *ptr_glBegin)(GLenum mode);
void (APIENTRY *ptr_glEnd)(void);
void (APIENTRY *ptr_glVertex2i)(GLint x, GLint y);
void (APIENTRY *ptr_glTexCoord2f)(GLfloat s, GLfloat t);

// optional multitexturing extension (core in OpenGL 1.3 and OpenGL ES 1.0)
void (APIENTRY *ptr_glActiveTexture)(GLenum texture);
void (APIENTRY *ptr_glMultiTexCoord2f)(GLenum texture, GLfloat s, GLfloat t);

PFNGLCREATESHADEROBJECTARBPROC glCreateShaderObjectARB;
PFNGLSHADERSOURCEARBPROC glShaderSourceARB;
PFNGLCOMPILESHADERARBPROC glCompileShaderARB;
PFNGLCREATEPROGRAMOBJECTARBPROC glCreateProgramObjectARB;
PFNGLATTACHOBJECTARBPROC glAttachObjectARB;
PFNGLLINKPROGRAMARBPROC glLinkProgramARB;
PFNGLUSEPROGRAMOBJECTARBPROC glUseProgramObjectARB;
PFNGLGETUNIFORMLOCATIONARBPROC glGetUniformLocationARB;
PFNGLUNIFORM1IARBPROC glUniform1iARB;
PFNGLUNIFORM2IARBPROC glUniform2iARB;
PFNGLUNIFORM1FARBPROC glUniform1fARB;
PFNGLUNIFORM2FARBPROC glUniform2fARB;
PFNGLUNIFORM4FARBPROC glUniform4fARB;

#define LOADFUNC(X,Y) { Y = SDL_GL_GetProcAddress(X); if(!Y) { printf("Failed to load OpenGL function " X "..."); return 0; } }

int LoadGLFunctions()
{
	// Load every OpenGL function we use in video.c
	LOADFUNC("glViewport", ptr_glViewport);
	LOADFUNC("glMatrixMode", ptr_glMatrixMode);
	LOADFUNC("glLoadIdentity", ptr_glLoadIdentity);

	LOADFUNC("glGenTextures", ptr_glGenTextures);
	LOADFUNC("glDeleteTextures", ptr_glDeleteTextures);
	LOADFUNC("glBindTexture", ptr_glBindTexture);
	LOADFUNC("glTexImage2D", ptr_glTexImage2D);
	LOADFUNC("glTexSubImage2D", ptr_glTexSubImage2D);

	LOADFUNC("glClear", ptr_glClear);
	LOADFUNC("glClearColor", ptr_glClearColor);
	LOADFUNC("glColor4f", ptr_glColor4f);
	LOADFUNC("glBlendFunc", ptr_glBlendFunc);
	LOADFUNC("glTexEnvi", ptr_glTexEnvi);
	LOADFUNC("glTexEnvfv", ptr_glTexEnvfv);

	LOADFUNC("glEnable", ptr_glEnable);
	LOADFUNC("glDisable", ptr_glDisable);

	LOADFUNC("glGetIntegerv", ptr_glGetIntegerv);
	LOADFUNC("glGetString", ptr_glGetString);
	LOADFUNC("glGetError", ptr_glGetError);

	LOADFUNC("glOrtho", ptr_glOrtho);
	LOADFUNC("glTexParameteri", ptr_glTexParameteri);
	LOADFUNC("glBegin", ptr_glBegin);
	LOADFUNC("glEnd", ptr_glEnd);
	LOADFUNC("glVertex2i", ptr_glVertex2i);
	LOADFUNC("glTexCoord2f", ptr_glTexCoord2f);

	// load multisampling functions; try the ARB versions if the core versions are not available
	ptr_glActiveTexture = SDL_GL_GetProcAddress("glActiveTexture");
	if(!ptr_glActiveTexture)
		LOADFUNC("glActiveTextureARB", ptr_glActiveTexture);
	ptr_glMultiTexCoord2f = SDL_GL_GetProcAddress("glMultiTexCoord2f");
	if(!ptr_glMultiTexCoord2f)
		LOADFUNC("glMultiTexCoord2fARB", ptr_glMultiTexCoord2f);

	// load optional GLSL extensions
	glCreateShaderObjectARB = SDL_GL_GetProcAddress("glCreateShader");
	glShaderSourceARB = SDL_GL_GetProcAddress("glShaderSource");
	glCompileShaderARB = SDL_GL_GetProcAddress("glCompileShader");
	glCreateProgramObjectARB = SDL_GL_GetProcAddress("glCreateProgram");
	glAttachObjectARB = SDL_GL_GetProcAddress("glAttachShader");
	glLinkProgramARB = SDL_GL_GetProcAddress("glLinkProgram");
	glUseProgramObjectARB = SDL_GL_GetProcAddress("glUseProgram");
	glGetUniformLocationARB = SDL_GL_GetProcAddress("glGetUniformLocation");
	glUniform1iARB = SDL_GL_GetProcAddress("glUniform1iARB");
	glUniform2iARB = SDL_GL_GetProcAddress("glUniform2iARB");
	glUniform1fARB = SDL_GL_GetProcAddress("glUniform1fARB");
	glUniform2fARB = SDL_GL_GetProcAddress("glUniform2fARB");
	glUniform4fARB = SDL_GL_GetProcAddress("glUniform4fARB");

	return 1;
}

#ifndef DARWIN
void APIENTRY glViewport(GLint x, GLint y, GLsizei width, GLsizei height)
{
	ptr_glViewport(x, y, width, height);
}

void APIENTRY glMatrixMode(GLenum mode)
{
	ptr_glMatrixMode(mode);
}

void APIENTRY glLoadIdentity(void)
{
	ptr_glLoadIdentity();
}

void APIENTRY glGenTextures(GLsizei n, GLuint *textures)
{
	ptr_glGenTextures(n, textures);
}

void APIENTRY glDeleteTextures(GLsizei n, const GLuint *textures)
{
	ptr_glDeleteTextures(n, textures);
}

void APIENTRY glActiveTexture(GLenum texture)
{
	ptr_glActiveTexture(texture);
}

void APIENTRY glBindTexture(GLenum target, GLuint texture)
{
	ptr_glBindTexture(target, texture);
}

void APIENTRY glTexImage2D(GLenum target, GLint level, GLint internalFormat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const GLvoid *pixels)
{
	ptr_glTexImage2D(target, level, internalFormat, width, height, border, format, type, pixels);
}

void APIENTRY glTexSubImage2D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid *pixels)
{
	ptr_glTexSubImage2D(target, level, xoffset, yoffset, width, height, format, type, pixels);
}

void APIENTRY glClear(GLbitfield mask)
{
	ptr_glClear(mask);
}

void APIENTRY glClearColor(GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha)
{
	ptr_glClearColor(red, green, blue, alpha);
}

void APIENTRY glColor4f(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha)
{
	ptr_glColor4f(red, green, blue, alpha);
}

void APIENTRY glBlendFunc(GLenum sfactor, GLenum dfactor)
{
	ptr_glBlendFunc(sfactor, dfactor);
}

void APIENTRY glTexEnvi(GLenum target, GLenum pname, GLint param)
{
	ptr_glTexEnvi(target, pname, param);
}

void APIENTRY glTexEnvfv(GLenum target, GLenum pname, const GLfloat* params)
{
	ptr_glTexEnvfv(target, pname, params);
}

void APIENTRY glEnable(GLenum cap)
{
	ptr_glEnable(cap);
}

void APIENTRY glDisable(GLenum cap)
{
	ptr_glDisable(cap);
}

void APIENTRY glGetIntegerv(GLenum pname, GLint* params)
{
	ptr_glGetIntegerv(pname, params);
}

const GLubyte* APIENTRY glGetString(GLenum name)
{
	return ptr_glGetString(name);
}

GLenum APIENTRY glGetError(void)
{
	return ptr_glGetError();
}

void APIENTRY glOrtho(GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble near_val, GLdouble far_val)
{
	ptr_glOrtho(left, right, bottom, top, near_val, far_val);
}

void APIENTRY glTexParameteri(GLenum target, GLenum pname, GLint param)
{
	ptr_glTexParameteri(target, pname, param);
}

void APIENTRY glBegin(GLenum mode)
{
	ptr_glBegin(mode);
}

void APIENTRY glEnd(void)
{
	ptr_glEnd();
}

void APIENTRY glVertex2i(GLint x, GLint y)
{
	ptr_glVertex2i(x, y);
}

void APIENTRY glTexCoord2f(GLfloat s, GLfloat t)
{
	ptr_glTexCoord2f(s, t);
}

void APIENTRY glMultiTexCoord2f(GLenum texture, GLfloat s, GLfloat t)
{
	ptr_glMultiTexCoord2f(texture, s, t);
}
#endif

#endif
