/*
 * OpenBOR - http://www.chronocrash.com
 * Copyright (c) OpenBOR Team. All rights reserved. See LICENSE.
 */
#if defined(ANDROID) || defined(OPENBOR_ANDROID_LAUNCHER_TEST)

#include <limits.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "gpu_launcher_android.h"
#include "gpu_sdl_gles2.h"

#define GPU_LAUNCHER_FONT_WIDTH 5
#define GPU_LAUNCHER_FONT_HEIGHT 10
#define GPU_LAUNCHER_FONT_GLYPHS 160
#define GPU_LAUNCHER_ATLAS_COLUMNS 16
#define GPU_LAUNCHER_ATLAS_WIDTH 80
#define GPU_LAUNCHER_ATLAS_HEIGHT 100
#define GPU_LAUNCHER_STREAM_VERTICES (6 * 1024)

typedef struct gpu_launcher_vertex
{
    GLfloat x, y, u, v;
} gpu_launcher_vertex;

typedef struct gpu_launcher_scope
{
    gpu_sdl_gles2_sdl_state sdl;
    gpu_sdl_gles2_gl_state gl;
    int saved_sdl, saved_gl;
} gpu_launcher_scope;

struct gpu_launcher_android
{
    SDL_Renderer *renderer;
    SDL_GLContext context;
    SDL_Texture *target;
    gpu_sdl_gles2_api gl;
    GLuint program, vertices, white, palette, source, font;
    GLint target_size, source_size, source_sampler, palette_sampler;
    GLint source_mode, foreground, background, fill;
    GLint maximum_texture_size;
    int width, height, begun, valid, warned;
    int source_width, source_height, source_format;
    const unsigned char *font_rows;
    gpu_launcher_vertex *geometry;
    size_t geometry_capacity, stream_capacity, stream_cursor;
    gpu_launcher_android_stats stats;
};

static const char *gpu_launcher_vertex_source =
    "#version 100\n"
    "precision highp float;\n"
    "uniform vec2 targetSize;\n"
    "attribute vec2 vertexPosition;\n"
    "attribute vec2 vertexTexcoord;\n"
    "varying vec2 sourcePosition;\n"
    "/* Damon V. Caskey - 2026-11-16: retain logical row zero at texture v=0. */\n"
    "void main()\n"
    "{\n"
    "    gl_Position = vec4(vertexPosition * 2.0 / targetSize - 1.0, 0.0, 1.0);\n"
    "    sourcePosition = vertexTexcoord;\n"
    "}\n";

static const char *gpu_launcher_fragment_source =
    "#version 100\n"
    "precision highp float;\n"
    "precision highp int;\n"
    "precision highp sampler2D;\n"
    "uniform sampler2D sourceTexture;\n"
    "uniform sampler2D paletteTexture;\n"
    "uniform vec2 sourceSize;\n"
    "uniform int sourceMode;\n"
    "uniform vec3 foreground;\n"
    "uniform vec3 background;\n"
    "uniform int fillBackground;\n"
    "varying vec2 sourcePosition;\n"
    "/* Damon V. Caskey - 2026-11-16: exact source texels and opaque RGB. */\n"
    "void main()\n"
    "{\n"
    "    vec3 color = foreground;\n"
    "    if(sourceMode != 0)\n"
    "    {\n"
    "        vec2 uv = (floor(sourcePosition) + 0.5) / sourceSize;\n"
    "        vec3 source = texture2D(sourceTexture, uv).rgb;\n"
    "        if(sourceMode == 1) color = source;\n"
    "        else if(sourceMode == 2)\n"
    "        {\n"
    "            float index = floor(source.r * 255.0 + 0.5);\n"
    "            color = texture2D(paletteTexture, vec2((index + 0.5) / 256.0, 0.5)).rgb;\n"
    "        }\n"
    "        else if(source.r < 0.5 / 255.0)\n"
    "        {\n"
    "            if(fillBackground == 0) discard;\n"
    "            color = background;\n"
    "        }\n"
    "    }\n"
    "    gl_FragColor = vec4(color, 1.0);\n"
    "}\n";

/*
 * Damon V. Caskey
 * 2026-11-16
 * Keep rejected command streams unavailable to presentation until begin starts
 * a new frame. One ordinary diagnostic identifies the first context-local cause.
 */
static int gpu_launcher_reject(gpu_launcher_android *launcher, const char *stage, GLenum error)
{
    if(!launcher) return 0;
    launcher->valid = 0;
    ++launcher->stats.rejections;
    if(!launcher->warned)
    {
        gpu_log("ERROR: Android GPU launcher failed at %s (GL error 0x%x); frame skipped without CPU retry.",
                stage, (unsigned)error);
        launcher->warned = 1;
    }
    return 0;
}

/*
 * Damon V. Caskey
 * 2026-11-16
 * Require a live successful begin before accepting more commands. Invalidation
 * is sticky, so later labels cannot accidentally publish a partly drawn frame.
 */
static int gpu_launcher_ready(gpu_launcher_android *launcher)
{
    if(!launcher || !launcher->begun || !launcher->valid) return 0;
    if(SDL_GL_GetCurrentContext() != launcher->context)
        return gpu_launcher_reject(launcher, "GLES context changed", GL_NO_ERROR);
    return 1;
}

/*
 * Damon V. Caskey
 * 2026-11-16
 * Enter the owned SDL target after flushing queued commands. Raw GLES drawing
 * changes only state captured by the same helper used by output filtering.
 */
static int gpu_launcher_scope_begin(gpu_launcher_android *launcher, gpu_launcher_scope *scope)
{
    size_t index;
    memset(scope, 0, sizeof(*scope));
    if(SDL_RenderFlush(launcher->renderer) < 0) return 0;
    gpu_sdl_gles2_save_sdl(launcher->renderer, &scope->sdl);
    scope->saved_sdl = 1;
    if(SDL_SetRenderTarget(launcher->renderer, launcher->target) < 0 ||
       SDL_RenderFlush(launcher->renderer) < 0 ||
       SDL_GL_GetCurrentContext() != launcher->context) return 0;
    gpu_sdl_gles2_save_gl(&launcher->gl, &scope->gl);
    scope->saved_gl = 1;
    for(index = 0; index < sizeof(gpu_sdl_gles2_capabilities) / sizeof(gpu_sdl_gles2_capabilities[0]); ++index)
        launcher->gl.Disable(gpu_sdl_gles2_capabilities[index]);
    launcher->gl.ColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    launcher->gl.Viewport(0, 0, launcher->width, launcher->height);
    launcher->gl.PixelStorei(GL_UNPACK_ALIGNMENT, 1);
    return launcher->gl.GetError() == GL_NO_ERROR;
}

/*
 * Damon V. Caskey
 * 2026-11-16
 * Restore SDL and raw GL ownership on every successful or rejected command.
 * Restoration failure also invalidates the frame before callers can present it.
 */
static int gpu_launcher_scope_end(gpu_launcher_android *launcher, gpu_launcher_scope *scope)
{
    int okay = 1;
    if(scope->saved_gl)
    {
        gpu_sdl_gles2_restore_gl(&launcher->gl, &scope->gl);
        if(launcher->gl.GetError() != GL_NO_ERROR) okay = 0;
    }
    if(scope->saved_sdl && !gpu_sdl_gles2_restore_sdl(launcher->renderer, &scope->sdl)) okay = 0;
    return okay;
}

/*
 * Damon V. Caskey
 * 2026-11-16
 * Allocate exact nearest-sampled source storage. Source bytes are uploaded
 * directly; indexed palettes and font masks remain separate GPU resources.
 */
static GLuint gpu_launcher_new_texture(gpu_launcher_android *launcher, int width, int height,
                                       GLenum format, const void *pixels)
{
    GLuint texture = 0;
    gpu_sdl_gles2_api *gl = &launcher->gl;
    gl->GenTextures(1, &texture);
    if(!texture || gl->GetError() != GL_NO_ERROR) goto failed;
    gl->BindTexture(GL_TEXTURE_2D, texture);
    gl->TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    gl->TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    gl->TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    gl->TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    gl->PixelStorei(GL_UNPACK_ALIGNMENT, 1);
    gl->TexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, pixels);
    if(gl->GetError() != GL_NO_ERROR) goto failed;
    return texture;
failed:
    if(texture) gl->DeleteTextures(1, &texture);
    return 0;
}

/*
 * Damon V. Caskey
 * 2026-11-16
 * Reserve CPU geometry before any command writes its destination. Only clipped
 * visible cells are retained, so very long offscreen labels need no large VBO.
 */
static int gpu_launcher_geometry(gpu_launcher_android *launcher, size_t vertices)
{
    size_t capacity = launcher->geometry_capacity ? launcher->geometry_capacity : 64 * 6;
    gpu_launcher_vertex *replacement;
    if(vertices <= launcher->geometry_capacity) return 1;
    if(vertices > (size_t)INT_MAX || vertices > SIZE_MAX / sizeof(*replacement)) return 0;
    while(capacity < vertices)
        capacity = capacity > (size_t)INT_MAX / 2 ? vertices : capacity * 2;
    if(capacity > SIZE_MAX / sizeof(*replacement)) return 0;
    replacement = realloc(launcher->geometry, capacity * sizeof(*replacement));
    if(!replacement) return 0;
    launcher->geometry = replacement;
    launcher->geometry_capacity = capacity;
    return 1;
}

/*
 * Damon V. Caskey
 * 2026-11-16
 * Emit two triangles in original painter order, with clipped source texel
 * coordinates retaining their exact five-pixel font or image sampling phase.
 */
static void gpu_launcher_quad(gpu_launcher_vertex *vertices, int64_t left, int64_t top,
                              int64_t right, int64_t bottom, float source_x, float source_y)
{
    float source_right = source_x + (float)(right - left);
    float source_bottom = source_y + (float)(bottom - top);
    vertices[0] = (gpu_launcher_vertex){(float)left, (float)top, source_x, source_y};
    vertices[1] = (gpu_launcher_vertex){(float)right, (float)top, source_right, source_y};
    vertices[2] = (gpu_launcher_vertex){(float)right, (float)bottom, source_right, source_bottom};
    vertices[3] = vertices[0];
    vertices[4] = vertices[2];
    vertices[5] = (gpu_launcher_vertex){(float)left, (float)bottom, source_x, source_bottom};
}

/*
 * Damon V. Caskey
 * 2026-11-16
 * Append each ordered command into reusable vertex storage. Wrapping or growing
 * orphans the buffer store so queued draws retain their previous vertices.
 */
static int gpu_launcher_upload_geometry(gpu_launcher_android *launcher, size_t count, GLint *first)
{
    size_t capacity = launcher->stream_capacity ? launcher->stream_capacity : GPU_LAUNCHER_STREAM_VERTICES;
    gpu_sdl_gles2_api *gl = &launcher->gl;
    if(count > (size_t)INT_MAX) return 0;
    while(capacity < count)
        capacity = capacity > (size_t)INT_MAX / 2 ? count : capacity * 2;
    if(capacity > (size_t)PTRDIFF_MAX / sizeof(gpu_launcher_vertex)) return 0;
    if(!launcher->vertices) gl->GenBuffers(1, &launcher->vertices);
    if(!launcher->vertices) return 0;
    gl->BindBuffer(GL_ARRAY_BUFFER, launcher->vertices);
    if(capacity > launcher->stream_capacity || count > launcher->stream_capacity - launcher->stream_cursor)
    {
        gl->BufferData(GL_ARRAY_BUFFER, (GLsizeiptr)(capacity * sizeof(gpu_launcher_vertex)), NULL, GL_STREAM_DRAW);
        if(gl->GetError() != GL_NO_ERROR) return 0;
        launcher->stream_capacity = capacity;
        launcher->stream_cursor = 0;
    }
    gl->BufferSubData(GL_ARRAY_BUFFER, (GLintptr)(launcher->stream_cursor * sizeof(gpu_launcher_vertex)),
                     (GLsizeiptr)(count * sizeof(gpu_launcher_vertex)), launcher->geometry);
    if(gl->GetError() != GL_NO_ERROR) return 0;
    *first = (GLint)launcher->stream_cursor;
    launcher->stream_cursor += count;
    return 1;
}

/*
 * Damon V. Caskey
 * 2026-11-16
 * Submit one opaque/source-keyed command with explicit neutral raster state.
 * Color correction, window scaling and touch controls remain later SDL passes.
 */
static int gpu_launcher_draw(gpu_launcher_android *launcher, size_t count, int mode,
                              GLuint source, int width, int height,
                              unsigned foreground, unsigned background, int fill)
{
    GLint first;
    gpu_sdl_gles2_api *gl = &launcher->gl;
    if(!gpu_launcher_upload_geometry(launcher, count, &first)) return 0;
    gl->UseProgram(launcher->program);
    gl->ActiveTexture(GL_TEXTURE0);
    gl->BindTexture(GL_TEXTURE_2D, source ? source : launcher->white);
    gl->ActiveTexture(GL_TEXTURE1);
    gl->BindTexture(GL_TEXTURE_2D, launcher->palette);
    gl->ActiveTexture(GL_TEXTURE0);
    gl->Uniform2f(launcher->target_size, (float)launcher->width, (float)launcher->height);
    gl->Uniform2f(launcher->source_size, (float)width, (float)height);
    gl->Uniform1i(launcher->source_sampler, 0);
    gl->Uniform1i(launcher->palette_sampler, 1);
    gl->Uniform1i(launcher->source_mode, mode);
    gl->Uniform1i(launcher->fill, fill != 0);
    gl->Uniform3f(launcher->foreground, (foreground & 255u) / 255.0f,
                  ((foreground >> 8) & 255u) / 255.0f, ((foreground >> 16) & 255u) / 255.0f);
    gl->Uniform3f(launcher->background, (background & 255u) / 255.0f,
                  ((background >> 8) & 255u) / 255.0f, ((background >> 16) & 255u) / 255.0f);
    gl->EnableVertexAttribArray(0);
    gl->EnableVertexAttribArray(1);
    gl->VertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(gpu_launcher_vertex), NULL);
    gl->VertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(gpu_launcher_vertex),
                            (const void *)offsetof(gpu_launcher_vertex, u));
    if(gl->GetError() != GL_NO_ERROR) return 0;
    gl->DrawArrays(GL_TRIANGLES, first, (GLsizei)count);
    return gl->GetError() == GL_NO_ERROR;
}

/*
 * Damon V. Caskey
 * 2026-11-16
 * Create the private RGB32 target and tiny material program independently of
 * the game's unmigrated Android scene renderer. SDL retains final presentation.
 */
gpu_launcher_android *gpu_launcher_android_create(SDL_Renderer *renderer, int width, int height)
{
    gpu_launcher_android *launcher;
    SDL_RendererInfo info;
    gpu_sdl_gles2_gl_state state;
    GLuint vertex = 0, fragment = 0;
    GLint linked = 0;
    unsigned index;
    int native_rgb32 = 0, saved = 0;
    const unsigned char white[4] = {255, 255, 255, 255};
    char message[2048] = {0};
    if(!renderer || width <= 0 || height <= 0 || SDL_GetRendererInfo(renderer, &info) < 0 ||
       !info.name || strcmp(info.name, "opengles2") || !(info.flags & SDL_RENDERER_TARGETTEXTURE))
    {
        gpu_log("ERROR: Android GPU launcher requires valid dimensions and an opengles2 target renderer.");
        return NULL;
    }
    for(index = 0; index < info.num_texture_formats; ++index)
        if(info.texture_formats[index] == SDL_PIXELFORMAT_BGR888) native_rgb32 = 1;
    if(!native_rgb32 || SDL_RenderFlush(renderer) < 0)
    {
        gpu_log("ERROR: Android GPU launcher requires native RGB32 SDL texture storage and a usable renderer.");
        return NULL;
    }
    launcher = calloc(1, sizeof(*launcher));
    if(!launcher)
    {
        gpu_log("ERROR: Android GPU launcher metadata allocation failed; rendering has no CPU retry.");
        return NULL;
    }
    launcher->renderer = renderer;
    launcher->context = SDL_GL_GetCurrentContext();
    launcher->width = width;
    launcher->height = height;
    if(!launcher->context || !gpu_sdl_gles2_load(&launcher->gl)) goto failed;
    if(!launcher->gl.GetString(GL_VERSION) ||
       !strstr((const char *)launcher->gl.GetString(GL_VERSION), "OpenGL ES")) goto failed;
    launcher->gl.GetIntegerv(GL_MAX_TEXTURE_SIZE, &launcher->maximum_texture_size);
    if(width > launcher->maximum_texture_size || height > launcher->maximum_texture_size ||
       launcher->maximum_texture_size < 256) goto failed;
    /* Leave the target's SDL sampling policy intact for final window scaling. */
    launcher->target = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_BGR888, SDL_TEXTUREACCESS_TARGET, width, height);
    if(!launcher->target || SDL_GL_GetCurrentContext() != launcher->context ||
       SDL_SetTextureBlendMode(launcher->target, SDL_BLENDMODE_NONE) < 0) goto failed;
    gpu_sdl_gles2_save_gl(&launcher->gl, &state);
    saved = 1;
    vertex = gpu_sdl_gles2_compile(&launcher->gl, GL_VERTEX_SHADER, gpu_launcher_vertex_source);
    fragment = gpu_sdl_gles2_compile(&launcher->gl, GL_FRAGMENT_SHADER, gpu_launcher_fragment_source);
    if(!vertex || !fragment) goto failed;
    launcher->program = launcher->gl.CreateProgram();
    if(!launcher->program) goto failed;
    launcher->gl.AttachShader(launcher->program, vertex);
    launcher->gl.AttachShader(launcher->program, fragment);
    launcher->gl.BindAttribLocation(launcher->program, 0, "vertexPosition");
    launcher->gl.BindAttribLocation(launcher->program, 1, "vertexTexcoord");
    launcher->gl.LinkProgram(launcher->program);
    launcher->gl.GetProgramiv(launcher->program, GL_LINK_STATUS, &linked);
    if(!linked)
    {
        launcher->gl.GetProgramInfoLog(launcher->program, sizeof(message), NULL, message);
        gpu_log("ERROR: Android GPU launcher shader link failed: %s.",
                message[0] ? message : "driver returned no diagnostic");
        goto failed;
    }
#define GPU_LAUNCHER_UNIFORM(member, name) \
    do { launcher->member = launcher->gl.GetUniformLocation(launcher->program, name); \
         if(launcher->member < 0) goto failed; } while(0)
    GPU_LAUNCHER_UNIFORM(target_size, "targetSize");
    GPU_LAUNCHER_UNIFORM(source_size, "sourceSize");
    GPU_LAUNCHER_UNIFORM(source_sampler, "sourceTexture");
    GPU_LAUNCHER_UNIFORM(palette_sampler, "paletteTexture");
    GPU_LAUNCHER_UNIFORM(source_mode, "sourceMode");
    GPU_LAUNCHER_UNIFORM(foreground, "foreground");
    GPU_LAUNCHER_UNIFORM(background, "background");
    GPU_LAUNCHER_UNIFORM(fill, "fillBackground");
#undef GPU_LAUNCHER_UNIFORM
    launcher->gl.ActiveTexture(GL_TEXTURE0);
    launcher->white = gpu_launcher_new_texture(launcher, 1, 1, GL_RGBA, white);
    launcher->palette = gpu_launcher_new_texture(launcher, 256, 1, GL_RGBA, NULL);
    if(!launcher->white || !launcher->palette) goto failed;
    launcher->gl.DeleteShader(vertex);
    launcher->gl.DeleteShader(fragment);
    vertex = fragment = 0;
    gpu_sdl_gles2_restore_gl(&launcher->gl, &state);
    saved = 0;
    if(launcher->gl.GetError() != GL_NO_ERROR) goto failed;
    gpu_log("Android GPU launcher enabled: resident RGB32 target, GPU indexed palettes, boxes and font batches; no CPU composition.");
    return launcher;
failed:
    if(vertex) launcher->gl.DeleteShader(vertex);
    if(fragment) launcher->gl.DeleteShader(fragment);
    if(saved) gpu_sdl_gles2_restore_gl(&launcher->gl, &state);
    gpu_log("ERROR: Android GPU launcher initialization failed; launcher rendering has no CPU retry.");
    gpu_launcher_android_destroy(launcher);
    return NULL;
}

/*
 * Damon V. Caskey
 * 2026-11-16
 * Start a new command stream with known opaque coverage. Only this operation
 * clears a prior frame failure; idle presentation cannot revive a failed frame.
 */
int gpu_launcher_android_begin(gpu_launcher_android *launcher)
{
    gpu_launcher_scope scope;
    int okay;
    if(!launcher) return 0;
    launcher->valid = launcher->begun = 0;
    okay = gpu_launcher_scope_begin(launcher, &scope);
    if(okay)
    {
        launcher->gl.ClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        launcher->gl.Clear(GL_COLOR_BUFFER_BIT);
        okay = launcher->gl.GetError() == GL_NO_ERROR;
    }
    if(!gpu_launcher_scope_end(launcher, &scope)) okay = 0;
    if(!okay) return gpu_launcher_reject(launcher, "frame begin", GL_NO_ERROR);
    launcher->begun = launcher->valid = 1;
    ++launcher->stats.frames;
    return 1;
}

/*
 * Damon V. Caskey
 * 2026-11-16
 * Clip an opaque rectangle using wide integer edges. Source and glyph callers
 * retain the amount clipped from left/top as their sampling origin.
 */
static int gpu_launcher_clip(const gpu_launcher_android *launcher, int x, int y,
                              int width, int height, int64_t rectangle[4])
{
    rectangle[0] = x > 0 ? x : 0;
    rectangle[1] = y > 0 ? y : 0;
    rectangle[2] = (int64_t)x + width;
    rectangle[3] = (int64_t)y + height;
    if(rectangle[2] > launcher->width) rectangle[2] = launcher->width;
    if(rectangle[3] > launcher->height) rectangle[3] = launcher->height;
    return width > 0 && height > 0 && rectangle[2] > rectangle[0] && rectangle[3] > rectangle[1];
}

/*
 * Damon V. Caskey
 * 2026-11-16
 * Upload the supplied asset version directly into reusable source storage.
 * Refresh every copy so freed/reused preview pointers and palette edits cannot
 * reuse stale pixels. Indexed data never expands into a CPU RGB image.
 */
static int gpu_launcher_upload_source(gpu_launcher_android *launcher, const s_screen *source)
{
    GLenum format = source->pixelformat == PIXEL_32 ? GL_RGBA : GL_LUMINANCE;
    GLuint texture;
    gpu_sdl_gles2_api *gl = &launcher->gl;
    gl->ActiveTexture(GL_TEXTURE0);
    if(!launcher->source || launcher->source_width != source->width ||
       launcher->source_height != source->height || launcher->source_format != source->pixelformat)
    {
        texture = gpu_launcher_new_texture(launcher, source->width, source->height, format, source->data);
        if(!texture) return 0;
        if(launcher->source) gl->DeleteTextures(1, &launcher->source);
        launcher->source = texture;
        launcher->source_width = source->width;
        launcher->source_height = source->height;
        launcher->source_format = source->pixelformat;
    }
    else
    {
        gl->BindTexture(GL_TEXTURE_2D, launcher->source);
        gl->TexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, source->width, source->height,
                          format, GL_UNSIGNED_BYTE, source->data);
        if(gl->GetError() != GL_NO_ERROR) return 0;
    }
    ++launcher->stats.source_uploads;
    if(source->pixelformat == PIXEL_x8)
    {
        gl->ActiveTexture(GL_TEXTURE1);
        gl->BindTexture(GL_TEXTURE_2D, launcher->palette);
        gl->TexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 256, 1, GL_RGBA, GL_UNSIGNED_BYTE, source->palette);
        if(gl->GetError() != GL_NO_ERROR) return 0;
        ++launcher->stats.palette_uploads;
    }
    return 1;
}

/*
 * Damon V. Caskey
 * 2026-11-16
 * Copy RGB32 or indexed/paletted launcher assets opaquely in immediate command
 * order. Source alpha and palette alpha do not change RGB scene coverage.
 */
int gpu_launcher_android_screen(gpu_launcher_android *launcher, const s_screen *source, int x, int y)
{
    int64_t rectangle[4];
    gpu_launcher_scope scope;
    int okay;
    if(!gpu_launcher_ready(launcher)) return 0;
    if(!source || source->width <= 0 || source->height <= 0 ||
       source->width > launcher->maximum_texture_size || source->height > launcher->maximum_texture_size ||
       (source->pixelformat != PIXEL_32 && source->pixelformat != PIXEL_x8) ||
       (source->pixelformat == PIXEL_x8 && !source->palette))
        return gpu_launcher_reject(launcher, "source image contract", GL_NO_ERROR);
    if(!gpu_launcher_clip(launcher, x, y, source->width, source->height, rectangle)) return 1;
    if(!gpu_launcher_geometry(launcher, 6)) return gpu_launcher_reject(launcher, "image geometry", GL_NO_ERROR);
    gpu_launcher_quad(launcher->geometry, rectangle[0], rectangle[1], rectangle[2], rectangle[3],
                       (float)(rectangle[0] - x), (float)(rectangle[1] - y));
    okay = gpu_launcher_scope_begin(launcher, &scope);
    if(okay) okay = gpu_launcher_upload_source(launcher, source);
    if(okay) okay = gpu_launcher_draw(launcher, 6, source->pixelformat == PIXEL_32 ? 1 : 2,
                                      launcher->source, source->width, source->height, 0, 0, 0);
    if(!gpu_launcher_scope_end(launcher, &scope)) okay = 0;
    if(!okay) return gpu_launcher_reject(launcher, "source upload or copy", GL_NO_ERROR);
    ++launcher->stats.draws;
    return 1;
}

/*
 * Damon V. Caskey
 * 2026-11-16
 * Replace one clipped rectangle with an opaque RGB color, including black.
 * Shared SDL blending and clipping never alter launcher command semantics.
 */
int gpu_launcher_android_box(gpu_launcher_android *launcher, int x, int y,
                             int width, int height, unsigned color)
{
    int64_t rectangle[4];
    gpu_launcher_scope scope;
    int okay;
    if(!gpu_launcher_ready(launcher)) return 0;
    if(!gpu_launcher_clip(launcher, x, y, width, height, rectangle)) return 1;
    if(!gpu_launcher_geometry(launcher, 6)) return gpu_launcher_reject(launcher, "box geometry", GL_NO_ERROR);
    gpu_launcher_quad(launcher->geometry, rectangle[0], rectangle[1], rectangle[2], rectangle[3], 0.0f, 0.0f);
    okay = gpu_launcher_scope_begin(launcher, &scope);
    if(okay) okay = gpu_launcher_draw(launcher, 6, 0, 0, 1, 1, color, 0, 0);
    if(!gpu_launcher_scope_end(launcher, &scope)) okay = 0;
    if(!okay) return gpu_launcher_reject(launcher, "box submission", GL_NO_ERROR);
    ++launcher->stats.draws;
    return 1;
}

/*
 * Damon V. Caskey
 * 2026-11-16
 * Expand immutable font bits once into an 80x100 index atlas. Transactional
 * replacement retains the prior font when a new upload fails.
 */
static int gpu_launcher_font(gpu_launcher_android *launcher, const unsigned char *rows)
{
    unsigned char pixels[GPU_LAUNCHER_ATLAS_WIDTH * GPU_LAUNCHER_ATLAS_HEIGHT];
    int glyph, row, column;
    GLuint texture;
    if(launcher->font && launcher->font_rows == rows) return 1;
    for(glyph = 0; glyph < GPU_LAUNCHER_FONT_GLYPHS; ++glyph)
        for(row = 0; row < GPU_LAUNCHER_FONT_HEIGHT; ++row)
            for(column = 0; column < GPU_LAUNCHER_FONT_WIDTH; ++column)
                pixels[(glyph / GPU_LAUNCHER_ATLAS_COLUMNS * GPU_LAUNCHER_FONT_HEIGHT + row) * GPU_LAUNCHER_ATLAS_WIDTH +
                        glyph % GPU_LAUNCHER_ATLAS_COLUMNS * GPU_LAUNCHER_FONT_WIDTH + column] =
                    (unsigned char)((rows[glyph * GPU_LAUNCHER_FONT_HEIGHT + row] >> column) & 1u);
    launcher->gl.ActiveTexture(GL_TEXTURE0);
    texture = gpu_launcher_new_texture(launcher, GPU_LAUNCHER_ATLAS_WIDTH, GPU_LAUNCHER_ATLAS_HEIGHT, GL_LUMINANCE, pixels);
    if(!texture) return 0;
    if(launcher->font) launcher->gl.DeleteTextures(1, &launcher->font);
    launcher->font = texture;
    launcher->font_rows = rows;
    ++launcher->stats.font_uploads;
    return 1;
}

/*
 * Damon V. Caskey
 * 2026-11-16
 * Draw the common mapped font run in one ordered triangle batch. Validate every
 * glyph first, clip in int64_t, and let the shader choose ink/background pixels.
 */
int gpu_launcher_android_text(gpu_launcher_android *launcher, int x, int y,
                              unsigned foreground, unsigned background, int fill,
                              const unsigned char *glyphs, size_t count,
                              const unsigned char *font_rows, size_t font_glyph_count)
{
    size_t index, first, end, vertices;
    int64_t top, bottom, right;
    gpu_launcher_scope scope;
    int okay;
    if(!gpu_launcher_ready(launcher)) return 0;
    if(!count) return 1;
    if(!glyphs || !font_rows || font_glyph_count != GPU_LAUNCHER_FONT_GLYPHS ||
       count > (size_t)((INT64_MAX - INT_MAX) / GPU_LAUNCHER_FONT_WIDTH))
        return gpu_launcher_reject(launcher, "font or text run contract", GL_NO_ERROR);
    for(index = 0; index < count; ++index)
        if(glyphs[index] >= GPU_LAUNCHER_FONT_GLYPHS)
            return gpu_launcher_reject(launcher, "font glyph index", GL_NO_ERROR);
    top = y > 0 ? y : 0;
    bottom = (int64_t)y + GPU_LAUNCHER_FONT_HEIGHT;
    if(bottom > launcher->height) bottom = launcher->height;
    right = (int64_t)x + (int64_t)count * GPU_LAUNCHER_FONT_WIDTH;
    if(bottom <= top || x >= launcher->width || right <= 0) return 1;
    first = x < 0 ? (size_t)(-(int64_t)x / GPU_LAUNCHER_FONT_WIDTH) : 0;
    end = (size_t)(((int64_t)launcher->width - x + GPU_LAUNCHER_FONT_WIDTH - 1) / GPU_LAUNCHER_FONT_WIDTH);
    if(end > count) end = count;
    if(end <= first) return 1;
    if(end - first > (size_t)INT_MAX / 6) return gpu_launcher_reject(launcher, "text geometry size", GL_NO_ERROR);
    vertices = (end - first) * 6;
    if(!gpu_launcher_geometry(launcher, vertices)) return gpu_launcher_reject(launcher, "text geometry", GL_NO_ERROR);
    for(index = first; index < end; ++index)
    {
        int glyph = glyphs[index];
        int64_t left = (int64_t)x + (int64_t)index * GPU_LAUNCHER_FONT_WIDTH;
        int64_t clipped_left = left > 0 ? left : 0;
        int64_t clipped_right = left + GPU_LAUNCHER_FONT_WIDTH;
        if(clipped_right > launcher->width) clipped_right = launcher->width;
        gpu_launcher_quad(launcher->geometry + (index - first) * 6, clipped_left, top, clipped_right, bottom,
            (float)(glyph % GPU_LAUNCHER_ATLAS_COLUMNS * GPU_LAUNCHER_FONT_WIDTH + clipped_left - left),
            (float)(glyph / GPU_LAUNCHER_ATLAS_COLUMNS * GPU_LAUNCHER_FONT_HEIGHT + top - y));
    }
    okay = gpu_launcher_scope_begin(launcher, &scope);
    if(okay) okay = gpu_launcher_font(launcher, font_rows);
    if(okay) okay = gpu_launcher_draw(launcher, vertices, 3, launcher->font,
        GPU_LAUNCHER_ATLAS_WIDTH, GPU_LAUNCHER_ATLAS_HEIGHT, foreground, background, fill);
    if(!gpu_launcher_scope_end(launcher, &scope)) okay = 0;
    if(!okay) return gpu_launcher_reject(launcher, "font upload or text submission", GL_NO_ERROR);
    ++launcher->stats.draws;
    ++launcher->stats.text_draws;
    launcher->stats.text_glyphs += end - first;
    return 1;
}

/*
 * Damon V. Caskey
 * 2026-11-16
 * Carry formatting or integration failures into the same sticky frame latch.
 * The caller already owns the cause-specific diagnostic.
 */
void gpu_launcher_android_invalidate(gpu_launcher_android *launcher)
{
    if(!launcher) return;
    if(launcher->valid) ++launcher->stats.rejections;
    launcher->valid = 0;
}

/*
 * Damon V. Caskey
 * 2026-11-16
 * Borrow only a valid current-context command stream for filters and SDL
 * presentation. Idle frames reuse it without any source upload or text redraw.
 */
SDL_Texture *gpu_launcher_android_texture(gpu_launcher_android *launcher)
{
    return gpu_launcher_ready(launcher) ? launcher->target : NULL;
}

/* Damon V. Caskey - 2026-11-16: expose context-local regression diagnostics. */
const gpu_launcher_android_stats *gpu_launcher_android_get_stats(const gpu_launcher_android *launcher)
{
    return launcher ? &launcher->stats : NULL;
}

/*
 * Damon V. Caskey
 * 2026-11-16
 * Retire normal resources only while their owning context is still current.
 * Context/device loss uses abandon instead of interpreting obsolete GL names.
 */
void gpu_launcher_android_destroy(gpu_launcher_android *launcher)
{
    if(!launcher) return;
    if(launcher->renderer) SDL_RenderFlush(launcher->renderer);
    if(launcher->context && SDL_GL_GetCurrentContext() == launcher->context)
    {
        if(launcher->vertices) launcher->gl.DeleteBuffers(1, &launcher->vertices);
        if(launcher->program) launcher->gl.DeleteProgram(launcher->program);
        if(launcher->white) launcher->gl.DeleteTextures(1, &launcher->white);
        if(launcher->palette) launcher->gl.DeleteTextures(1, &launcher->palette);
        if(launcher->source) launcher->gl.DeleteTextures(1, &launcher->source);
        if(launcher->font) launcher->gl.DeleteTextures(1, &launcher->font);
    }
    if(launcher->target) SDL_DestroyTexture(launcher->target);
    free(launcher->geometry);
    free(launcher);
}

/*
 * Damon V. Caskey
 * 2026-11-16
 * Release metadata after a reset without flushing or deleting private names
 * from an invalid context. SDL remains responsible for its own texture handle.
 */
void gpu_launcher_android_abandon(gpu_launcher_android *launcher)
{
    if(!launcher) return;
    if(launcher->target) SDL_DestroyTexture(launcher->target);
    free(launcher->geometry);
    free(launcher);
}

#endif
