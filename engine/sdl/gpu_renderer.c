/*
 * OpenBOR - http://www.chronocrash.com
 * All rights reserved, see LICENSE in OpenBOR root for details.
 * Copyright (c) OpenBOR Team
 */
#if defined(OPENGL) && !defined(ANDROID)

#include <limits.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "SDL.h"
#include "loadgl.h"
#include "gpu.h"
#include "gpu_shader.h"
#include "gpu_transform.h"
#include "gpu_renderer.h"

#define GPU_CACHE_BUCKETS 1024
#define GPU_BLEND_TABLE_SIZE (256 * 256)
#if REVERSE_COLOR
#define GPU_PACKED_FORMAT GL_BGRA
#else
#define GPU_PACKED_FORMAT GL_RGBA
#endif

/* Use private entry points so the presentation backend's ARB shader loader
 * and its current program do not become dependencies of scene rendering. */
static PFNGLGENFRAMEBUFFERSPROC gpu_GenFramebuffers;
static PFNGLBINDFRAMEBUFFERPROC gpu_BindFramebuffer;
static PFNGLFRAMEBUFFERTEXTURE2DPROC gpu_FramebufferTexture2D;
static PFNGLCHECKFRAMEBUFFERSTATUSPROC gpu_CheckFramebufferStatus;
static PFNGLDELETEFRAMEBUFFERSPROC gpu_DeleteFramebuffers;
static PFNGLCREATESHADERPROC gpu_CreateShader;
static PFNGLSHADERSOURCEPROC gpu_ShaderSource;
static PFNGLCOMPILESHADERPROC gpu_CompileShader;
static PFNGLGETSHADERIVPROC gpu_GetShaderiv;
static PFNGLGETSHADERINFOLOGPROC gpu_GetShaderInfoLog;
static PFNGLDELETESHADERPROC gpu_DeleteShader;
static PFNGLCREATEPROGRAMPROC gpu_CreateProgram;
static PFNGLATTACHSHADERPROC gpu_AttachShader;
static PFNGLLINKPROGRAMPROC gpu_LinkProgram;
static PFNGLGETPROGRAMIVPROC gpu_GetProgramiv;
static PFNGLGETPROGRAMINFOLOGPROC gpu_GetProgramInfoLog;
static PFNGLUSEPROGRAMPROC gpu_UseProgram;
static PFNGLDELETEPROGRAMPROC gpu_DeleteProgram;
static PFNGLGETUNIFORMLOCATIONPROC gpu_GetUniformLocation;
static PFNGLUNIFORM1IPROC gpu_Uniform1i;
static PFNGLUNIFORM2FPROC gpu_Uniform2f;
static PFNGLUNIFORM3FPROC gpu_Uniform3f;
static PFNGLACTIVETEXTUREPROC gpu_ActiveTexture;

typedef struct gpu_image
{
    const void *owner;
    int sprite;
    int width, height, format;
    /* Exactly one side may hold newer pixels. Clearing both flags means
     * the CPU shadow and GPU texture describe the same screen version. */
    int cpu_dirty, gpu_dirty;
    GLuint texture, framebuffer, snapshot;
    struct gpu_image *next;
} gpu_image;

typedef struct gpu_quad
{
    gpu_vertex vertex[4];
    int wrap;
} gpu_quad;

typedef struct gpu_uniforms
{
    GLint target_size, source_size;
    GLint source, palette, mask, destination, tables;
    GLint indexed, masked, color_key, fill, has_destination, wrap;
    GLint blend_mode, tint_mode, table_mask;
    GLint fill_color, tint_color, channels;
} gpu_uniforms;

static gpu_image *image_cache[GPU_CACHE_BUCKETS];
static gpu_quad *quad_buffer;
static size_t quad_count, quad_capacity;
static float bounds_left, bounds_top, bounds_right, bounds_bottom;
static GLuint program, white_texture, palette_texture, table_texture;
static gpu_uniforms uniforms;
static gpu_renderer_stats stats;
static GLint maximum_texture_size;
static int initialized, tables_dirty = 1, table_mask, palette_valid;
static unsigned palette_cache[256];
static const unsigned char *table_pointers[MAX_BLENDINGS];

/*
 * Damon V. Caskey
 * 2026-11-12
 * Report an unrecoverable error after a GPU operation has begun. Retrying
 * the operation in software after partial drawing would blend it twice.
 */
static void gpu_fatal(const char *operation)
{
    fprintf(stderr, "GPU renderer: %s failed; refusing to use stale image data.\n", operation);
    abort();
}

/*
 * Damon V. Caskey
 * 2026-11-12
 * Obtain shader/FBO entry points from SDL's current context. EXT FBO names
 * retain support for the engine's existing desktop compatibility context.
 */
static int gpu_load_functions(void)
{
    /* Resolve the complete dispatch set before registering the backend.
     * Missing entry points leave the existing software renderer available. */
#define GPU_LOAD(name, type) \
    do \
    { \
        gpu_##name = (type)SDL_GL_GetProcAddress("gl" #name); \
        if(!gpu_##name) \
        { \
            return 0; \
        } \
    } while(0)
#define GPU_LOAD_FBO(name, type) \
    do \
    { \
        gpu_##name = (type)SDL_GL_GetProcAddress("gl" #name); \
        if(!gpu_##name) \
        { \
            gpu_##name = (type)SDL_GL_GetProcAddress("gl" #name "EXT"); \
        } \
        if(!gpu_##name) \
        { \
            return 0; \
        } \
    } while(0)
    GPU_LOAD_FBO(GenFramebuffers, PFNGLGENFRAMEBUFFERSPROC);
    GPU_LOAD_FBO(BindFramebuffer, PFNGLBINDFRAMEBUFFERPROC);
    GPU_LOAD_FBO(FramebufferTexture2D, PFNGLFRAMEBUFFERTEXTURE2DPROC);
    GPU_LOAD_FBO(CheckFramebufferStatus, PFNGLCHECKFRAMEBUFFERSTATUSPROC);
    GPU_LOAD_FBO(DeleteFramebuffers, PFNGLDELETEFRAMEBUFFERSPROC);
    GPU_LOAD(CreateShader, PFNGLCREATESHADERPROC);
    GPU_LOAD(ShaderSource, PFNGLSHADERSOURCEPROC);
    GPU_LOAD(CompileShader, PFNGLCOMPILESHADERPROC);
    GPU_LOAD(GetShaderiv, PFNGLGETSHADERIVPROC);
    GPU_LOAD(GetShaderInfoLog, PFNGLGETSHADERINFOLOGPROC);
    GPU_LOAD(DeleteShader, PFNGLDELETESHADERPROC);
    GPU_LOAD(CreateProgram, PFNGLCREATEPROGRAMPROC);
    GPU_LOAD(AttachShader, PFNGLATTACHSHADERPROC);
    GPU_LOAD(LinkProgram, PFNGLLINKPROGRAMPROC);
    GPU_LOAD(GetProgramiv, PFNGLGETPROGRAMIVPROC);
    GPU_LOAD(GetProgramInfoLog, PFNGLGETPROGRAMINFOLOGPROC);
    GPU_LOAD(UseProgram, PFNGLUSEPROGRAMPROC);
    GPU_LOAD(DeleteProgram, PFNGLDELETEPROGRAMPROC);
    GPU_LOAD(GetUniformLocation, PFNGLGETUNIFORMLOCATIONPROC);
    GPU_LOAD(Uniform1i, PFNGLUNIFORM1IPROC);
    GPU_LOAD(Uniform2f, PFNGLUNIFORM2FPROC);
    GPU_LOAD(Uniform3f, PFNGLUNIFORM3FPROC);
    GPU_LOAD(ActiveTexture, PFNGLACTIVETEXTUREPROC);
#undef GPU_LOAD_FBO
#undef GPU_LOAD
    return 1;
}

/*
 * Damon V. Caskey
 * 2026-11-12
 * Compile one scene shader, preserving the driver's diagnostic on failure.
 */
static GLuint gpu_compile_shader(GLenum type, const char *source)
{
    GLuint shader = gpu_CreateShader(type);
    GLint success = 0;
    char message[4096];
    if(!shader)
    {
        return 0;
    }

    gpu_ShaderSource(shader, 1, &source, NULL);
    gpu_CompileShader(shader);
    gpu_GetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if(!success)
    {
        /* Keep the driver's explanation before releasing the failed object,
         * so startup can report the shader issue and retain software drawing. */
        gpu_GetShaderInfoLog(shader, sizeof(message), NULL, message);
        fprintf(stderr, "GPU renderer shader: %s\n", message);
        gpu_DeleteShader(shader);
        return 0;
    }
    return shader;
}

/*
 * Damon V. Caskey
 * 2026-11-12
 * Bind a texture with pixel-art sampling, irrespective of the presenter's
 * scaling filter. Rendering and display scaling deliberately use separate state.
 */
static void gpu_bind_texture(int unit, GLuint texture)
{
    gpu_ActiveTexture(GL_TEXTURE0 + unit);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
}

/*
 * Damon V. Caskey
 * 2026-11-12
 * Allocate a reusable texture with tightly packed rows and no mipmaps.
 */
static GLuint gpu_new_texture(int width, int height, GLenum internal_format,
                              GLenum format, GLenum type, const void *pixels)
{
    GLuint texture = 0;
    glGenTextures(1, &texture);
    gpu_bind_texture(0, texture);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
    glPixelStorei(GL_UNPACK_SKIP_ROWS, 0);
    glPixelStorei(GL_UNPACK_SKIP_PIXELS, 0);
    glTexImage2D(GL_TEXTURE_2D, 0, internal_format, width, height, 0, format, type, pixels);
    if(glGetError() != GL_NO_ERROR)
    {
        glDeleteTextures(1, &texture);
        return 0;
    }
    return texture;
}

/*
 * Damon V. Caskey
 * 2026-11-12
 * Hash image identity without dereferencing an allocation that may already
 * be on its destruction or re-encoding path.
 */
static size_t gpu_image_bucket(const void *owner)
{
    uintptr_t value = (uintptr_t)owner;
    return ((value >> 4) ^ (value >> 14)) & (GPU_CACHE_BUCKETS - 1);
}

/*
 * Damon V. Caskey
 * 2026-11-12
 * Find a cached GPU resource by stable CPU object identity and object kind.
 */
static gpu_image *gpu_find_image(const void *owner, int sprite)
{
    gpu_image *image;
    for(image = image_cache[gpu_image_bucket(owner)]; image; image = image->next)
    {
        if(image->owner == owner && image->sprite == sprite)
        {
            return image;
        }
    }
    return NULL;
}

/*
 * Damon V. Caskey
 * 2026-11-12
 * Release GPU handles. OpenGL defers physical destruction until previously
 * submitted commands using those handles are complete; no CPU wait is needed.
 */
static void gpu_delete_image(gpu_image *image)
{
    if(image->framebuffer)
    {
        gpu_DeleteFramebuffers(1, &image->framebuffer);
    }
    if(image->snapshot)
    {
        glDeleteTextures(1, &image->snapshot);
    }
    if(image->texture)
    {
        glDeleteTextures(1, &image->texture);
    }
    free(image);
}

/*
 * Damon V. Caskey
 * 2026-11-12
 * Detach an image before its owner address can be reused for another object.
 */
static void gpu_forget_image(const void *owner, int sprite)
{
    gpu_image **link = &image_cache[gpu_image_bucket(owner)];
    while(*link)
    {
        gpu_image *image = *link;
        if(image->owner == owner && image->sprite == sprite)
        {
            /* Updating the incoming link also handles the bucket head. The
             * remaining collision chain stays reachable after this removal. */
            *link = image->next;
            gpu_delete_image(image);
            return;
        }
        link = &image->next;
    }
}

/*
 * Damon V. Caskey
 * 2026-11-12
 * Upload a CPU-owned screen version only after an actual software mutation.
 * Unchanged GPU textures bypass the transfer and remain ready for sampling.
 */
static int gpu_upload_screen(gpu_image *image, s_screen *screen)
{
    GLenum format = image->format == PIXEL_32 ? GPU_PACKED_FORMAT : GL_LUMINANCE;
    GLenum type = image->format == PIXEL_32 ? GL_UNSIGNED_INT_8_8_8_8_REV : GL_UNSIGNED_BYTE;
    if(!image->cpu_dirty)
    {
        return 1;
    }

    /* CPU mutations own the complete screen version. Reset unpack state
     * explicitly because uploads share a context with presentation code. */
    gpu_bind_texture(0, image->texture);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
    glPixelStorei(GL_UNPACK_SKIP_ROWS, 0);
    glPixelStorei(GL_UNPACK_SKIP_PIXELS, 0);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, image->width, image->height,
                   format, type, screen->data);
    if(glGetError() != GL_NO_ERROR)
    {
        return 0;
    }

    /* Both copies now describe the same pixels; transfer ownership only
     * after the driver has accepted the upload. */
    image->cpu_dirty = image->gpu_dirty = 0;
    stats.uploads++;
    return 1;
}

/*
 * Damon V. Caskey
 * 2026-11-12
 * Lazily allocate a screen texture and, for RGB targets, its framebuffer.
 * Oversized or unsupported images remain on the coherent software path.
 */
static gpu_image *gpu_screen_image(s_screen *screen, int upload)
{
    gpu_image *image;
    size_t bucket;

    /* Validate the storage contract before allocating or exposing a cache
     * entry. Indexed screens are sources; RGB screens can also be targets. */
    if(!screen || screen->width <= 0 || screen->height <= 0 ||
       screen->width > maximum_texture_size || screen->height > maximum_texture_size ||
       (screen->pixelformat != PIXEL_32 && screen->pixelformat != PIXEL_x8))
    {
        return NULL;
    }
    image = gpu_find_image(screen, 0);
    if(image && (image->width != screen->width || image->height != screen->height ||
                 image->format != screen->pixelformat))
    {
        return NULL;
    }
    if(!image)
    {
        image = (gpu_image *)calloc(1, sizeof(*image));
        if(!image)
        {
            return NULL;
        }
        image->owner = screen;
        image->width = screen->width;
        image->height = screen->height;
        image->format = screen->pixelformat;
        image->cpu_dirty = 1;
        image->texture = gpu_new_texture(image->width, image->height,
            image->format == PIXEL_32 ? GL_RGBA8 : GL_LUMINANCE8,
            image->format == PIXEL_32 ? GL_RGBA : GL_LUMINANCE, GL_UNSIGNED_BYTE, NULL);
        if(!image->texture)
        {
            gpu_delete_image(image);
            return NULL;
        }
        if(image->format == PIXEL_32)
        {
            /* Rendering writes directly into the texture attached here.
             * Later screen draws can sample that texture without readback. */
            gpu_GenFramebuffers(1, &image->framebuffer);
            gpu_BindFramebuffer(GL_FRAMEBUFFER, image->framebuffer);
            gpu_FramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                                     GL_TEXTURE_2D, image->texture, 0);
            if(gpu_CheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
            {
                gpu_BindFramebuffer(GL_FRAMEBUFFER, 0);
                gpu_delete_image(image);
                return NULL;
            }
        }

        /* Publish only fully usable resources, so a later lookup cannot
         * find an image whose framebuffer failed during construction. */
        bucket = gpu_image_bucket(screen);
        image->next = image_cache[bucket];
        image_cache[bucket] = image;
    }
    if(upload && !gpu_upload_screen(image, screen))
    {
        return NULL;
    }
    return image;
}

/*
 * Damon V. Caskey
 * 2026-11-12
 * Decode the engine's RLE sprite once into an indexed GPU source. Palette
 * choice remains a separate draw-time lookup, preserving cheap color swaps.
 */
static gpu_image *gpu_sprite_image(s_sprite *sprite)
{
    gpu_image *image;
    unsigned char *pixels;
    int x, y;
    size_t bucket;
    if(!sprite || sprite->width <= 0 || sprite->height <= 0 ||
       sprite->width > maximum_texture_size || sprite->height > maximum_texture_size)
    {
        return NULL;
    }
    image = gpu_find_image(sprite, 1);
    if(image)
    {
        return image;
    }
    if((size_t)sprite->width > SIZE_MAX / (size_t)sprite->height)
    {
        return NULL;
    }
    pixels = (unsigned char *)calloc((size_t)sprite->width, (size_t)sprite->height);
    if(!pixels)
    {
        return NULL;
    }

    /* Each row offset is relative to its own table entry. Zero-filled gaps
     * preserve the transparent palette index while encoded runs supply the
     * visible indices. Check both lengths before writing into the row. */
    for(y = 0; y < sprite->height; y++)
    {
        const unsigned char *line = (const unsigned char *)&sprite->data[y] + sprite->data[y];
        x = 0;
        while(*line != 0xff)
        {
            int skip = *line++;
            int run = *line++;
            if(skip > sprite->width - x || run > sprite->width - x - skip)
            {
                free(pixels);
                return NULL;
            }
            x += skip;
            memcpy(pixels + (size_t)y * sprite->width + x, line, (size_t)run);
            line += run;
            x += run;
        }
    }
    image = (gpu_image *)calloc(1, sizeof(*image));
    if(!image)
    {
        free(pixels);
        return NULL;
    }
    image->owner = sprite;
    image->sprite = 1;
    image->width = sprite->width;
    image->height = sprite->height;
    image->format = PIXEL_x8;
    image->texture = gpu_new_texture(image->width, image->height, GL_LUMINANCE8,
                                     GL_LUMINANCE, GL_UNSIGNED_BYTE, pixels);
    free(pixels);
    if(!image->texture)
    {
        gpu_delete_image(image);
        return NULL;
    }
    /* The decoded staging buffer is finished after upload. Keep the texture
     * associated with sprite identity until release or re-encoding. */
    bucket = gpu_image_bucket(sprite);
    image->next = image_cache[bucket];
    image_cache[bucket] = image;
    stats.uploads++;
    return image;
}

/*
 * Damon V. Caskey
 * 2026-11-12
 * Read a newer GPU screen back exactly once for explicit CPU access. The
 * unused packed alpha byte is cleared to retain the engine's RGB contract.
 */
static int gpu_read_screen(s_screen *screen)
{
    gpu_image *image = gpu_find_image(screen, 0);
    size_t i, count;
    unsigned *pixels;
    if(!image || !image->gpu_dirty)
    {
        return 1;
    }

    /* This is the synchronization boundary for CPU consumers. Repeated CPU
     * reads reuse the populated shadow until another GPU draw changes it. */
    gpu_BindFramebuffer(GL_FRAMEBUFFER, image->framebuffer);
    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    glPixelStorei(GL_PACK_ROW_LENGTH, 0);
    glPixelStorei(GL_PACK_SKIP_ROWS, 0);
    glPixelStorei(GL_PACK_SKIP_PIXELS, 0);
    glReadPixels(0, 0, image->width, image->height, GPU_PACKED_FORMAT,
                 GL_UNSIGNED_INT_8_8_8_8_REV, screen->data);
    if(glGetError() != GL_NO_ERROR)
    {
        return 0;
    }
    count = (size_t)image->width * image->height;
    pixels = (unsigned *)screen->data;
    for(i = 0; i < count; i++)
    {
        pixels[i] &= 0x00ffffffu;
    }
    image->gpu_dirty = 0;
    stats.readbacks++;
    return 1;
}

/*
 * Damon V. Caskey
 * 2026-11-12
 * Record CPU ownership after a synchronized software writer changes pixels.
 */
static void gpu_dirty_screen(s_screen *screen)
{
    gpu_image *image = gpu_find_image(screen, 0);
    if(image)
    {
        image->cpu_dirty = 1;
        image->gpu_dirty = 0;
    }
}

/*
 * Damon V. Caskey
 * 2026-11-12
 * Evict a screen by identity without downloading its discarded contents.
 */
static void gpu_free_screen(s_screen *screen)
{
    gpu_forget_image(screen, 0);
}

/*
 * Damon V. Caskey
 * 2026-11-12
 * Evict a sprite or mask by identity, including before in-place re-encoding.
 */
static void gpu_free_sprite(s_sprite *sprite)
{
    gpu_forget_image(sprite, 1);
}

/*
 * Damon V. Caskey
 * 2026-11-12
 * Mark blend lookup textures obsolete after engine table replacement.
 */
static void gpu_dirty_tables(void)
{
    tables_dirty = 1;
}

/*
 * Damon V. Caskey
 * 2026-11-12
 * Update the small draw-time palette only when its actual values change.
 * Pointer identity alone is insufficient because scripts can edit a palette.
 */
static int gpu_set_palette(const unsigned char *palette)
{
    if(!palette)
    {
        return 0;
    }

    /* Comparing 256 entries also catches in-place palette edits. Cache the
     * accepted values only after upload so failures remain retryable. */
    if(!palette_valid || memcmp(palette_cache, palette, sizeof(palette_cache)))
    {
        gpu_bind_texture(1, palette_texture);
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
        glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 256, 1, GPU_PACKED_FORMAT,
                       GL_UNSIGNED_INT_8_8_8_8_REV, palette);
        if(glGetError() != GL_NO_ERROR)
        {
            return 0;
        }
        memcpy(palette_cache, palette, sizeof(palette_cache));
        palette_valid = 1;
        stats.uploads++;
    }
    return 1;
}

/*
 * Damon V. Caskey
 * 2026-11-12
 * Preserve channel lookup blends on the GPU. Cached table values are reused
 * across draws and refreshed after replacement or explicit invalidation.
 */
static int gpu_set_tables(void)
{
    int i;

    /* Pointer replacement catches newly allocated tables; the explicit
     * invalidation callback also covers edits to an existing allocation. */
    for(i = 0; i < MAX_BLENDINGS; i++)
    {
        if(table_pointers[i] != blendtables[i])
        {
            tables_dirty = 1;
        }
    }
    if(!tables_dirty)
    {
        return 1;
    }
    table_mask = 0;
    /* The 16-bit mode's tables are smaller, with different channel domains.
     * Those drawing operations must remain in their existing software path. */
    if(pixelformat == PIXEL_16)
    {
        for(i = 0; i < MAX_BLENDINGS; i++)
        {
            if(blendtables[i])
            {
                return 0;
            }
        }
    }
    gpu_bind_texture(4, table_texture);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
    /* Each mode occupies one 256-row slice. The presence mask lets the
     * shader choose between a custom lookup and its built-in blend equation. */
    for(i = 0; i < MAX_BLENDINGS; i++)
    {
        if(blendtables[i])
        {
            glTexSubImage2D(GL_TEXTURE_2D, 0, 0, i * 256, 256, 256,
                           GL_LUMINANCE, GL_UNSIGNED_BYTE, blendtables[i]);
            if(glGetError() != GL_NO_ERROR)
            {
                return 0;
            }
            table_mask |= 1 << i;
            stats.uploads++;
        }
        table_pointers[i] = blendtables[i];
    }
    tables_dirty = 0;
    return 1;
}

/*
 * Damon V. Caskey
 * 2026-11-12
 * Convert packed engine RGB into normalized shader channel values without
 * assigning transparency to the unused high byte.
 */
static void gpu_uniform_color(GLint location, unsigned color)
{
#if REVERSE_COLOR
    gpu_Uniform3f(location, ((color >> 16) & 255) / 255.0f,
                  ((color >> 8) & 255) / 255.0f, (color & 255) / 255.0f);
#else
    gpu_Uniform3f(location, (color & 255) / 255.0f,
                  ((color >> 8) & 255) / 255.0f, ((color >> 16) & 255) / 255.0f);
#endif
}

/*
 * Damon V. Caskey
 * 2026-11-12
 * Collect geometry before touching the target. This allows parameter or
 * allocation failure to select software without replaying a partial GPU draw.
 */
static int gpu_collect_quad(const gpu_vertex vertices[4], int wrap, void *unused)
{
    int i;
    (void)unused;
    if(quad_count == quad_capacity)
    {
        /* Grow geometrically and retain the allocation between draws. Size
         * checks keep an oversized request on the untouched fallback path. */
        size_t capacity = quad_capacity ? quad_capacity * 2 : 64;
        gpu_quad *replacement;
        if(capacity < quad_capacity || capacity > SIZE_MAX / sizeof(*quad_buffer))
        {
            return 0;
        }
        replacement = (gpu_quad *)realloc(quad_buffer, capacity * sizeof(*quad_buffer));
        if(!replacement)
        {
            return 0;
        }
        quad_buffer = replacement;
        quad_capacity = capacity;
    }
    if(!quad_count)
    {
        bounds_left = bounds_right = vertices[0].x;
        bounds_top = bounds_bottom = vertices[0].y;
    }
    /* Track one conservative rectangle for clipping and destination copies.
     * Reject non-finite coordinates before submitting any collected geometry. */
    for(i = 0; i < 4; i++)
    {
        if(!isfinite(vertices[i].x) || !isfinite(vertices[i].y) ||
           !isfinite(vertices[i].u) || !isfinite(vertices[i].v))
        {
            return 0;
        }
        if(vertices[i].x < bounds_left)
        {
            bounds_left = vertices[i].x;
        }
        if(vertices[i].x > bounds_right)
        {
            bounds_right = vertices[i].x;
        }
        if(vertices[i].y < bounds_top)
        {
            bounds_top = vertices[i].y;
        }
        if(vertices[i].y > bounds_bottom)
        {
            bounds_bottom = vertices[i].y;
        }
    }
    memcpy(quad_buffer[quad_count].vertex, vertices, sizeof(quad_buffer[quad_count].vertex));
    quad_buffer[quad_count++].wrap = wrap;
    return 1;
}

/*
 * Damon V. Caskey
 * 2026-11-12
 * Intersect geometry and explicit clipping with target bounds before any
 * float-to-int conversion. Large offscreen transforms cannot overflow a cast.
 */
static int gpu_draw_bounds(gpu_image *target, const s_drawmethod *method, int rectangle[4])
{
    double left = fmax(0.0, floor(bounds_left));
    double top = fmax(0.0, floor(bounds_top));
    double right = fmin((double)target->width, ceil(bounds_right));
    double bottom = fmin((double)target->height, ceil(bounds_bottom));
    if(method->clipw > 0 && method->cliph > 0)
    {
        left = fmax(left, method->clipx);
        top = fmax(top, method->clipy);
        right = fmin(right, (double)method->clipx + method->clipw);
        bottom = fmin(bottom, (double)method->clipy + method->cliph);
    }
    if(right <= left || bottom <= top)
    {
        return 0;
    }

    /* Surviving coordinates are bounded by the target dimensions, making
     * these integer values safe for both scissoring and texture copies. */
    rectangle[0] = (int)left;
    rectangle[1] = (int)top;
    rectangle[2] = (int)(right - left);
    rectangle[3] = (int)(bottom - top);
    return 1;
}

/*
 * Damon V. Caskey
 * 2026-11-12
 * Snapshot the previous destination entirely on the GPU. Ordinary blending
 * copies only the affected rectangle; self-drawing snapshots the whole source
 * so a transform can read outside its destination rectangle without feedback.
 */
static int gpu_snapshot_target(gpu_image *target, const int rectangle[4], int self_draw)
{
    int x = self_draw ? 0 : rectangle[0];
    int y = self_draw ? 0 : rectangle[1];
    int width = self_draw ? target->width : rectangle[2];
    int height = self_draw ? target->height : rectangle[3];
    if(!target->snapshot)
    {
        target->snapshot = gpu_new_texture(target->width, target->height,
                                           GL_RGBA8, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
        if(!target->snapshot)
        {
            return 0;
        }
    }

    /* Matching source/destination offsets preserve screen-space addressing.
     * Blends sample only the copied rectangle; self-draws need the whole image. */
    gpu_BindFramebuffer(GL_FRAMEBUFFER, target->framebuffer);
    gpu_bind_texture(3, target->snapshot);
    glCopyTexSubImage2D(GL_TEXTURE_2D, 0, x, y, x, y, width, height);
    if(glGetError() != GL_NO_ERROR)
    {
        return 0;
    }
    stats.destination_copies++;
    return 1;
}

/*
 * Damon V. Caskey
 * 2026-11-12
 * Render a prepared image draw in painter order. Source palette/tint work
 * stays in the shader; destination-dependent effects sample a separate GPU
 * snapshot, never the attachment currently being written.
 */
static int gpu_render_quads(gpu_image *target, gpu_image *source, gpu_image *mask,
                            const unsigned char *palette, const s_drawmethod *method,
                            int source_is_sprite, int primitive, unsigned color)
{
    int rectangle[4], i, wrap = -1;
    int indexed = source && source->format == PIXEL_x8;
    int masked = mask != NULL;
    int blend_mode = method->alpha > 0 ? method->alpha : 0;
    int needs_destination = masked || blend_mode != 0;
    int self_draw = source == target;
    int fill = primitive || method->fillcolor != 0;
    GLuint texture = source ? source->texture : white_texture;
    size_t q;

    /* Finish resource preparation before touching destination pixels. Empty
     * or fully clipped geometry is already a successful, completed draw. */
    if(!quad_count || !gpu_draw_bounds(target, method, rectangle))
    {
        return 1;
    }
    if(blend_mode > MAX_BLENDINGS || method->tintmode > MAX_BLENDINGS)
    {
        return 0;
    }
    if(indexed && !palette && !fill)
    {
        return 0;
    }
    if(indexed && palette && !gpu_set_palette(palette))
    {
        return 0;
    }
    if((blend_mode || method->tintmode) && !gpu_set_tables())
    {
        return 0;
    }

    /* Sampling a separate pre-draw image gives blending and overlapping
     * self-copies a stable source throughout rasterization. */
    if((needs_destination || self_draw) && !gpu_snapshot_target(target, rectangle, self_draw))
    {
        return 0;
    }
    if(self_draw)
    {
        texture = target->snapshot;
    }

    /* Establish every raster state shared with the presenter. The material
     * shader computes the final RGB value, including destination blending. */
    gpu_BindFramebuffer(GL_FRAMEBUFFER, target->framebuffer);
    glViewport(0, 0, target->width, target->height);
    glEnable(GL_SCISSOR_TEST);
    glScissor(rectangle[0], rectangle[1], rectangle[2], rectangle[3]);
    glDisable(GL_BLEND);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    glDisable(GL_ALPHA_TEST);
    glDisable(GL_STENCIL_TEST);
    glDisable(GL_DITHER);
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    gpu_UseProgram(program);
    gpu_bind_texture(0, texture);
    gpu_bind_texture(1, palette_texture);
    gpu_bind_texture(2, masked ? mask->texture : white_texture);
    gpu_bind_texture(3, needs_destination ? target->snapshot : white_texture);
    gpu_bind_texture(4, table_texture);
    gpu_ActiveTexture(GL_TEXTURE0);

    /* Keep geometry in engine pixel units and pass the existing drawmethod
     * as material state. Palette, mask and blend tables remain independent
     * textures so changing a color effect preserves the cached source image. */
    gpu_Uniform2f(uniforms.target_size, (float)target->width, (float)target->height);
    gpu_Uniform2f(uniforms.source_size, source ? (float)source->width : 1.0f,
                  source ? (float)source->height : 1.0f);
    gpu_Uniform1i(uniforms.source, 0);
    gpu_Uniform1i(uniforms.palette, 1);
    gpu_Uniform1i(uniforms.mask, 2);
    gpu_Uniform1i(uniforms.destination, 3);
    gpu_Uniform1i(uniforms.tables, 4);
    gpu_Uniform1i(uniforms.indexed, indexed);
    gpu_Uniform1i(uniforms.masked, masked);
    gpu_Uniform1i(uniforms.color_key, !primitive &&
                  (source_is_sprite || (method->config & DRAWMETHOD_CONFIG_BACKGROUND_TRANSPARENCY)));
    gpu_Uniform1i(uniforms.fill, fill);
    gpu_Uniform1i(uniforms.has_destination, needs_destination);
    gpu_Uniform1i(uniforms.blend_mode, blend_mode);
    gpu_Uniform1i(uniforms.tint_mode, (int)method->tintmode);
    gpu_Uniform1i(uniforms.table_mask, table_mask);
    gpu_uniform_color(uniforms.fill_color, primitive ? color : method->fillcolor);
    gpu_uniform_color(uniforms.tint_color, method->tintcolor);
    gpu_Uniform3f(uniforms.channels, method->channelr / 255.0f,
                  method->channelg / 255.0f, method->channelb / 255.0f);

    /* Adjacent row strips share state and one submission. Queue entries are
     * never sorted by texture or moved across capture/mutation boundaries. */
    for(q = 0; q < quad_count; q++)
    {
        if(quad_buffer[q].wrap != wrap)
        {
            if(wrap != -1)
            {
                glEnd();
            }
            wrap = quad_buffer[q].wrap;
            gpu_Uniform1i(uniforms.wrap, wrap);
            glBegin(GL_QUADS);
        }
        for(i = 0; i < 4; i++)
        {
            const gpu_vertex *vertex = &quad_buffer[q].vertex[i];
            glTexCoord2f(vertex->u, vertex->v);
            glVertex2f(vertex->x, vertex->y);
        }
    }
    if(wrap != -1)
    {
        glEnd();
    }
    if(glGetError() != GL_NO_ERROR)
    {
        gpu_fatal("scene draw");
    }

    /* Subsequent GPU consumers can use the texture immediately. Its CPU
     * shadow becomes current only when an explicit reader synchronizes it. */
    target->gpu_dirty = 1;
    target->cpu_dirty = 0;
    stats.draws++;
    return 1;
}

/*
 * Damon V. Caskey
 * 2026-11-12
 * Apply existing repeat/span semantics using the same prepared source image.
 * Collect each instance before drawing, and reject overflowing spans before
 * submission so software fallback cannot repeat already-composited instances.
 */
static int gpu_render_image(gpu_image *target, gpu_image *source, gpu_image *mask,
                            const unsigned char *palette, int x, int y,
                            int centerx, int centery, int sprite,
                            const s_drawmethod *drawmethod)
{
    const s_drawmethod *method = drawmethod && (drawmethod->config & DRAWMETHOD_CONFIG_ENABLED)
                              ? drawmethod : &plainmethod;
    gpu_transform_parameters parameters;
    long long final_x, final_y;
    int i, j, submitted = 0;
    if(method->xrepeat <= 0 || method->yrepeat <= 0)
    {
        return 1;
    }

    /* Each coordinate advances monotonically by its span. Checking the last
     * instance in wider arithmetic also bounds all intermediate instances. */
    final_x = (long long)x + (long long)(method->xrepeat - 1) * method->xspan;
    final_y = (long long)y + (long long)(method->yrepeat - 1) * method->yspan;
    if(final_x < INT_MIN || final_x > INT_MAX || final_y < INT_MIN || final_y > INT_MAX)
    {
        return 0;
    }
    parameters.source_width = source->width;
    parameters.source_height = source->height;
    parameters.target_width = target->width;
    parameters.target_height = target->height;
    parameters.centerx = centerx;
    parameters.centery = centery;
    parameters.is_sprite = sprite;

    /* Submit repeats in their original order so overlapping instances blend
     * against prior ones. Each instance shares the source and scratch storage. */
    for(j = 0; j < method->yrepeat; j++)
    {
        for(i = 0; i < method->xrepeat; i++)
        {
            parameters.x = (int)((long long)x + (long long)i * method->xspan);
            parameters.y = (int)((long long)y + (long long)j * method->yspan);
            quad_count = 0;
            if(gpu_transform_draw(&parameters, method, gpu_collect_quad, NULL) == GPU_TRANSFORM_FAILED ||
               !gpu_render_quads(target, source, mask, palette, method, sprite, 0, 0))
            {
                /* Earlier instances may already have changed the target.
                 * Only failure before any submission can safely fall back. */
                if(submitted)
                {
                    gpu_fatal("repeated image draw");
                }
                return 0;
            }
            if(quad_count)
            {
                submitted = 1;
            }
        }
    }
    return 1;
}

/*
 * Damon V. Caskey
 * 2026-11-12
 * Draw one screen through the native RGB target path. Unsupported formats
 * return before writes and retain their original software implementation.
 */
static int gpu_screen_draw(s_screen *dest, s_screen *src, int x, int y, const s_drawmethod *method)
{
    gpu_image *target, *source;
    const unsigned char *palette;
    if(!dest || !src || dest->pixelformat != PIXEL_32)
    {
        goto fallback;
    }
    if(src->width <= 0 || src->height <= 0)
    {
        return 1;
    }

    /* Bring CPU-authored images up to date before sampling or modifying
     * them. GPU-authored screens already have a current cached texture. */
    target = gpu_screen_image(dest, 1);
    source = gpu_screen_image(src, 1);
    if(!target || !source)
    {
        goto fallback;
    }
    palette = method && (method->config & DRAWMETHOD_CONFIG_ENABLED) && method->table
            ? method->table : src->palette;
    if(gpu_render_image(target, source, NULL, palette, x, y, 0, 0, 0, method))
    {
        return 1;
    }
fallback:
    /* The caller synchronizes CPU shadows before entering the software
     * rasterizer. Reaching here guarantees that this draw changed no pixels. */
    stats.fallback_draws++;
    return 0;
}

/*
 * Damon V. Caskey
 * 2026-11-12
 * Draw a cached indexed sprite with its current palette, pivot and optional
 * independent opacity mask. Cached uploads are reused across frames.
 */
static int gpu_sprite_draw(s_screen *dest, s_sprite *sprite, int x, int y, const s_drawmethod *method)
{
    gpu_image *target, *source, *mask = NULL;
    const unsigned char *palette;
    if(!dest || !sprite || dest->pixelformat != PIXEL_32)
    {
        goto fallback;
    }
    if(sprite->width <= 0 || sprite->height <= 0)
    {
        return 1;
    }
    if(sprite->pixelformat != PIXEL_x8 && sprite->pixelformat != PIXEL_8)
    {
        goto fallback;
    }
    if(sprite->mask && (sprite->mask->width != sprite->width || sprite->mask->height != sprite->height))
    {
        goto fallback;
    }

    /* Sprite and opacity mask use the same texel coordinates but independent
     * cached textures. All three resources must be ready before submission. */
    target = gpu_screen_image(dest, 1);
    source = gpu_sprite_image(sprite);
    if(sprite->mask)
    {
        mask = gpu_sprite_image(sprite->mask);
    }
    if(!target || !source || (sprite->mask && !mask))
    {
        goto fallback;
    }
    palette = method && (method->config & DRAWMETHOD_CONFIG_ENABLED) && method->table
            ? method->table : sprite->palette;
    if(gpu_render_image(target, source, mask, palette, x, y, sprite->centerx, sprite->centery, 1, method))
    {
        return 1;
    }
fallback:
    stats.fallback_draws++;
    return 0;
}

/*
 * Damon V. Caskey
 * 2026-11-12
 * Append a solid primitive rectangle. Constant source coordinates allow the
 * same material shader to handle black fills, tint and destination blending.
 */
static int gpu_collect_rectangle(float x, float y, float width, float height)
{
    gpu_vertex vertex[4] = {
        {x, y, 0.5f, 0.5f}, {x + width, y, 0.5f, 0.5f},
        {x + width, y + height, 0.5f, 0.5f}, {x, y + height, 0.5f, 0.5f}
    };
    return gpu_collect_quad(vertex, 0, NULL);
}

/*
 * Damon V. Caskey
 * 2026-11-12
 * Render box and dot primitives in target pixel coordinates, retaining
 * their existing clipping, tint and destination-blend behavior.
 */
static int gpu_box_draw(s_screen *dest, int x, int y, int width, int height,
                        unsigned color, const s_drawmethod *drawmethod)
{
    gpu_image *target;
    const s_drawmethod *method = drawmethod && (drawmethod->config & DRAWMETHOD_CONFIG_ENABLED)
                              ? drawmethod : &plainmethod;
    if(!dest || dest->pixelformat != PIXEL_32)
    {
        goto fallback;
    }
    if(width <= 0 || height <= 0)
    {
        return 1;
    }

    /* Geometry is already expressed in target pixels. Material processing
     * still supplies clipping, tint and blend behavior for every primitive. */
    target = gpu_screen_image(dest, 1);
    quad_count = 0;
    if(target && gpu_collect_rectangle((float)x, (float)y, (float)width, (float)height) &&
       gpu_render_quads(target, NULL, NULL, NULL, method, 0, 1, color))
    {
        return 1;
    }
fallback:
    stats.fallback_draws++;
    return 0;
}

/*
 * Damon V. Caskey
 * 2026-11-12
 * Clip line endpoints before enumerating visible points. Wider intermediates
 * preserve the existing integer clipping without overflowing offscreen input.
 */
static int gpu_clip_line(int *x1, int *y1, int *x2, int *y2, int width, int height)
{
    long long sx = *x1, sy = *y1, ex = *x2, ey = *y2;
    long long dx = ex - sx, dy = ey - sy;
    if((sx < 0 && ex < 0) || (sy < 0 && ey < 0) ||
       (sx >= width && ex >= width) || (sy >= height && ey >= height))
    {
        return 0;
    }

    /* Retain the legacy clipping order and integer division at each edge.
     * Reordering the intersections can shift endpoint coverage by one pixel. */
    if(sx < 0)
    {
        if(!dx)
        {
            return 0;
        }
        sy -= sx * dy / dx;
        sx = 0;
    }
    if(sy < 0)
    {
        if(!dy)
        {
            return 0;
        }
        sx -= sy * dx / dy;
        sy = 0;
    }
    if(sx >= width)
    {
        if(!dx)
        {
            return 0;
        }
        sy -= (sx - width) * dy / dx;
        sx = width - 1;
    }
    if(sy >= height)
    {
        if(!dy)
        {
            return 0;
        }
        sx -= (sy - height) * dx / dy;
        sy = height - 1;
    }
    if(ex < 0)
    {
        if(!dx)
        {
            return 0;
        }
        ey -= ex * dy / dx;
        ex = 0;
    }
    if(ey < 0)
    {
        if(!dy)
        {
            return 0;
        }
        ex -= ey * dx / dy;
        ey = 0;
    }
    if(ex >= width)
    {
        if(!dx)
        {
            return 0;
        }
        ey -= (ex - width) * dy / dx;
        ex = width - 1;
    }
    if(ey >= height)
    {
        if(!dy)
        {
            return 0;
        }
        ex -= (ey - height) * dx / dy;
        ey = height - 1;
    }

    /* Intersections on a shallow or steep line may leave an endpoint outside
     * another edge. Accept only bounded endpoints before narrowing to int. */
    if(sx < 0 || sy < 0 || ex < 0 || ey < 0 ||
       sx >= width || ex >= width || sy >= height || ey >= height)
    {
        return 0;
    }
    *x1 = (int)sx;
    *y1 = (int)sy;
    *x2 = (int)ex;
    *y2 = (int)ey;
    return 1;
}

/*
 * Damon V. Caskey
 * 2026-11-12
 * Retain the engine's integer line coverage while batching visible points
 * into one GPU draw. This avoids implementation-dependent GL line endpoints
 * and keeps all color blending and framebuffer access off the CPU.
 */
static int gpu_line_draw(s_screen *dest, int sx, int sy, int ex, int ey,
                         unsigned color, const s_drawmethod *drawmethod)
{
    gpu_image *target;
    int dx, dy, step_x, step_y, threshold;
    const s_drawmethod *method = drawmethod && (drawmethod->config & DRAWMETHOD_CONFIG_ENABLED)
                              ? drawmethod : &plainmethod;
    if(!dest || dest->pixelformat != PIXEL_32)
    {
        goto fallback;
    }
    if(!gpu_clip_line(&sx, &sy, &ex, &ey, dest->width, dest->height))
    {
        return 1;
    }
    target = gpu_screen_image(dest, 1);
    if(!target)
    {
        goto fallback;
    }
    dx = abs(ex - sx);
    dy = abs(ey - sy);
    step_x = ex < sx ? -1 : 1;
    step_y = ey < sy ? -1 : 1;
    quad_count = 0;

    /* Walk the dominant axis with the engine's integer error threshold.
     * Each covered pixel becomes a unit quad; collect the entire line before
     * drawing so scratch-allocation failures leave the target unchanged. */
    if(dx > dy)
    {
        threshold = dx >> 1;
        while(sx != ex)
        {
            if(!gpu_collect_rectangle((float)sx, (float)sy, 1.0f, 1.0f))
            {
                goto fallback;
            }
            sx += step_x;
            if((threshold -= dy) <= 0)
            {
                sy += step_y;
                threshold += dx;
            }
        }
    }
    else
    {
        threshold = dy >> 1;
        while(sy != ey)
        {
            if(!gpu_collect_rectangle((float)sx, (float)sy, 1.0f, 1.0f))
            {
                goto fallback;
            }
            sy += step_y;
            if((threshold -= dx) <= 0)
            {
                sx += step_x;
                threshold += dy;
            }
        }
    }
    /* Both axis loops stop before the endpoint. Appending it explicitly also
     * covers zero-length lines through the same material and clipping path. */
    if(gpu_collect_rectangle((float)ex, (float)ey, 1.0f, 1.0f) &&
       gpu_render_quads(target, NULL, NULL, NULL, method, 0, 1, color))
    {
        return 1;
    }
fallback:
    stats.fallback_draws++;
    return 0;
}

/*
 * Damon V. Caskey
 * 2026-11-12
 * Clear a native screen without uploading the old image or reading it back.
 * RGB black remains opaque; color-key transparency is a draw-time option.
 */
static int gpu_screen_clear(s_screen *screen)
{
    gpu_image *target;
    if(!screen || screen->pixelformat != PIXEL_32)
    {
        return 0;
    }

    /* Full replacement discards the previous screen version, including any
     * pending CPU edits. Allocation can therefore skip the old-image upload. */
    target = gpu_screen_image(screen, 0);
    if(!target)
    {
        return 0;
    }
    gpu_BindFramebuffer(GL_FRAMEBUFFER, target->framebuffer);
    glDisable(GL_SCISSOR_TEST);
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    if(glGetError() != GL_NO_ERROR)
    {
        gpu_fatal("screen clear");
    }
    target->gpu_dirty = 1;
    target->cpu_dirty = 0;
    return 1;
}

/*
 * Damon V. Caskey
 * 2026-11-12
 * Copy the overlapping screen area with replacement semantics, independent
 * of any previously selected drawing method or clipping globals.
 */
static int gpu_screen_copy(s_screen *dest, s_screen *src)
{
    s_drawmethod method = plainmethod;
    if(!dest || !src || dest->pixelformat != src->pixelformat)
    {
        return 0;
    }
    if(dest == src)
    {
        return 1;
    }
    method.alpha = 0;
    return gpu_screen_draw(dest, src, 0, 0, &method);
}

static const gpu_backend renderer_backend = {
    gpu_screen_draw, gpu_sprite_draw, gpu_box_draw, gpu_line_draw,
    gpu_screen_clear, gpu_screen_copy, gpu_read_screen, gpu_dirty_screen,
    gpu_free_screen, gpu_free_sprite, gpu_dirty_tables
};

/*
 * Damon V. Caskey
 * 2026-11-12
 * Initialize native scene rendering only after all required capabilities and
 * shader resources are usable. Failure leaves the existing renderer intact.
 */
int gpu_gl_init(void)
{
    GLuint vertex = 0, fragment = 0;
    GLint linked = 0, units = 0;
    char message[4096];
    const unsigned char white[4] = {255, 255, 255, 255};
    const char *setting = getenv("OPENBOR_GPU");
    if(initialized)
    {
        return 1;
    }
    if(setting && !strcmp(setting, "0"))
    {
        fprintf(stderr, "GPU scene rendering disabled by OPENBOR_GPU=0.\n");
        return 0;
    }
    if(!SDL_GL_GetCurrentContext() || !gpu_load_functions())
    {
        return 0;
    }
    /* Clear errors left by capability probes before checking our allocations. */
    while(glGetError() != GL_NO_ERROR)
    {
        /* Reading the error flag consumes one pending driver error. */
    }
    glGetIntegerv(GL_MAX_TEXTURE_SIZE, &maximum_texture_size);
    glGetIntegerv(GL_MAX_TEXTURE_IMAGE_UNITS, &units);

    /* The blend atlas needs six 256-row slices. Source, palette, mask,
     * destination snapshot and atlas occupy five independent texture units. */
    if(maximum_texture_size < 1536 || units < 5)
    {
        return 0;
    }
    vertex = gpu_compile_shader(GL_VERTEX_SHADER, gpu_vertex_shader_source);
    fragment = gpu_compile_shader(GL_FRAGMENT_SHADER, gpu_fragment_shader_source);
    if(!vertex || !fragment)
    {
        goto failed;
    }
    program = gpu_CreateProgram();
    if(!program)
    {
        goto failed;
    }
    gpu_AttachShader(program, vertex);
    gpu_AttachShader(program, fragment);
    gpu_LinkProgram(program);
    gpu_GetProgramiv(program, GL_LINK_STATUS, &linked);
    if(!linked)
    {
        gpu_GetProgramInfoLog(program, sizeof(message), NULL, message);
        fprintf(stderr, "GPU renderer program: %s\n", message);
        goto failed;
    }
    white_texture = gpu_new_texture(1, 1, GL_RGBA8, GL_RGBA, GL_UNSIGNED_BYTE, white);
    palette_texture = gpu_new_texture(256, 1, GL_RGBA8, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    table_texture = gpu_new_texture(256, 1536, GL_LUMINANCE8, GL_LUMINANCE, GL_UNSIGNED_BYTE, NULL);
    if(!white_texture || !palette_texture || !table_texture)
    {
        goto failed;
    }

    /* Resolve uniform locations once for this linked program. Every draw
     * updates their values without another string lookup in the driver. */
#define GPU_UNIFORM(member, name) uniforms.member = gpu_GetUniformLocation(program, name)
    GPU_UNIFORM(target_size, "targetSize");
    GPU_UNIFORM(source_size, "sourceSize");
    GPU_UNIFORM(source, "sourceTexture");
    GPU_UNIFORM(palette, "paletteTexture");
    GPU_UNIFORM(mask, "maskTexture");
    GPU_UNIFORM(destination, "destinationTexture");
    GPU_UNIFORM(tables, "blendTableTexture");
    GPU_UNIFORM(indexed, "indexedSource");
    GPU_UNIFORM(masked, "maskEnabled");
    GPU_UNIFORM(color_key, "colorKey");
    GPU_UNIFORM(fill, "fillEnabled");
    GPU_UNIFORM(has_destination, "hasDestination");
    GPU_UNIFORM(wrap, "wrapSource");
    GPU_UNIFORM(blend_mode, "blendMode");
    GPU_UNIFORM(tint_mode, "tintMode");
    GPU_UNIFORM(table_mask, "blendTableMask");
    GPU_UNIFORM(fill_color, "fillColor");
    GPU_UNIFORM(tint_color, "tintColor");
    GPU_UNIFORM(channels, "channelOpacity");
#undef GPU_UNIFORM

    /* Publish the backend only after shader linking and shared allocation
     * succeed. Image resources remain lazy and follow their owners' lifetime. */
    gpu_DeleteShader(vertex);
    gpu_DeleteShader(fragment);
    memset(&stats, 0, sizeof(stats));
    memset(table_pointers, 0, sizeof(table_pointers));
    palette_valid = 0;
    tables_dirty = 1;
    table_mask = 0;
    initialized = 1;
    gpu_register_backend(&renderer_backend);
    gpu_gl_prepare_present();
    fprintf(stderr, "Native GPU scene renderer enabled (RGB32 targets, indexed textures).\n");
    return 1;
failed:
    /* Initialization can fail at any stage. Release only acquired resources;
     * dispatch remains unregistered so callers continue with software draws. */
    if(vertex)
    {
        gpu_DeleteShader(vertex);
    }
    if(fragment)
    {
        gpu_DeleteShader(fragment);
    }
    if(program)
    {
        gpu_DeleteProgram(program);
    }
    if(white_texture)
    {
        glDeleteTextures(1, &white_texture);
    }
    if(palette_texture)
    {
        glDeleteTextures(1, &palette_texture);
    }
    if(table_texture)
    {
        glDeleteTextures(1, &table_texture);
    }
    program = white_texture = palette_texture = table_texture = 0;
    fprintf(stderr, "Native GPU scene renderer unavailable; retaining software rendering.\n");
    return 0;
}

/*
 * Damon V. Caskey
 * 2026-11-12
 * Preserve live screen contents before destroying a graphics context. This
 * infrequent readback keeps pause buffers, scripted targets and mode changes
 * functional without maintaining a CPU copy after every rendered frame.
 */
void gpu_gl_shutdown(void)
{
    size_t bucket;
    if(!initialized)
    {
        return;
    }

    /* Preserve screens while their framebuffers and context still exist.
     * Sprites retain their encoded CPU source and can be uploaded afresh. */
    for(bucket = 0; bucket < GPU_CACHE_BUCKETS; bucket++)
    {
        gpu_image *image = image_cache[bucket];
        while(image)
        {
            gpu_image *next = image->next;
            if(!image->sprite && !gpu_read_screen((s_screen *)image->owner))
            {
                gpu_fatal("context shutdown readback");
            }

            /* Readback locates images through this cache. Advance the bucket
             * head before freeing so the next lookup follows only live nodes. */
            image_cache[bucket] = next;
            gpu_delete_image(image);
            image = next;
        }
        image_cache[bucket] = NULL;
    }

    /* Disconnect scene dispatch before destroying shared state. Leave GL
     * ready for the presenter's own teardown or a replacement video mode. */
    gpu_unregister_backend();
    gpu_gl_prepare_present();
    gpu_DeleteProgram(program);
    glDeleteTextures(1, &white_texture);
    glDeleteTextures(1, &palette_texture);
    glDeleteTextures(1, &table_texture);
    free(quad_buffer);
    quad_buffer = NULL;
    quad_count = quad_capacity = 0;
    program = white_texture = palette_texture = table_texture = 0;
    initialized = 0;
}

/*
 * Damon V. Caskey
 * 2026-11-12
 * Expose a completed screen texture to presentation without downloading its
 * pixels. CPU-authored screens are uploaded only when their version changed.
 */
int gpu_gl_get_screen_texture(s_screen *screen, unsigned *texture, int *width, int *height)
{
    gpu_image *image;
    if(!initialized || !screen || screen->pixelformat != PIXEL_32)
    {
        return 0;
    }
    image = gpu_screen_image(screen, 1);
    if(!image)
    {
        return 0;
    }
    *texture = image->texture;
    *width = image->width;
    *height = image->height;
    return 1;
}

/*
 * Damon V. Caskey
 * 2026-11-12
 * Restore neutral GL state before the existing presenter or YUV renderer
 * draws to the window. Scene drawing restores its own state on its next use.
 */
void gpu_gl_prepare_present(void)
{
    int unit;
    if(!initialized)
    {
        return;
    }

    /* Return drawing to the window and clear scene-specific state before
     * the presenter chooses its scaling shader and window-space viewport. */
    gpu_BindFramebuffer(GL_FRAMEBUFFER, 0);
    gpu_UseProgram(0);
    glDisable(GL_SCISSOR_TEST);
    glDisable(GL_BLEND);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_ALPHA_TEST);
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);

    /* Reset every texture unit used by scene rendering. Descending order
     * leaves unit zero active for the existing single-texture presenter. */
    for(unit = 4; unit >= 0; unit--)
    {
        gpu_ActiveTexture(GL_TEXTURE0 + unit);
        glBindTexture(GL_TEXTURE_2D, 0);
        glDisable(GL_TEXTURE_2D);
    }
    glEnable(GL_TEXTURE_2D);
}

/*
 * Damon V. Caskey
 * 2026-11-12
 * Expose internal counters so tests can distinguish native rendering from
 * a visually identical but expensive CPU-readback/upload implementation.
 */
const gpu_renderer_stats *gpu_gl_get_stats(void)
{
    return &stats;
}

#endif
