/*
 * OpenBOR - https://www.chronocrash.com
 * Copyright (c) OpenBOR Team. All rights reserved. See LICENSE.
 *
 * Damon V. Caskey - 2026-11-27
 * One native projection pass turns a terrain grid into the existing textured
 * GPU stream. Cell presence and the fixed top-left/bottom-right diagonal are
 * shared with collision. Artwork pixels never pass through a CPU rasterizer.
 */
#include "meshmap_render.h"
#include "spriteq.h"
#include <float.h>
#include <limits.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

typedef struct meshmap_projected_point
{
    double x, y, w, u, v;
} meshmap_projected_point;

/* The engine renders on one thread. Reusing one scratch stream bounds retained
 * CPU memory by the largest visible draw rather than the number of maps. */
static s_meshmap_render_buffer meshmap_draw_buffer;

void meshmap_view_orthographic(s_meshmap_view *view, double offset_x, double offset_y)
{
    if(!view) return;
    memset(view, 0, sizeof(*view));
    view->projection[0] = 1.0;
    view->projection[3] = offset_x;
    view->projection[5] = -1.0;
    view->projection[6] = 1.0;
    view->projection[7] = offset_y;
    view->projection[11] = 1.0;
    view->near_w = 0.0001;
}

int meshmap_view_valid(const s_meshmap_view *view)
{
    unsigned index;
    if(!view || !isfinite(view->near_w) || view->near_w <= 0.0 || view->near_w > FLT_MAX)
        return 0;
    for(index = 0; index < 12; ++index)
        if(!isfinite(view->projection[index])) return 0;
    return 1;
}

void meshmap_render_buffer_free(s_meshmap_render_buffer *buffer)
{
    if(!buffer) return;
    free(buffer->vertices);
    memset(buffer, 0, sizeof(*buffer));
}

void meshmap_render_shutdown(void)
{
    meshmap_render_buffer_free(&meshmap_draw_buffer);
}

static int meshmap_render_reserve(s_meshmap_render_buffer *buffer, size_t triangles)
{
    size_t capacity = buffer->triangle_capacity ? buffer->triangle_capacity : 64;
    size_t limit = (size_t)INT_MAX / 6;
    gpu_mesh_vertex *replacement;
    /* The GPU index limit and host address limit differ on 32-bit ports.
     * Clamp geometric growth to both before multiplying an allocation size. */
    if(limit > SIZE_MAX / (3 * sizeof(*replacement)))
        limit = SIZE_MAX / (3 * sizeof(*replacement));
    if(triangles <= buffer->triangle_capacity) return 1;
    if(triangles > limit) return 0;
    while(capacity < triangles) capacity = capacity > limit / 2 ? triangles : capacity * 2;
    replacement = (gpu_mesh_vertex *)realloc(buffer->vertices, capacity * 3 * sizeof(*replacement));
    if(!replacement) return 0;
    buffer->vertices = replacement;
    buffer->triangle_capacity = capacity;
    return 1;
}

static int meshmap_render_point(const s_meshmap_vertex *vertex, const s_meshmap_view *view,
                                unsigned art_width, unsigned art_height,
                                meshmap_projected_point *point)
{
    const double *p = view->projection;
    const s_meshmap_vec3 *v = &vertex->position;
    point->x = p[0] * v->x + p[1] * v->y + p[2] * v->z + p[3];
    point->y = p[4] * v->x + p[5] * v->y + p[6] * v->z + p[7];
    point->w = p[8] * v->x + p[9] * v->y + p[10] * v->z + p[11];
    point->u = vertex->u * art_width;
    point->v = vertex->v * art_height;
    return isfinite(point->x) && isfinite(point->y) && isfinite(point->w) &&
           isfinite(point->u) && isfinite(point->v);
}

/* Clip before dividing by W. Intersections interpolate homogeneous position
 * and ordinary surface UV together, preserving perspective sampling where a
 * triangle crosses the camera or an edge of its destination screen. */
static double meshmap_clip_distance(const meshmap_projected_point *point, unsigned plane,
                                    const s_meshmap_view *view, unsigned width, unsigned height)
{
    switch(plane)
    {
    case 0: return point->w - view->near_w;
    case 1: return point->x;
    case 2: return width * point->w - point->x;
    case 3: return point->y;
    default: return height * point->w - point->y;
    }
}

static meshmap_projected_point meshmap_lerp(meshmap_projected_point a,
                                           meshmap_projected_point b, double fraction)
{
    meshmap_projected_point result;
    double inverse = 1.0 - fraction;
    result.x = inverse * a.x + fraction * b.x;
    result.y = inverse * a.y + fraction * b.y;
    result.w = inverse * a.w + fraction * b.w;
    result.u = inverse * a.u + fraction * b.u;
    result.v = inverse * a.v + fraction * b.v;
    return result;
}

static int meshmap_emit_triangle(s_meshmap_render_buffer *buffer,
                                 const meshmap_projected_point *a,
                                 const meshmap_projected_point *b,
                                 const meshmap_projected_point *c)
{
    const meshmap_projected_point *input[3] = {a, b, c};
    gpu_mesh_vertex output[3];
    double x[3], y[3], area;
    unsigned corner;
    for(corner = 0; corner < 3; ++corner)
    {
        const meshmap_projected_point *point = input[corner];
        if(!isfinite(point->w) || point->w <= 0.0 || point->w > FLT_MAX) return 0;
        x[corner] = point->x / point->w;
        y[corner] = point->y / point->w;
        if(!isfinite(x[corner]) || !isfinite(y[corner]) ||
           fabs(x[corner]) > FLT_MAX || fabs(y[corner]) > FLT_MAX ||
           fabs(point->u) > FLT_MAX || fabs(point->v) > FLT_MAX) return 0;
        output[corner] = (gpu_mesh_vertex){(float)x[corner], (float)y[corner],
                                          (float)point->u, (float)point->v, (float)point->w};
    }
    area = (x[1] - x[0]) * (y[2] - y[0]) - (y[1] - y[0]) * (x[2] - x[0]);
    if(fabs(area) <= 1.0e-12) return 1;
    if(!meshmap_render_reserve(buffer, buffer->triangle_count + 1)) return 0;
    memcpy(buffer->vertices + buffer->triangle_count * 3, output, sizeof(output));
    ++buffer->triangle_count;
    return 1;
}

static int meshmap_clip_triangle(s_meshmap_render_buffer *buffer,
                                 const meshmap_projected_point triangle[3],
                                 const s_meshmap_view *view, unsigned width, unsigned height)
{
    meshmap_projected_point polygon[2][12];
    unsigned count = 3, current = 0, plane, index;
    memcpy(polygon[0], triangle, 3 * sizeof(*triangle));
    for(plane = 0; plane < 5 && count; ++plane)
    {
        unsigned written = 0;
        for(index = 0; index < count; ++index)
        {
            meshmap_projected_point a = polygon[current][index];
            meshmap_projected_point b = polygon[current][(index + 1) % count];
            double da = meshmap_clip_distance(&a, plane, view, width, height);
            double db = meshmap_clip_distance(&b, plane, view, width, height);
            if(!isfinite(da) || !isfinite(db)) return 0;
            if(da >= 0.0)
            {
                if(written >= 12) return 0;
                polygon[1 - current][written++] = a;
            }
            if((da < 0.0) != (db < 0.0))
            {
                double denominator = da - db;
                if(!isfinite(denominator) || denominator == 0.0 || written >= 12) return 0;
                polygon[1 - current][written++] = meshmap_lerp(a, b, da / denominator);
            }
        }
        current = 1 - current;
        count = written;
    }
    for(index = 1; index + 1 < count; ++index)
        if(!meshmap_emit_triangle(buffer, &polygon[current][0], &polygon[current][index],
                                  &polygon[current][index + 1])) return 0;
    return 1;
}

int meshmap_project(const s_meshmap *map, const s_meshmap_view *view,
                     unsigned target_width, unsigned target_height,
                     unsigned artwork_width, unsigned artwork_height,
                     s_meshmap_render_buffer *buffer)
{
    uint32_t column, row;
    if(!buffer) return 0;
    buffer->triangle_count = 0;
    if(!map || !meshmap_view_valid(view) || !target_width || !target_height ||
       !artwork_width || !artwork_height) return 0;
    if(map->width < 2 || map->height < 2 || !map->present_cells) return 1;
    for(row = 0; row + 1 < map->height; ++row)
    {
        for(column = 0; column + 1 < map->width; ++column)
        {
            static const unsigned corner_u[4] = {0, 1, 1, 0};
            static const unsigned corner_v[4] = {0, 0, 1, 1};
            static const unsigned triangles[2][3] = {{0, 1, 2}, {0, 2, 3}};
            meshmap_projected_point points[4];
            unsigned corner, triangle;
            if(!meshmap_cell_present(map, column, row)) continue;
            for(corner = 0; corner < 4; ++corner)
            {
                s_meshmap_vertex vertex;
                if(!meshmap_vertex(map, column + corner_u[corner], row + corner_v[corner], &vertex) ||
                   !meshmap_render_point(&vertex, view, artwork_width, artwork_height, &points[corner]))
                    goto failed;
            }
            for(triangle = 0; triangle < 2; ++triangle)
            {
                meshmap_projected_point face[3];
                for(corner = 0; corner < 3; ++corner) face[corner] = points[triangles[triangle][corner]];
                if(!meshmap_clip_triangle(buffer, face, view, target_width, target_height)) goto failed;
            }
        }
    }
    return 1;
failed:
    buffer->triangle_count = 0;
    return 0;
}

int meshmap_draw(const s_meshmap_store *store, uint32_t id, s_screen *target,
                  const s_meshmap_view *view, const s_drawmethod *method)
{
    const s_meshmap *map;
    s_screen *artwork;
    s_meshmap_view default_view;
    if(!store || !target || target->pixelformat != PIXEL_32) return 0;
    map = meshmap_get_const(store, id);
    /* DRAW controls automatic scene participation. An explicit draw remains
     * available for active maps used exclusively in scripted view screens. */
    if(!map || !meshmap_is_active(store, id) || !map->artwork)
        return 1;
    artwork = (s_screen *)map->artwork;
    if(artwork->magic != screen_magic || artwork->width <= 0 || artwork->height <= 0 ||
       target->width <= 0 || target->height <= 0) return 0;
    if(!view)
    {
        meshmap_view_orthographic(&default_view, 0.0, 0.0);
        view = &default_view;
    }
    if(!meshmap_project(map, view, (unsigned)target->width, (unsigned)target->height,
                        (unsigned)artwork->width, (unsigned)artwork->height, &meshmap_draw_buffer)) return 0;
    return gpu_draw_mesh(target, artwork, meshmap_draw_buffer.vertices,
                         meshmap_draw_buffer.triangle_count, method);
}

static void meshmap_queue_draw(s_screen *target, void *owner, unsigned id,
                               int offset_x, int offset_y, const s_drawmethod *method)
{
    const s_meshmap_store *store = (const s_meshmap_store *)owner;
    const s_meshmap *map = meshmap_get_const(store, id);
    s_meshmap_view view;
    /* Resolve visibility at playback, just like resource identity. Scripts
     * may hide or remove this map after the native scene queued its ID. */
    if(!map || !(map->config.flags & MESHMAP_DRAW)) return;
    meshmap_view_orthographic(&view, offset_x, offset_y);
    if(!meshmap_draw(store, id, target, &view, method))
        gpu_report_rejected_image_operation(target, GPU_OPERATION_MESH, method);
}

typedef struct meshmap_queue_context
{
    s_meshmap_store *store;
    int offset_x, offset_y;
} meshmap_queue_context;

static int meshmap_queue_visit(const s_meshmap *map, void *context)
{
    meshmap_queue_context *queue = (meshmap_queue_context *)context;
    if((map->config.flags & MESHMAP_DRAW) && map->artwork && map->present_cells)
        spriteq_add_mesh(queue->store, map->id, queue->offset_x, queue->offset_y,
                         map->config.layer, NULL, meshmap_queue_draw);
    return 1;
}

void meshmap_queue_native(s_meshmap_store *store, int offset_x, int offset_y)
{
    meshmap_queue_context context = {store, offset_x, offset_y};
    if(store) meshmap_foreach(store, NULL, meshmap_queue_visit, &context);
}
