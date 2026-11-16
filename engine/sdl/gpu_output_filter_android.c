/*
 * OpenBOR - http://www.chronocrash.com
 * Copyright (c) OpenBOR Team. All rights reserved. See LICENSE.
 */
#if defined(ANDROID) || defined(OPENBOR_ANDROID_FILTER_TEST)

#include <limits.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include "SDL.h"
#if defined(ANDROID)
#include "SDL_opengles2.h"
#else
/* SDL's bundled portable GLES2 declarations also support the host fixture. */
#include "SDL_opengles2_khrplatform.h"
#include "SDL_opengles2_gl2platform.h"
#include "SDL_opengles2_gl2.h"
#endif
#include "gpu.h"
#include "gpu_shader.h"
#include "gfx.h"
#include "gpu_output_filter_android.h"

#define GPU_FILTER_ES_FUNCTIONS(X) \
    X(void, ActiveTexture, (GLenum texture)) \
    X(void, AttachShader, (GLuint program, GLuint shader)) \
    X(void, BindAttribLocation, (GLuint program, GLuint index, const GLchar *name)) \
    X(void, BindBuffer, (GLenum target, GLuint buffer)) \
    X(void, BindTexture, (GLenum target, GLuint texture)) \
    X(void, BufferData, (GLenum target, GLsizeiptr size, const void *data, GLenum usage)) \
    X(void, ColorMask, (GLboolean red, GLboolean green, GLboolean blue, GLboolean alpha)) \
    X(void, CompileShader, (GLuint shader)) \
    X(GLuint, CreateProgram, (void)) \
    X(GLuint, CreateShader, (GLenum type)) \
    X(void, DeleteBuffers, (GLsizei count, const GLuint *buffers)) \
    X(void, DeleteProgram, (GLuint program)) \
    X(void, DeleteShader, (GLuint shader)) \
    X(void, Disable, (GLenum capability)) \
    X(void, DisableVertexAttribArray, (GLuint index)) \
    X(void, DrawArrays, (GLenum mode, GLint first, GLsizei count)) \
    X(void, Enable, (GLenum capability)) \
    X(void, EnableVertexAttribArray, (GLuint index)) \
    X(void, GenBuffers, (GLsizei count, GLuint *buffers)) \
    X(void, GetBooleanv, (GLenum name, GLboolean *value)) \
    X(GLenum, GetError, (void)) \
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
    X(void, TexParameteri, (GLenum target, GLenum name, GLint value)) \
    X(void, Uniform1i, (GLint location, GLint value)) \
    X(void, Uniform2f, (GLint location, GLfloat first, GLfloat second)) \
    X(void, UseProgram, (GLuint program)) \
    X(void, VertexAttribPointer, (GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const void *pointer)) \
    X(void, Viewport, (GLint x, GLint y, GLsizei width, GLsizei height))

typedef struct gpu_filter_es_api
{
#define GPU_FILTER_ES_DECLARE(result, name, arguments) result (GL_APIENTRY *name) arguments;
    GPU_FILTER_ES_FUNCTIONS(GPU_FILTER_ES_DECLARE)
#undef GPU_FILTER_ES_DECLARE
} gpu_filter_es_api;

struct gpu_output_filter_android
{
    SDL_Renderer *renderer;
    SDL_GLContext context;
    SDL_Texture *target;
    gpu_filter_es_api gl;
    GLuint program, vertices;
    GLint sampler, source_size, mode;
    int width, height, filter, warned;
};

typedef struct gpu_filter_attribute_state
{
    GLint enabled, size, type, normalized, stride, buffer;
    void *pointer;
} gpu_filter_attribute_state;

static const GLenum gpu_filter_capabilities[] = {
    GL_BLEND, GL_SCISSOR_TEST, GL_DEPTH_TEST, GL_STENCIL_TEST,
    GL_CULL_FACE, GL_DITHER, GL_SAMPLE_ALPHA_TO_COVERAGE, GL_SAMPLE_COVERAGE
};

typedef struct gpu_filter_gl_state
{
    GLint program, array_buffer, active_texture, texture, viewport[4];
    GLboolean color_mask[4];
    GLboolean enabled[sizeof(gpu_filter_capabilities) / sizeof(gpu_filter_capabilities[0])];
    gpu_filter_attribute_state attributes[2];
} gpu_filter_gl_state;

typedef struct gpu_filter_sdl_state
{
    SDL_Texture *target;
    SDL_Rect viewport, clip;
    SDL_bool clipped;
    float scale_x, scale_y;
    int logical_width, logical_height;
} gpu_filter_sdl_state;

static const char *gpu_filter_vertex_source =
    "#version 100\n"
    "precision highp float;\n"
    "attribute vec2 vertexPosition;\n"
    "attribute vec2 vertexTexcoord;\n"
    "varying vec2 sourcePosition;\n"
    "void main() {\n"
    "    gl_Position = vec4(vertexPosition, 0.0, 1.0);\n"
    "    sourcePosition = vertexTexcoord;\n"
    "}\n";

/* Damon V. Caskey - 2026-11-16: resolve GLES2 before allocating filter storage. */
static int gpu_filter_load(gpu_filter_es_api *gl)
{
#define GPU_FILTER_ES_LOAD(result, name, arguments) \
    gl->name = (result (GL_APIENTRY *) arguments)SDL_GL_GetProcAddress("gl" #name); \
    if(!gl->name) { gpu_log("ERROR: Android GPU output filter requires gl%s.", #name); return 0; }
    GPU_FILTER_ES_FUNCTIONS(GPU_FILTER_ES_LOAD)
#undef GPU_FILTER_ES_LOAD
    return 1;
}

/* Damon V. Caskey - 2026-11-16: report complete driver diagnostics on failure. */
static GLuint gpu_filter_compile(gpu_filter_es_api *gl, GLenum kind, const char *source)
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
        gpu_log("ERROR: Android GPU output filter shader compilation failed: %s.",
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
static void gpu_filter_save_gl(gpu_filter_es_api *gl, gpu_filter_gl_state *state)
{
    size_t index;
    gl->GetIntegerv(GL_CURRENT_PROGRAM, &state->program);
    gl->GetIntegerv(GL_ARRAY_BUFFER_BINDING, &state->array_buffer);
    gl->GetIntegerv(GL_ACTIVE_TEXTURE, &state->active_texture);
    gl->ActiveTexture(GL_TEXTURE0);
    gl->GetIntegerv(GL_TEXTURE_BINDING_2D, &state->texture);
    gl->GetIntegerv(GL_VIEWPORT, state->viewport);
    gl->GetBooleanv(GL_COLOR_WRITEMASK, state->color_mask);
    for(index = 0; index < sizeof(gpu_filter_capabilities) / sizeof(gpu_filter_capabilities[0]); ++index)
        state->enabled[index] = gl->IsEnabled(gpu_filter_capabilities[index]);
    for(index = 0; index < 2; ++index)
    {
        gpu_filter_attribute_state *attribute = &state->attributes[index];
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
static void gpu_filter_restore_gl(gpu_filter_es_api *gl, const gpu_filter_gl_state *state)
{
    size_t index;
    gl->UseProgram(state->program);
    gl->Viewport(state->viewport[0], state->viewport[1], state->viewport[2], state->viewport[3]);
    gl->ColorMask(state->color_mask[0], state->color_mask[1], state->color_mask[2], state->color_mask[3]);
    for(index = 0; index < sizeof(gpu_filter_capabilities) / sizeof(gpu_filter_capabilities[0]); ++index)
    {
        if(state->enabled[index]) gl->Enable(gpu_filter_capabilities[index]);
        else gl->Disable(gpu_filter_capabilities[index]);
    }
    for(index = 0; index < 2; ++index)
    {
        const gpu_filter_attribute_state *attribute = &state->attributes[index];
        gl->BindBuffer(GL_ARRAY_BUFFER, attribute->buffer);
        gl->VertexAttribPointer(index, attribute->size, attribute->type,
            attribute->normalized, attribute->stride, attribute->pointer);
        if(attribute->enabled) gl->EnableVertexAttribArray(index);
        else gl->DisableVertexAttribArray(index);
    }
    gl->BindBuffer(GL_ARRAY_BUFFER, state->array_buffer);
    gl->BindTexture(GL_TEXTURE_2D, state->texture);
    gl->ActiveTexture(state->active_texture);
}

/* Damon V. Caskey - 2026-11-16: preserve public SDL viewport and target state. */
static void gpu_filter_save_sdl(SDL_Renderer *renderer, gpu_filter_sdl_state *state)
{
    state->target = SDL_GetRenderTarget(renderer);
    SDL_RenderGetViewport(renderer, &state->viewport);
    SDL_RenderGetClipRect(renderer, &state->clip);
    state->clipped = SDL_RenderIsClipEnabled(renderer);
    SDL_RenderGetScale(renderer, &state->scale_x, &state->scale_y);
    SDL_RenderGetLogicalSize(renderer, &state->logical_width, &state->logical_height);
}

/* Damon V. Caskey - 2026-11-16: restore target state on success and failure. */
static int gpu_filter_restore_sdl(SDL_Renderer *renderer, const gpu_filter_sdl_state *state)
{
    int okay = SDL_SetRenderTarget(renderer, state->target) == 0;
    if(SDL_RenderSetLogicalSize(renderer, state->logical_width, state->logical_height) < 0) okay = 0;
    if(SDL_RenderSetScale(renderer, state->scale_x, state->scale_y) < 0) okay = 0;
    if(SDL_RenderSetViewport(renderer, &state->viewport) < 0) okay = 0;
    if(SDL_RenderSetClipRect(renderer, state->clipped ? &state->clip : NULL) < 0) okay = 0;
    return okay;
}

/*
 * Damon V. Caskey
 * 2026-11-16
 * Allocate one full-color target and shared static filter shader. GLES2 target
 * row zero is texture v=0, matching SDL's uploaded texture convention; the
 * ordinary SDL window copy supplies the final display orientation.
 */
gpu_output_filter_android *gpu_output_filter_android_create(
    SDL_Renderer *renderer, int width, int height, int filter)
{
    gpu_output_filter_android *result;
    SDL_RendererInfo info;
    GLuint vertex = 0, fragment = 0;
    GLint linked = 0, limit = 0, previous_buffer = 0;
    unsigned index;
    int native_rgb32 = 0;
    GLfloat vertices[16];
    char log[2048] = { 0 };
    if(!renderer || width <= 0 || height <= 0 || width > INT_MAX / 2 ||
       height > INT_MAX / 2 || filter <= BLITTER_NONE || filter >= BLITTER_MAX ||
       SDL_GetRendererInfo(renderer, &info) < 0 || !info.name ||
       strcmp(info.name, "opengles2") || !(info.flags & SDL_RENDERER_TARGETTEXTURE))
    {
        gpu_log("ERROR: Android GPU output filter requires valid RGB32 dimensions and an opengles2 target renderer.");
        return NULL;
    }
    for(index = 0; index < info.num_texture_formats; ++index)
        if(info.texture_formats[index] == SDL_PIXELFORMAT_BGR888) native_rgb32 = 1;
    if(!native_rgb32)
    {
        gpu_log("ERROR: Android GPU output filter requires native SDL RGB32 texture storage.");
        return NULL;
    }
    if(SDL_RenderFlush(renderer) < 0)
    {
        gpu_log("ERROR: Android GPU output filter cannot flush SDL: %s.", SDL_GetError());
        return NULL;
    }
    result = calloc(1, sizeof(*result));
    if(!result)
    {
        gpu_log("ERROR: Android GPU output filter allocation failed.");
        return NULL;
    }
    result->renderer = renderer;
    result->width = width;
    result->height = height;
    result->filter = filter;
    /* The caller has just created/uploaded its SDL RGB32 source texture. */
    result->context = SDL_GL_GetCurrentContext();
    if(!result->context || !gpu_filter_load(&result->gl)) goto failed;
    if(!result->gl.GetString(GL_VERSION) ||
       !strstr((const char *)result->gl.GetString(GL_VERSION), "OpenGL ES"))
    {
        gpu_log("ERROR: Android GPU output filter requires an OpenGL ES context.");
        goto failed;
    }
    result->gl.GetIntegerv(GL_MAX_TEXTURE_SIZE, &limit);
    if(width > limit / 2 || height > limit / 2)
    {
        gpu_log("ERROR: Android GPU output filter target %dx%d exceeds texture limit %d.", width * 2, height * 2, limit);
        goto failed;
    }
    /* RGBX is native to both bundled and current SDL GLES2 renderers. This
     * prevents SDL from inserting a differently ordered native texture. SDL
     * owns its FBO, but dimension validation precedes the target allocation. */
    result->target = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_BGR888,
        SDL_TEXTUREACCESS_TARGET, width * 2, height * 2);
    if(!result->target || result->context != SDL_GL_GetCurrentContext()) goto failed;
    if(SDL_SetTextureBlendMode(result->target, SDL_BLENDMODE_NONE) < 0) goto failed;
    vertex = gpu_filter_compile(&result->gl, GL_VERTEX_SHADER, gpu_filter_vertex_source);
    fragment = gpu_filter_compile(&result->gl, GL_FRAGMENT_SHADER, gpu_output_filter_fragment_shader_es_source);
    if(!vertex || !fragment) goto failed;
    result->program = result->gl.CreateProgram();
    if(!result->program) goto failed;
    result->gl.AttachShader(result->program, vertex);
    result->gl.AttachShader(result->program, fragment);
    result->gl.BindAttribLocation(result->program, 0, "vertexPosition");
    result->gl.BindAttribLocation(result->program, 1, "vertexTexcoord");
    result->gl.LinkProgram(result->program);
    result->gl.GetProgramiv(result->program, GL_LINK_STATUS, &linked);
    if(!linked)
    {
        result->gl.GetProgramInfoLog(result->program, sizeof(log), NULL, log);
        gpu_log("ERROR: Android GPU output filter shader link failed: %s.", log);
        goto failed;
    }
    result->sampler = result->gl.GetUniformLocation(result->program, "sourceTexture");
    result->source_size = result->gl.GetUniformLocation(result->program, "sourceSize");
    result->mode = result->gl.GetUniformLocation(result->program, "filterMode");
    if(result->sampler < 0 || result->source_size < 0 || result->mode < 0) goto failed;
    vertices[0] = -1; vertices[1] = -1; vertices[2] = 0; vertices[3] = 0;
    vertices[4] = 1; vertices[5] = -1; vertices[6] = width; vertices[7] = 0;
    vertices[8] = -1; vertices[9] = 1; vertices[10] = 0; vertices[11] = height;
    vertices[12] = 1; vertices[13] = 1; vertices[14] = width; vertices[15] = height;
    result->gl.GetIntegerv(GL_ARRAY_BUFFER_BINDING, &previous_buffer);
    result->gl.GenBuffers(1, &result->vertices);
    result->gl.BindBuffer(GL_ARRAY_BUFFER, result->vertices);
    result->gl.BufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    result->gl.BindBuffer(GL_ARRAY_BUFFER, previous_buffer);
    if(!result->vertices || result->gl.GetError() != GL_NO_ERROR) goto failed;
    result->gl.DeleteShader(vertex);
    result->gl.DeleteShader(fragment);
    return result;

failed:
    if(vertex) result->gl.DeleteShader(vertex);
    if(fragment) result->gl.DeleteShader(fragment);
    gpu_log("ERROR: Android GPU output filter %d initialization failed: %s.", filter, SDL_GetError());
    gpu_output_filter_android_destroy(result);
    return NULL;
}

/*
 * Damon V. Caskey
 * 2026-11-16
 * Run the selected full-color filter once, then give SDL its state back before
 * composition of brightness and touch overlays. Never read pixels or retry
 * failed work through the CPU. Source sampling remains nearest for exact taps.
 */
SDL_Texture *gpu_output_filter_android_apply(gpu_output_filter_android *filter, SDL_Texture *source)
{
    gpu_filter_sdl_state sdl_state;
    gpu_filter_gl_state gl_state;
    gpu_filter_es_api *gl;
    Uint32 format;
    int width, height, success = 0, bound = 0, saved_gl = 0;
    GLint minimum = GL_NEAREST, maximum = GL_NEAREST;
    GLfloat texture_width = 0, texture_height = 0;
    GLenum error = GL_NO_ERROR;
    const char *failure = "invalid RGB32 source texture";
    size_t index;
    if(!filter) return NULL;
    if(!source || source == filter->target ||
       SDL_QueryTexture(source, &format, NULL, &width, &height) < 0 ||
       format != SDL_PIXELFORMAT_BGR888 || width != filter->width || height != filter->height)
        goto rejected;
    failure = "SDL render flush";
    if(SDL_RenderFlush(filter->renderer) < 0) goto rejected;
    gpu_filter_save_sdl(filter->renderer, &sdl_state);
    failure = "SDL target selection";
    if(SDL_SetRenderTarget(filter->renderer, filter->target) < 0 ||
       SDL_RenderFlush(filter->renderer) < 0) goto restore_sdl;
    failure = "GLES context changed";
    if(SDL_GL_GetCurrentContext() != filter->context) goto restore_sdl;
    gl = &filter->gl;
    gpu_filter_save_gl(gl, &gl_state);
    saved_gl = 1;
    failure = "SDL source texture binding";
    if(SDL_GL_BindTexture(source, &texture_width, &texture_height) < 0) goto restore_gl;
    bound = 1;
    gl->GetTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, &minimum);
    gl->GetTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, &maximum);
    /* The selected SDL GLES2 backend uses exact-sized, normalized textures. */
    failure = "unsupported source texture scale";
    if(texture_width != 1.0f || texture_height != 1.0f) goto restore_gl;
    gl->TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    gl->TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    for(index = 0; index < sizeof(gpu_filter_capabilities) / sizeof(gpu_filter_capabilities[0]); ++index)
        gl->Disable(gpu_filter_capabilities[index]);
    gl->ColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    gl->Viewport(0, 0, width * 2, height * 2);
    gl->UseProgram(filter->program);
    gl->Uniform1i(filter->sampler, 0);
    gl->Uniform2f(filter->source_size, width, height);
    gl->Uniform1i(filter->mode, filter->filter);
    gl->BindBuffer(GL_ARRAY_BUFFER, filter->vertices);
    gl->EnableVertexAttribArray(0);
    gl->EnableVertexAttribArray(1);
    gl->VertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(GLfloat) * 4, NULL);
    gl->VertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(GLfloat) * 4, (const void *)(sizeof(GLfloat) * 2));
    gl->DrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    failure = "GLES filter draw";
    error = gl->GetError();
    success = error == GL_NO_ERROR;

restore_gl:
    if(bound)
    {
        gl->TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, minimum);
        gl->TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, maximum);
        if(SDL_GL_UnbindTexture(source) < 0)
        {
            failure = "SDL source texture release";
            success = 0;
        }
    }
    if(saved_gl) gpu_filter_restore_gl(gl, &gl_state);
restore_sdl:
    if(!gpu_filter_restore_sdl(filter->renderer, &sdl_state))
    {
        failure = "SDL target state restoration";
        success = 0;
    }
    if(success) return filter->target;
rejected:
    if(!filter->warned)
    {
        gpu_log("ERROR: Android GPU output filter %d failed at %s (GL error 0x%x); frame skipped.",
            filter->filter, failure, error);
        filter->warned = 1;
    }
    return NULL;
}

/* Damon V. Caskey - 2026-11-16: release resources before destroying SDL renderer. */
void gpu_output_filter_android_destroy(gpu_output_filter_android *filter)
{
    if(!filter) return;
    if(filter->renderer) SDL_RenderFlush(filter->renderer);
    if(filter->context && SDL_GL_GetCurrentContext() == filter->context)
    {
        if(filter->vertices) filter->gl.DeleteBuffers(1, &filter->vertices);
        if(filter->program) filter->gl.DeleteProgram(filter->program);
    }
    if(filter->target) SDL_DestroyTexture(filter->target);
    free(filter);
}

#endif
