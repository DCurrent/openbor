/*
 * OpenBOR - http://www.chronocrash.com
 * Copyright (c) OpenBOR Team. All rights reserved. See LICENSE.
 */
#ifndef OPENBOR_GPU_SDL_GLES2_H
#define OPENBOR_GPU_SDL_GLES2_H

/*
 * Damon V. Caskey
 * 2026-11-16
 * Share SDL/GLES context ownership between native output filters and launcher
 * composition. Restore both sampled units, upload packing and clear color as
 * well as SDL's cached program, raster, attributes and render-target state.
 */
#include <stddef.h>
#include "SDL.h"
#if defined(ANDROID)
#include "SDL_opengles2.h"
#else
#include "SDL_opengles2_khrplatform.h"
#include "SDL_opengles2_gl2platform.h"
#include "SDL_opengles2_gl2.h"
#endif
#include "gpu.h"

#define GPU_SDL_GLES2_FUNCTIONS(X) \
    X(void, ActiveTexture, (GLenum texture)) \
    X(void, AttachShader, (GLuint program, GLuint shader)) \
    X(void, BindAttribLocation, (GLuint program, GLuint index, const GLchar *name)) \
    X(void, BindBuffer, (GLenum target, GLuint buffer)) \
    X(void, BindTexture, (GLenum target, GLuint texture)) \
    X(void, BufferSubData, (GLenum target, GLintptr offset, GLsizeiptr size, const void *data)) \
    X(void, Clear, (GLbitfield mask)) \
    X(void, ClearColor, (GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha)) \
    X(void, BufferData, (GLenum target, GLsizeiptr size, const void *data, GLenum usage)) \
    X(void, ColorMask, (GLboolean red, GLboolean green, GLboolean blue, GLboolean alpha)) \
    X(void, CompileShader, (GLuint shader)) \
    X(GLuint, CreateProgram, (void)) \
    X(GLuint, CreateShader, (GLenum type)) \
    X(void, DeleteTextures, (GLsizei count, const GLuint *textures)) \
    X(void, DeleteBuffers, (GLsizei count, const GLuint *buffers)) \
    X(void, DeleteProgram, (GLuint program)) \
    X(void, DeleteShader, (GLuint shader)) \
    X(void, Disable, (GLenum capability)) \
    X(void, DisableVertexAttribArray, (GLuint index)) \
    X(void, DrawArrays, (GLenum mode, GLint first, GLsizei count)) \
    X(void, Enable, (GLenum capability)) \
    X(void, EnableVertexAttribArray, (GLuint index)) \
    X(void, GenTextures, (GLsizei count, GLuint *textures)) \
    X(void, GenBuffers, (GLsizei count, GLuint *buffers)) \
    X(void, GetBooleanv, (GLenum name, GLboolean *value)) \
    X(GLenum, GetError, (void)) \
    X(void, GetFloatv, (GLenum name, GLfloat *value)) \
    X(void, GetIntegerv, (GLenum name, GLint *value)) \
    X(void, GetProgramInfoLog, (GLuint program, GLsizei size, GLsizei *length, GLchar *log)) \
    X(void, GetProgramiv, (GLuint program, GLenum name, GLint *value)) \
    X(void, GetShaderInfoLog, (GLuint shader, GLsizei size, GLsizei *length, GLchar *log)) \
    X(void, GetShaderiv, (GLuint shader, GLenum name, GLint *value)) \
    X(const GLubyte *, GetString, (GLenum name)) \
    X(void, GetTexParameteriv, (GLenum target, GLenum name, GLint *value)) \
    X(GLint, GetUniformLocation, (GLuint program, const GLchar *name)) \
    X(void, GetVertexAttribiv, (GLuint index, GLenum name, GLint *value)) \
    X(void, GetVertexAttribPointerv, (GLuint index, GLenum name, void **value)) \
    X(GLboolean, IsEnabled, (GLenum capability)) \
    X(void, LinkProgram, (GLuint program)) \
    X(void, ShaderSource, (GLuint shader, GLsizei count, const GLchar *const *source, const GLint *length)) \
    X(void, PixelStorei, (GLenum name, GLint value)) \
    X(void, TexImage2D, (GLenum target, GLint level, GLint internal_format, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const void *pixels)) \
    X(void, TexSubImage2D, (GLenum target, GLint level, GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type, const void *pixels)) \
    X(void, TexParameteri, (GLenum target, GLenum name, GLint value)) \
    X(void, Uniform1i, (GLint location, GLint value)) \
    X(void, Uniform3f, (GLint location, GLfloat first, GLfloat second, GLfloat third)) \
    X(void, Uniform2f, (GLint location, GLfloat first, GLfloat second)) \
    X(void, UseProgram, (GLuint program)) \
    X(void, VertexAttribPointer, (GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const void *pointer)) \
    X(void, Viewport, (GLint x, GLint y, GLsizei width, GLsizei height))

typedef struct gpu_sdl_gles2_api
{
#define GPU_SDL_GLES2_DECLARE(result, name, arguments) result (GL_APIENTRY *name) arguments;
    GPU_SDL_GLES2_FUNCTIONS(GPU_SDL_GLES2_DECLARE)
#undef GPU_SDL_GLES2_DECLARE
} gpu_sdl_gles2_api;

typedef struct gpu_sdl_gles2_attribute_state
{
    GLint enabled, size, type, normalized, stride, buffer;
    void *pointer;
} gpu_sdl_gles2_attribute_state;

static const GLenum gpu_sdl_gles2_capabilities[] = {
    GL_BLEND, GL_SCISSOR_TEST, GL_DEPTH_TEST, GL_STENCIL_TEST,
    GL_CULL_FACE, GL_DITHER, GL_SAMPLE_ALPHA_TO_COVERAGE, GL_SAMPLE_COVERAGE
};

typedef struct gpu_sdl_gles2_gl_state
{
    GLint program, array_buffer, active_texture, texture[2], viewport[4], unpack_alignment;
    GLfloat clear_color[4];
    GLboolean color_mask[4];
    GLboolean enabled[sizeof(gpu_sdl_gles2_capabilities) / sizeof(gpu_sdl_gles2_capabilities[0])];
    gpu_sdl_gles2_attribute_state attributes[2];
} gpu_sdl_gles2_gl_state;

typedef struct gpu_sdl_gles2_sdl_state
{
    SDL_Texture *target;
    SDL_Rect viewport, clip;
    SDL_bool clipped;
    float scale_x, scale_y;
    int logical_width, logical_height;
} gpu_sdl_gles2_sdl_state;

/* Damon V. Caskey - 2026-11-16: resolve GLES2 before allocating filter storage. */
static int gpu_sdl_gles2_load(gpu_sdl_gles2_api *gl)
{
#define GPU_SDL_GLES2_LOAD(result, name, arguments) \
    gl->name = (result (GL_APIENTRY *) arguments)SDL_GL_GetProcAddress("gl" #name); \
    if(!gl->name) { gpu_log("ERROR: Android GPU presentation requires gl%s.", #name); return 0; }
    GPU_SDL_GLES2_FUNCTIONS(GPU_SDL_GLES2_LOAD)
#undef GPU_SDL_GLES2_LOAD
    return 1;
}

/* Damon V. Caskey - 2026-11-16: report complete driver diagnostics on failure. */
static GLuint gpu_sdl_gles2_compile(gpu_sdl_gles2_api *gl, GLenum kind, const char *source)
{
    GLuint shader = gl->CreateShader(kind);
    GLint okay = 0;
    char log[2048] = { 0 };
    if(shader)
    {
        gl->ShaderSource(shader, 1, &source, NULL);
        gl->CompileShader(shader);
        gl->GetShaderiv(shader, GL_COMPILE_STATUS, &okay);
        if(!okay) gl->GetShaderInfoLog(shader, sizeof(log), NULL, log);
    }
    if(!okay)
    {
        gpu_log("ERROR: Android GPU shader compilation failed: %s.",
            log[0] ? log : "driver returned no diagnostic");
        if(shader) gl->DeleteShader(shader);
        return 0;
    }
    return shader;
}

/*
 * Damon V. Caskey
 * 2026-11-16
 * SDL 2.0.10 retains a GL program/state cache after SDL_RenderFlush. Preserve
 * every raw state this pass changes so the subsequent SDL draw stays valid.
 * SDL_GL_UnbindTexture separately invalidates its cached source texture.
 */
static void gpu_sdl_gles2_save_gl(gpu_sdl_gles2_api *gl, gpu_sdl_gles2_gl_state *state)
{
    size_t index;
    gl->GetIntegerv(GL_CURRENT_PROGRAM, &state->program);
    gl->GetIntegerv(GL_ARRAY_BUFFER_BINDING, &state->array_buffer);
    gl->GetIntegerv(GL_ACTIVE_TEXTURE, &state->active_texture);
    for(index = 0; index < 2; ++index)
    {
        gl->ActiveTexture(GL_TEXTURE0 + index);
        gl->GetIntegerv(GL_TEXTURE_BINDING_2D, &state->texture[index]);
    }
    gl->ActiveTexture(GL_TEXTURE0);
    gl->GetIntegerv(GL_UNPACK_ALIGNMENT, &state->unpack_alignment);
    gl->GetFloatv(GL_COLOR_CLEAR_VALUE, state->clear_color);
    gl->GetIntegerv(GL_VIEWPORT, state->viewport);
    gl->GetBooleanv(GL_COLOR_WRITEMASK, state->color_mask);
    for(index = 0; index < sizeof(gpu_sdl_gles2_capabilities) / sizeof(gpu_sdl_gles2_capabilities[0]); ++index)
        state->enabled[index] = gl->IsEnabled(gpu_sdl_gles2_capabilities[index]);
    for(index = 0; index < 2; ++index)
    {
        gpu_sdl_gles2_attribute_state *attribute = &state->attributes[index];
        gl->GetVertexAttribiv(index, GL_VERTEX_ATTRIB_ARRAY_ENABLED, &attribute->enabled);
        gl->GetVertexAttribiv(index, GL_VERTEX_ATTRIB_ARRAY_SIZE, &attribute->size);
        gl->GetVertexAttribiv(index, GL_VERTEX_ATTRIB_ARRAY_TYPE, &attribute->type);
        gl->GetVertexAttribiv(index, GL_VERTEX_ATTRIB_ARRAY_NORMALIZED, &attribute->normalized);
        gl->GetVertexAttribiv(index, GL_VERTEX_ATTRIB_ARRAY_STRIDE, &attribute->stride);
        gl->GetVertexAttribiv(index, GL_VERTEX_ATTRIB_ARRAY_BUFFER_BINDING, &attribute->buffer);
        gl->GetVertexAttribPointerv(index, GL_VERTEX_ATTRIB_ARRAY_POINTER, &attribute->pointer);
    }
}

/* Damon V. Caskey - 2026-11-16: return the shared GLES2 context to SDL intact. */
static void gpu_sdl_gles2_restore_gl(gpu_sdl_gles2_api *gl, const gpu_sdl_gles2_gl_state *state)
{
    size_t index;
    gl->UseProgram(state->program);
    gl->Viewport(state->viewport[0], state->viewport[1], state->viewport[2], state->viewport[3]);
    gl->ColorMask(state->color_mask[0], state->color_mask[1], state->color_mask[2], state->color_mask[3]);
    for(index = 0; index < sizeof(gpu_sdl_gles2_capabilities) / sizeof(gpu_sdl_gles2_capabilities[0]); ++index)
    {
        if(state->enabled[index]) gl->Enable(gpu_sdl_gles2_capabilities[index]);
        else gl->Disable(gpu_sdl_gles2_capabilities[index]);
    }
    for(index = 0; index < 2; ++index)
    {
        const gpu_sdl_gles2_attribute_state *attribute = &state->attributes[index];
        gl->BindBuffer(GL_ARRAY_BUFFER, attribute->buffer);
        gl->VertexAttribPointer(index, attribute->size, attribute->type,
            attribute->normalized, attribute->stride, attribute->pointer);
        if(attribute->enabled) gl->EnableVertexAttribArray(index);
        else gl->DisableVertexAttribArray(index);
    }
    gl->BindBuffer(GL_ARRAY_BUFFER, state->array_buffer);
    for(index = 0; index < 2; ++index)
    {
        gl->ActiveTexture(GL_TEXTURE0 + index);
        gl->BindTexture(GL_TEXTURE_2D, state->texture[index]);
    }
    gl->ActiveTexture(state->active_texture);
    gl->PixelStorei(GL_UNPACK_ALIGNMENT, state->unpack_alignment);
    gl->ClearColor(state->clear_color[0], state->clear_color[1],
                    state->clear_color[2], state->clear_color[3]);
}

/* Damon V. Caskey - 2026-11-16: preserve public SDL viewport and target state. */
static void gpu_sdl_gles2_save_sdl(SDL_Renderer *renderer, gpu_sdl_gles2_sdl_state *state)
{
    state->target = SDL_GetRenderTarget(renderer);
    SDL_RenderGetViewport(renderer, &state->viewport);
    SDL_RenderGetClipRect(renderer, &state->clip);
    state->clipped = SDL_RenderIsClipEnabled(renderer);
    SDL_RenderGetScale(renderer, &state->scale_x, &state->scale_y);
    SDL_RenderGetLogicalSize(renderer, &state->logical_width, &state->logical_height);
}

/* Damon V. Caskey - 2026-11-16: restore target state on success and failure. */
static int gpu_sdl_gles2_restore_sdl(SDL_Renderer *renderer, const gpu_sdl_gles2_sdl_state *state)
{
    int okay = SDL_SetRenderTarget(renderer, state->target) == 0;
    if(SDL_RenderSetLogicalSize(renderer, state->logical_width, state->logical_height) < 0) okay = 0;
    if(SDL_RenderSetScale(renderer, state->scale_x, state->scale_y) < 0) okay = 0;
    if(SDL_RenderSetViewport(renderer, &state->viewport) < 0) okay = 0;
    if(SDL_RenderSetClipRect(renderer, state->clipped ? &state->clip : NULL) < 0) okay = 0;
    return okay;
}

#undef GPU_SDL_GLES2_FUNCTIONS

#endif
