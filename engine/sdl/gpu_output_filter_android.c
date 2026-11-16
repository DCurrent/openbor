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
#include "gpu.h"
#include "gpu_shader.h"
#include "gfx.h"
#include "gpu_output_filter_android.h"

#include "gpu_sdl_gles2.h"

struct gpu_output_filter_android
{
    SDL_Renderer *renderer;
    SDL_GLContext context;
    SDL_Texture *target;
    gpu_sdl_gles2_api gl;
    GLuint program, vertices;
    GLint sampler, source_size, mode;
    int width, height, filter, warned;
};

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
    if(!result->context || !gpu_sdl_gles2_load(&result->gl)) goto failed;
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
    vertex = gpu_sdl_gles2_compile(&result->gl, GL_VERTEX_SHADER, gpu_filter_vertex_source);
    fragment = gpu_sdl_gles2_compile(&result->gl, GL_FRAGMENT_SHADER, gpu_output_filter_fragment_shader_es_source);
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
    gpu_sdl_gles2_sdl_state sdl_state;
    gpu_sdl_gles2_gl_state gl_state;
    gpu_sdl_gles2_api *gl;
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
    gpu_sdl_gles2_save_sdl(filter->renderer, &sdl_state);
    failure = "SDL target selection";
    if(SDL_SetRenderTarget(filter->renderer, filter->target) < 0 ||
       SDL_RenderFlush(filter->renderer) < 0) goto restore_sdl;
    failure = "GLES context changed";
    if(SDL_GL_GetCurrentContext() != filter->context) goto restore_sdl;
    gl = &filter->gl;
    gpu_sdl_gles2_save_gl(gl, &gl_state);
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
    for(index = 0; index < sizeof(gpu_sdl_gles2_capabilities) / sizeof(gpu_sdl_gles2_capabilities[0]); ++index)
        gl->Disable(gpu_sdl_gles2_capabilities[index]);
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
    if(saved_gl) gpu_sdl_gles2_restore_gl(gl, &gl_state);
restore_sdl:
    if(!gpu_sdl_gles2_restore_sdl(filter->renderer, &sdl_state))
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

/*
 * Damon V. Caskey
 * 2026-11-16
 * Forget resources after a device/context reset without issuing commands
 * through obsolete private GL names. SDL remains responsible for its texture.
 */
void gpu_output_filter_android_abandon(gpu_output_filter_android *filter)
{
    if(!filter) return;
    if(filter->target) SDL_DestroyTexture(filter->target);
    free(filter);
}

#endif
