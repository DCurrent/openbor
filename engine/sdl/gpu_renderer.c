/*
 * OpenBOR - http://www.chronocrash.com
 * All rights reserved, see LICENSE in OpenBOR root for details.
 * Copyright (c) OpenBOR Team
 */
#if defined(OPENGL) && !defined(ANDROID)

#include <limits.h>
#include <inttypes.h>
#include <math.h>
#include <stddef.h>
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
#include "engine_profile.h"

#define GPU_CACHE_BUCKETS 1024
#define GPU_BLEND_TABLE_SIZE (256 * 256)
#define GPU_PALETTE_CACHE_BUCKETS 2048
#define GPU_PALETTE_NO_SLOT (-1)
#define GPU_PALETTE_HASH_SEED UINT32_C(2166136261)
#define GPU_PALETTE_HASH_MULTIPLIER UINT32_C(16777619)
#define GPU_PALETTE_HASH_MIX_FIRST UINT32_C(0x85ebca6b)
#define GPU_PALETTE_HASH_MIX_SECOND UINT32_C(0xc2b2ae35)
#define GPU_TEXTURE_UNITS 5
#define GPU_TRACKED_TEXTURE_UNITS 6
#define GPU_AFFINE_COMPONENTS 256
#define GPU_COLOR_COMPONENTS 160
#define GPU_COMBINED_COMPONENTS 352
#define GPU_SHEAR_COMPONENTS 32
#define GPU_WATER_COMPONENTS 16
#define GPU_PLANE_COMPONENTS 16
#define GPU_MATERIAL_COMPONENTS 64
#define GPU_FIXED_RASTER_CALLS 6
#define GPU_GEOMETRY_STATE_CALLS 6
#define GPU_STREAM_INITIAL_QUADS 1024
#define GPU_AFFINE_PREPARED_LIMIT 8192
#define GPU_PLANE_CACHE_CAPACITY 16
#define GPU_PLANE_MAX_ROWS 16384
#define GPU_POSITION_ATTRIBUTE 0
#define GPU_TEXCOORD_ATTRIBUTE 1
#define GPU_QUERY_CAPACITY 256
#define GPU_QUERY_TOKEN_BITS 9
#define GPU_QUERY_SLOT_MASK ((UINT64_C(1) << GPU_QUERY_TOKEN_BITS) - 1)
#ifndef GL_TIMESTAMP
#define GL_TIMESTAMP 0x8E28
#endif
#ifndef GL_RGBA32F
#define GL_RGBA32F 0x8814
#endif
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
static PFNGLUNIFORM1FPROC gpu_Uniform1f;
static PFNGLUNIFORM2FPROC gpu_Uniform2f;
static PFNGLUNIFORM3FPROC gpu_Uniform3f;
static PFNGLACTIVETEXTUREPROC gpu_ActiveTexture;
static PFNGLGENBUFFERSPROC gpu_GenBuffers;
static PFNGLBINDBUFFERPROC gpu_BindBuffer;
static PFNGLBUFFERDATAPROC gpu_BufferData;
static PFNGLBUFFERSUBDATAPROC gpu_BufferSubData;
static PFNGLDELETEBUFFERSPROC gpu_DeleteBuffers;
static PFNGLBINDATTRIBLOCATIONPROC gpu_BindAttribLocation;
static PFNGLENABLEVERTEXATTRIBARRAYPROC gpu_EnableVertexAttribArray;
static PFNGLDISABLEVERTEXATTRIBARRAYPROC gpu_DisableVertexAttribArray;
static PFNGLVERTEXATTRIBPOINTERPROC gpu_VertexAttribPointer;
/* Core 1.1 function-pointer typedefs are absent from some desktop headers. */
typedef void (APIENTRYP gpu_draw_elements_fn)(GLenum mode, GLsizei count,
                                            GLenum type, const void *indices);
static gpu_draw_elements_fn gpu_DrawElements;

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

typedef struct gpu_movie_source
{
    const void *owner;
    uint64_t generation;
    int width, height, uploaded;
    GLuint texture[3];
    struct gpu_movie_source *next;
} gpu_movie_source;

typedef struct gpu_movie_uniforms
{
    GLint target_size, source_size, y, u, v;
    GLint resample, black_filter, gamma, brightness;
} gpu_movie_uniforms;

typedef struct gpu_output_filter_uniforms
{
    GLint target_size, source_size, source, mode;
} gpu_output_filter_uniforms;

typedef struct gpu_uniforms
{
    GLint target_size, source_size;
    GLint source, palette, mask, destination, tables;
    GLint palette_row;
    GLint indexed, masked, color_key, fill, has_destination, wrap, resample;
    GLint blend_mode, tint_mode, table_mask;
    GLint fill_color, tint_color, channels;
    GLint chroma_enabled, substitute_enabled, primitive_source;
    GLint chroma_keys[DRAWMETHOD_COLOR_SLOTS];
    GLint substitute_match[DRAWMETHOD_COLOR_SLOTS], substitute_value[DRAWMETHOD_COLOR_SLOTS];
    GLint affine_approximate_x, affine_approximate_y, affine_approximate_d;
    GLint affine_anchor, affine_pivot;
    GLint affine_x[8], affine_y[8], affine_d[8];
    GLint shear_origin, shear_extent, shear_shift, shear_source, shear_step, shear_clip;
    GLint water_origin, water_wave, water_anchor;
    GLint plane_rows, plane_origin, plane_info;
} gpu_uniforms;

typedef struct gpu_uniform_value
{
    int valid;
    GLint integer;
    GLfloat vector[3];
} gpu_uniform_value;

typedef struct gpu_material_state
{
    gpu_uniform_value target_size, source_size;
    gpu_uniform_value source, palette, mask, destination, tables;
    gpu_uniform_value palette_row;
    gpu_uniform_value indexed, masked, color_key, fill, has_destination, wrap, resample;
    gpu_uniform_value blend_mode, tint_mode, table_mask;
    gpu_uniform_value fill_color, tint_color, channels;
    gpu_uniform_value chroma_enabled, substitute_enabled, primitive_source;
    gpu_uniform_value chroma_keys[DRAWMETHOD_COLOR_SLOTS];
    gpu_uniform_value substitute_match[DRAWMETHOD_COLOR_SLOTS], substitute_value[DRAWMETHOD_COLOR_SLOTS];
    gpu_uniform_value affine_approximate_x, affine_approximate_y, affine_approximate_d;
    gpu_uniform_value affine_anchor, affine_pivot;
    gpu_uniform_value affine_x[8], affine_y[8], affine_d[8];
    gpu_uniform_value shear_origin, shear_extent, shear_shift, shear_source, shear_step, shear_clip;
    gpu_uniform_value water_origin, water_wave, water_anchor;
    gpu_uniform_value plane_rows, plane_origin, plane_info;
} gpu_material_state;

typedef struct gpu_state_cache
{
    /* Validity belongs to the current context and renderer ownership period,
     * never to numeric GL names alone. Unknown state is restored on demand. */
    int active_unit_valid, active_unit;
    unsigned texture_valid;
    GLuint textures[GPU_TRACKED_TEXTURE_UNITS];
    int framebuffer_valid, program_valid;
    GLuint framebuffer, program;
    int viewport_valid, viewport_width, viewport_height;
    int scissor_enabled_valid, scissor_enabled, scissor_rectangle_valid;
    int scissor_rectangle[4];
    int raster_valid, color_mask_valid, clear_color_valid;
    int geometry_valid;
    gpu_material_state material;
} gpu_state_cache;

typedef struct gpu_palette_entry
{
    /* Own the exact values instead of retaining a caller's palette pointer.
     * Scripts may mutate or release that allocation before the next draw. */
    uint32_t colors[GPU_PALETTE_COLORS];
    uint32_t hash;
    int hash_next;
    int lru_previous, lru_next;
} gpu_palette_entry;

/* Retain validated geometry and shader state for each affine repeat. One
 * entry costs a few hundred bytes, so bound residency while preserving the
 * original two-pass path for unusually large repeat grids or low memory. */
typedef struct gpu_prepared_affine
{
    gpu_affine_sampling sampling;
    gpu_vertex vertices[4];
    int result;
} gpu_prepared_affine;

/* Plane recurrences depend on source dimensions and scale settings, not the
 * current pixels, draw position or scrolling phase. Own that compact data and
 * its immutable GPU texture so moving or capturing an image does not rebuild it. */
typedef struct gpu_plane_map
{
    int occupied, source_width, source_height, perspective, valid_rows;
    float begin, end;
    size_t capacity;
    gpu_plane_row *rows;
    GLuint texture;
    uint64_t last_used;
} gpu_plane_map;

static gpu_image *image_cache[GPU_CACHE_BUCKETS];
static gpu_vertex *vertex_buffer;
static int *quad_wraps;
static size_t quad_count, quad_capacity;
static gpu_prepared_affine *affine_preparations;
static size_t affine_preparation_capacity;
static gpu_plane_map plane_maps[GPU_PLANE_CACHE_CAPACITY];
static uint64_t plane_map_clock;
static int plane_cache_warning, plane_upload_warning;
static GLuint vertex_buffer_object, index_buffer_object;
static size_t stream_capacity, index_capacity, stream_cursor;
static float bounds_left, bounds_top, bounds_right, bounds_bottom;
static GLuint program, white_texture, palette_texture, table_texture;
static gpu_uniforms uniforms;
static gpu_state_cache gl_state;
static gpu_renderer_stats stats;
static GLint maximum_texture_size;
static int initialized, tables_dirty = 1, table_mask, affine_supported, color_rules_supported, shear_supported;
static int water_supported, plane_supported;
static gpu_palette_entry palette_cache[GPU_PALETTE_CACHE_CAPACITY];
static int palette_buckets[GPU_PALETTE_CACHE_BUCKETS];
static int palette_cache_count;
static int palette_lru_head = GPU_PALETTE_NO_SLOT;
static int palette_lru_tail = GPU_PALETTE_NO_SLOT;
static int palette_selected_slot = GPU_PALETTE_NO_SLOT;
static const unsigned char *table_pointers[MAX_BLENDINGS];
static ep_fallback native_failure = EP_FALLBACK_OTHER;
static gpu_movie_source *movie_sources;
static GLuint movie_program;
static gpu_movie_uniforms movie_uniforms;
static int movie_program_attempted, movie_present_warning;
/* Presentation filters never acquire a CPU-backed screen identity. Their
 * single reusable intermediate remains private to this graphics context. */
static GLuint output_filter_program, output_filter_texture, output_filter_framebuffer;
static gpu_output_filter_uniforms output_filter_uniforms;
static int output_filter_width, output_filter_height;
static int output_filter_program_attempted, output_filter_warning;

/* Private declarations keep optional timer entry points independent from the
 * GL 2.1 presentation loader and from newer typedefs in platform GL headers. */
typedef void (APIENTRYP gpu_query_generate_fn)(GLsizei count, GLuint *ids);
typedef void (APIENTRYP gpu_query_delete_fn)(GLsizei count, const GLuint *ids);
typedef void (APIENTRYP gpu_query_counter_fn)(GLuint id, GLenum target);
typedef void (APIENTRYP gpu_query_info_fn)(GLenum target, GLenum name, GLint *value);
typedef void (APIENTRYP gpu_query_available_fn)(GLuint id, GLenum name, GLint *value);
typedef void (APIENTRYP gpu_query_result_fn)(GLuint id, GLenum name, uint64_t *value);

typedef enum gpu_query_state
{
    GPU_QUERY_FREE,
    GPU_QUERY_OPEN,
    GPU_QUERY_PENDING
} gpu_query_state;

typedef struct gpu_query_pair
{
    GLuint begin;
    GLuint end;
    uint64_t token;
    uint64_t frame_id;
    ep_gpu_kind kind;
    gpu_query_state state;
} gpu_query_pair;

static gpu_query_generate_fn query_generate;
static gpu_query_delete_fn query_delete;
static gpu_query_counter_fn query_counter;
static gpu_query_info_fn query_info;
static gpu_query_available_fn query_available;
static gpu_query_result_fn query_result;
static gpu_query_pair query_pairs[GPU_QUERY_CAPACITY];
static unsigned query_free[GPU_QUERY_CAPACITY];
static unsigned query_pending[GPU_QUERY_CAPACITY];
static unsigned query_free_count, query_pending_head, query_pending_count;
static int query_objects_created;
static uint64_t query_generation;
static gpu_query_stats query_stats;

/*
 * Damon V. Caskey
 * 2026-11-12
 * 2026-11-13 - Keep disabled timing diagnostics readable in the engine log.
 * Stop optional timestamp sampling after unusable results or a setup error,
 * leaving native drawing and CPU profiling available for the current context.
 */
static void gpu_query_disable(const char *reason, GLenum observed_error)
{
    /* Detach first so any later generic scope begins become explicit skipped
     * samples. Keep existing query objects until teardown, without waiting for
     * their results or changing the renderer's framebuffer/material state. */
    engine_profile_set_gpu_backend(NULL);
    query_stats.enabled = 0;
    query_stats.errors++;
    engine_profile_log("GPU Timing\n"
                       "\tStatus:                            Disabled\n"
                       "\tReason:                            %s\n"
                       "\tObserved OpenGL error:             %u\n"
                       "\tPending queries:                   %u",
                       reason, (unsigned)observed_error, query_stats.pending);
}

/*
 * Damon V. Caskey
 * 2026-11-12
 *
 * Reserve a timestamp pair for one recorded engine scope. Saturation drops
 * measurement work instead of waiting for the GPU or recycling live results.
 */
static uint64_t gpu_query_begin(uint64_t frame_id, ep_gpu_kind kind)
{
    gpu_query_pair *pair;
    unsigned index;
    if(!query_stats.enabled || !frame_id || (unsigned)kind >= EP_GPU_COUNT)
    {
        return 0;
    }

    /* Tokens encode a slot and a generation that survives context recreation.
     * Old scope ends therefore cannot accidentally close a newly allocated
     * scope at the same slot. Exhaustion is handled like a full query ring. */
    if(!query_free_count || query_generation == (UINT64_MAX >> GPU_QUERY_TOKEN_BITS))
    {
        query_stats.dropped++;
        engine_profile_count(EP_COUNT_GPU_QUERY_DROPPED, 1);
        return 0;
    }
    index = query_free[--query_free_count];
    pair = &query_pairs[index];
    pair->token = (++query_generation << GPU_QUERY_TOKEN_BITS) | (index + 1);
    pair->frame_id = frame_id;
    pair->kind = kind;
    pair->state = GPU_QUERY_OPEN;
    query_stats.pending++;

    /* Timestamp commands do not open a GL query target, so scene, queue and
     * presentation scopes can nest. Leave the shared GL error state untouched:
     * a diagnostic scope must not consume an earlier renderer error. */
    query_counter(pair->begin, GL_TIMESTAMP);
    query_stats.issued++;
    engine_profile_count(EP_COUNT_GPU_QUERY_PAIRS, 1);
    return pair->token;
}

/*
 * Damon V. Caskey
 * 2026-11-12
 *
 * End a live timestamp scope and queue its pair for later nonblocking polling.
 */
static void gpu_query_end(uint64_t handle)
{
    unsigned slot = (unsigned)(handle & GPU_QUERY_SLOT_MASK);
    gpu_query_pair *pair;
    if(!query_stats.enabled || !slot || slot > GPU_QUERY_CAPACITY)
    {
        return;
    }
    pair = &query_pairs[slot - 1];
    if(pair->token != handle || pair->state != GPU_QUERY_OPEN)
    {
        return;
    }

    /* End-order FIFO matches the order of timestamp commands in the GL
     * stream. If its oldest end is unavailable, later ends can be left for
     * the next poll without scanning every pending pair. */
    query_counter(pair->end, GL_TIMESTAMP);
    pair->state = GPU_QUERY_PENDING;
    query_pending[(query_pending_head + query_pending_count) % GPU_QUERY_CAPACITY] = slot - 1;
    query_pending_count++;
}

/*
 * Damon V. Caskey
 * 2026-11-12
 *
 * Consume only timestamp pairs whose two endpoints are already available.
 * Results keep their originating frame ID even when collection is delayed.
 */
static void gpu_query_poll(void)
{
    if(!query_stats.enabled)
    {
        return;
    }

    /* Readiness checks precede every 64-bit result request. Unavailable work
     * remains in the ring; there is no flush, completion wait, or reuse of an
     * object whose prior measurement is still in flight. */
    while(query_pending_count)
    {
        unsigned index = query_pending[query_pending_head];
        gpu_query_pair *pair = &query_pairs[index];
        GLint begin_ready = 0, end_ready = 0;
        uint64_t begin_ns = UINT64_MAX, end_ns = UINT64_MAX;
        query_available(pair->begin, GL_QUERY_RESULT_AVAILABLE, &begin_ready);
        query_available(pair->end, GL_QUERY_RESULT_AVAILABLE, &end_ready);
        if(!begin_ready || !end_ready)
        {
            query_stats.deferred++;
            return;
        }
        query_result(pair->begin, GL_QUERY_RESULT, &begin_ns);
        query_result(pair->end, GL_QUERY_RESULT, &end_ns);

        /* Availability failures leave their initialized flags false. Result
         * failures must not turn an unwritten output into a believable elapsed
         * time either. Sentinels and ordering protect the measurement without
         * consuming error flags owned by the surrounding renderer. */
        if(begin_ns == UINT64_MAX || end_ns == UINT64_MAX || end_ns < begin_ns)
        {
            gpu_query_disable("Invalid timestamp result", GL_NO_ERROR);
            return;
        }

        /* These values measure elapsed GPU command spans, including possible
         * idle time between CPU submissions. The core decides whether the
         * original frame is still reportable or must count as a late result. */
        engine_profile_gpu_result(pair->frame_id, pair->kind, end_ns - begin_ns);
        pair->state = GPU_QUERY_FREE;
        pair->token = 0;
        query_free[query_free_count++] = index;
        query_pending_head = (query_pending_head + 1) % GPU_QUERY_CAPACITY;
        query_pending_count--;
        query_stats.pending--;
        query_stats.completed++;
    }
}

static const ep_gpu_backend query_backend =
{
    gpu_query_begin, gpu_query_end, gpu_query_poll
};

/*
 * Damon V. Caskey
 * 2026-11-12
 * 2026-11-13 - Present optional timing capabilities as labeled fields.
 * Enable optional timestamp pairs only when the current desktop context
 * supplies a complete 64-bit timer-query implementation.
 */
static void gpu_query_init(void)
{
    const char *setting = getenv("OPENBOR_PROFILE_GPU");
    const char *version;
    GLuint ids[GPU_QUERY_CAPACITY * 2];
    GLint counter_bits = 0;
    int major = 0, minor = 0;
    unsigned index;
    GLenum error;

    /* CPU profiling and native drawing remain independent of timer support.
     * GL 2.1 contexts without ARB_timer_query simply report an unsupported
     * optional measurement path, retaining all ordinary rendering behavior. */
    memset(&query_stats, 0, sizeof(query_stats));
    memset(query_pairs, 0, sizeof(query_pairs));
    query_free_count = query_pending_head = query_pending_count = 0;
    query_objects_created = 0;
    engine_profile_set_gpu_backend(NULL);
    if(!engine_profile_enabled() || (setting && !strcmp(setting, "0")))
    {
        engine_profile_log("GPU Timing\n"
                           "\tStatus:                            Disabled\n"
                           "\tReason:                            %s",
                           engine_profile_enabled() ? "OPENBOR_PROFILE_GPU=0" : "OPENBOR_PROFILE=0");
        return;
    }
    version = (const char *)glGetString(GL_VERSION);
    if(version)
    {
        sscanf(version, "%d.%d", &major, &minor);
    }
    if(!(major > 3 || (major == 3 && minor >= 3)) &&
       !SDL_GL_ExtensionSupported("GL_ARB_timer_query"))
    {
        engine_profile_log("GPU Timing\n"
                           "\tStatus:                            Unavailable\n"
                           "\tReason:                            OpenGL 3.3 or GL_ARB_timer_query is required");
        return;
    }

    /* Resolve optional symbols separately from mandatory drawing functions.
     * Missing timer entry points must never disable the native renderer. */
    query_generate = (gpu_query_generate_fn)SDL_GL_GetProcAddress("glGenQueries");
    query_delete = (gpu_query_delete_fn)SDL_GL_GetProcAddress("glDeleteQueries");
    query_counter = (gpu_query_counter_fn)SDL_GL_GetProcAddress("glQueryCounter");
    query_info = (gpu_query_info_fn)SDL_GL_GetProcAddress("glGetQueryiv");
    query_available = (gpu_query_available_fn)SDL_GL_GetProcAddress("glGetQueryObjectiv");
    query_result = (gpu_query_result_fn)SDL_GL_GetProcAddress("glGetQueryObjectui64v");
    if(!query_generate || !query_delete || !query_counter || !query_info ||
       !query_available || !query_result)
    {
        engine_profile_log("GPU Timing\n"
                           "\tStatus:                            Unavailable\n"
                           "\tReason:                            Required timer functions are missing");
        return;
    }
    query_info(GL_TIMESTAMP, GL_QUERY_COUNTER_BITS, &counter_bits);
    error = glGetError();
    if(error != GL_NO_ERROR || counter_bits != 64)
    {
        engine_profile_log("GPU Timing\n"
                           "\tStatus:                            Unavailable\n"
                           "\tReason:                            Usable 64-bit timestamp counter required\n"
                           "\tReported counter width:            %d bits\n"
                           "\tOpenGL error:                      %u",
                           counter_bits, (unsigned)error);
        return;
    }
    query_stats.supported = 1;

    /* Allocate a bounded pool once per context. Query storage can stay busy
     * across several frames; exhaustion drops samples rather than delaying
     * drawing. Generations intentionally survive this initialization. */
    memset(ids, 0, sizeof(ids));
    query_generate(GPU_QUERY_CAPACITY * 2, ids);
    error = glGetError();
    if(error != GL_NO_ERROR)
    {
        query_delete(GPU_QUERY_CAPACITY * 2, ids);
        gpu_query_disable("GPU timer allocation failed", error);
        return;
    }
    for(index = 0; index < GPU_QUERY_CAPACITY; index++)
    {
        query_pairs[index].begin = ids[index * 2];
        query_pairs[index].end = ids[index * 2 + 1];
        query_free[index] = GPU_QUERY_CAPACITY - index - 1;
    }
    query_free_count = GPU_QUERY_CAPACITY;
    query_objects_created = 1;
    query_stats.enabled = 1;
    engine_profile_set_gpu_backend(&query_backend);
    engine_profile_log("GPU Timing\n"
                       "\tStatus:                            Enabled\n"
                       "\tSupported:                         Yes\n"
                       "\tTimestamp counter:                 64 bits\n"
                       "\tQuery capacity:                    %d pairs\n"
                       "\tMeasurement:                       Elapsed GPU command span\n"
                       "\tGPU idle time:                     May be included in elapsed time\n"
                       "\tResult collection:                 Nonblocking",
                       GPU_QUERY_CAPACITY);
}

/*
 * Damon V. Caskey
 * 2026-11-12
 * 2026-11-13 - Put each lifecycle counter on its own labeled line.
 * Release optional query objects while their context still exists, abandoning
 * unavailable samples rather than forcing completion during a mode change.
 */
static void gpu_query_shutdown(void)
{
    GLuint ids[GPU_QUERY_CAPACITY * 2];
    unsigned index;

    /* Accept any results already ready, then detach callbacks before deleting
     * names. Pending/open handles are invalidated; a future context receives
     * fresh tokens even if the driver happens to recycle the same GL names. */
    gpu_query_poll();
    engine_profile_set_gpu_backend(NULL);
    query_stats.enabled = 0;
    query_stats.abandoned += query_stats.pending;
    if(query_objects_created)
    {
        for(index = 0; index < GPU_QUERY_CAPACITY; index++)
        {
            ids[index * 2] = query_pairs[index].begin;
            ids[index * 2 + 1] = query_pairs[index].end;
        }
        query_delete(GPU_QUERY_CAPACITY * 2, ids);
    }
    if(query_stats.supported)
    {
        engine_profile_log("GPU Timing - Shutdown\n"
                           "\tQueries issued:                    %" PRIu64 "\n"
                           "\tQueries completed:                 %" PRIu64 "\n"
                           "\tQueries dropped:                   %" PRIu64 "\n"
                           "\tDeferred polls:                    %" PRIu64 "\n"
                           "\tErrors:                            %" PRIu64 "\n"
                           "\tQueries abandoned:                 %" PRIu64,
                           query_stats.issued, query_stats.completed, query_stats.dropped,
                           query_stats.deferred, query_stats.errors, query_stats.abandoned);
    }
    memset(query_pairs, 0, sizeof(query_pairs));
    query_free_count = query_pending_head = query_pending_count = 0;
    query_objects_created = 0;
    query_stats.pending = 0;
}

/*
 * Damon V. Caskey
 * 2026-11-12
 * Report an unrecoverable error after a GPU operation has begun. Retrying
 * the operation in software after partial drawing would blend it twice.
 */
static void gpu_fatal(const char *operation)
{
    engine_profile_log("kind=renderer state=fatal operation=\"%s\"", operation);
    fprintf(stderr, "GPU renderer: %s failed; refusing to use stale image data.\n", operation);
    abort();
}

/*
 * Damon V. Caskey
 * 2026-11-12
 * 2026-11-13 - Report every missing required entry point in the startup log.
 * 2026-11-14 - Require buffered geometry and explicit vertex attributes.
 * Obtain shader/FBO entry points from SDL's current context. EXT FBO names
 * retain support for the engine's existing desktop compatibility context.
 */
static int gpu_load_functions(void)
{
    int available = 1;
    /* Resolve the complete dispatch set before registering the backend.
     * Missing entry points leave native RGB32 drawing unavailable. */
#define GPU_LOAD(name, type) \
    do \
    { \
        gpu_##name = (type)SDL_GL_GetProcAddress("gl" #name); \
        if(!gpu_##name) \
        { \
            gpu_log("WARNING: Required OpenGL entry point gl" #name " is missing."); \
            available = 0; \
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
            gpu_log("WARNING: Required OpenGL entry point gl" #name " is missing (including its EXT alternative)."); \
            available = 0; \
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
    GPU_LOAD(Uniform1f, PFNGLUNIFORM1FPROC);
    GPU_LOAD(Uniform2f, PFNGLUNIFORM2FPROC);
    GPU_LOAD(Uniform3f, PFNGLUNIFORM3FPROC);
    GPU_LOAD(ActiveTexture, PFNGLACTIVETEXTUREPROC);
    GPU_LOAD(GenBuffers, PFNGLGENBUFFERSPROC);
    GPU_LOAD(BindBuffer, PFNGLBINDBUFFERPROC);
    GPU_LOAD(BufferData, PFNGLBUFFERDATAPROC);
    GPU_LOAD(BufferSubData, PFNGLBUFFERSUBDATAPROC);
    GPU_LOAD(DeleteBuffers, PFNGLDELETEBUFFERSPROC);
    GPU_LOAD(BindAttribLocation, PFNGLBINDATTRIBLOCATIONPROC);
    GPU_LOAD(EnableVertexAttribArray, PFNGLENABLEVERTEXATTRIBARRAYPROC);
    GPU_LOAD(DisableVertexAttribArray, PFNGLDISABLEVERTEXATTRIBARRAYPROC);
    GPU_LOAD(VertexAttribPointer, PFNGLVERTEXATTRIBPOINTERPROC);
    GPU_LOAD(DrawElements, gpu_draw_elements_fn);
#undef GPU_LOAD_FBO
#undef GPU_LOAD
    return available;
}

/*
 * Damon V. Caskey
 * 2026-11-12
 * 2026-11-13 - Select optional affine arithmetic without raising legacy limits.
 * 2026-11-14 - Select color rules independently from optional affine support.
 * 2026-11-14 - Select exact legacy shear independently from other features.
 * 2026-11-15 - Select water sampling and compact plane maps independently.
 * Compile one scene shader, preserving the driver's diagnostic on failure.
 */
static GLuint gpu_compile_shader(GLenum type, const char *source)
{
    GLuint shader = gpu_CreateShader(type);
    GLint success = 0;
    char message[4096], defines[192];
    const char *parts[2];
    if(!shader)
    {
        gpu_log("WARNING: %s %s shader allocation failed.", affine_supported ? "affine" : "material",
                type == GL_VERTEX_SHADER ? "vertex" : "fragment");
        return 0;
    }

    /* Both internal sources start with the same version directive. Insert
     * capability defines after it, retaining GLSL's required first line. */
    snprintf(defines, sizeof(defines), "#version 120\n#define GPU_AFFINE %d\n#define GPU_COLOR_RULES %d\n#define GPU_SHEAR %d\n#define GPU_WATER %d\n#define GPU_PLANE %d\n",
             affine_supported, color_rules_supported, shear_supported,
             water_supported, plane_supported);
    parts[0] = defines;
    parts[1] = strchr(source, '\n') + 1;
    gpu_ShaderSource(shader, 2, parts, NULL);
    gpu_CompileShader(shader);
    gpu_GetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if(!success)
    {
        /* Keep the driver's explanation before releasing the failed object,
         * so startup can report why this native shader variant is unusable. */
        message[0] = '\0';
        gpu_GetShaderInfoLog(shader, sizeof(message), NULL, message);
        message[sizeof(message) - 1] = '\0';
        gpu_log("WARNING: %s %s shader compilation failed.",
                affine_supported ? "affine" : "material",
                type == GL_VERTEX_SHADER ? "vertex" : "fragment");
        gpu_log("\tDriver diagnostic:\n%s", message[0] ? message : "not provided");
        fprintf(stderr, "GPU renderer shader: %s\n", message);
        gpu_DeleteShader(shader);
        return 0;
    }
    return shader;
}

/*
 * Damon V. Caskey
 * 2026-11-12
 * Forget context-local GL assumptions before another rendering owner takes
 * control, or after resource destruction/recovery makes those values uncertain.
 */
void gpu_gl_invalidate_state(void)
{
    /* No GL queries or synchronization are needed. The next native operation
     * restores each required value, including the private program's uniforms. */
    memset(&gl_state, 0, sizeof(gl_state));
}

/*
 * Damon V. Caskey
 * 2026-11-14
 * Bind the compact vertex stream and its fixed attribute layout once per
 * ownership period. Explicit shader locations keep the layout suitable for
 * desktop GL and a future GLES backend.
 */
static void gpu_bind_geometry(void)
{
    if(gl_state.geometry_valid)
    {
        stats.state_reuses += GPU_GEOMETRY_STATE_CALLS;
        return;
    }
    gpu_BindBuffer(GL_ARRAY_BUFFER, vertex_buffer_object);
    gpu_BindBuffer(GL_ELEMENT_ARRAY_BUFFER, index_buffer_object);
    gpu_EnableVertexAttribArray(GPU_POSITION_ATTRIBUTE);
    gpu_EnableVertexAttribArray(GPU_TEXCOORD_ATTRIBUTE);
    gpu_VertexAttribPointer(GPU_POSITION_ATTRIBUTE, 2, GL_FLOAT, GL_FALSE,
                            sizeof(gpu_vertex), (const void *)offsetof(gpu_vertex, x));
    gpu_VertexAttribPointer(GPU_TEXCOORD_ATTRIBUTE, 2, GL_FLOAT, GL_FALSE,
                            sizeof(gpu_vertex), (const void *)offsetof(gpu_vertex, u));
    gl_state.geometry_valid = 1;
    stats.state_changes += GPU_GEOMETRY_STATE_CALLS;
}

/*
 * Damon V. Caskey
 * 2026-11-14
 * Reject an incomplete buffer update before drawing. Discard capacity claims
 * so a later attempt rebuilds both stores instead of trusting failed storage.
 */
static int gpu_geometry_failure(void)
{
    stream_capacity = index_capacity = stream_cursor = 0;
    native_failure = EP_FALLBACK_RESOURCE;
    gpu_gl_invalidate_state();
    while(glGetError() != GL_NO_ERROR)
    {
        /* Consume allocation/update errors before returning the rejection. */
    }
    return 0;
}

/*
 * Damon V. Caskey
 * 2026-11-14
 * Allocate reusable vertex storage and ordered quad-to-triangle indices.
 * Grow only for a draw larger than the retained stream; ordinary draws append.
 */
static int gpu_reserve_geometry(size_t required)
{
    size_t capacity = stream_capacity ? stream_capacity : GPU_STREAM_INITIAL_QUADS;
    size_t quad;
    GLuint *indices;
    const size_t maximum = (size_t)INT_MAX / 6;

    /* Bound draw counts, vertex indices, and signed GL byte offsets before
     * multiplying. Four vertices per quad fit GLuint under this count limit. */
    if(required > maximum)
    {
        return gpu_geometry_failure();
    }
    while(capacity < required)
    {
        capacity = capacity > maximum / 2 ? required : capacity * 2;
    }
    if(capacity > (size_t)PTRDIFF_MAX / (4 * sizeof(gpu_vertex)) ||
       capacity > (size_t)PTRDIFF_MAX / (6 * sizeof(GLuint)))
    {
        return gpu_geometry_failure();
    }
    if(!vertex_buffer_object || !index_buffer_object)
    {
        if(!vertex_buffer_object) gpu_GenBuffers(1, &vertex_buffer_object);
        if(!index_buffer_object) gpu_GenBuffers(1, &index_buffer_object);
        if(!vertex_buffer_object || !index_buffer_object || glGetError() != GL_NO_ERROR)
        {
            return gpu_geometry_failure();
        }
    }
    gpu_bind_geometry();

    /* New storage lets earlier queued draws retain their previous contents.
     * No explicit GPU wait or overwrite of an in-use vertex range is needed. */
    if(stream_capacity < capacity)
    {
        gpu_BufferData(GL_ARRAY_BUFFER, (GLsizeiptr)(capacity * 4 * sizeof(gpu_vertex)),
                       NULL, GL_STREAM_DRAW);
        if(glGetError() != GL_NO_ERROR) return gpu_geometry_failure();
        stream_capacity = capacity;
        stream_cursor = 0;
    }
    if(index_capacity < capacity)
    {
        indices = (GLuint *)malloc(capacity * 6 * sizeof(*indices));
        if(!indices) return gpu_geometry_failure();
        /* Shared diagonals belong to exactly one triangle. Preserve each
         * quad's perimeter order and the original order between quads. */
        for(quad = 0; quad < capacity; ++quad)
        {
            GLuint vertex = (GLuint)(quad * 4);
            indices[quad * 6] = vertex;
            indices[quad * 6 + 1] = vertex + 1;
            indices[quad * 6 + 2] = vertex + 2;
            indices[quad * 6 + 3] = vertex;
            indices[quad * 6 + 4] = vertex + 2;
            indices[quad * 6 + 5] = vertex + 3;
        }
        gpu_BufferData(GL_ELEMENT_ARRAY_BUFFER, (GLsizeiptr)(capacity * 6 * sizeof(*indices)),
                       indices, GL_STATIC_DRAW);
        free(indices);
        if(glGetError() != GL_NO_ERROR) return gpu_geometry_failure();
        index_capacity = capacity;
    }
    return 1;
}

/*
 * Damon V. Caskey
 * 2026-11-14
 * Append one draw's vertices before any destination write. Reaching the end
 * orphans the stream, retaining queued draws without a per-draw allocation.
 */
static int gpu_upload_geometry(size_t *first_quad)
{
    if(!gpu_reserve_geometry(quad_count)) return 0;
    if(quad_count > stream_capacity - stream_cursor)
    {
        gpu_BufferData(GL_ARRAY_BUFFER, (GLsizeiptr)(stream_capacity * 4 * sizeof(gpu_vertex)),
                       NULL, GL_STREAM_DRAW);
        if(glGetError() != GL_NO_ERROR) return gpu_geometry_failure();
        stream_cursor = 0;
    }
    gpu_BufferSubData(GL_ARRAY_BUFFER, (GLintptr)(stream_cursor * 4 * sizeof(gpu_vertex)),
                      (GLsizeiptr)(quad_count * 4 * sizeof(gpu_vertex)), vertex_buffer);
    if(glGetError() != GL_NO_ERROR) return gpu_geometry_failure();
    *first_quad = stream_cursor;
    stream_cursor += quad_count;
    return 1;
}

/*
 * Damon V. Caskey
 * 2026-11-14
 * Release partially initialized or live geometry buffers while their context
 * exists, clearing array state before names can be reused by another owner.
 */
static void gpu_release_geometry(void)
{
    if(vertex_buffer_object || index_buffer_object)
    {
        gpu_DisableVertexAttribArray(GPU_POSITION_ATTRIBUTE);
        gpu_DisableVertexAttribArray(GPU_TEXCOORD_ATTRIBUTE);
        gpu_BindBuffer(GL_ARRAY_BUFFER, 0);
        gpu_BindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
        if(vertex_buffer_object) gpu_DeleteBuffers(1, &vertex_buffer_object);
        if(index_buffer_object) gpu_DeleteBuffers(1, &index_buffer_object);
    }
    vertex_buffer_object = index_buffer_object = 0;
    stream_capacity = index_capacity = stream_cursor = 0;
    gl_state.geometry_valid = 0;
}

/*
 * Damon V. Caskey
 * 2026-11-12
 * Select a texture unit only when it differs from the known active unit.
 */
static void gpu_select_texture_unit(int unit)
{
    if(gl_state.active_unit_valid && gl_state.active_unit == unit)
    {
        stats.state_reuses++;
        return;
    }
    gpu_ActiveTexture(GL_TEXTURE0 + unit);
    gl_state.active_unit = unit;
    gl_state.active_unit_valid = 1;
    stats.state_changes++;
}

/*
 * Damon V. Caskey
 * 2026-11-12
 * Establish a sampler binding without disturbing the active unit on a hit.
 * Texture edits use gpu_edit_texture() because they also require an active unit.
 */
static void gpu_bind_texture(int unit, GLuint texture)
{
    unsigned bit = 1u << unit;
    if((gl_state.texture_valid & bit) && gl_state.textures[unit] == texture)
    {
        stats.state_reuses++;
        return;
    }

    /* Sampling does not depend on the current active unit. Only a changed
     * binding needs to select that unit before issuing its GL call. */
    gpu_select_texture_unit(unit);
    glBindTexture(GL_TEXTURE_2D, texture);
    gl_state.textures[unit] = texture;
    gl_state.texture_valid |= bit;
    stats.state_changes++;
}

/*
 * Damon V. Caskey
 * 2026-11-12
 * Select both the active unit and texture for allocation, upload, or copying.
 */
static void gpu_edit_texture(int unit, GLuint texture)
{
    /* A cached binding does not imply that its unit remains active. Uploads
     * and texture-copy commands always operate on the currently active unit. */
    gpu_select_texture_unit(unit);
    gpu_bind_texture(unit, texture);
}

/*
 * Damon V. Caskey
 * 2026-11-12
 * Share framebuffer tracking across drawing, allocation, copies and readbacks.
 */
static void gpu_bind_framebuffer(GLuint framebuffer)
{
    if(gl_state.framebuffer_valid && gl_state.framebuffer == framebuffer)
    {
        stats.state_reuses++;
        return;
    }
    gpu_BindFramebuffer(GL_FRAMEBUFFER, framebuffer);
    gl_state.framebuffer = framebuffer;
    gl_state.framebuffer_valid = 1;
    stats.state_changes++;
}

/*
 * Damon V. Caskey
 * 2026-11-12
 * Restore the private scene program only when its current binding is unknown
 * or different. External program changes invalidate this shared assumption.
 */
static void gpu_use_program(void)
{
    if(gl_state.program_valid && gl_state.program == program)
    {
        stats.state_reuses++;
        return;
    }
    gpu_UseProgram(program);
    gl_state.program = program;
    gl_state.program_valid = 1;
    stats.state_changes++;
}

/*
 * Damon V. Caskey
 * 2026-11-12
 * Set the native target's zero-origin viewport only when its dimensions change.
 */
static void gpu_set_viewport(int width, int height)
{
    if(gl_state.viewport_valid && gl_state.viewport_width == width &&
       gl_state.viewport_height == height)
    {
        stats.state_reuses++;
        return;
    }
    glViewport(0, 0, width, height);
    gl_state.viewport_width = width;
    gl_state.viewport_height = height;
    gl_state.viewport_valid = 1;
    stats.state_changes++;
}

/*
 * Damon V. Caskey
 * 2026-11-12
 * Track scissor enable separately from its rectangle so clears and draws can
 * alternate without invalidating unrelated material or raster state.
 */
static void gpu_set_scissor_enabled(int enabled)
{
    if(gl_state.scissor_enabled_valid && gl_state.scissor_enabled == enabled)
    {
        stats.state_reuses++;
        return;
    }
    if(enabled)
    {
        glEnable(GL_SCISSOR_TEST);
    }
    else
    {
        glDisable(GL_SCISSOR_TEST);
    }
    gl_state.scissor_enabled = enabled;
    gl_state.scissor_enabled_valid = 1;
    stats.state_changes++;
}

/*
 * Damon V. Caskey
 * 2026-11-12
 * Preserve exact per-draw clipping while avoiding an identical scissor call.
 */
static void gpu_set_scissor_rectangle(const int rectangle[4])
{
    if(gl_state.scissor_rectangle_valid &&
       memcmp(gl_state.scissor_rectangle, rectangle, sizeof(gl_state.scissor_rectangle)) == 0)
    {
        stats.state_reuses++;
        return;
    }
    glScissor(rectangle[0], rectangle[1], rectangle[2], rectangle[3]);
    memcpy(gl_state.scissor_rectangle, rectangle, sizeof(gl_state.scissor_rectangle));
    gl_state.scissor_rectangle_valid = 1;
    stats.state_changes++;
}

/*
 * Damon V. Caskey
 * 2026-11-12
 * Restore the scene's fixed raster policy once per ownership period. Shader
 * composition owns blending; depth, culling and legacy tests stay disabled.
 */
static void gpu_set_draw_raster(void)
{
    if(gl_state.raster_valid)
    {
        stats.state_reuses += GPU_FIXED_RASTER_CALLS;
        return;
    }
    glDisable(GL_BLEND);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    glDisable(GL_ALPHA_TEST);
    glDisable(GL_STENCIL_TEST);
    glDisable(GL_DITHER);
    gl_state.raster_valid = 1;
    stats.state_changes += GPU_FIXED_RASTER_CALLS;
}

/*
 * Damon V. Caskey
 * 2026-11-12
 * Enable complete color writes for both clear and draw operations without
 * incorrectly marking the rest of the native raster configuration as known.
 */
static void gpu_set_color_mask(void)
{
    if(gl_state.color_mask_valid)
    {
        stats.state_reuses++;
        return;
    }
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    gl_state.color_mask_valid = 1;
    stats.state_changes++;
}

/*
 * Damon V. Caskey
 * 2026-11-12
 * Retain the native opaque-black clear color independently of draw state.
 */
static void gpu_set_clear_color(void)
{
    if(gl_state.clear_color_valid)
    {
        stats.state_reuses++;
        return;
    }
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    gl_state.clear_color_valid = 1;
    stats.state_changes++;
}

/*
 * Damon V. Caskey
 * 2026-11-12
 * Update a scalar integer material value only when its exact value changes.
 */
static void gpu_uniform_integer(GLint location, gpu_uniform_value *cached, GLint value)
{
    if(cached->valid && cached->integer == value)
    {
        stats.state_reuses++;
        return;
    }
    gpu_Uniform1i(location, value);
    cached->integer = value;
    cached->valid = 1;
    stats.state_changes++;
}

/*
 * Damon V. Caskey
 * 2026-11-12
 * Update a scalar floating-point material value without approximating equality.
 */
static void gpu_uniform_float(GLint location, gpu_uniform_value *cached, GLfloat value)
{
    if(cached->valid && cached->vector[0] == value)
    {
        stats.state_reuses++;
        return;
    }
    gpu_Uniform1f(location, value);
    cached->vector[0] = value;
    cached->valid = 1;
    stats.state_changes++;
}

/*
 * Damon V. Caskey
 * 2026-11-12
 * Update both dimensions together when a source or target size changes.
 */
static void gpu_uniform_pair(GLint location, gpu_uniform_value *cached, GLfloat x, GLfloat y)
{
    if(cached->valid && cached->vector[0] == x && cached->vector[1] == y)
    {
        stats.state_reuses++;
        return;
    }
    gpu_Uniform2f(location, x, y);
    cached->vector[0] = x;
    cached->vector[1] = y;
    cached->valid = 1;
    stats.state_changes++;
}

/*
 * Damon V. Caskey
 * 2026-11-12
 * Keep independent channel values exact while reusing an unchanged vector.
 */
static void gpu_uniform_vector(GLint location, gpu_uniform_value *cached,
                               GLfloat x, GLfloat y, GLfloat z)
{
    if(cached->valid && cached->vector[0] == x && cached->vector[1] == y && cached->vector[2] == z)
    {
        stats.state_reuses++;
        return;
    }
    gpu_Uniform3f(location, x, y, z);
    cached->vector[0] = x;
    cached->vector[1] = y;
    cached->vector[2] = z;
    cached->valid = 1;
    stats.state_changes++;
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
    ep_scope upload_scope = engine_profile_span_begin();
    glGenTextures(1, &texture);
    gpu_edit_texture(0, texture);
    /* Texture sampling belongs to the object, not the active binding. Set
     * it once; the presenter must restore NEAREST after borrowing a target. */
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
    glPixelStorei(GL_UNPACK_SKIP_ROWS, 0);
    glPixelStorei(GL_UNPACK_SKIP_PIXELS, 0);
    glTexImage2D(GL_TEXTURE_2D, 0, internal_format, width, height, 0, format, type, pixels);
    if(glGetError() != GL_NO_ERROR)
    {
        engine_profile_span_end(upload_scope, EP_SPAN_UPLOAD);
        native_failure = EP_FALLBACK_RESOURCE;
        gpu_gl_invalidate_state();
        glDeleteTextures(1, &texture);
        return 0;
    }
    engine_profile_span_end(upload_scope, EP_SPAN_UPLOAD);

    /* Null input allocates storage without transferring CPU image pixels.
     * Count initial sprite uploads separately from later screen refreshes;
     * both contribute to the same image-transfer total and byte measure. */
    if(pixels)
    {
        engine_profile_count(EP_COUNT_IMAGE_UPLOADS, 1);
        engine_profile_count(EP_COUNT_IMAGE_UPLOAD_BYTES,
                             (uint64_t)width * height * (format == GL_LUMINANCE ? 1 : 4));
    }
    return texture;
}

/*
 * Damon V. Caskey
 * 2026-11-15
 * Verify floating row textures by allocating the actual sampled format.
 * Missing support only retains native row geometry; ordinary drawing stays
 * available and sine-water sampling does not depend on this texture format.
 */
static int gpu_probe_plane_texture(void)
{
    GLuint texture = gpu_new_texture(1, 1, GL_RGBA32F, GL_RGBA, GL_FLOAT, NULL);
    if(!texture)
    {
        gpu_log("WARNING: Floating plane-row textures unavailable; retaining CPU row geometry with native pixel rendering.");
        return 0;
    }
    gpu_gl_invalidate_state();
    glDeleteTextures(1, &texture);
    return 1;
}

/*
 * Damon V. Caskey
 * 2026-11-15
 * Release context-owned plane maps and their bounded CPU preparation cache.
 * Stored rows contain values only; no script or image allocation is retained.
 */
static void gpu_release_plane_maps(void)
{
    int slot;
    gpu_gl_invalidate_state();
    for(slot = 0; slot < GPU_PLANE_CACHE_CAPACITY; ++slot)
    {
        if(plane_maps[slot].texture)
        {
            glDeleteTextures(1, &plane_maps[slot].texture);
        }
        free(plane_maps[slot].rows);
    }
    memset(plane_maps, 0, sizeof(plane_maps));
    plane_map_clock = 0;
}

/*
 * Damon V. Caskey
 * 2026-11-15
 * Reuse the exact plane scale/source-row recurrence independently of scrolling,
 * placement and image pixels. Bound cache size and evict the least recent map;
 * allocation failure leaves native row geometry available for the same draw.
 */
static gpu_plane_map *gpu_prepare_plane_map(const gpu_transform_parameters *parameters,
                                           const s_drawmethod *method)
{
    gpu_plane_map *entry = NULL;
    gpu_plane_row *replacement;
    int slot;
    if(parameters->source_width <= 0 || parameters->source_height <= 0 ||
       parameters->source_height > GPU_PLANE_MAX_ROWS ||
       !isfinite(method->water.beginsize) || !isfinite(method->water.endsize) ||
       method->water.beginsize <= 0.0f || method->water.endsize < 0.0f)
    {
        return NULL;
    }

    /* Sixteen small keys avoid hashing raw float bytes or retaining pointers
     * to mutable drawmethods. Equality matches the recurrence's actual inputs. */
    for(slot = 0; slot < GPU_PLANE_CACHE_CAPACITY; ++slot)
    {
        gpu_plane_map *candidate = &plane_maps[slot];
        if(candidate->occupied &&
           candidate->source_width == parameters->source_width &&
           candidate->source_height == parameters->source_height &&
           candidate->begin == method->water.beginsize &&
           candidate->end == method->water.endsize &&
           candidate->perspective == method->water.perspective)
        {
            candidate->last_used = ++plane_map_clock;
            ++stats.plane_map_reuses;
            engine_profile_count(EP_COUNT_PLANE_MAP_REUSES, 1);
            return candidate;
        }
        if(!entry || (!candidate->occupied && entry->occupied) ||
           (candidate->occupied == entry->occupied && candidate->last_used < entry->last_used))
        {
            entry = candidate;
        }
    }

    /* Grow before eviction so failed allocation preserves the old cache entry.
     * Each entry owns at most 16,384 rows of sixteen bytes each. */
    if(entry->capacity < (size_t)parameters->source_height)
    {
        replacement = (gpu_plane_row *)realloc(entry->rows,
                       (size_t)parameters->source_height * sizeof(*replacement));
        if(!replacement)
        {
            if(!plane_cache_warning)
            {
                plane_cache_warning = 1;
                gpu_log("WARNING: Plane row-map cache allocation failed; retaining CPU row geometry with native pixel rendering.");
            }
            return NULL;
        }
        entry->rows = replacement;
        entry->capacity = parameters->source_height;
    }
    if(entry->texture)
    {
        gpu_gl_invalidate_state();
        glDeleteTextures(1, &entry->texture);
        entry->texture = 0;
    }
    entry->source_width = parameters->source_width;
    entry->source_height = parameters->source_height;
    entry->begin = method->water.beginsize;
    entry->end = method->water.endsize;
    entry->perspective = method->water.perspective;
    entry->valid_rows = gpu_transform_prepare_plane_rows(parameters, method, entry->rows);
    entry->last_used = ++plane_map_clock;
    entry->occupied = 1;
    ++stats.plane_map_builds;
    stats.plane_rows_prepared += entry->valid_rows;
    engine_profile_count(EP_COUNT_PLANE_MAP_BUILDS, 1);
    engine_profile_count(EP_COUNT_PLANE_ROWS_PREPARED, entry->valid_rows);
    return entry;
}

/*
 * Damon V. Caskey
 * 2026-11-15
 * Upload a prepared plane map once, separate from image-pixel transfers.
 * Failure occurs before destination writes and permits native row geometry;
 * successful maps remain immutable until cache eviction or context shutdown.
 */
static int gpu_upload_plane_map(gpu_plane_map *entry)
{
    ep_scope upload_scope;
    uint64_t bytes;
    if(entry->texture)
    {
        return 1;
    }
    if(entry->valid_rows <= 0)
    {
        return 0;
    }
    entry->texture = gpu_new_texture(1, entry->valid_rows, GL_RGBA32F, GL_RGBA, GL_FLOAT, NULL);
    if(entry->texture)
    {
        upload_scope = engine_profile_span_begin();
        gpu_edit_texture(5, entry->texture);
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
        glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
        glPixelStorei(GL_UNPACK_SKIP_ROWS, 0);
        glPixelStorei(GL_UNPACK_SKIP_PIXELS, 0);
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 1, entry->valid_rows,
                        GL_RGBA, GL_FLOAT, entry->rows);
        if(glGetError() == GL_NO_ERROR)
        {
            engine_profile_span_end(upload_scope, EP_SPAN_UPLOAD);
            bytes = (uint64_t)entry->valid_rows * sizeof(*entry->rows);
            ++stats.plane_map_uploads;
            stats.plane_map_upload_bytes += bytes;
            engine_profile_count(EP_COUNT_PLANE_MAP_UPLOADS, 1);
            engine_profile_count(EP_COUNT_PLANE_MAP_UPLOAD_BYTES, bytes);
            return 1;
        }
        engine_profile_span_end(upload_scope, EP_SPAN_UPLOAD);
        gpu_gl_invalidate_state();
        glDeleteTextures(1, &entry->texture);
        entry->texture = 0;
    }
    if(!plane_upload_warning)
    {
        plane_upload_warning = 1;
        gpu_log("WARNING: Plane row-map upload failed; retaining CPU row geometry with native pixel rendering.");
    }
    return 0;
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
    /* Deleting a bound object can change GL state, and its numeric name may
     * be recycled. Forget cached assumptions before either can be observed. */
    gpu_gl_invalidate_state();
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
    ep_scope upload_scope;
    if(!image->cpu_dirty)
    {
        return 1;
    }

    /* CPU mutations own the complete screen version. Reset unpack state
     * explicitly because uploads share a context with presentation code. */
    upload_scope = engine_profile_span_begin();
    gpu_edit_texture(0, image->texture);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
    glPixelStorei(GL_UNPACK_SKIP_ROWS, 0);
    glPixelStorei(GL_UNPACK_SKIP_PIXELS, 0);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, image->width, image->height,
                   format, type, screen->data);
    if(glGetError() != GL_NO_ERROR)
    {
        engine_profile_span_end(upload_scope, EP_SPAN_UPLOAD);
        native_failure = EP_FALLBACK_RESOURCE;
        gpu_gl_invalidate_state();
        return 0;
    }
    engine_profile_span_end(upload_scope, EP_SPAN_UPLOAD);
    engine_profile_count(EP_COUNT_IMAGE_UPLOADS, 1);
    engine_profile_count(EP_COUNT_IMAGE_UPLOAD_BYTES,
                         (uint64_t)image->width * image->height * (image->format == PIXEL_32 ? 4 : 1));

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
 * Reject oversized or unsupported images before changing their contents.
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
        native_failure = screen && screen->pixelformat != PIXEL_32 && screen->pixelformat != PIXEL_x8
                       ? EP_FALLBACK_SOURCE_FORMAT : EP_FALLBACK_IMAGE_SIZE;
        return NULL;
    }
    image = gpu_find_image(screen, 0);
    if(image && (image->width != screen->width || image->height != screen->height ||
                 image->format != screen->pixelformat))
    {
        native_failure = EP_FALLBACK_RESOURCE;
        return NULL;
    }
    if(!image)
    {
        image = (gpu_image *)calloc(1, sizeof(*image));
        if(!image)
        {
            native_failure = EP_FALLBACK_RESOURCE;
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
            gpu_bind_framebuffer(image->framebuffer);
            gpu_FramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                                     GL_TEXTURE_2D, image->texture, 0);
            if(gpu_CheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
            {
                native_failure = EP_FALLBACK_RESOURCE;
                gpu_bind_framebuffer(0);
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
        native_failure = EP_FALLBACK_IMAGE_SIZE;
        return NULL;
    }
    image = gpu_find_image(sprite, 1);
    if(image)
    {
        return image;
    }
    if((size_t)sprite->width > SIZE_MAX / (size_t)sprite->height)
    {
        native_failure = EP_FALLBACK_IMAGE_SIZE;
        return NULL;
    }
    pixels = (unsigned char *)calloc((size_t)sprite->width, (size_t)sprite->height);
    if(!pixels)
    {
        native_failure = EP_FALLBACK_RESOURCE;
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
                native_failure = EP_FALLBACK_OTHER;
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
        native_failure = EP_FALLBACK_RESOURCE;
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
    ep_scope readback_scope;
    if(!image || !image->gpu_dirty)
    {
        return 1;
    }

    /* This is the synchronization boundary for CPU consumers. Repeated CPU
     * reads reuse the populated shadow until another GPU draw changes it. */
    readback_scope = engine_profile_span_begin();
    gpu_bind_framebuffer(image->framebuffer);
    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    glPixelStorei(GL_PACK_ROW_LENGTH, 0);
    glPixelStorei(GL_PACK_SKIP_ROWS, 0);
    glPixelStorei(GL_PACK_SKIP_PIXELS, 0);
    glReadPixels(0, 0, image->width, image->height, GPU_PACKED_FORMAT,
                 GL_UNSIGNED_INT_8_8_8_8_REV, screen->data);
    if(glGetError() != GL_NO_ERROR)
    {
        engine_profile_span_end(readback_scope, EP_SPAN_READBACK);
        gpu_gl_invalidate_state();
        return 0;
    }
    count = (size_t)image->width * image->height;
    pixels = (unsigned *)screen->data;
    for(i = 0; i < count; i++)
    {
        pixels[i] &= 0x00ffffffu;
    }
    /* Wall time includes the existing blocking readback and RGB-shadow scrub.
     * Byte counts describe the whole transfer even for a single-pixel caller. */
    engine_profile_span_end(readback_scope, EP_SPAN_READBACK);
    engine_profile_count(EP_COUNT_READBACKS, 1);
    engine_profile_count(EP_COUNT_READBACK_BYTES, (uint64_t)count * sizeof(*pixels));
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
 *
 * Forget all palette residency after context replacement or an uncertain
 * upload. Unreachable old CPU snapshots need not be cleared or reallocated.
 */
static void gpu_reset_palette_cache(void)
{
    int bucket;

    /* Only entries reachable through a bucket or the selected-slot shortcut
     * can be reused. Emptying those roots invalidates every previous row. */
    for(bucket = 0; bucket < GPU_PALETTE_CACHE_BUCKETS; bucket++)
    {
        palette_buckets[bucket] = GPU_PALETTE_NO_SLOT;
    }
    palette_cache_count = 0;
    palette_lru_head = GPU_PALETTE_NO_SLOT;
    palette_lru_tail = GPU_PALETTE_NO_SLOT;
    palette_selected_slot = GPU_PALETTE_NO_SLOT;
}

/*
 * Damon V. Caskey
 * 2026-11-12
 *
 * Hash the complete palette for bounded bucket lookup. Exact content checks
 * resolve collisions; this process-local word hash is not an identity token.
 */
static uint32_t gpu_palette_hash(const unsigned char *palette)
{
    uint32_t hash = GPU_PALETTE_HASH_SEED;
    unsigned color;

    /* memcpy permits palettes supplied at an unaligned address and avoids
     * aliasing assumptions. Word-at-a-time mixing shortens the dependency
     * chain compared with separately hashing each of the palette's bytes. */
    for(color = 0; color < GPU_PALETTE_COLORS; color++)
    {
        uint32_t value;
        memcpy(&value, palette + color * sizeof(value), sizeof(value));
        hash = (hash ^ value) * GPU_PALETTE_HASH_MULTIPLIER;
    }

    /* Diffuse upper color-channel bits into low bucket bits. Without this
     * final mixing, palettes that differ only in upper bytes could collect
     * in one bucket even when their full word hashes differ. */
    hash ^= hash >> 16;
    hash *= GPU_PALETTE_HASH_MIX_FIRST;
    hash ^= hash >> 13;
    hash *= GPU_PALETTE_HASH_MIX_SECOND;
    hash ^= hash >> 16;
    return hash;
}

/*
 * Damon V. Caskey
 * 2026-11-12
 *
 * Detach one resident slot from the recency list before promotion or reuse.
 * Hash membership is independent and remains unchanged by a cache hit.
 */
static void gpu_palette_unlink_lru(int slot)
{
    gpu_palette_entry *entry = &palette_cache[slot];

    /* Repair both neighbors, including either end of the list. A slot is
     * always resident when this helper is called, so its links are current. */
    if(entry->lru_previous != GPU_PALETTE_NO_SLOT)
    {
        palette_cache[entry->lru_previous].lru_next = entry->lru_next;
    }
    else
    {
        palette_lru_head = entry->lru_next;
    }
    if(entry->lru_next != GPU_PALETTE_NO_SLOT)
    {
        palette_cache[entry->lru_next].lru_previous = entry->lru_previous;
    }
    else
    {
        palette_lru_tail = entry->lru_previous;
    }
}

/*
 * Damon V. Caskey
 * 2026-11-12
 *
 * Place a detached or newly populated slot at the most-recently-used end.
 */
static void gpu_palette_link_front(int slot)
{
    gpu_palette_entry *entry = &palette_cache[slot];

    /* New slots and promoted hits use the same list insertion. The first
     * resident entry owns both endpoints; subsequent insertions keep tail. */
    entry->lru_previous = GPU_PALETTE_NO_SLOT;
    entry->lru_next = palette_lru_head;
    if(palette_lru_head != GPU_PALETTE_NO_SLOT)
    {
        palette_cache[palette_lru_head].lru_previous = slot;
    }
    else
    {
        palette_lru_tail = slot;
    }
    palette_lru_head = slot;
}

/*
 * Damon V. Caskey
 * 2026-11-12
 *
 * Select an exact resident palette match and retain hot rows in constant
 * recency-list time. No GL calls or uploads are required on this path.
 */
static void gpu_palette_select(int slot)
{
    /* Consecutive matching draws already use the list head. Other hits
     * move only their own links; no full-cache scan or age counter is needed. */
    if(palette_lru_head != slot)
    {
        gpu_palette_unlink_lru(slot);
        gpu_palette_link_front(slot);
    }
    palette_selected_slot = slot;
    stats.palette_cache_hits++;
    engine_profile_count(EP_COUNT_PALETTE_CACHE_HITS, 1);
}

/*
 * Damon V. Caskey
 * 2026-11-12
 *
 * Remove an evicted row from both lookup and recency structures. Collision
 * chains retain their remaining entries even when the removed row is inside.
 */
static void gpu_palette_remove(int slot)
{
    gpu_palette_entry *entry = &palette_cache[slot];
    unsigned bucket = entry->hash & (GPU_PALETTE_CACHE_BUCKETS - 1);
    int *link = &palette_buckets[bucket];

    /* Follow links by address so removing a chain head or interior entry
     * uses the same update and never discards a colliding neighbor. */
    while(*link != GPU_PALETTE_NO_SLOT && *link != slot)
    {
        link = &palette_cache[*link].hash_next;
    }
    if(*link == slot)
    {
        *link = entry->hash_next;
    }
    gpu_palette_unlink_lru(slot);
    if(palette_selected_slot == slot)
    {
        palette_selected_slot = GPU_PALETTE_NO_SLOT;
    }
}

/*
 * Damon V. Caskey
 * 2026-11-12
 *
 * Resolve a palette by current contents and upload only cache misses. Each
 * atlas row retains its own snapshot across draws and frames until evicted.
 */
static int gpu_set_palette(const unsigned char *palette)
{
    uint32_t hash;
    unsigned bucket;
    int slot;
    gpu_palette_entry *entry;
    ep_scope lookup_scope, upload_scope;

    if(!palette)
    {
        native_failure = EP_FALLBACK_PALETTE;
        return 0;
    }

    /* Consecutive identical values need no hash traversal. Compare contents
     * even when callers reuse one address so in-place edits remain visible. */
    lookup_scope = engine_profile_span_begin();
    if(palette_selected_slot != GPU_PALETTE_NO_SLOT &&
       !memcmp(palette_cache[palette_selected_slot].colors, palette, GPU_PALETTE_BYTES))
    {
        gpu_palette_select(palette_selected_slot);
        engine_profile_span_end(lookup_scope, EP_SPAN_PALETTE_LOOKUP);
        return 1;
    }

    /* Values, not pointer identity, determine residency. Matching hashes
     * still require an exact full-palette comparison, including entry zero. */
    hash = gpu_palette_hash(palette);
    bucket = hash & (GPU_PALETTE_CACHE_BUCKETS - 1);
    for(slot = palette_buckets[bucket]; slot != GPU_PALETTE_NO_SLOT;
        slot = palette_cache[slot].hash_next)
    {
        entry = &palette_cache[slot];
        if(entry->hash == hash && !memcmp(entry->colors, palette, GPU_PALETTE_BYTES))
        {
            gpu_palette_select(slot);
            engine_profile_span_end(lookup_scope, EP_SPAN_PALETTE_LOOKUP);
            return 1;
        }
    }
    stats.palette_cache_misses++;
    engine_profile_count(EP_COUNT_PALETTE_CACHE_MISSES, 1);
    slot = palette_cache_count < GPU_PALETTE_CACHE_CAPACITY
         ? palette_cache_count : palette_lru_tail;
    engine_profile_span_end(lookup_scope, EP_SPAN_PALETTE_LOOKUP);

    /* Fill unused rows first, then replace the least-recently-used row.
     * Ordered GL commands preserve earlier draws that sampled an evicted
     * row; the new contents apply only to subsequent draw submissions. */
    upload_scope = engine_profile_span_begin();
    gpu_edit_texture(1, palette_texture);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
    glPixelStorei(GL_UNPACK_SKIP_ROWS, 0);
    glPixelStorei(GL_UNPACK_SKIP_PIXELS, 0);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, slot, GPU_PALETTE_COLORS, 1, GPU_PACKED_FORMAT,
                   GL_UNSIGNED_INT_8_8_8_8_REV, palette);
    if(glGetError() != GL_NO_ERROR)
    {
        engine_profile_span_end(upload_scope, EP_SPAN_UPLOAD);

        /* A pending driver error does not prove the upload left its row
         * unchanged. Forget all lookup roots, including the fast path, so
         * later draws cannot reuse a CPU snapshot with uncertain GPU data. */
        gpu_reset_palette_cache();
        native_failure = EP_FALLBACK_RESOURCE;
        gpu_gl_invalidate_state();
        return 0;
    }
    engine_profile_span_end(upload_scope, EP_SPAN_UPLOAD);
    engine_profile_count(EP_COUNT_PALETTE_UPLOADS, 1);
    engine_profile_count(EP_COUNT_PALETTE_UPLOAD_BYTES, GPU_PALETTE_BYTES);
    stats.palette_uploads++;
    stats.uploads++;

    /* Publish the new contents only after a successful transfer. Old rows
     * remain immutable snapshots until selected as victims; returning to an
     * earlier palette value can therefore hit even after in-place edits. */
    if(palette_cache_count == GPU_PALETTE_CACHE_CAPACITY)
    {
        gpu_palette_remove(slot);
        stats.palette_cache_evictions++;
        engine_profile_count(EP_COUNT_PALETTE_CACHE_EVICTIONS, 1);
    }
    else
    {
        palette_cache_count++;
    }
    entry = &palette_cache[slot];
    memcpy(entry->colors, palette, GPU_PALETTE_BYTES);
    entry->hash = hash;
    entry->hash_next = palette_buckets[bucket];
    palette_buckets[bucket] = slot;
    gpu_palette_link_front(slot);
    palette_selected_slot = slot;
    return 1;
}

/*
 * Damon V. Caskey
 * 2026-11-12
 * 2026-11-15 - Remove the obsolete 16-bit scene blend-table exception.
 * Preserve channel lookup blends on the GPU. Cached table values are reused
 * across draws and refreshed after replacement or explicit invalidation.
 */
static int gpu_set_tables(void)
{
    int i;
    ep_scope upload_scope;

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
    upload_scope = engine_profile_span_begin();
    gpu_edit_texture(4, table_texture);
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
                engine_profile_span_end(upload_scope, EP_SPAN_UPLOAD);
                native_failure = EP_FALLBACK_RESOURCE;
                gpu_gl_invalidate_state();
                return 0;
            }
            engine_profile_count(EP_COUNT_TABLE_UPLOADS, 1);
            engine_profile_count(EP_COUNT_TABLE_UPLOAD_BYTES, GPU_BLEND_TABLE_SIZE);
            table_mask |= 1 << i;
            stats.uploads++;
        }
        table_pointers[i] = blendtables[i];
    }
    engine_profile_span_end(upload_scope, EP_SPAN_UPLOAD);
    tables_dirty = 0;
    return 1;
}

/*
 * Damon V. Caskey
 * 2026-11-12
 * Convert packed engine RGB into normalized shader channel values without
 * assigning transparency to the unused high byte.
 */
static void gpu_uniform_color(GLint location, gpu_uniform_value *cached, unsigned color)
{
    /* The packed alpha byte is outside the engine RGB contract. Comparing
     * the effective color also avoids repeat normalization of unchanged RGB. */
    color &= 0x00ffffffu;
    if(cached->valid && cached->integer == (GLint)color)
    {
        stats.state_reuses++;
        return;
    }
#if REVERSE_COLOR
    gpu_Uniform3f(location, ((color >> 16) & 255) / 255.0f,
                  ((color >> 8) & 255) / 255.0f, (color & 255) / 255.0f);
#else
    gpu_Uniform3f(location, (color & 255) / 255.0f,
                  ((color >> 8) & 255) / 255.0f, ((color >> 16) & 255) / 255.0f);
#endif
    cached->integer = (GLint)color;
    cached->valid = 1;
    stats.state_changes++;
}

/*
 * Damon V. Caskey
 * 2026-11-14
 * Upload exact RGB byte values for chroma and Substitute comparisons.
 * Keeping keys in byte space avoids per-fragment normalization/rounding.
 */
static void gpu_uniform_color_bytes(GLint location, gpu_uniform_value *cached, unsigned color)
{
    /* The unused high byte cannot change a color match or cache identity. */
    color &= 0x00ffffffu;
    if(cached->valid && cached->integer == (GLint)color)
    {
        stats.state_reuses++;
        return;
    }
#if REVERSE_COLOR
    gpu_Uniform3f(location, (float)((color >> 16) & 255),
                  (float)((color >> 8) & 255), (float)(color & 255));
#else
    gpu_Uniform3f(location, (float)(color & 255),
                  (float)((color >> 8) & 255), (float)((color >> 16) & 255));
#endif
    cached->integer = (GLint)color;
    cached->valid = 1;
    stats.state_changes++;
}

/*
 * Damon V. Caskey
 * 2026-11-12
 * 2026-11-14 - Keep vertices contiguous and sampling metadata separate.
 * Collect geometry before touching the target. Parameter or allocation
 * failure then rejects the operation without leaving a partial GPU draw.
 */
static int gpu_collect_quad(const gpu_vertex vertices[4], int wrap, void *unused)
{
    int i;
    (void)unused;
    if(quad_count == quad_capacity)
    {
        /* Grow geometrically and retain the allocation between draws. Size
         * checks reject an oversized request without changing its target. */
        size_t maximum = (size_t)INT_MAX / 6;
        size_t capacity = quad_capacity ? quad_capacity * 2 : 64;
        gpu_vertex *replacement;
        int *replacement_wraps;
        if(capacity > maximum) capacity = maximum;
        if(capacity <= quad_capacity || capacity > SIZE_MAX / (4 * sizeof(*vertex_buffer)))
        {
            native_failure = EP_FALLBACK_RESOURCE;
            return 0;
        }
        replacement = (gpu_vertex *)realloc(vertex_buffer, capacity * 4 * sizeof(*vertex_buffer));
        if(!replacement)
        {
            native_failure = EP_FALLBACK_RESOURCE;
            return 0;
        }
        vertex_buffer = replacement;
        replacement_wraps = (int *)realloc(quad_wraps, capacity * sizeof(*quad_wraps));
        if(!replacement_wraps)
        {
            native_failure = EP_FALLBACK_RESOURCE;
            return 0;
        }
        quad_wraps = replacement_wraps;
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
            native_failure = EP_FALLBACK_TRANSFORM;
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
    memcpy(vertex_buffer + quad_count * 4, vertices, 4 * sizeof(*vertex_buffer));
    quad_wraps[quad_count++] = wrap;
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
    ep_scope copy_scope;
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
    copy_scope = engine_profile_span_begin();
    gpu_bind_framebuffer(target->framebuffer);
    gpu_edit_texture(3, target->snapshot);
    glCopyTexSubImage2D(GL_TEXTURE_2D, 0, x, y, x, y, width, height);
    if(glGetError() != GL_NO_ERROR)
    {
        engine_profile_span_end(copy_scope, EP_SPAN_DEST_COPY);
        native_failure = EP_FALLBACK_RESOURCE;
        gpu_gl_invalidate_state();
        return 0;
    }
    engine_profile_span_end(copy_scope, EP_SPAN_DEST_COPY);
    engine_profile_count(EP_COUNT_DEST_COPIES, 1);
    engine_profile_count(EP_COUNT_DEST_COPY_BYTES, (uint64_t)width * height * 4);
    if(self_draw)
    {
        engine_profile_count(EP_COUNT_SELF_COPIES, 1);
    }
    stats.destination_copies++;
    return 1;
}

/*
 * Damon V. Caskey
 * 2026-11-12
 * 2026-11-13 - Supply independent affine sample state for either source format.
 * 2026-11-14 - Bind supported chroma and Substitute without changing draw eligibility.
 * 2026-11-14 - Upload a contiguous vertex stream and submit indexed triangles.
 * 2026-11-14 - Supply exact legacy shear state through the common material path.
 * 2026-11-15 - Sample water and cached plane rows through the same materials.
 * 2026-11-15 - Reconstruct selected filters and composite fractional key coverage.
 * Render a prepared image draw in painter order. Source palette/tint work
 * stays in the shader; destination-dependent effects sample a separate GPU
 * snapshot, never the attachment currently being written.
 */
static int gpu_render_quads(gpu_image *target, gpu_image *source, gpu_image *mask,
                            const unsigned char *palette, const s_drawmethod *method,
                            int source_is_sprite, int primitive, unsigned color,
                            const gpu_affine_sampling *affine, const gpu_shear_sampling *shear,
                            const gpu_water_sampling *water, const gpu_plane_sampling *plane,
                            GLuint plane_texture)
{
    int rectangle[4], i, wrap, geometry_ready;
    int indexed = source && source->format == PIXEL_x8;
    int masked = mask != NULL;
    int blend_mode = method->alpha > 0 ? method->alpha : 0;
    int resample = primitive ? DC_RESAMPLE_NEAREST : method->resample;
    int color_key = !primitive && (source_is_sprite ||
                    (method->config & DRAWMETHOD_CONFIG_BACKGROUND_TRANSPARENCY));
    int needs_destination;
    int self_draw = source == target;
    int fill = primitive || method->fillcolor != 0;
    /* Optional color rules never move an ordinary draw to software. Indexed
     * fill can proceed without a palette, but cannot resolve source chroma. */
    unsigned chroma_enabled = color_rules_supported && (!indexed || palette)
                            ? method->colors.chroma_enabled & DRAWMETHOD_COLOR_MASK : 0;
    unsigned substitute_enabled = color_rules_supported
                                ? method->colors.substitute_enabled & DRAWMETHOD_COLOR_MASK : 0;
    GLuint texture = source ? source->texture : white_texture;
    size_t q, batches = 0, stream_first = 0;
    uint64_t previous_state_changes = stats.state_changes;
    uint64_t previous_state_reuses = stats.state_reuses;
    ep_scope state_scope, submit_scope;

    /* Filtered key edges contain fractional coverage even without an explicit
     * blend mode. Opaque filtered copies retain the snapshot-free fast path;
     * partial coverage always reads the completed destination on the GPU. */
    needs_destination = masked || blend_mode != 0 ||
                        (resample != DC_RESAMPLE_NEAREST && (color_key || chroma_enabled));

    /* Finish resource preparation before touching destination pixels. Empty
     * or fully clipped geometry is already a successful, completed draw. */
    if(!quad_count || !gpu_draw_bounds(target, method, rectangle))
    {
        return 1;
    }
    if(resample < DC_RESAMPLE_NEAREST || resample >= DC_RESAMPLE_END)
    {
        native_failure = EP_FALLBACK_TRANSFORM;
        return 0;
    }
    if(blend_mode > MAX_BLENDINGS || method->tintmode > MAX_BLENDINGS)
    {
        native_failure = EP_FALLBACK_BLEND;
        return 0;
    }
    if(indexed && !palette && !fill)
    {
        native_failure = EP_FALLBACK_PALETTE;
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

    /* Storage/update failure must still leave the destination untouched.
     * Include stream preparation in CPU submission timing, separate from
     * material setup and the existing GPU-to-GPU snapshot measurement. */
    submit_scope = engine_profile_span_begin();
    geometry_ready = gpu_upload_geometry(&stream_first);
    engine_profile_span_end(submit_scope, EP_SPAN_GPU_SUBMIT);
    if(!geometry_ready)
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

    /* Reassert only unknown or changed state. Internal resource operations
     * use these same setters; an external owner invalidates them at handoff.
     * The material shader still computes final RGB and destination blending. */
    state_scope = engine_profile_span_begin();
    gpu_bind_framebuffer(target->framebuffer);
    gpu_set_viewport(target->width, target->height);
    gpu_set_scissor_enabled(1);
    gpu_set_scissor_rectangle(rectangle);
    gpu_set_draw_raster();
    gpu_set_color_mask();
    gpu_use_program();
    gpu_bind_texture(0, texture);
    gpu_bind_texture(1, palette_texture);
    gpu_bind_texture(2, masked ? mask->texture : white_texture);
    gpu_bind_texture(3, needs_destination ? target->snapshot : white_texture);
    gpu_bind_texture(4, table_texture);
    if(plane)
    {
        gpu_bind_texture(5, plane_texture);
    }
    gpu_select_texture_unit(0);

    /* Keep geometry in engine pixel units and pass the existing drawmethod
     * as material state. Palette, mask and blend tables remain independent
     * textures so changing a color effect preserves the cached source image. */
    gpu_uniform_pair(uniforms.target_size, &gl_state.material.target_size,
                      (float)target->width, (float)target->height);
    gpu_uniform_pair(uniforms.source_size, &gl_state.material.source_size,
                      source ? (float)source->width : 1.0f,
                      source ? (float)source->height : 1.0f);
    gpu_uniform_integer(uniforms.source, &gl_state.material.source, 0);
    gpu_uniform_integer(uniforms.palette, &gl_state.material.palette, 1);
    /* Palette selection changes a row coordinate rather than replacing a
     * shared 1 KiB texture on every alternating indexed draw. Non-indexed
     * draws ignore it; indexed fills retain their source-coverage behavior. */
    gpu_uniform_float(uniforms.palette_row, &gl_state.material.palette_row,
                       ((float)(palette_selected_slot == GPU_PALETTE_NO_SLOT
                                ? 0 : palette_selected_slot) + 0.5f) / GPU_PALETTE_CACHE_CAPACITY);
    gpu_uniform_integer(uniforms.mask, &gl_state.material.mask, 2);
    gpu_uniform_integer(uniforms.destination, &gl_state.material.destination, 3);
    gpu_uniform_integer(uniforms.tables, &gl_state.material.tables, 4);
    if(plane)
    {
        gpu_uniform_integer(uniforms.plane_rows, &gl_state.material.plane_rows, 5);
    }
    gpu_uniform_integer(uniforms.indexed, &gl_state.material.indexed, indexed);
    gpu_uniform_integer(uniforms.masked, &gl_state.material.masked, masked);
    gpu_uniform_integer(uniforms.color_key, &gl_state.material.color_key, color_key);
    gpu_uniform_integer(uniforms.resample, &gl_state.material.resample, resample);
    gpu_uniform_integer(uniforms.fill, &gl_state.material.fill, fill);
    gpu_uniform_integer(uniforms.has_destination, &gl_state.material.has_destination, needs_destination);
    gpu_uniform_integer(uniforms.blend_mode, &gl_state.material.blend_mode, blend_mode);
    gpu_uniform_integer(uniforms.tint_mode, &gl_state.material.tint_mode, (int)method->tintmode);
    gpu_uniform_integer(uniforms.table_mask, &gl_state.material.table_mask, table_mask);
    gpu_uniform_color(uniforms.fill_color, &gl_state.material.fill_color, primitive ? color : method->fillcolor);
    gpu_uniform_color(uniforms.tint_color, &gl_state.material.tint_color, method->tintcolor);
    gpu_uniform_vector(uniforms.channels, &gl_state.material.channels, method->channelr / 255.0f,
                        method->channelg / 255.0f, method->channelb / 255.0f);
    if(color_rules_supported)
    {
        /* Effective masks are independent from RGB zero. Only active slots
         * require values; disabling all rules does not add texture reads or
         * destination copies. Ignored chroma also clears any earlier mask. */
        gpu_uniform_integer(uniforms.chroma_enabled, &gl_state.material.chroma_enabled,
                            (int)chroma_enabled);
        gpu_uniform_integer(uniforms.substitute_enabled, &gl_state.material.substitute_enabled,
                            (int)substitute_enabled);
        if(chroma_enabled)
        {
            gpu_uniform_integer(uniforms.primitive_source, &gl_state.material.primitive_source, primitive);
        }
        if(chroma_enabled || substitute_enabled)
        {
            for(i = 0; i < DRAWMETHOD_COLOR_SLOTS; ++i)
            {
                if(chroma_enabled & (1u << i))
                {
                    gpu_uniform_color_bytes(uniforms.chroma_keys[i], &gl_state.material.chroma_keys[i],
                                            method->colors.chroma[i]);
                }
                if(substitute_enabled & (1u << i))
                {
                    gpu_uniform_color_bytes(uniforms.substitute_match[i], &gl_state.material.substitute_match[i],
                                            method->colors.substitute[i].match);
                    gpu_uniform_color_bytes(uniforms.substitute_value[i], &gl_state.material.substitute_value[i],
                                            method->colors.substitute[i].replacement);
                }
            }
        }
    }
    if(shear)
    {
        /* Legacy shear owns exact row coverage and clipping-adjusted source
         * phases. Its uniforms are independent of explicit affine matrices. */
        gpu_uniform_vector(uniforms.shear_origin, &gl_state.material.shear_origin,
                           shear->origin[0], shear->origin[1], shear->origin[2]);
        gpu_uniform_vector(uniforms.shear_extent, &gl_state.material.shear_extent,
                           shear->extent[0], shear->extent[1], shear->extent[2]);
        gpu_uniform_pair(uniforms.shear_shift, &gl_state.material.shear_shift,
                         shear->shift[0], shear->shift[1]);
        gpu_uniform_pair(uniforms.shear_source, &gl_state.material.shear_source,
                         shear->source[0], shear->source[1]);
        gpu_uniform_pair(uniforms.shear_step, &gl_state.material.shear_step,
                         shear->step[0], shear->step[1]);
        gpu_uniform_pair(uniforms.shear_clip, &gl_state.material.shear_clip,
                         shear->clip[0], shear->clip[1]);
    }
    if(water)
    {
        /* One inverse-mapped quad evaluates wave phase/displacement per row
         * in the shader; no CPU-generated strip geometry is submitted. */
        gpu_uniform_vector(uniforms.water_origin, &gl_state.material.water_origin,
                           water->origin[0], water->origin[1], water->origin[2]);
        gpu_uniform_vector(uniforms.water_wave, &gl_state.material.water_wave,
                           water->wave[0], water->wave[1], water->wave[2]);
        gpu_uniform_float(uniforms.water_anchor, &gl_state.material.water_anchor, water->anchor);
    }
    if(plane)
    {
        /* The cached recurrence contains only intrinsic source-row data.
         * Position and scrolling remain small independent uniform updates. */
        gpu_uniform_vector(uniforms.plane_origin, &gl_state.material.plane_origin,
                           plane->origin[0], plane->origin[1], plane->origin[2]);
        gpu_uniform_pair(uniforms.plane_info, &gl_state.material.plane_info,
                         plane->info[0], plane->info[1]);
    }
    if(affine)
    {
        /* Geometry and color remain separate. These small per-draw values
         * select affine texels for indexed and direct-color materials alike. */
        gpu_uniform_vector(uniforms.affine_approximate_x, &gl_state.material.affine_approximate_x,
                           affine->approximate_x[0], affine->approximate_x[1], affine->approximate_x[2]);
        gpu_uniform_vector(uniforms.affine_approximate_y, &gl_state.material.affine_approximate_y,
                           affine->approximate_y[0], affine->approximate_y[1], affine->approximate_y[2]);
        gpu_uniform_float(uniforms.affine_approximate_d, &gl_state.material.affine_approximate_d,
                          affine->approximate_denominator);
        gpu_uniform_pair(uniforms.affine_anchor, &gl_state.material.affine_anchor,
                         affine->anchor[0], affine->anchor[1]);
        gpu_uniform_pair(uniforms.affine_pivot, &gl_state.material.affine_pivot,
                         affine->pivot[0], affine->pivot[1]);
        for(i = 0; i < 8; ++i)
        {
            gpu_uniform_vector(uniforms.affine_x[i], &gl_state.material.affine_x[i],
                               affine->x[i][0], affine->x[i][1], affine->x[i][2]);
            gpu_uniform_vector(uniforms.affine_y[i], &gl_state.material.affine_y[i],
                               affine->y[i][0], affine->y[i][1], affine->y[i][2]);
            gpu_uniform_float(uniforms.affine_d[i], &gl_state.material.affine_d[i],
                              affine->denominator[i]);
        }
    }
    engine_profile_span_end(state_scope, EP_SPAN_GPU_STATE);

    /* Adjacent row strips share state and one indexed triangle submission.
     * Keep each wrap run and draw boundary in original painter order. The
     * fixed index pattern reuses the four vertices without CPU expansion. */
    submit_scope = engine_profile_span_begin();
    for(q = 0; q < quad_count;)
    {
        size_t end = q + 1;
        wrap = quad_wraps[q];
        while(end < quad_count && quad_wraps[end] == wrap) ++end;
        gpu_uniform_integer(uniforms.wrap, &gl_state.material.wrap, wrap);
        gpu_DrawElements(GL_TRIANGLES, (GLsizei)((end - q) * 6), GL_UNSIGNED_INT,
                         (const void *)(uintptr_t)((stream_first + q) * 6 * sizeof(GLuint)));
        batches++;
        q = end;
    }
    if(glGetError() != GL_NO_ERROR)
    {
        gpu_fatal("scene draw");
    }
    /* Submission time includes stream upload and driver error checks. Quad
     * and batch counts retain their meaning: one plane image may emit many
     * logical row quads while sharing one indexed triangle draw. */
    engine_profile_span_end(submit_scope, EP_SPAN_GPU_SUBMIT);
    engine_profile_count(EP_COUNT_NATIVE_DRAWS, 1);
    if(water)
    {
        ++stats.water_draws;
        engine_profile_count(EP_COUNT_NATIVE_WATER_DRAWS, 1);
    }
    if(plane)
    {
        ++stats.plane_draws;
        engine_profile_count(EP_COUNT_NATIVE_PLANE_DRAWS, 1);
    }
    if(chroma_enabled)
    {
        engine_profile_count(EP_COUNT_NATIVE_CHROMA_DRAWS, 1);
    }
    if(substitute_enabled)
    {
        engine_profile_count(EP_COUNT_NATIVE_SUBSTITUTE_DRAWS, 1);
    }
    engine_profile_count(EP_COUNT_NATIVE_QUADS, quad_count);
    engine_profile_count(EP_COUNT_NATIVE_BATCHES, batches);
    /* Aggregate once per completed draw rather than adding profiling calls
     * to every cached setter. These counts include preparation and wrap state;
     * the existing setup timer intentionally retains its narrower boundary. */
    engine_profile_count(EP_COUNT_GPU_STATE_CHANGES, stats.state_changes - previous_state_changes);
    engine_profile_count(EP_COUNT_GPU_STATE_REUSES, stats.state_reuses - previous_state_reuses);

    /* Subsequent GPU consumers can use the texture immediately. Its CPU
     * shadow becomes current only when an explicit reader synchronizes it. */
    target->gpu_dirty = 1;
    target->cpu_dirty = 0;
    stats.draws++;
    return 1;
}

/*
 * Damon V. Caskey
 * 2026-11-13
 *
 * Accept already validated transform output without retaining or drawing it.
 * Affine repeats use this callback to prove that every instance is supported
 * before any earlier instance can change the target.
 */
static int gpu_preflight_quad(const gpu_vertex vertices[4], int wrap, void *userdata)
{
    /* The transform validates coordinates before emission. This pass needs
     * only its result, so it neither allocates a batch nor touches GPU state. */
    (void)vertices;
    (void)wrap;
    (void)userdata;
    return 1;
}

/*
 * Damon V. Caskey
 * 2026-11-15
 * Retain the exact quad emitted by affine preflight alongside its sampling
 * values, allowing the submission pass to skip a second matrix preparation.
 */
static int gpu_save_affine_quad(const gpu_vertex vertices[4], int wrap, void *userdata)
{
    gpu_prepared_affine *entry = (gpu_prepared_affine *)userdata;
    if(wrap != GPU_SAMPLE_AFFINE)
    {
        return 0;
    }
    memcpy(entry->vertices, vertices, sizeof(entry->vertices));
    return 1;
}

/*
 * Damon V. Caskey
 * 2026-11-15
 * Grow preparation storage on demand and reuse it across subsequent draws.
 * Large grids or allocation failure retain validation and recomputation;
 * either path continues to rasterize every accepted draw on the GPU.
 */
static int gpu_reserve_affine_preparations(size_t count)
{
    size_t capacity;
    gpu_prepared_affine *replacement;
    if(count > GPU_AFFINE_PREPARED_LIMIT)
    {
        return 0;
    }
    if(count <= affine_preparation_capacity)
    {
        return 1;
    }
    capacity = affine_preparation_capacity ? affine_preparation_capacity : 8;
    while(capacity < count)
    {
        capacity *= 2;
    }
    replacement = (gpu_prepared_affine *)realloc(affine_preparations,
                                                capacity * sizeof(*replacement));
    if(!replacement)
    {
        return 0;
    }
    affine_preparations = replacement;
    affine_preparation_capacity = capacity;
    return 1;
}

/*
 * Damon V. Caskey
 * 2026-11-12
 * 2026-11-13 - Preflight all affine repeats before the first target write.
 * 2026-11-14 - Prepare shader shear independently for each repeated instance.
 * 2026-11-15 - Reuse validated affine quads and sampling across both passes.
 * 2026-11-15 - Reuse plane recurrence maps and move bounded waves to sampling.
 * Apply existing repeat/span semantics using the same prepared source image.
 * Collect each instance before drawing, and reject overflowing spans before
 * submission so a rejected repeat never leaves a partially drawn image.
 */
static int gpu_render_image(gpu_image *target, gpu_image *source, gpu_image *mask,
                            const unsigned char *palette, int x, int y,
                            int centerx, int centery, int sprite,
                            const s_drawmethod *drawmethod)
{
    const s_drawmethod *method = drawmethod && (drawmethod->config & DRAWMETHOD_CONFIG_ENABLED)
                              ? drawmethod : &plainmethod;
    gpu_transform_parameters parameters;
    gpu_plane_map *plane_map = NULL;
    long long final_x, final_y;
    int i, j, submitted = 0;
    int use_affine = (method->config & DRAWMETHOD_CONFIG_AFFINE) != 0;
    int reuse_affine = 0;
    if(use_affine && !affine_supported)
    {
        native_failure = EP_FALLBACK_TRANSFORM;
        return 0;
    }
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
        native_failure = EP_FALLBACK_TRANSFORM;
        return 0;
    }
    parameters.source_width = source->width;
    parameters.source_height = source->height;
    parameters.target_width = target->width;
    parameters.target_height = target->height;
    parameters.centerx = centerx;
    parameters.centery = centery;
    parameters.is_sprite = sprite;

    /* Prepare intrinsic plane rows once for every repeated instance. Moving
     * or scrolling the image changes its uniforms, never this recurrence. */
    if(!use_affine && plane_supported && method->water.watermode == WATER_MODE_SHEAR &&
       method->water.beginsize > 0.0f && (!sprite || (method->scalex && method->scaley)))
    {
        ep_scope geometry_scope = engine_profile_span_begin();
        plane_map = gpu_prepare_plane_map(&parameters, method);
        engine_profile_span_end(geometry_scope, EP_SPAN_GPU_GEOMETRY);
    }

    /* Extreme affine inverses can be representable for one clipped instance
     * but not another. Validate every mapping before any destination write;
     * the common single-instance draw needs no duplicate setup. */
    if(use_affine && (method->xrepeat > 1 || method->yrepeat > 1))
    {
        ep_scope geometry_scope = engine_profile_span_begin();
        /* Failed reservation must not change draw eligibility. For large
         * grids, retain the old preflight and recompute each submitted quad. */
        if((size_t)method->xrepeat <= SIZE_MAX / (size_t)method->yrepeat)
        {
            size_t count = (size_t)method->xrepeat * (size_t)method->yrepeat;
            reuse_affine = gpu_reserve_affine_preparations(count);
        }
        for(j = 0; j < method->yrepeat; j++)
        {
            for(i = 0; i < method->xrepeat; i++)
            {
                gpu_prepared_affine *entry = reuse_affine
                    ? &affine_preparations[(size_t)j * method->xrepeat + i] : NULL;
                int result;
                parameters.x = (int)((long long)x + (long long)i * method->xspan);
                parameters.y = (int)((long long)y + (long long)j * method->yspan);
                result = entry
                    ? gpu_transform_draw_affine(&parameters, method, &entry->sampling,
                                                gpu_save_affine_quad, entry)
                    : gpu_transform_draw(&parameters, method, gpu_preflight_quad, NULL);
                if(entry)
                {
                    entry->result = result;
                }
                if(result == GPU_TRANSFORM_FAILED)
                {
                    engine_profile_span_end(geometry_scope, EP_SPAN_GPU_GEOMETRY);
                    native_failure = EP_FALLBACK_TRANSFORM;
                    return 0;
                }
            }
        }
        engine_profile_span_end(geometry_scope, EP_SPAN_GPU_GEOMETRY);
    }

    /* Submit repeats in their original order so overlapping instances blend
     * against prior ones. Each instance shares the source and scratch storage. */
    for(j = 0; j < method->yrepeat; j++)
    {
        for(i = 0; i < method->xrepeat; i++)
        {
            gpu_affine_sampling affine;
            gpu_affine_sampling *sampling = &affine;
            gpu_shear_sampling shear;
            gpu_water_sampling water;
            gpu_plane_sampling plane;
            ep_scope geometry_scope;
            int geometry_result;
            parameters.x = (int)((long long)x + (long long)i * method->xspan);
            parameters.y = (int)((long long)y + (long long)j * method->yspan);
            memset(&plane, 0, sizeof(plane));
            if(plane_map)
            {
                plane.rows = plane_map->rows;
                plane.valid_rows = plane_map->valid_rows;
            }
            quad_count = 0;
            geometry_scope = engine_profile_span_begin();
            if(reuse_affine)
            {
                gpu_prepared_affine *entry =
                    &affine_preparations[(size_t)j * method->xrepeat + i];
                sampling = &entry->sampling;
                geometry_result = entry->result;
                stats.affine_preparation_reuses++;
                if(geometry_result == GPU_TRANSFORM_DRAWN &&
                   !gpu_collect_quad(entry->vertices, GPU_SAMPLE_AFFINE, NULL))
                {
                    geometry_result = GPU_TRANSFORM_FAILED;
                }
            }
            else
            {
                geometry_result = use_affine
                    ? gpu_transform_draw_affine(&parameters, method, sampling, gpu_collect_quad, NULL)
                    : gpu_transform_draw_sampled(&parameters, method,
                                                 shear_supported ? &shear : NULL,
                                                 water_supported ? &water : NULL,
                                                 plane_map ? &plane : NULL,
                                                 gpu_collect_quad, NULL);
            }
            engine_profile_span_end(geometry_scope, EP_SPAN_GPU_GEOMETRY);
            if(geometry_result == GPU_TRANSFORM_DRAWN && plane.enabled && quad_count &&
               !gpu_upload_plane_map(plane_map))
            {
                /* Optional row-texture failure has not touched target pixels.
                 * Preserve the effect through its existing native geometry,
                 * without selecting CPU pixel rendering or changing draw order. */
                plane.enabled = 0;
                quad_count = 0;
                native_failure = EP_FALLBACK_OTHER;
                geometry_scope = engine_profile_span_begin();
                geometry_result = gpu_transform_draw_sampled(&parameters, method,
                                  shear_supported ? &shear : NULL,
                                  water_supported ? &water : NULL, NULL,
                                  gpu_collect_quad, NULL);
                engine_profile_span_end(geometry_scope, EP_SPAN_GPU_GEOMETRY);
            }
            if(geometry_result == GPU_TRANSFORM_FAILED && native_failure == EP_FALLBACK_OTHER)
            {
                native_failure = EP_FALLBACK_TRANSFORM;
            }
            if(geometry_result == GPU_TRANSFORM_FAILED ||
               !gpu_render_quads(target, source, mask, palette, method, sprite, 0, 0,
                                 use_affine ? sampling : NULL,
                                 !use_affine && shear_supported && shear.enabled ? &shear : NULL,
                                 !use_affine && water_supported && water.enabled ? &water : NULL,
                                 plane.enabled ? &plane : NULL,
                                 plane.enabled ? plane_map->texture : 0))
            {
                /* Earlier instances may already have changed the target.
                 * Only failure before any submission can safely return. */
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
 * 2026-11-15 - Allow verified full replacements to discard destination upload.
 * Draw one screen through the native RGB target path. Unsupported operations
 * return before writes, allowing dispatch to report the rejected request.
 */
static int gpu_screen_draw_internal(s_screen *dest, s_screen *src, int x, int y,
                                    const s_drawmethod *method, int discard_destination)
{
    gpu_image *target, *source;
    const unsigned char *palette;
    native_failure = EP_FALLBACK_OTHER;
    if(!dest || !src || dest->pixelformat != PIXEL_32)
    {
        native_failure = dest && src ? EP_FALLBACK_TARGET_FORMAT : EP_FALLBACK_OTHER;
        goto fallback;
    }
    if(src->width <= 0 || src->height <= 0)
    {
        return 1;
    }

    /* Ordinary draws need destination contents for preserved pixels or
     * blending. A strict full-screen replacement never samples those pixels,
     * so skip uploading any CPU changes that the copy will overwrite. */
    target = gpu_screen_image(dest, !discard_destination);
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
    /* Reaching here guarantees that this draw changed no pixels. Native-only
     * dispatch reports the rejection without retrying RGB32 drawing on CPU. */
    engine_profile_fallback(native_failure);
    stats.fallback_draws++;
    return 0;
}

/*
 * Damon V. Caskey
 * 2026-11-15
 * Ordinary screen draws preserve prior destination pixels for clipping and
 * material effects. Only the verified screen-copy entry can discard them.
 */
static int gpu_screen_draw(s_screen *dest, s_screen *src, int x, int y,
                           const s_drawmethod *method)
{
    return gpu_screen_draw_internal(dest, src, x, y, method, 0);
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
    native_failure = EP_FALLBACK_OTHER;
    if(!dest || !sprite || dest->pixelformat != PIXEL_32)
    {
        native_failure = dest && sprite ? EP_FALLBACK_TARGET_FORMAT : EP_FALLBACK_OTHER;
        goto fallback;
    }
    if(sprite->width <= 0 || sprite->height <= 0)
    {
        return 1;
    }
    if(sprite->pixelformat != PIXEL_x8 && sprite->pixelformat != PIXEL_8)
    {
        native_failure = EP_FALLBACK_SOURCE_FORMAT;
        goto fallback;
    }
    if(sprite->mask && (sprite->mask->width != sprite->width || sprite->mask->height != sprite->height))
    {
        native_failure = EP_FALLBACK_MASK;
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
    engine_profile_fallback(native_failure);
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
 * 2026-11-15 - Leave optional row sampling disabled for solid primitives.
 * Render box and dot primitives in target pixel coordinates, retaining
 * their existing clipping, tint and destination-blend behavior.
 */
static int gpu_box_draw(s_screen *dest, int x, int y, int width, int height,
                        unsigned color, const s_drawmethod *drawmethod)
{
    gpu_image *target;
    const s_drawmethod *method = drawmethod && (drawmethod->config & DRAWMETHOD_CONFIG_ENABLED)
                              ? drawmethod : &plainmethod;
    native_failure = EP_FALLBACK_OTHER;
    if(!dest || dest->pixelformat != PIXEL_32)
    {
        native_failure = dest ? EP_FALLBACK_TARGET_FORMAT : EP_FALLBACK_OTHER;
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
    if(target)
    {
        ep_scope geometry_scope = engine_profile_span_begin();
        int geometry_ready = gpu_collect_rectangle((float)x, (float)y, (float)width, (float)height);
        engine_profile_span_end(geometry_scope, EP_SPAN_GPU_GEOMETRY);
        if(geometry_ready && gpu_render_quads(target, NULL, NULL, NULL, method, 0, 1, color,
                                             NULL, NULL, NULL, NULL, 0))
        {
            return 1;
        }
    }
fallback:
    engine_profile_fallback(native_failure);
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
 * 2026-11-14 - Rasterize horizontal and vertical lines as one rectangle.
 * 2026-11-15 - Leave optional row sampling disabled for line primitives.
 * Retain the engine's integer line coverage while preparing one GPU draw.
 * Axis-aligned lines use inclusive rectangles; slopes retain visible points.
 * Material processing and framebuffer access remain on the GPU.
 */
static int gpu_line_draw(s_screen *dest, int sx, int sy, int ex, int ey,
                         unsigned color, const s_drawmethod *drawmethod)
{
    gpu_image *target;
    int dx, dy, step_x, step_y, threshold, geometry_ready;
    ep_scope geometry_scope = {0, 0};
    const s_drawmethod *method = drawmethod && (drawmethod->config & DRAWMETHOD_CONFIG_ENABLED)
                              ? drawmethod : &plainmethod;
    native_failure = EP_FALLBACK_OTHER;
    if(!dest || dest->pixelformat != PIXEL_32)
    {
        native_failure = dest ? EP_FALLBACK_TARGET_FORMAT : EP_FALLBACK_OTHER;
        goto fallback;
    }
    geometry_scope = engine_profile_span_begin();
    geometry_ready = gpu_clip_line(&sx, &sy, &ex, &ey, dest->width, dest->height);
    engine_profile_span_end(geometry_scope, EP_SPAN_GPU_GEOMETRY);
    geometry_scope.frame_id = 0;
    if(!geometry_ready)
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
    quad_count = 0;
    geometry_scope = engine_profile_span_begin();

    if(!dx || !dy)
    {
        /* Clipped endpoints bound each inclusive extent to the target size.
         * One rectangle covers the same pixels once in either direction,
         * including a zero-length point, without CPU pixel enumeration. */
        geometry_ready = gpu_collect_rectangle((float)(sx < ex ? sx : ex),
                                                (float)(sy < ey ? sy : ey),
                                                (float)(dx + 1), (float)(dy + 1));
    }
    else
    {
        step_x = ex < sx ? -1 : 1;
        step_y = ey < sy ? -1 : 1;

        /* Sloped lines retain the engine's integer error threshold. Collect
         * every point before drawing so allocation failure leaves the target
         * unchanged when dispatch reports the rejected request. */
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
        /* Both axis loops stop before the endpoint. Append it explicitly
         * before the shared material, clipping, and submission path. */
        geometry_ready = gpu_collect_rectangle((float)ex, (float)ey, 1.0f, 1.0f);
    }
    engine_profile_span_end(geometry_scope, EP_SPAN_GPU_GEOMETRY);
    geometry_scope.frame_id = 0;
    if(geometry_ready && gpu_render_quads(target, NULL, NULL, NULL, method, 0, 1, color,
                                         NULL, NULL, NULL, NULL, 0))
    {
        return 1;
    }
fallback:
    /* Failed collection still consumed preparation time. Clearing frame_id
     * above ensures paths that already ended the span cannot charge it twice. */
    engine_profile_span_end(geometry_scope, EP_SPAN_GPU_GEOMETRY);
    engine_profile_fallback(native_failure);
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
    uint64_t previous_state_changes = stats.state_changes;
    uint64_t previous_state_reuses = stats.state_reuses;
    ep_scope state_scope, submit_scope;
    native_failure = EP_FALLBACK_OTHER;
    if(!screen || screen->pixelformat != PIXEL_32)
    {
        engine_profile_fallback(screen ? EP_FALLBACK_TARGET_FORMAT : EP_FALLBACK_OTHER);
        return 0;
    }

    /* Full replacement discards the previous screen version, including any
     * pending CPU edits. Allocation can therefore skip the old-image upload. */
    target = gpu_screen_image(screen, 0);
    if(!target)
    {
        engine_profile_fallback(native_failure);
        return 0;
    }
    state_scope = engine_profile_span_begin();
    gpu_bind_framebuffer(target->framebuffer);
    gpu_set_scissor_enabled(0);
    gpu_set_color_mask();
    gpu_set_clear_color();
    engine_profile_span_end(state_scope, EP_SPAN_GPU_STATE);
    submit_scope = engine_profile_span_begin();
    glClear(GL_COLOR_BUFFER_BIT);
    if(glGetError() != GL_NO_ERROR)
    {
        gpu_fatal("screen clear");
    }
    engine_profile_span_end(submit_scope, EP_SPAN_GPU_SUBMIT);
    engine_profile_count(EP_COUNT_NATIVE_CLEARS, 1);
    engine_profile_count(EP_COUNT_GPU_STATE_CHANGES, stats.state_changes - previous_state_changes);
    engine_profile_count(EP_COUNT_GPU_STATE_REUSES, stats.state_reuses - previous_state_reuses);
    target->gpu_dirty = 1;
    target->cpu_dirty = 0;
    return 1;
}

/*
 * Damon V. Caskey
 * 2026-11-12
 * 2026-11-15 - Bypass obsolete destination uploads for complete RGB32 copies.
 * Copy the overlapping screen area with replacement semantics, independent
 * of any previously selected drawing method or clipping globals.
 */
static int gpu_screen_copy(s_screen *dest, s_screen *src)
{
    s_drawmethod method = plainmethod;
    if(!dest || !src || dest->pixelformat != src->pixelformat)
    {
        engine_profile_fallback(dest && src ? EP_FALLBACK_SOURCE_FORMAT : EP_FALLBACK_OTHER);
        return 0;
    }
    if(dest == src)
    {
        return 1;
    }
    method.alpha = 0;
    /* Partial copies preserve uncovered pixels. Distinct RGB32 sources
     * covering both destination axes replace every pixel, including black,
     * with no blending, clipping override or transparent-color rejection. */
    return gpu_screen_draw_internal(dest, src, 0, 0, &method,
                                    dest->pixelformat == PIXEL_32 &&
                                    src->width >= dest->width &&
                                    src->height >= dest->height);
}

/*
 * Damon V. Caskey
 * 2026-11-16
 * Compile the common output-filter equations only when an active filter is
 * presented. Failure disables this pass without retiring scene resources.
 */
static int gpu_output_filter_program_ready(void)
{
    GLuint vertex = 0, fragment = 0;
    GLint linked = 0;
    char message[4096];
    if(output_filter_program) return 1;
    if(output_filter_program_attempted) return 0;
    output_filter_program_attempted = 1;
    vertex = gpu_compile_shader(GL_VERTEX_SHADER, gpu_vertex_shader_source);
    fragment = gpu_compile_shader(GL_FRAGMENT_SHADER, gpu_output_filter_fragment_shader_source);
    if(!vertex || !fragment) goto failed;
    output_filter_program = gpu_CreateProgram();
    if(!output_filter_program) goto failed;
    gpu_AttachShader(output_filter_program, vertex);
    gpu_AttachShader(output_filter_program, fragment);
    gpu_BindAttribLocation(output_filter_program, GPU_POSITION_ATTRIBUTE, "vertexPosition");
    gpu_BindAttribLocation(output_filter_program, GPU_TEXCOORD_ATTRIBUTE, "vertexTexcoord");
    gpu_LinkProgram(output_filter_program);
    gpu_GetProgramiv(output_filter_program, GL_LINK_STATUS, &linked);
    if(!linked)
    {
        message[0] = '\0';
        gpu_GetProgramInfoLog(output_filter_program, sizeof(message), NULL, message);
        message[sizeof(message) - 1] = '\0';
        gpu_log("WARNING: Native output-filter shader link failed.\n\tDriver diagnostic:\n%s",
                message[0] ? message : "not provided");
        goto failed;
    }
#define GPU_OUTPUT_FILTER_UNIFORM(member, name) \
    do { output_filter_uniforms.member = gpu_GetUniformLocation(output_filter_program, name); \
         if(output_filter_uniforms.member < 0) goto failed; } while(0)
    GPU_OUTPUT_FILTER_UNIFORM(target_size, "targetSize");
    GPU_OUTPUT_FILTER_UNIFORM(source_size, "sourceSize");
    GPU_OUTPUT_FILTER_UNIFORM(source, "sourceTexture");
    GPU_OUTPUT_FILTER_UNIFORM(mode, "filterMode");
#undef GPU_OUTPUT_FILTER_UNIFORM
    if(glGetError() != GL_NO_ERROR) goto failed;
    gpu_DeleteShader(vertex);
    gpu_DeleteShader(fragment);
    gpu_log("Native output filters enabled: six RGB8 filters, one reusable 2x RGBA8 target, no scene readback.");
    return 1;
failed:
    if(vertex) gpu_DeleteShader(vertex);
    if(fragment) gpu_DeleteShader(fragment);
    if(output_filter_program) gpu_DeleteProgram(output_filter_program);
    output_filter_program = 0;
    native_failure = EP_FALLBACK_RESOURCE;
    gpu_log("WARNING: Native output-filter shader unavailable; filtered presentation is disabled without CPU retry.");
    gpu_gl_invalidate_state();
    return 0;
}

/*
 * Damon V. Caskey
 * 2026-11-16
 * Reuse one exact-size intermediate across frames and filter choices. Build
 * replacements completely before retiring the previous usable allocation.
 */
static int gpu_output_filter_target_ready(int width, int height)
{
    GLuint texture = 0, framebuffer = 0;
    GLenum status, error;
    if(output_filter_texture && output_filter_framebuffer &&
       output_filter_width == width && output_filter_height == height) return 1;
    texture = gpu_new_texture(width, height, GL_RGBA8, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    if(!texture) goto failed;
    gpu_GenFramebuffers(1, &framebuffer);
    if(!framebuffer) goto failed;
    gpu_bind_framebuffer(framebuffer);
    gpu_FramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture, 0);
    status = gpu_CheckFramebufferStatus(GL_FRAMEBUFFER);
    error = glGetError();
    if(status != GL_FRAMEBUFFER_COMPLETE || error != GL_NO_ERROR) goto failed;

    /* Deleting old names invalidates binding assumptions even if the driver
     * immediately reuses those names for another scene or presentation owner. */
    gpu_gl_invalidate_state();
    if(output_filter_framebuffer) gpu_DeleteFramebuffers(1, &output_filter_framebuffer);
    if(output_filter_texture) glDeleteTextures(1, &output_filter_texture);
    output_filter_texture = texture;
    output_filter_framebuffer = framebuffer;
    output_filter_width = width;
    output_filter_height = height;
    ++stats.output_filter_allocations;
    stats.output_filter_resources = 1;
    return 1;
failed:
    gpu_gl_invalidate_state();
    if(framebuffer) gpu_DeleteFramebuffers(1, &framebuffer);
    if(texture) glDeleteTextures(1, &texture);
    native_failure = EP_FALLBACK_RESOURCE;
    return 0;
}

/*
 * Damon V. Caskey
 * 2026-11-16
 * Retire the recreatable presentation target without downloading its pixels.
 * Context replacement recreates both the program and target on first use.
 */
static void gpu_output_filter_shutdown(void)
{
    gpu_gl_invalidate_state();
    if(output_filter_framebuffer) gpu_DeleteFramebuffers(1, &output_filter_framebuffer);
    if(output_filter_texture) glDeleteTextures(1, &output_filter_texture);
    if(output_filter_program) gpu_DeleteProgram(output_filter_program);
    output_filter_framebuffer = output_filter_texture = output_filter_program = 0;
    output_filter_width = output_filter_height = 0;
    output_filter_program_attempted = output_filter_warning = 0;
    stats.output_filter_resources = 0;
    memset(&output_filter_uniforms, 0, sizeof(output_filter_uniforms));
}

/*
 * Damon V. Caskey
 * 2026-11-15
 * Compile the shared movie program on first use. A rejected movie shader
 * leaves scene rendering available and remains rejected for this context.
 */
static int gpu_movie_program_ready(void)
{
    GLuint vertex = 0, fragment = 0;
    GLint linked = 0;
    char message[4096];
    if(movie_program) return 1;
    if(movie_program_attempted) return 0;
    movie_program_attempted = 1;
    vertex = gpu_compile_shader(GL_VERTEX_SHADER, gpu_vertex_shader_source);
    fragment = gpu_compile_shader(GL_FRAGMENT_SHADER, gpu_movie_fragment_shader_source);
    if(!vertex || !fragment) goto failed;
    movie_program = gpu_CreateProgram();
    if(!movie_program) goto failed;
    gpu_AttachShader(movie_program, vertex);
    gpu_AttachShader(movie_program, fragment);
    gpu_BindAttribLocation(movie_program, GPU_POSITION_ATTRIBUTE, "vertexPosition");
    gpu_BindAttribLocation(movie_program, GPU_TEXCOORD_ATTRIBUTE, "vertexTexcoord");
    gpu_LinkProgram(movie_program);
    gpu_GetProgramiv(movie_program, GL_LINK_STATUS, &linked);
    if(!linked)
    {
        message[0] = '\0';
        gpu_GetProgramInfoLog(movie_program, sizeof(message), NULL, message);
        message[sizeof(message) - 1] = '\0';
        gpu_log("WARNING: Native movie shader link failed.\n\tDriver diagnostic:\n%s",
                message[0] ? message : "not provided");
        goto failed;
    }
#define GPU_MOVIE_UNIFORM(member, name) \
    do { movie_uniforms.member = gpu_GetUniformLocation(movie_program, name); \
         if(movie_uniforms.member < 0) goto failed; } while(0)
    GPU_MOVIE_UNIFORM(target_size, "targetSize");
    GPU_MOVIE_UNIFORM(source_size, "sourceSize");
    GPU_MOVIE_UNIFORM(y, "yTexture");
    GPU_MOVIE_UNIFORM(u, "uTexture");
    GPU_MOVIE_UNIFORM(v, "vTexture");
    GPU_MOVIE_UNIFORM(resample, "resampleMode");
    GPU_MOVIE_UNIFORM(black_filter, "blackFilter");
    GPU_MOVIE_UNIFORM(gamma, "movieGamma");
    GPU_MOVIE_UNIFORM(brightness, "movieBrightness");
#undef GPU_MOVIE_UNIFORM
    if(glGetError() != GL_NO_ERROR) goto failed;
    gpu_DeleteShader(vertex);
    gpu_DeleteShader(fragment);
    gpu_log("Native movie rendering enabled: retained YUV420 planes, direct GPU conversion and static resampling.");
    return 1;
failed:
    if(vertex) gpu_DeleteShader(vertex);
    if(fragment) gpu_DeleteShader(fragment);
    if(movie_program) gpu_DeleteProgram(movie_program);
    movie_program = 0;
    native_failure = EP_FALLBACK_RESOURCE;
    gpu_log("WARNING: Native movie shader unavailable; movie drawing is disabled without CPU conversion or rendering retry.");
    gpu_gl_invalidate_state();
    return 0;
}

/*
 * Damon V. Caskey
 * 2026-11-15
 * Delete one decoder owner's retained planes before its identity is reused.
 * No color conversion or readback is needed to discard a recreatable source.
 */
static void gpu_movie_release(const void *owner)
{
    gpu_movie_source **link = &movie_sources;
    while(*link)
    {
        gpu_movie_source *entry = *link;
        if(entry->owner == owner)
        {
            *link = entry->next;
            gpu_gl_invalidate_state();
            glDeleteTextures(3, entry->texture);
            free(entry);
            --stats.movie_resources;
            return;
        }
        link = &entry->next;
    }
}

/*
 * Damon V. Caskey
 * 2026-11-15
 * Retire all movie resources while their GL context still exists. Decoder
 * frames remain CPU-owned and can repopulate a replacement context lazily.
 */
static void gpu_movie_shutdown(void)
{
    while(movie_sources) gpu_movie_release(movie_sources->owner);
    if(movie_program) gpu_DeleteProgram(movie_program);
    movie_program = 0;
    movie_program_attempted = movie_present_warning = 0;
    memset(&movie_uniforms, 0, sizeof(movie_uniforms));
}

/*
 * Damon V. Caskey
 * 2026-11-15
 * Reuse one complete uploaded frame per stable owner and generation. Source
 * addresses are borrowed only for these synchronous, tightly packed uploads.
 */
static gpu_movie_source *gpu_movie_upload(const gpu_yuv_source *source)
{
    gpu_movie_source *entry = movie_sources;
    const unsigned char *planes[3] = { source->y, source->u, source->v };
    int plane;
    ep_scope upload_scope;
    while(entry && entry->owner != source->owner) entry = entry->next;
    if(entry && (entry->width != source->width || entry->height != source->height))
    {
        gpu_movie_release(source->owner);
        entry = NULL;
    }
    if(!entry)
    {
        entry = (gpu_movie_source *)calloc(1, sizeof(*entry));
        if(!entry)
        {
            native_failure = EP_FALLBACK_RESOURCE;
            return NULL;
        }
        entry->owner = source->owner;
        entry->width = source->width;
        entry->height = source->height;
        for(plane = 0; plane < 3; ++plane)
        {
            int width = source->width / (plane ? 2 : 1);
            int height = source->height / (plane ? 2 : 1);
            entry->texture[plane] = gpu_new_texture(width, height, GL_LUMINANCE8,
                                                    GL_LUMINANCE, GL_UNSIGNED_BYTE, NULL);
            if(!entry->texture[plane]) break;
            /* Linear plane filtering is the original hardware movie policy.
             * Other kernels sample exact centers through this same storage. */
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            if(glGetError() != GL_NO_ERROR) break;
        }
        if(plane != 3)
        {
            gpu_gl_invalidate_state();
            glDeleteTextures(3, entry->texture);
            free(entry);
            native_failure = EP_FALLBACK_RESOURCE;
            return NULL;
        }
        entry->next = movie_sources;
        movie_sources = entry;
        ++stats.movie_resources;
    }
    if(entry->uploaded && entry->generation == source->generation)
    {
        ++stats.movie_cache_hits;
        return entry;
    }

    /* An interrupted update must never publish a mixture of two frames.
     * Retrying reuploads all planes before this generation becomes visible. */
    entry->uploaded = 0;
    upload_scope = engine_profile_span_begin();
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
    glPixelStorei(GL_UNPACK_SKIP_ROWS, 0);
    glPixelStorei(GL_UNPACK_SKIP_PIXELS, 0);
    for(plane = 0; plane < 3; ++plane)
    {
        int width = source->width / (plane ? 2 : 1);
        int height = source->height / (plane ? 2 : 1);
        gpu_edit_texture(plane, entry->texture[plane]);
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width, height,
                       GL_LUMINANCE, GL_UNSIGNED_BYTE, planes[plane]);
        if(glGetError() != GL_NO_ERROR)
        {
            engine_profile_span_end(upload_scope, EP_SPAN_UPLOAD);
            native_failure = EP_FALLBACK_RESOURCE;
            gpu_gl_invalidate_state();
            return NULL;
        }
        ++stats.uploads;
        engine_profile_count(EP_COUNT_IMAGE_UPLOADS, 1);
        engine_profile_count(EP_COUNT_IMAGE_UPLOAD_BYTES, (uint64_t)width * height);
    }
    engine_profile_span_end(upload_scope, EP_SPAN_UPLOAD);
    entry->generation = source->generation;
    entry->uploaded = 1;
    ++stats.movie_uploads;
    return entry;
}

/*
 * Damon V. Caskey
 * 2026-11-15
 * Validate decoded storage and every movie draw value before GL allocation or
 * target ownership changes. Movie color stays RGB32 and decoder planes 8-bit.
 */
static int gpu_movie_parameters_valid(const gpu_yuv_source *source,
                                       const gpu_movie_draw *draw)
{
    if(!initialized || !source || !draw || !source->owner ||
       !source->y || !source->u || !source->v ||
       source->width < 2 || source->height < 2 ||
       source->width > maximum_texture_size || source->height > maximum_texture_size ||
       (source->width & 1) || (source->height & 1) ||
       draw->width <= 0 || draw->height <= 0 ||
       draw->resample < DC_RESAMPLE_NEAREST || draw->resample >= DC_RESAMPLE_END ||
       !isfinite(draw->gamma) || !isfinite(draw->brightness) ||
       draw->gamma < -1.0f || draw->gamma > 1.0f ||
       draw->brightness < -1.0f || draw->brightness > 1.0f)
    {
        native_failure = EP_FALLBACK_SOURCE_FORMAT;
        return 0;
    }
    return 1;
}

/*
 * Damon V. Caskey
 * 2026-11-15
 * Clip in double precision before making float vertices, retaining signed
 * placement and extreme requested extents without overflowing integer sums.
 * Window coordinates preserve the old projection and reversed movie Y axis.
 */
static int gpu_movie_vertices(const gpu_yuv_source *source, const gpu_movie_draw *draw,
                               int width, int height, int window_target,
                               gpu_vertex vertices[4])
{
    double left = draw->x, bottom = draw->y;
    double right = left + draw->width, top = bottom + draw->height;
    double clipped_left, clipped_right, clipped_bottom, clipped_top;
    double u0, u1, v0, v1;
    if(window_target)
    {
        /* The old projection has no defined span for a one-pixel axis.
         * Use ordinary pixel edges on that axis so tiny windows still draw. */
        double inset_x = width > 1 ? 1.0 : 0.0;
        double inset_y = height > 1 ? 1.0 : 0.0;
        double scale_x = width > 1 ? (double)width / (width - 1) : 1.0;
        double scale_y = height > 1 ? (double)height / (height - 1) : 1.0;
        right = (right - inset_x) * scale_x;
        top = (top - inset_y) * scale_y;
        left *= scale_x;
        bottom *= scale_y;
    }
    if(right <= left || top <= bottom) return 0;
    clipped_left = left < 0.0 ? 0.0 : left;
    clipped_bottom = bottom < 0.0 ? 0.0 : bottom;
    clipped_right = right > width ? width : right;
    clipped_top = top > height ? height : top;
    if(clipped_left >= clipped_right || clipped_bottom >= clipped_top) return 0;
    u0 = (clipped_left - left) / (right - left) * source->width;
    u1 = (clipped_right - left) / (right - left) * source->width;
    v0 = (clipped_bottom - bottom) / (top - bottom) * source->height;
    v1 = (clipped_top - bottom) / (top - bottom) * source->height;
    if(window_target)
    {
        v0 = source->height - v0;
        v1 = source->height - v1;
    }
    vertices[0] = (gpu_vertex){ (float)clipped_left, (float)clipped_bottom, (float)u0, (float)v0 };
    vertices[1] = (gpu_vertex){ (float)clipped_right, (float)clipped_bottom, (float)u1, (float)v0 };
    vertices[2] = (gpu_vertex){ (float)clipped_right, (float)clipped_top, (float)u1, (float)v1 };
    vertices[3] = (gpu_vertex){ (float)clipped_left, (float)clipped_top, (float)u0, (float)v1 };
    return 1;
}

/*
 * Damon V. Caskey
 * 2026-11-15
 * Draw filtered decoder planes directly into an existing RGB target or the
 * window. Resource and state preparation finish before the first pixel write.
 */
static int gpu_movie_render(s_screen *destination, const gpu_yuv_source *source,
                             const gpu_movie_draw *draw, int width, int height)
{
    gpu_movie_source *movie = NULL;
    gpu_image *target = NULL;
    gpu_vertex vertices[4];
    int visible, discard_destination;
    size_t stream_first = 0;
    ep_scope state_scope, submit_scope;
    uint64_t previous_state_changes = stats.state_changes;
    uint64_t previous_state_reuses = stats.state_reuses;
    native_failure = EP_FALLBACK_OTHER;
    if(!gpu_movie_parameters_valid(source, draw)) goto failed;
    if(width <= 0 || height <= 0 ||
       (destination && (destination->pixelformat != PIXEL_32 ||
                        width > maximum_texture_size || height > maximum_texture_size)))
    {
        native_failure = EP_FALLBACK_TARGET_FORMAT;
        goto failed;
    }
    visible = gpu_movie_vertices(source, draw, width, height, !destination, vertices);
    if(!visible && destination) return 1;
    if(!gpu_movie_program_ready()) goto failed;
    if(visible)
    {
        movie = gpu_movie_upload(source);
        if(!movie) goto failed;
        quad_count = 0;
        if(!gpu_collect_quad(vertices, 0, NULL)) goto failed;
        submit_scope = engine_profile_span_begin();
        if(!gpu_upload_geometry(&stream_first))
        {
            engine_profile_span_end(submit_scope, EP_SPAN_GPU_SUBMIT);
            goto failed;
        }
        engine_profile_span_end(submit_scope, EP_SPAN_GPU_SUBMIT);
    }
    if(destination)
    {
        discard_destination = draw->x <= 0 && draw->y <= 0 &&
                              (int64_t)draw->x + draw->width >= width &&
                              (int64_t)draw->y + draw->height >= height;
        target = gpu_screen_image(destination, !discard_destination);
        if(!target) goto failed;
    }

    state_scope = engine_profile_span_begin();
    gpu_bind_framebuffer(target ? target->framebuffer : 0);
    gpu_set_viewport(width, height);
    gpu_set_scissor_enabled(0);
    gpu_set_draw_raster();
    gpu_set_color_mask();
    if(!destination) gpu_set_clear_color();
    if(visible)
    {
        gpu_UseProgram(movie_program);
        gl_state.program = movie_program;
        gl_state.program_valid = 1;
        ++stats.state_changes;
        gpu_bind_texture(0, movie->texture[0]);
        gpu_bind_texture(1, movie->texture[1]);
        gpu_bind_texture(2, movie->texture[2]);
        gpu_select_texture_unit(0);
        gpu_bind_geometry();
        gpu_Uniform2f(movie_uniforms.target_size, (float)width, (float)height);
        gpu_Uniform2f(movie_uniforms.source_size, (float)source->width, (float)source->height);
        gpu_Uniform1i(movie_uniforms.y, 0);
        gpu_Uniform1i(movie_uniforms.u, 1);
        gpu_Uniform1i(movie_uniforms.v, 2);
        gpu_Uniform1i(movie_uniforms.resample, draw->resample);
        gpu_Uniform1i(movie_uniforms.black_filter, draw->black_filter != 0);
        gpu_Uniform1f(movie_uniforms.gamma, draw->gamma);
        gpu_Uniform1f(movie_uniforms.brightness, draw->brightness);
        stats.state_changes += 9;
    }
    if(glGetError() != GL_NO_ERROR)
    {
        engine_profile_span_end(state_scope, EP_SPAN_GPU_STATE);
        native_failure = EP_FALLBACK_RESOURCE;
        gpu_gl_invalidate_state();
        goto failed;
    }
    engine_profile_span_end(state_scope, EP_SPAN_GPU_STATE);

    submit_scope = engine_profile_span_begin();
    if(!destination) glClear(GL_COLOR_BUFFER_BIT);
    if(visible)
    {
        gpu_DrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT,
                         (const void *)(uintptr_t)(stream_first * 6 * sizeof(GLuint)));
    }
    if(glGetError() != GL_NO_ERROR) gpu_fatal("movie draw");
    engine_profile_span_end(submit_scope, EP_SPAN_GPU_SUBMIT);
    if(target)
    {
        target->gpu_dirty = 1;
        target->cpu_dirty = 0;
    }
    if(visible)
    {
        ++stats.movie_draws;
        ++stats.draws;
        engine_profile_count(EP_COUNT_NATIVE_DRAWS, 1);
        engine_profile_count(EP_COUNT_NATIVE_QUADS, 1);
        engine_profile_count(EP_COUNT_NATIVE_BATCHES, 1);
    }
    engine_profile_count(EP_COUNT_GPU_STATE_CHANGES, stats.state_changes - previous_state_changes);
    engine_profile_count(EP_COUNT_GPU_STATE_REUSES, stats.state_reuses - previous_state_reuses);
    return 1;
failed:
    engine_profile_fallback(native_failure);
    ++stats.fallback_draws;
    return 0;
}

/*
 * Damon V. Caskey
 * 2026-11-15
 * Preserve caller-owned screen layering: only the clipped movie rectangle
 * changes, with no global drawmethod, implicit clear, or CPU pixel access.
 */
static int gpu_movie_draw_screen(s_screen *destination, const gpu_yuv_source *source,
                                  const gpu_movie_draw *draw)
{
    if(!destination) return 0;
    return gpu_movie_render(destination, source, draw, destination->width, destination->height);
}

/*
 * Damon V. Caskey
 * 2026-11-15
 * Present at the window's native resolution while keeping scene rendering
 * active. Border clearing occurs only after all required setup succeeds.
 */
int gpu_gl_present_yuv(const gpu_yuv_source *source, const gpu_movie_draw *draw,
                       int viewport_width, int viewport_height)
{
    int result;
    gpu_gl_prepare_present();
    result = gpu_movie_render(NULL, source, draw, viewport_width, viewport_height);
    if(!result && !movie_present_warning)
    {
        movie_present_warning = 1;
        gpu_log("WARNING: Native movie presentation unavailable or rejected; presentation skipped without CPU retry.");
    }
    return result;
}


static const gpu_backend renderer_backend = {
    gpu_screen_draw, gpu_sprite_draw, gpu_box_draw, gpu_line_draw,
    gpu_screen_clear, gpu_screen_copy, gpu_read_screen, gpu_dirty_screen,
    gpu_free_screen, gpu_free_sprite, gpu_dirty_tables,
    gpu_movie_draw_screen, gpu_movie_release
};

/*
 * Damon V. Caskey
 * 2026-11-13
 * 2026-11-13 - Present probe success as a readable startup checklist item.
 * Prove the RGB32 render-target format works before advertising native scene
 * drawing. Entry points alone cannot establish framebuffer completeness.
 */
static int gpu_probe_rgb32_target(void)
{
    GLuint framebuffer = 0;
    GLenum status = 0, error, cleanup_error;

    /* The shared white texel already has the required RGBA8 storage. Attaching
     * it temporarily requires no scene owner, pixel transfer, clear or draw. */
    gpu_GenFramebuffers(1, &framebuffer);
    if(framebuffer)
    {
        gpu_BindFramebuffer(GL_FRAMEBUFFER, framebuffer);
        gpu_FramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                                GL_TEXTURE_2D, white_texture, 0);
        status = gpu_CheckFramebufferStatus(GL_FRAMEBUFFER);
    }
    error = glGetError();

    /* Detach the probe from the active context before deleting it, including
     * failed probes. Subsequent owners must restore their own cached state. */
    gpu_BindFramebuffer(GL_FRAMEBUFFER, 0);
    if(framebuffer)
    {
        gpu_DeleteFramebuffers(1, &framebuffer);
    }
    gpu_gl_invalidate_state();
    cleanup_error = glGetError();
    if(error == GL_NO_ERROR)
    {
        error = cleanup_error;
    }
    if(!framebuffer || status != GL_FRAMEBUFFER_COMPLETE || error != GL_NO_ERROR)
    {
        gpu_log("WARNING: RGB32 framebuffer probe failed.\n"
                "\tFramebuffer status:                0x%x\n"
                "\tOpenGL error:                      0x%x",
                (unsigned)status, (unsigned)error);
        return 0;
    }
    return 1;
}

/*
 * Damon V. Caskey
 * 2026-11-13
 * 2026-11-13 - Separate supported work from conditional fallback information.
 * 2026-11-14 - Report native-only color rules and explicit ignore behavior.
 * 2026-11-14 - Identify native horizontal/vertical line rectangle coverage.
 * 2026-11-14 - Report the initialized buffered triangle submission path.
 * 2026-11-14 - Distinguish shader shear from retained CPU row preparation.
 * 2026-11-15 - Report rejected RGB32 operations without software retry.
 * 2026-11-15 - Identify RGB32 as the sole scene format.
 * 2026-11-15 - Report water sampling and compact plane recurrence support.
 * 2026-11-15 - Report shared movie reconstruction and lazy shader validation.
 * 2026-11-16 - Report shared RGB8 output filters and their GPU-only intermediate.
 * Close every startup capability report with its actual drawing path and
 * performance consequences, independently of optional profiling records.
 */
static void gpu_log_capability_result(int native_available)
{
    int native_colors = native_available && color_rules_supported;
    if(native_available)
    {
        if(affine_supported)
        {
            gpu_log("RESULT: NATIVE - native scene rendering and affine transforms available.");
        }
        else
        {
            gpu_log("RESULT: LIMITED NATIVE - native scene rendering available; affine transforms unavailable.");
            gpu_log("WARNING: Affine draws are rejected; RGB32 drawing has no software retry.");
        }
        /* Startup validates this backend, not every future resource or effect.
         * State these restrictions before gameplay can encounter a rejection. */
        gpu_log("Native Drawing Scope\n"
                "\tDestination format:                RGB32\n"
                "\tSource formats:                    Indexed and RGB32\n"
                "\tDraw methods:                      Supported native operations\n"
                "\tBuffered submission:               NATIVE - indexed triangles\n"
                "\tHorizontal/vertical lines:         NATIVE - one rectangle per visible line\n");
        gpu_log("Native Drawing Limits\n"
                "\tThese RGB32 operations are rejected without software retry:\n"
                "\t  - Unsupported draw methods\n"
                "\t  - Numerically extreme transforms\n"
                "\t  - Images larger than %d pixels on either axis\n"
                "\t  - Resource allocation failures\n"
                "\tScene format:                      RGB32 only; indexed artwork is source data\n",
                maximum_texture_size);
    }
    else
    {
        gpu_log("RESULT: UNAVAILABLE - native scene rendering unavailable; RGB32 drawing is disabled.");
        gpu_log("WARNING: RGB32 scene drawing requires the GPU; no software retry is available.");
    }
    gpu_log("Legacy Shear\n"
            "\tLegacy shear setup:                %s\n"
            "\tNearest sampling:                  Existing 8.8 rounding and clipped source phase\n"
            "\tManual affine:                     Takes priority over legacy shear\n"
            "\tImages exceeding 65535 per axis:    CPU row geometry; native pixels when otherwise supported\n",
            !native_available ? "UNAVAILABLE - RGB32 drawing disabled" :
            shear_supported ? "NATIVE - shader sampling, one quad" :
                              "CPU - row geometry; native pixels");
    if(native_available && !shear_supported)
    {
        gpu_log("WARNING: Legacy shear shader unavailable; retaining CPU row geometry with native pixel rendering.");
    }
    gpu_log("Water and Plane Effects\n"
            "\tSine water sampling:               %s\n"
            "\tPlane row sampling:                %s\n"
            "\tWave accuracy:                    Quantized phase; bounded one-pixel horizontal approximation\n"
            "\tExtreme wave parameters:          CPU row geometry; native pixels\n"
            "\tPlane preparation:                Exact CPU recurrence cached independently of position/scroll\n"
            "\tNearest plane accuracy:           Exact source row; bounded one-source-texel horizontal rounding\n"
            "\tPlane data transfer:              16 bytes per row once per resident map\n",
            native_available && water_supported ? "NATIVE - shader sampling, one quad" :
                native_available ? "CPU - row geometry; native pixels" : "UNAVAILABLE",
            native_available && plane_supported ? "NATIVE - cached rows, one quad" :
                native_available ? "CPU - row geometry; native pixels" : "UNAVAILABLE");
    if(native_available && !water_supported)
    {
        gpu_log("WARNING: Water sampling shader unavailable; retaining CPU row geometry with native pixel rendering.");
    }
    if(native_available && !plane_supported)
    {
        gpu_log("WARNING: Compact plane sampling unavailable; retaining CPU row geometry with native pixel rendering.");
    }
    gpu_log("Drawmethod Resampling\n"
            "\tBuilt-in formulas:                 %s\n"
            "\tDefault:                           Nearest (existing exact sampling)\n"
            "\tSmooth formulas:                   Bilinear, Catmull-Rom, Mitchell, B-spline, Lanczos 2/3\n"
            "\tMovie resample:                    Bilinear alias; each decoded plane is filtered before conversion\n"
            "\tSampling:                          One master filter; center-aligned encoded RGB\n"
            "\tCoverage:                          Per-texel keys/palettes/masks before reconstruction\n"
            "\tReduction:                         Fixed support; no mipmap/area prefilter\n"
            "\tSoftware implementation:           None; unavailable filtered draws logged and skipped\n",
            native_available ? "NATIVE - available" : "UNAVAILABLE");
    gpu_log("Output Filters\n"
            "\tRendering path:                    %s\n"
            "\tShader validation:                 Compiled and linked on first filtered presentation\n"
            "\tAvailable filters:                 Bilinear, Advance Mame2x, Scanlines, ScanlinesTV, TV2x, DotMatrix\n"
            "\tColor precision:                   Full RGB8 channels; no reduced-color conversion\n"
            "\tIntermediate:                      One reusable exact 2x RGBA8 texture\n"
            "\tScene rendering:                   Remains native when filters change\n"
            "\tCPU pixel processing/readback:     None\n"
            "\tSoftware implementation:           None; unavailable filtered presentation logged and skipped\n",
            native_available ? "NATIVE - shared desktop/GLES equations; shader pending first use" :
                               "UNAVAILABLE - native scene backend required");
    gpu_log("Movie Rendering\n"
            "\tRendering path:                    %s\n"
            "\tShader validation:                 Compiled and linked on first movie draw\n"
            "\tSource storage:                    Retained YUV420 planes per owner and frame generation\n"
            "\tOutput:                            Direct RGB32 target or native-resolution window\n"
            "\tColor conversion:                  Established hardware matrix and plane sampling\n"
            "\tSoftware implementation:           None; unavailable movie draws logged and skipped\n",
            native_available ? "Shared native screen/fullscreen pipeline; shader pending first use" :
                               "UNAVAILABLE - native scene backend required");
    gpu_log("Color Rules\n"
            "\tChroma key execution:              %s\n"
            "\tSubstitute execution:              %s\n"
            "\tSlots per drawmethod:              %d chroma keys; %d substitutes\n"
            "\tDestination format:                RGB32 native targets\n"
            "\tSoftware implementation:           None; rules ignored\n"
            "\tOther image formats:               Rules ignored\n"
            "\tChroma matching:                   Source RGB after palette lookup, before effects\n"
            "\tSubstitute matching:               Final RGB bytes after blending\n",
            native_colors ? "NATIVE - available" : "UNAVAILABLE - ignored",
            native_colors ? "NATIVE - available" : "UNAVAILABLE - ignored",
            DRAWMETHOD_COLOR_SLOTS, DRAWMETHOD_COLOR_SLOTS);
    if(!native_colors)
    {
        gpu_log("WARNING: Chroma and Substitute rules are unavailable and will be ignored. %s",
                native_available ? "Other native drawing continues." : "RGB32 drawing is unavailable.");
    }
    gpu_log("========== END GPU CAPABILITY REPORT ==========");
}

/*
 * Damon V. Caskey
 * 2026-11-12
 * 2026-11-13 - Keep affine uniform capacity optional for existing GL contexts.
 * 2026-11-13 - Report startup capabilities and fallback costs without profiling.
 * 2026-11-13 - Group startup information into readable fields and checks.
 * 2026-11-14 - Probe optional color rules without retiring earlier native paths.
 * 2026-11-14 - Validate vertex/index storage before enabling native drawing.
 * 2026-11-14 - Preserve existing capabilities when optional shear is unavailable.
 * 2026-11-15 - Require native RGB32 drawing even when initialization fails.
 * 2026-11-15 - Probe optional water/plane sampling without retiring other features.
 * Initialize native scene rendering only after all required capabilities and
 * shader resources are usable. Failure leaves RGB32 drawing unavailable.
 */
int gpu_gl_init(void)
{
    GLuint vertex = 0, fragment = 0;
    GLenum limits_error;
    GLint linked = 0, units = 0, fragment_components = 0;
    int digit, optional_index, functions_available, plane_textures = 0;
    int shader_variants[32], variant_count = 0, variant_index = 0;
    const int base_variants[4] = {3, 1, 2, 0};
    const int base_components[4] = {GPU_COMBINED_COMPONENTS, GPU_AFFINE_COMPONENTS,
                                    GPU_COLOR_COMPONENTS, GPU_MATERIAL_COMPONENTS};
    const int optional_variants[8] = {28, 12, 20, 4, 24, 8, 16, 0};
    char message[4096];
    const unsigned char white[4] = {255, 255, 255, 255};
    const char *setting = getenv("OPENBOR_GPU");
    const char *vendor, *device, *version, *shader_version;
    /* Selecting this backend commits RGB32 dispatch to native drawing even
     * if configuration or capability checks reject initialization. */
    gpu_set_native_required(1);
    if(initialized)
    {
        return 1;
    }
    /* State and object names from a previous context must never influence
     * setup of its replacement, including initialization failure retries. */
    gpu_gl_invalidate_state();
    affine_supported = color_rules_supported = shear_supported = 0;
    water_supported = plane_supported = 0;
    plane_cache_warning = plane_upload_warning = 0;
    output_filter_warning = 0;
    maximum_texture_size = 0;
    gpu_log("========== GPU CAPABILITY REPORT ==========");
    if(setting && !strcmp(setting, "0"))
    {
        gpu_log("Native Scene Drawing\n"
                "\tStatus:                            Disabled by OPENBOR_GPU=0\n"
                "\tGPU capability checks:             Not run; configuration disables native drawing\n");
        gpu_log_capability_result(0);
        fprintf(stderr, "GPU scene rendering disabled by OPENBOR_GPU=0.\n");
        return 0;
    }
    if(!SDL_GL_GetCurrentContext())
    {
        gpu_log("WARNING: Native GPU capabilities cannot be checked: no current OpenGL context.");
        gpu_log_capability_result(0);
        return 0;
    }

    /* Ordinary records always reach OpenBorLog.txt. One readable report owns
     * startup capability information regardless of the profiling setting. */
    vendor = (const char *)glGetString(GL_VENDOR);
    device = (const char *)glGetString(GL_RENDERER);
    version = (const char *)glGetString(GL_VERSION);
    shader_version = (const char *)glGetString(GL_SHADING_LANGUAGE_VERSION);
    gpu_log("Driver\n"
            "\tVendor:                            %s\n"
            "\tRenderer:                          %s\n"
            "\tOpenGL version:                    %s\n"
            "\tGLSL version:                      %s\n",
            vendor ? vendor : "unavailable", device ? device : "unavailable",
            version ? version : "unavailable", shader_version ? shader_version : "unavailable");
    /* These established renderer names explicitly identify software drivers.
     * Capability success on such a driver is not hardware acceleration. */
    if(device && (strstr(device, "llvmpipe") || strstr(device, "softpipe") ||
                  strstr(device, "SwiftShader") || strstr(device, "Software Rasterizer") ||
                  strstr(device, "Software Renderer") ||
                  strstr(device, "GDI Generic") || strstr(device, "Microsoft Basic Render Driver")))
    {
        gpu_log("WARNING: OpenGL renderer \"%s\" identifies a software implementation.\n"
                "\tRendering executes on the CPU even when the native backend is available.\n"
                "\tSignificant slowdown is possible.", device);
    }
    functions_available = gpu_load_functions();
    gpu_log("Required Functions\n"
            "\tShader, framebuffer and buffer functions: %s\n",
            functions_available ? "AVAILABLE" : "MISSING - see warnings above");
    /* Clear errors left by capability probes before checking our allocations. */
    while(glGetError() != GL_NO_ERROR)
    {
        /* Reading the error flag consumes one pending driver error. */
    }
    glGetIntegerv(GL_MAX_TEXTURE_SIZE, &maximum_texture_size);
    glGetIntegerv(GL_MAX_TEXTURE_IMAGE_UNITS, &units);
    glGetIntegerv(GL_MAX_FRAGMENT_UNIFORM_COMPONENTS, &fragment_components);
    limits_error = glGetError();
    if(limits_error != GL_NO_ERROR)
    {
        gpu_log("WARNING: GPU limits could not be verified.\n"
                "\tOpenGL capability query failed with error: 0x%x",
                (unsigned)limits_error);
        /* Unsupported capability enums must not leave errors for the existing
         * presenter after native initialization has declined this context. */
        while(glGetError() != GL_NO_ERROR)
        {
            /* Consume any remaining errors from the three independent queries. */
        }
    }
    /* Probe the actual row texture format only when the ordinary backend and
     * sixth sampler are available. Water needs uniforms only and remains
     * independent from this optional floating-texture capability. */
    if(functions_available && limits_error == GL_NO_ERROR &&
       maximum_texture_size >= 1536 && units >= GPU_TRACKED_TEXTURE_UNITS)
    {
        plane_textures = gpu_probe_plane_texture();
    }

    /* Preserve affine/color and then shear preference while trying optional
     * water/plane combinations. Failed optional shaders must not retire an
     * already supported material feature or select CPU pixel rendering. */
    for(digit = 0; digit < 4; ++digit)
    {
        for(optional_index = 0; optional_index < 8; ++optional_index)
        {
            int optional = optional_variants[optional_index];
            int required = base_components[digit] +
                ((optional & 4) ? GPU_SHEAR_COMPONENTS : 0) +
                ((optional & 8) ? GPU_WATER_COMPONENTS : 0) +
                ((optional & 16) ? GPU_PLANE_COMPONENTS : 0);
            if((optional & 16) && !plane_textures)
            {
                continue;
            }
            if(fragment_components >= required || (!base_variants[digit] && !optional))
            {
                shader_variants[variant_count++] = base_variants[digit] | optional;
            }
        }
    }
    gpu_log("Resource Limits\n"
            "\t%-34s %-16s %s\n"
            "\t%-34s %-16d %d pixels per axis\n"
            "\t%-34s %-16d %d\n"
            "\t%-34s %-16d %d for native affine\n"
            "\t%-34s %-16d %d for color rules; %d with affine\n"
            "\t%-34s %-16d %d additional to the selected shader\n"
            "\t%-34s %-16d %d additional to the selected shader\n"
            "\t%-34s %-16d %d additional to the selected shader\n"
            "\tFloating plane-row textures:       %s (sixth sampler required)\n",
            "Capability", "Available", "Required",
            "Maximum texture size:", maximum_texture_size, 1536,
            "Fragment texture units:", units, GPU_TEXTURE_UNITS,
            "Fragment uniform components:", fragment_components, GPU_AFFINE_COMPONENTS,
            "Color-rule uniform components:", fragment_components, GPU_COLOR_COMPONENTS,
            GPU_COMBINED_COMPONENTS,
            "Legacy shear uniform components:", fragment_components, GPU_SHEAR_COMPONENTS,
            "Water uniform components:", fragment_components, GPU_WATER_COMPONENTS,
            "Plane uniform components:", fragment_components, GPU_PLANE_COMPONENTS,
            plane_textures ? "AVAILABLE" : "UNAVAILABLE");
    if(fragment_components < GPU_AFFINE_COMPONENTS)
    {
        gpu_log("WARNING: Insufficient fragment uniform components for native affine transforms.\n"
                "\tAvailable:                         %d\n"
                "\tRequired:                          %d\n"
                "\tAffine draws are rejected; no software retry is available.",
                fragment_components, GPU_AFFINE_COMPONENTS);
    }
    if(fragment_components < GPU_COLOR_COMPONENTS ||
       (fragment_components >= GPU_AFFINE_COMPONENTS && fragment_components < GPU_COMBINED_COMPONENTS))
    {
        gpu_log("WARNING: Insufficient fragment uniform components for color rules in the preferred shader.\n"
                "\tAvailable:                         %d\n"
                "\tRequired with selected transforms: %d\n"
                "\tChroma and Substitute may be unavailable and ignored.",
                fragment_components, fragment_components >= GPU_AFFINE_COMPONENTS
                                     ? GPU_COMBINED_COMPONENTS : GPU_COLOR_COMPONENTS);
    }
    if(maximum_texture_size < 1536)
    {
        gpu_log("WARNING: Insufficient maximum texture size for the native blend atlas.\n"
                "\tAvailable:                         %d pixels per axis\n"
                "\tRequired:                          1536 pixels per axis", maximum_texture_size);
    }
    if(units < GPU_TEXTURE_UNITS)
    {
        gpu_log("WARNING: Insufficient fragment texture units for native scene rendering.\n"
                "\tAvailable:                         %d\n"
                "\tRequired:                          %d", units, GPU_TEXTURE_UNITS);
    }

    /* The blend atlas needs six 256-row slices. Source, palette, mask,
     * destination snapshot and atlas occupy five independent texture units. */
    if(!functions_available || limits_error != GL_NO_ERROR ||
       maximum_texture_size < 1536 || units < GPU_TEXTURE_UNITS)
    {
        gpu_log_capability_result(0);
        return 0;
    }
compile_scene_shader:
    affine_supported = (shader_variants[variant_index] & 1) != 0;
    color_rules_supported = (shader_variants[variant_index] & 2) != 0;
    shear_supported = (shader_variants[variant_index] & 4) != 0;
    water_supported = (shader_variants[variant_index] & 8) != 0;
    plane_supported = (shader_variants[variant_index] & 16) != 0;
    vertex = gpu_compile_shader(GL_VERTEX_SHADER, gpu_vertex_shader_source);
    fragment = gpu_compile_shader(GL_FRAGMENT_SHADER, gpu_fragment_shader_source);
    if(!vertex || !fragment)
    {
        goto shader_failed;
    }
    program = gpu_CreateProgram();
    if(!program)
    {
        gpu_log("WARNING: Native shader program allocation failed.");
        goto failed;
    }
    gpu_AttachShader(program, vertex);
    gpu_AttachShader(program, fragment);
    gpu_BindAttribLocation(program, GPU_POSITION_ATTRIBUTE, "vertexPosition");
    gpu_BindAttribLocation(program, GPU_TEXCOORD_ATTRIBUTE, "vertexTexcoord");
    gpu_LinkProgram(program);
    gpu_GetProgramiv(program, GL_LINK_STATUS, &linked);
    if(!linked)
    {
        message[0] = '\0';
        gpu_GetProgramInfoLog(program, sizeof(message), NULL, message);
        message[sizeof(message) - 1] = '\0';
        gpu_log("WARNING: %s shader link failed.", affine_supported ? "affine" : "material");
        gpu_log("\tDriver diagnostic:\n%s", message[0] ? message : "not provided");
        fprintf(stderr, "GPU renderer program: %s\n", message);
        goto shader_failed;
    }
    white_texture = gpu_new_texture(1, 1, GL_RGBA8, GL_RGBA, GL_UNSIGNED_BYTE, white);
    /* Reserve one bounded atlas for all palette rows. Content snapshots
     * become resident lazily, while the allocation stays fixed at 1 MiB. */
    palette_texture = gpu_new_texture(GPU_PALETTE_COLORS, GPU_PALETTE_CACHE_CAPACITY,
                                      GL_RGBA8, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    table_texture = gpu_new_texture(256, 1536, GL_LUMINANCE8, GL_LUMINANCE, GL_UNSIGNED_BYTE, NULL);
    if(!white_texture || !palette_texture || !table_texture)
    {
        gpu_log("WARNING: Native shared texture allocation failed.\n"
                "\tWhite texture:                     %s\n"
                "\tPalette texture:                   %s\n"
                "\tBlend atlas:                       %s",
                white_texture ? "Ready" : "FAILED", palette_texture ? "Ready" : "FAILED",
                table_texture ? "Ready" : "FAILED");
        goto failed;
    }
    if(!gpu_probe_rgb32_target())
    {
        goto failed;
    }
    if(!gpu_reserve_geometry(GPU_STREAM_INITIAL_QUADS))
    {
        gpu_log("WARNING: Native vertex/index buffer allocation failed.");
        goto failed;
    }
    gpu_log("Initialization Checks\n"
            "\t%-34s PASS (compiled and linked)\n"
            "\tShared textures:                   PASS (allocated)\n"
            "\tRGB32 framebuffer probe:           PASS (1x1 RGBA8 render target)\n"
            "\tVertex/index buffers:              PASS (allocated)\n",
            affine_supported ? "Native affine/material shader:" : "Native material shader:");

    /* Resolve locations once for this linked program. Cached values update
     * only on change or ownership invalidation, without driver string lookups. */
#define GPU_UNIFORM(member, name) uniforms.member = gpu_GetUniformLocation(program, name)
    GPU_UNIFORM(target_size, "targetSize");
    GPU_UNIFORM(source_size, "sourceSize");
    GPU_UNIFORM(source, "sourceTexture");
    GPU_UNIFORM(palette, "paletteTexture");
    GPU_UNIFORM(palette_row, "paletteRow");
    GPU_UNIFORM(mask, "maskTexture");
    GPU_UNIFORM(destination, "destinationTexture");
    GPU_UNIFORM(tables, "blendTableTexture");
    GPU_UNIFORM(indexed, "indexedSource");
    GPU_UNIFORM(masked, "maskEnabled");
    GPU_UNIFORM(color_key, "colorKey");
    GPU_UNIFORM(fill, "fillEnabled");
    GPU_UNIFORM(has_destination, "hasDestination");
    GPU_UNIFORM(wrap, "wrapSource");
    GPU_UNIFORM(resample, "resampleMode");
    GPU_UNIFORM(blend_mode, "blendMode");
    GPU_UNIFORM(tint_mode, "tintMode");
    GPU_UNIFORM(table_mask, "blendTableMask");
    GPU_UNIFORM(fill_color, "fillColor");
    GPU_UNIFORM(tint_color, "tintColor");
    GPU_UNIFORM(channels, "channelOpacity");
    GPU_UNIFORM(chroma_enabled, "chromaEnabled");
    GPU_UNIFORM(substitute_enabled, "substituteEnabled");
    GPU_UNIFORM(primitive_source, "primitiveSource");
    for(digit = 0; digit < DRAWMETHOD_COLOR_SLOTS; ++digit)
    {
        char name[32];
        snprintf(name, sizeof(name), "chromaKeys[%d]", digit);
        GPU_UNIFORM(chroma_keys[digit], name);
        snprintf(name, sizeof(name), "substituteMatch[%d]", digit);
        GPU_UNIFORM(substitute_match[digit], name);
        snprintf(name, sizeof(name), "substituteValue[%d]", digit);
        GPU_UNIFORM(substitute_value[digit], name);
    }
    GPU_UNIFORM(affine_approximate_x, "affineApproximateX");
    GPU_UNIFORM(affine_approximate_y, "affineApproximateY");
    GPU_UNIFORM(affine_approximate_d, "affineApproximateD");
    GPU_UNIFORM(affine_anchor, "affineAnchor");
    GPU_UNIFORM(affine_pivot, "affinePivot");
    GPU_UNIFORM(shear_origin, "shearOrigin");
    GPU_UNIFORM(shear_extent, "shearExtent");
    GPU_UNIFORM(shear_shift, "shearShift");
    GPU_UNIFORM(shear_source, "shearSource");
    GPU_UNIFORM(shear_step, "shearStep");
    GPU_UNIFORM(shear_clip, "shearClip");
    GPU_UNIFORM(water_origin, "waterOrigin");
    GPU_UNIFORM(water_wave, "waterWave");
    GPU_UNIFORM(water_anchor, "waterAnchor");
    GPU_UNIFORM(plane_rows, "planeRowsTexture");
    GPU_UNIFORM(plane_origin, "planeOrigin");
    GPU_UNIFORM(plane_info, "planeInfo");
    for(digit = 0; digit < 8; ++digit)
    {
        char name[32];
        snprintf(name, sizeof(name), "affineX[%d]", digit);
        GPU_UNIFORM(affine_x[digit], name);
        snprintf(name, sizeof(name), "affineY[%d]", digit);
        GPU_UNIFORM(affine_y[digit], name);
        snprintf(name, sizeof(name), "affineD[%d]", digit);
        GPU_UNIFORM(affine_d[digit], name);
    }
#undef GPU_UNIFORM

    /* Publish the backend only after shader linking and shared allocation
     * succeed. Image resources remain lazy and follow their owners' lifetime. */
    gpu_DeleteShader(vertex);
    gpu_DeleteShader(fragment);
    memset(&stats, 0, sizeof(stats));
    memset(table_pointers, 0, sizeof(table_pointers));
    gpu_reset_palette_cache();
    tables_dirty = 1;
    table_mask = 0;
    initialized = 1;
    gpu_register_backend(&renderer_backend);
    gpu_gl_prepare_present();
    gpu_log("Renderer Caches\n"
            "\tPalette cache:                     Enabled\n"
            "\tPalette capacity:                  %u palettes\n"
            "\tSize of each palette:              %" PRIu64 " bytes\n"
            "\tPalette GPU storage:               %" PRIu64 " bytes\n"
            "\tPalette CPU storage:               %" PRIu64 " bytes\n"
            "\tPalette policy:                    Match content; replace least recently used\n"
            "\tGraphics state cache:              Enabled\n"
            "\tCached texture units:              %u\n"
            "\tState cache CPU storage:           %" PRIu64 " bytes\n"
            "\tState cache policy:                Compare on change\n"
            "\tAffine repeat preparations:        Cached after complete validation\n"
            "\tAffine repeat cache limit:         %u instances; %" PRIu64 " bytes\n"
            "\tAffine repeat cache allocation:    Grows on demand; reused until shutdown\n"
            "\tPlane row cache:                   %u maps; at most %u rows per map\n"
            "\tPlane row cache payload limit:     %" PRIu64 " bytes each on CPU and GPU\n"
            "\tPlane row cache policy:            Match dimensions/scales/perspective; replace least recently used\n",
            (unsigned)GPU_PALETTE_CACHE_CAPACITY, (uint64_t)GPU_PALETTE_BYTES,
            (uint64_t)GPU_PALETTE_CACHE_CAPACITY * GPU_PALETTE_BYTES,
            (uint64_t)(sizeof(palette_cache) + sizeof(palette_buckets)
                       + sizeof(palette_cache_count) + sizeof(palette_lru_head)
                       + sizeof(palette_lru_tail) + sizeof(palette_selected_slot)),
            (unsigned)(plane_supported ? GPU_TRACKED_TEXTURE_UNITS : GPU_TEXTURE_UNITS), (uint64_t)sizeof(gl_state),
            (unsigned)GPU_AFFINE_PREPARED_LIMIT,
            (uint64_t)GPU_AFFINE_PREPARED_LIMIT * sizeof(gpu_prepared_affine),
            (unsigned)GPU_PLANE_CACHE_CAPACITY, (unsigned)GPU_PLANE_MAX_ROWS,
            (uint64_t)GPU_PLANE_CACHE_CAPACITY * GPU_PLANE_MAX_ROWS * sizeof(gpu_plane_row));
    gpu_query_init();
    gpu_log_capability_result(1);
    fprintf(stderr, "Native GPU scene renderer enabled (RGB32 targets, indexed textures).\n");
    return 1;
shader_failed:
    /* Retry only less demanding variants, releasing every failed program.
     * Existing native affine draws take priority over new optional color
     * rules; neither feature may retire a usable plain material renderer. */
    if(variant_index + 1 < variant_count)
    {
        if(vertex) gpu_DeleteShader(vertex);
        if(fragment) gpu_DeleteShader(fragment);
        if(program) gpu_DeleteProgram(program);
        vertex = fragment = program = 0;
        ++variant_index;
        gpu_log("WARNING: Optional scene shader features failed; trying another native variant.\n"
                "\tAffine requested by retry:         %s\n"
                "\tColor rules requested by retry:    %s\n"
                "\tLegacy shear requested by retry:   %s\n"
                "\tWater sampling requested by retry: %s\n"
                "\tPlane sampling requested by retry: %s\n"
                "\tIf native affine is unavailable:   Draw rejected; no software retry\n"
                "\tIf color rules are unavailable:    Ignored; no software implementation.\n"
                "\tIf shader row effects unavailable: CPU row geometry; native pixels.",
                (shader_variants[variant_index] & 1) ? "Yes" : "No",
                (shader_variants[variant_index] & 2) ? "Yes" : "No",
                (shader_variants[variant_index] & 4) ? "Yes" : "No",
                (shader_variants[variant_index] & 8) ? "Yes" : "No",
                (shader_variants[variant_index] & 16) ? "Yes" : "No");
        goto compile_scene_shader;
    }
failed:
    /* Initialization can fail at any stage. Release only acquired resources;
     * native-required dispatch remains in force without a registered backend. */
    gpu_gl_invalidate_state();
    gpu_release_geometry();
    gpu_release_plane_maps();
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
    gpu_reset_palette_cache();
    gpu_log_capability_result(0);
    fprintf(stderr, "Native GPU scene renderer unavailable; RGB32 drawing disabled without software retry.\n");
    return 0;
}

/*
 * Damon V. Caskey
 * 2026-11-12
 * 2026-11-15 - Retire context-owned plane maps with the other native resources.
 * 2026-11-16 - Discard private output-filter storage without a screen readback.
 * Preserve live screen contents before destroying a graphics context. This
 * infrequent readback keeps pause buffers, scripted targets and mode changes
 * functional without maintaining a CPU copy after every rendered frame.
 */
void gpu_gl_shutdown(void)
{
    size_t bucket;
    gpu_gl_invalidate_state();
    if(!initialized)
    {
        return;
    }

    /* Timestamp collection must stop before context teardown. Polling here
     * accepts only ready samples; normal screen preservation below retains its
     * existing behavior independently of optional measurement completion. */
    gpu_query_shutdown();

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
    gpu_movie_shutdown();
    gpu_output_filter_shutdown();
    gpu_DeleteProgram(program);
    glDeleteTextures(1, &white_texture);
    glDeleteTextures(1, &palette_texture);
    glDeleteTextures(1, &table_texture);
    gpu_release_geometry();
    gpu_release_plane_maps();
    free(vertex_buffer);
    free(quad_wraps);
    free(affine_preparations);
    vertex_buffer = NULL;
    quad_wraps = NULL;
    affine_preparations = NULL;
    quad_count = quad_capacity = 0;
    affine_preparation_capacity = 0;
    program = white_texture = palette_texture = table_texture = 0;
    gpu_reset_palette_cache();
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
 * 2026-11-16
 * Filter the completed native scene without downloading or repacking pixels.
 * Borrowed results retain source row orientation and are ready for the normal
 * window scaling/color-correction pass. Failed work never publishes a texture.
 */
int gpu_gl_get_filtered_screen_texture(s_screen *screen, int filter,
                                       unsigned *texture, int *width, int *height)
{
    gpu_image *source;
    gpu_vertex vertices[4];
    int target_width, target_height;
    size_t stream_first = 0;
    ep_scope state_scope, submit_scope;
    const char *failure = "output-filter resources unavailable";
    uint64_t previous_state_changes = stats.state_changes;
    uint64_t previous_state_reuses = stats.state_reuses;
    native_failure = EP_FALLBACK_OTHER;
    if(!initialized || !texture || !width || !height)
    {
        failure = !initialized ? "native renderer unavailable" : "invalid result pointers";
        goto failed;
    }
    if(filter < 0 || filter > 6)
    {
        failure = "unsupported filter selection";
        goto failed;
    }
    if(!screen || screen->pixelformat != PIXEL_32)
    {
        failure = "RGB32 source required";
        native_failure = EP_FALLBACK_SOURCE_FORMAT;
        goto failed;
    }
    if(!filter)
    {
        /* Disabled filtering borrows the original image with no intermediate
         * allocation, shader compilation, or extra submission. */
        if(gpu_gl_get_screen_texture(screen, texture, width, height)) return 1;
        failure = "source screen texture unavailable";
        goto failed;
    }
    if(screen->width <= 0 || screen->height <= 0 ||
       screen->width > maximum_texture_size / 2 || screen->height > maximum_texture_size / 2)
    {
        failure = "2x output exceeds texture limits or has invalid dimensions";
        native_failure = EP_FALLBACK_IMAGE_SIZE;
        goto failed;
    }

    /* Validate both doubled dimensions before multiplication or source upload.
     * Each pass samples only the original screen, never its attached output. */
    target_width = screen->width * 2;
    target_height = screen->height * 2;
    if(!gpu_output_filter_program_ready()) goto failed;
    source = gpu_screen_image(screen, 1);
    if(!source || !gpu_output_filter_target_ready(target_width, target_height)) goto failed;
    vertices[0] = (gpu_vertex){0.0f, 0.0f, 0.0f, 0.0f};
    vertices[1] = (gpu_vertex){(float)target_width, 0.0f, (float)source->width, 0.0f};
    vertices[2] = (gpu_vertex){(float)target_width, (float)target_height,
                                (float)source->width, (float)source->height};
    vertices[3] = (gpu_vertex){0.0f, (float)target_height, 0.0f, (float)source->height};
    quad_count = 0;
    if(!gpu_collect_quad(vertices, 0, NULL)) goto failed;
    submit_scope = engine_profile_span_begin();
    if(!gpu_upload_geometry(&stream_first))
    {
        engine_profile_span_end(submit_scope, EP_SPAN_GPU_SUBMIT);
        goto failed;
    }
    engine_profile_span_end(submit_scope, EP_SPAN_GPU_SUBMIT);

    /* Preparation is complete before the first output write. Scene and movie
     * programs retain their own uniforms; selecting this program changes only
     * the shared binding tracked by the context-local state cache. */
    state_scope = engine_profile_span_begin();
    gpu_bind_framebuffer(output_filter_framebuffer);
    gpu_set_viewport(target_width, target_height);
    gpu_set_scissor_enabled(0);
    gpu_set_draw_raster();
    gpu_set_color_mask();
    gpu_UseProgram(output_filter_program);
    gl_state.program = output_filter_program;
    gl_state.program_valid = 1;
    ++stats.state_changes;
    gpu_bind_texture(0, source->texture);
    gpu_select_texture_unit(0);
    gpu_bind_geometry();
    gpu_Uniform2f(output_filter_uniforms.target_size, (float)target_width, (float)target_height);
    gpu_Uniform2f(output_filter_uniforms.source_size, (float)source->width, (float)source->height);
    gpu_Uniform1i(output_filter_uniforms.source, 0);
    gpu_Uniform1i(output_filter_uniforms.mode, filter);
    stats.state_changes += 4;
    if(glGetError() != GL_NO_ERROR)
    {
        engine_profile_span_end(state_scope, EP_SPAN_GPU_STATE);
        native_failure = EP_FALLBACK_RESOURCE;
        gpu_gl_invalidate_state();
        goto failed;
    }
    engine_profile_span_end(state_scope, EP_SPAN_GPU_STATE);

    submit_scope = engine_profile_span_begin();
    gpu_DrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT,
                     (const void *)(uintptr_t)(stream_first * 6 * sizeof(GLuint)));
    engine_profile_span_end(submit_scope, EP_SPAN_GPU_SUBMIT);
    if(glGetError() != GL_NO_ERROR)
    {
        /* Only private output storage may have changed. Rejecting this frame
         * leaves the scene coherent and permits a subsequent native draw. */
        failure = "output-filter submission failed";
        native_failure = EP_FALLBACK_RESOURCE;
        gpu_gl_invalidate_state();
        goto failed;
    }
    ++stats.output_filter_draws;
    ++stats.draws;
    engine_profile_count(EP_COUNT_NATIVE_DRAWS, 1);
    engine_profile_count(EP_COUNT_NATIVE_QUADS, 1);
    engine_profile_count(EP_COUNT_NATIVE_BATCHES, 1);
    engine_profile_count(EP_COUNT_GPU_STATE_CHANGES, stats.state_changes - previous_state_changes);
    engine_profile_count(EP_COUNT_GPU_STATE_REUSES, stats.state_reuses - previous_state_reuses);
    *texture = output_filter_texture;
    *width = target_width;
    *height = target_height;
    return 1;
failed:
    if(!output_filter_warning)
    {
        gpu_log("WARNING: Filtered presentation rejected (filter %d): %s; no CPU retry.", filter, failure);
        output_filter_warning = 1;
    }
    engine_profile_fallback(native_failure);
    ++stats.fallback_draws;
    return 0;
}

/*
 * Damon V. Caskey
 * 2026-11-12
 * 2026-11-14 - Release scene vertex-array state at the presentation boundary.
 * 2026-11-15 - Release the optional sixth row-map texture unit when selected.
 * Restore neutral GL state before the existing presenter or YUV renderer
 * draws to the window. Scene drawing restores its own state on its next use.
 */
void gpu_gl_prepare_present(void)
{
    int unit;
    /* The external owner may change any shared binding, raster value or
     * program. Forget assumptions now, before its first raw GL operation. */
    gpu_gl_invalidate_state();
    if(!initialized)
    {
        return;
    }

    /* Return drawing to the window and clear scene-specific state before
     * the presenter chooses its scaling shader and window-space viewport. */
    gpu_BindFramebuffer(GL_FRAMEBUFFER, 0);
    gpu_UseProgram(0);
    gpu_DisableVertexAttribArray(GPU_POSITION_ATTRIBUTE);
    gpu_DisableVertexAttribArray(GPU_TEXCOORD_ATTRIBUTE);
    gpu_BindBuffer(GL_ARRAY_BUFFER, 0);
    gpu_BindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    glDisable(GL_SCISSOR_TEST);
    glDisable(GL_BLEND);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_ALPHA_TEST);
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);

    /* Reset every texture unit used by scene rendering. Descending order
     * leaves unit zero active for the existing single-texture presenter. */
    for(unit = (plane_supported ? GPU_TRACKED_TEXTURE_UNITS : GPU_TEXTURE_UNITS) - 1;
        unit >= 0; unit--)
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

/*
 * Damon V. Caskey
 * 2026-11-12
 *
 * Expose optional query coverage and lifecycle totals to native regressions.
 */
const gpu_query_stats *gpu_gl_get_query_stats(void)
{
    /* Reading diagnostics never polls the driver or changes query ownership.
     * Tests can distinguish unsupported timing from native drawing failure. */
    return &query_stats;
}

#endif
