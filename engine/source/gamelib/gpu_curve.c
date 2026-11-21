/*
 * OpenBOR - http://www.chronocrash.com
 * Copyright (c) OpenBOR Team. All rights reserved. See LICENSE.
 *
 * Damon V. Caskey
 * 2026-11-20
 * Construct a continuous, textured bend from a curve cross-section. Uniform
 * curvature is circular; the smooth profile concentrates the same total turn
 * with continuous curvature at both ends. Full-width adaptive bands share
 * their edges, so the GPU receives no T junctions or script-generated strips.
 */
#include "gpu_curve.h"
#include "drawmethod_curve.h"
#include <float.h>
#include <math.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define CURVE_PI 3.1415926535897932384626433832795
#define CURVE_NEAR_W 0.0001
#define CURVE_MAX_DEPTH 18
#define CURVE_POLYGON_CAPACITY 12

typedef struct curve_point
{
    double x, y, w, u, v;
} curve_point;

typedef struct curve_setup
{
    const gpu_transform_parameters *parameters;
    const s_drawmethod *method;
    double bx, by, tx, ty, plane_cos, plane_sin;
    double angle, focus, extent, focal, tilt_cos, tilt_sin;
    double pivot[3];
    double a, b, c, d, offset_x, offset_y, handedness;
    double left, top, right, bottom;
    gpu_curve_mesh *mesh;
} curve_setup;

static int curve_finite(double value)
{
    return value == value && value <= FLT_MAX && value >= -FLT_MAX;
}

static void curve_sincos(double degrees, double *sine, double *cosine)
{
    double reduced = fmod(degrees, 360.0);
    *sine = sin(reduced * (CURVE_PI / 180.0));
    *cosine = cos(reduced * (CURVE_PI / 180.0));
    if(fmod(reduced, 90.0) == 0.0)
    {
        *sine = round(*sine);
        *cosine = round(*cosine);
    }
}

static double curve_profile_angle(const curve_setup *setup, double normalized)
{
    double value = fmax(-0.5, fmin(0.5, normalized));
    if(setup->method->curve.profile == DC_CURVE_PROFILE_SMOOTH)
    {
        double position = value + 0.5;
        value = position * position * (3.0 - 2.0 * position) - 0.5;
    }
    return setup->angle * value;
}

/* Integrate the tangent in source-distance units. Eight-point Gauss-Legendre
 * integration is confined to half of the smooth profile, whose angle never
 * exceeds pi/2. Circular profiles and their straight continuations are exact. */
static void curve_section(const curve_setup *setup, double s,
                          double *along, double *deflection, double *theta)
{
    static const double nodes[4] = {
        0.18343464249564980494, 0.52553240991632898582,
        0.79666647741362673959, 0.96028985649753623168
    };
    static const double weights[4] = {
        0.36268378337836198297, 0.31370664587788728734,
        0.22238103445337447054, 0.10122853629037625915
    };
    double position = (s - setup->focus) / setup->extent;
    double limited = fmax(-0.5, fmin(0.5, position));
    double tail = (position - limited) * setup->extent;
    double length = 0.0, depth = 0.0;
    int index;

    *theta = curve_profile_angle(setup, limited);
    if(setup->angle == 0.0)
    {
        *along = s;
        *deflection = 0.0;
        return;
    }
    if(setup->method->curve.profile == DC_CURVE_PROFILE_UNIFORM)
    {
        double radius = setup->extent / setup->angle;
        length = radius * sin(*theta);
        /* 2 sin^2(theta/2) avoids cancellation near the flat limit. */
        depth = radius * 2.0 * pow(sin(*theta * 0.5), 2.0);
    }
    else
    {
        double half = limited * 0.5;
        for(index = 0; index < 4; ++index)
        {
            double first = curve_profile_angle(setup, half * (1.0 - nodes[index]));
            double second = curve_profile_angle(setup, half * (1.0 + nodes[index]));
            length += weights[index] * (cos(first) + cos(second));
            depth += weights[index] * (sin(first) + sin(second));
        }
        length *= half * setup->extent;
        depth *= half * setup->extent;
    }
    *along = setup->focus + length + tail * cos(*theta);
    *deflection = depth + tail * sin(*theta);
}

/* B follows the source bend direction and T is its perpendicular. Rotating T
 * with the curve gives a constant-width planar strip; its component parallel
 * to the extrusion axis stays fixed for an out-of-plane cylindrical surface. */
static void curve_surface(const curve_setup *setup, double s, double t, double out[3])
{
    double along, bend, theta, sine, cosine, width_b, width_e;
    double ex = setup->plane_cos * setup->tx;
    double ey = setup->plane_cos * setup->ty;
    curve_section(setup, s, &along, &bend, &theta);
    sine = sin(theta);
    cosine = cos(theta);
    width_b = -setup->plane_cos * sine;
    width_e = setup->plane_cos * (cosine - 1.0);
    out[0] = setup->bx * along + ex * bend + t *
             (setup->tx + setup->bx * width_b + ex * width_e);
    out[1] = setup->by * along + ey * bend + t *
             (setup->ty + setup->by * width_b + ey * width_e);
    out[2] = setup->plane_sin * (bend + t * width_e);
}

static int curve_initialize(const gpu_transform_parameters *p, const s_drawmethod *m,
                            curve_setup *setup)
{
    double axis_sin, axis_cos, rotation_sin, rotation_cos;
    double pivot_u, pivot_v, pivot_s, pivot_t, span, determinant;
    if(!p || !m || !setup || p->source_width <= 0 || p->source_height <= 0 ||
       p->target_width <= 0 || p->target_height <= 0 || !drawmethod_curve_valid(&m->curve))
        return GPU_TRANSFORM_FAILED;
    memset(setup, 0, sizeof(*setup));
    setup->parameters = p;
    setup->method = m;
    curve_sincos(m->curve.axis, &axis_sin, &axis_cos);
    setup->bx = axis_cos;
    setup->by = axis_sin;
    setup->tx = -axis_sin;
    setup->ty = axis_cos;
    curve_sincos(m->curve.plane, &setup->plane_sin, &setup->plane_cos);
    curve_sincos(m->curve.tilt, &setup->tilt_sin, &setup->tilt_cos);
    span = fabs(axis_cos) * p->source_width + fabs(axis_sin) * p->source_height;
    setup->focus = (m->curve.focus - 0.5) * span;
    setup->extent = m->curve.extent * span;
    setup->angle = m->curve.angle * (CURVE_PI / 180.0);
    setup->focal = m->curve.perspective * fmax(p->source_width, p->source_height);
    pivot_u = (double)p->centerx + m->centerx - p->source_width * 0.5;
    pivot_v = (double)p->centery + m->centery - p->source_height * 0.5;
    pivot_s = pivot_u * setup->bx + pivot_v * setup->by;
    pivot_t = pivot_u * setup->tx + pivot_v * setup->ty;
    curve_surface(setup, pivot_s, pivot_t, setup->pivot);

    setup->offset_x = p->x;
    setup->offset_y = p->y;
    if(m->config & DRAWMETHOD_CONFIG_AFFINE)
    {
        setup->a = m->affine.m00;
        setup->b = m->affine.m01;
        setup->c = m->affine.m10;
        setup->d = m->affine.m11;
        setup->offset_x += m->affine.tx;
        setup->offset_y += m->affine.ty;
        if(m->config & DRAWMETHOD_CONFIG_FLIP_X)
        {
            setup->a = -setup->a;
            setup->c = -setup->c;
        }
        if(m->config & DRAWMETHOD_CONFIG_FLIP_Y)
        {
            setup->b = -setup->b;
            setup->d = -setup->d;
        }
    }
    else
    {
        double sx = m->scalex / 256.0, sy = m->scaley / 256.0;
        double rotation = m->rotate;
        if(m->scalex < 0 || m->scaley < 0 || !curve_finite(rotation))
            return GPU_TRANSFORM_FAILED;
        if(m->config & DRAWMETHOD_CONFIG_FLIP_X) sx = -sx;
        else rotation = -rotation;
        if(m->config & DRAWMETHOD_CONFIG_FLIP_Y)
        {
            sy = -sy;
            rotation = -rotation;
        }
        curve_sincos(rotation, &rotation_sin, &rotation_cos);
        setup->a = sx * rotation_cos;
        setup->b = sx * rotation_sin;
        setup->c = -sy * rotation_sin;
        setup->d = sy * rotation_cos;
        /* Curve is a continuous, pivot-anchored operation. Retain legacy
         * rotation priority; otherwise compose shear about that same pivot. */
        if(m->rotate == 0.0f) setup->b -= (m->shiftx / 256.0) * sy;
    }
    determinant = setup->a * setup->d - setup->b * setup->c;
    if(!curve_finite(setup->a) || !curve_finite(setup->b) ||
       !curve_finite(setup->c) || !curve_finite(setup->d) ||
       !curve_finite(setup->offset_x) || !curve_finite(setup->offset_y) ||
       !curve_finite(determinant)) return GPU_TRANSFORM_FAILED;
    if(determinant == 0.0) return GPU_TRANSFORM_EMPTY;
    setup->handedness = determinant < 0.0 ? -1.0 : 1.0;
    setup->right = p->target_width;
    setup->bottom = p->target_height;
    if(m->clipw > 0 && m->cliph > 0)
    {
        setup->left = fmax(0.0, m->clipx);
        setup->top = fmax(0.0, m->clipy);
        setup->right = fmin(setup->right, (double)m->clipx + m->clipw);
        setup->bottom = fmin(setup->bottom, (double)m->clipy + m->cliph);
    }
    return GPU_TRANSFORM_DRAWN;
}

static int curve_project(const curve_setup *setup, double s, double t, curve_point *out)
{
    double point[3], along, rotated, w;
    const gpu_transform_parameters *p = setup->parameters;
    curve_surface(setup, s, t, point);
    point[0] -= setup->pivot[0];
    point[1] -= setup->pivot[1];
    point[2] -= setup->pivot[2];
    along = point[0] * setup->bx + point[1] * setup->by;
    rotated = along * setup->tilt_cos - point[2] * setup->tilt_sin;
    point[0] += setup->bx * (rotated - along);
    point[1] += setup->by * (rotated - along);
    point[2] = along * setup->tilt_sin + point[2] * setup->tilt_cos;
    w = setup->focal > 0.0 ? 1.0 + point[2] / setup->focal : 1.0;
    out->x = setup->offset_x * w + setup->a * point[0] + setup->b * point[1];
    out->y = setup->offset_y * w + setup->c * point[0] + setup->d * point[1];
    out->w = w;
    out->u = p->source_width * 0.5 + setup->bx * s + setup->tx * t + setup->method->curve.scrollx;
    out->v = p->source_height * 0.5 + setup->by * s + setup->ty * t + setup->method->curve.scrolly;
    return curve_finite(out->x) && curve_finite(out->y) && curve_finite(out->w) &&
           curve_finite(out->u) && curve_finite(out->v);
}

int gpu_curve_map_point(const gpu_transform_parameters *p, const s_drawmethod *m,
                        double u, double v, gpu_curve_vertex *vertex)
{
    curve_setup setup;
    curve_point point;
    double s, t;
    int result;
    if(!vertex || !curve_finite(u) || !curve_finite(v)) return GPU_TRANSFORM_FAILED;
    result = curve_initialize(p, m, &setup);
    if(result != GPU_TRANSFORM_DRAWN) return result;
    u -= p->source_width * 0.5;
    v -= p->source_height * 0.5;
    s = u * setup.bx + v * setup.by;
    t = u * setup.tx + v * setup.ty;
    if(!curve_project(&setup, s, t, &point)) return GPU_TRANSFORM_FAILED;
    if(point.w < CURVE_NEAR_W) return GPU_TRANSFORM_EMPTY;
    if(!curve_finite(point.x / point.w) || !curve_finite(point.y / point.w))
        return GPU_TRANSFORM_FAILED;
    vertex->x = (float)(point.x / point.w);
    vertex->y = (float)(point.y / point.w);
    vertex->u = (float)point.u;
    vertex->v = (float)point.v;
    vertex->w = (float)point.w;
    return GPU_TRANSFORM_DRAWN;
}

static int curve_cross_section(const curve_setup *setup, double s, double *low, double *high)
{
    double center[2] = {setup->parameters->source_width * 0.5,
                        setup->parameters->source_height * 0.5};
    double direction[2] = {setup->bx, setup->by};
    double width[2] = {setup->tx, setup->ty};
    double extent[2] = {setup->parameters->source_width, setup->parameters->source_height};
    int axis;
    *low = -DBL_MAX;
    *high = DBL_MAX;
    for(axis = 0; axis < 2; ++axis)
    {
        double coordinate = center[axis] + s * direction[axis];
        if(fabs(width[axis]) < 1e-14)
        {
            if(coordinate < -1e-7 || coordinate > extent[axis] + 1e-7) return 0;
        }
        else
        {
            double first = -coordinate / width[axis];
            double last = (extent[axis] - coordinate) / width[axis];
            *low = fmax(*low, fmin(first, last));
            *high = fmin(*high, fmax(first, last));
        }
    }
    if(*low > *high && *low - *high <= 1e-7) *high = *low;
    return *low <= *high;
}

/* Source-corner and profile breakpoints make both width boundaries linear on
 * each interval. The local Jacobian is 1 - curvature(s)*t*cos(plane). Uniform
 * curvature needs only endpoint checks; smooth curvature times a sloped width
 * is cubic, so its interior extrema are found exactly instead of sampled. */
static int curve_interval_valid(const curve_setup *setup, double first_s, double last_s)
{
    double first[2], last[2], first_r, last_r;
    double factor = setup->angle / setup->extent * setup->plane_cos;
    int edge;
    if(factor == 0.0) return 1;
    first_s = fmax(first_s, setup->focus - setup->extent * 0.5);
    last_s = fmin(last_s, setup->focus + setup->extent * 0.5);
    if(first_s >= last_s) return 1;
    if(!curve_cross_section(setup, first_s, &first[0], &first[1]) ||
       !curve_cross_section(setup, last_s, &last[0], &last[1])) return 0;
    first_r = fmax(0.0, fmin(1.0, (first_s - setup->focus) / setup->extent + 0.5));
    last_r = fmax(0.0, fmin(1.0, (last_s - setup->focus) / setup->extent + 0.5));
    for(edge = 0; edge < 2; ++edge)
    {
        double samples[4] = {first_r, last_r, 0.0, 0.0};
        int count = 2, i;
        if(setup->method->curve.profile == DC_CURVE_PROFILE_SMOOTH && last_r > first_r)
        {
            double slope = (last[edge] - first[edge]) / (last_r - first_r);
            double intercept = first[edge] - first_r * slope;
            double scale = fmax(fabs(slope), fabs(intercept));
            if(scale > 0.0)
            {
                double a, b, c, discriminant, q;
                slope /= scale;
                intercept /= scale;
                /* d[(intercept+slope*r)*r*(1-r)]/dr. */
                a = -3.0 * slope;
                b = 2.0 * (slope - intercept);
                c = intercept;
                if(a == 0.0)
                {
                    if(b != 0.0) samples[count++] = -c / b;
                }
                else
                {
                    discriminant = b * b - 4.0 * a * c;
                    if(discriminant >= 0.0)
                    {
                        q = -0.5 * (b + copysign(sqrt(discriminant), b));
                        samples[count++] = q / a;
                        if(q != 0.0) samples[count++] = c / q;
                    }
                }
            }
        }
        for(i = 0; i < count; ++i)
        {
            double r = samples[i], t, curvature = factor;
            if(r < first_r || r > last_r) continue;
            t = i == 0 ? first[edge] : i == 1 ? last[edge] : first[edge] +
                (last[edge] - first[edge]) * ((r - first_r) / (last_r - first_r));
            if(setup->method->curve.profile == DC_CURVE_PROFILE_SMOOTH)
                curvature *= 6.0 * r * (1.0 - r);
            if(1.0 - curvature * t <= 1e-8) return 0;
        }
    }
    return 1;
}

static int curve_reserve(gpu_curve_mesh *mesh, size_t count)
{
    size_t capacity;
    gpu_curve_vertex *replacement;
    if(count > GPU_CURVE_MAX_QUADS) return 0;
    if(count <= mesh->quad_capacity) return 1;
    capacity = mesh->quad_capacity ? mesh->quad_capacity : 64;
    while(capacity < count) capacity *= 2;
    if(capacity > GPU_CURVE_MAX_QUADS) capacity = GPU_CURVE_MAX_QUADS;
    replacement = (gpu_curve_vertex *)realloc(mesh->vertices, capacity * 4 * sizeof(*replacement));
    if(!replacement) return 0;
    mesh->vertices = replacement;
    mesh->quad_capacity = capacity;
    return 1;
}

static double curve_clip_distance(const curve_setup *setup, const curve_point *p, int plane)
{
    switch(plane)
    {
        case 0: return p->w - CURVE_NEAR_W;
        case 1: return p->x - setup->left * p->w;
        case 2: return setup->right * p->w - p->x;
        case 3: return p->y - setup->top * p->w;
        default: return setup->bottom * p->w - p->y;
    }
}

static curve_point curve_interpolate(curve_point a, curve_point b, double t)
{
    curve_point p;
    p.x = a.x + t * (b.x - a.x);
    p.y = a.y + t * (b.y - a.y);
    p.w = a.w + t * (b.w - a.w);
    p.u = a.u + t * (b.u - a.u);
    p.v = a.v + t * (b.v - a.v);
    return p;
}

static int curve_front(const curve_setup *setup, const curve_point *a,
                        const curve_point *b, const curve_point *c)
{
    double area = a->x * (b->y * c->w - c->y * b->w) -
                  a->y * (b->x * c->w - c->x * b->w) +
                  a->w * (b->x * c->y - c->x * b->y);
    return area * setup->handedness > 1e-12;
}

static int curve_append(curve_setup *setup, const curve_point points[4])
{
    gpu_curve_vertex *vertices;
    int i;
    if(!curve_reserve(setup->mesh, setup->mesh->quad_count + 1)) return 0;
    vertices = setup->mesh->vertices + setup->mesh->quad_count * 4;
    for(i = 0; i < 4; ++i)
    {
        double x = points[i].x / points[i].w;
        double y = points[i].y / points[i].w;
        if(points[i].w < CURVE_NEAR_W * 0.999999 || !curve_finite(x) || !curve_finite(y)) return 0;
        vertices[i].x = (float)x;
        vertices[i].y = (float)y;
        vertices[i].u = (float)points[i].u;
        vertices[i].v = (float)points[i].v;
        vertices[i].w = (float)points[i].w;
    }
    ++setup->mesh->quad_count;
    return 1;
}

static int curve_emit_triangle(curve_setup *setup, curve_point a, curve_point b, curve_point c)
{
    curve_point buffers[2][CURVE_POLYGON_CAPACITY];
    int current = 0, count = 3, plane, i;
    buffers[0][0] = a;
    buffers[0][1] = b;
    buffers[0][2] = c;
    for(plane = 0; plane < 5 && count; ++plane)
    {
        int output_count = 0;
        for(i = 0; i < count; ++i)
        {
            curve_point first = buffers[current][i];
            curve_point second = buffers[current][(i + 1) % count];
            double first_distance = curve_clip_distance(setup, &first, plane);
            double second_distance = curve_clip_distance(setup, &second, plane);
            int first_inside = first_distance >= 0.0;
            int second_inside = second_distance >= 0.0;
            if(first_inside)
            {
                if(output_count >= CURVE_POLYGON_CAPACITY) return 0;
                buffers[1 - current][output_count++] = first;
            }
            if(first_inside != second_inside)
            {
                double t = first_distance / (first_distance - second_distance);
                if(output_count >= CURVE_POLYGON_CAPACITY) return 0;
                buffers[1 - current][output_count++] = curve_interpolate(first, second, t);
            }
        }
        count = output_count;
        current = 1 - current;
    }
    for(i = 1; i + 1 < count; ++i)
    {
        curve_point quad[4] = {buffers[current][0], buffers[current][i],
                               buffers[current][i + 1], buffers[current][i + 1]};
        if(curve_front(setup, &quad[0], &quad[1], &quad[2]) && !curve_append(setup, quad)) return 0;
    }
    return 1;
}

static int curve_emit_band(curve_setup *setup, curve_point points[4])
{
    int i, plane, unclipped = 1;
    for(i = 0; i < 4; ++i)
        for(plane = 0; plane < 5; ++plane)
            if(curve_clip_distance(setup, &points[i], plane) < 0.0) unclipped = 0;
    if(unclipped && curve_front(setup, &points[0], &points[1], &points[2]) &&
       curve_front(setup, &points[0], &points[2], &points[3]))
        return curve_append(setup, points);
    return curve_emit_triangle(setup, points[0], points[1], points[2]) &&
           curve_emit_triangle(setup, points[0], points[2], points[3]);
}

static double curve_midpoint_error(curve_point first, curve_point middle, curve_point last)
{
    if(first.w >= CURVE_NEAR_W && middle.w >= CURVE_NEAR_W && last.w >= CURVE_NEAR_W)
    {
        double predicted_x = (first.x + last.x) / (first.w + last.w);
        double predicted_y = (first.y + last.y) / (first.w + last.w);
        return hypot(middle.x / middle.w - predicted_x, middle.y / middle.w - predicted_y);
    }
    return 0.0;
}

static int curve_band(curve_setup *setup, double first_s, double last_s, int depth)
{
    double first_low, first_high, last_low, last_high, middle_low, middle_high;
    double middle_s = (first_s + last_s) * 0.5;
    double first_angle, last_angle, ignored1, ignored2;
    curve_point first[3], middle[3], last[3], diagonal, quad[4];
    double maximum_error = 0.0;
    int i, split = 0;
    if(!curve_cross_section(setup, first_s, &first_low, &first_high) ||
       !curve_cross_section(setup, last_s, &last_low, &last_high) ||
       !curve_cross_section(setup, middle_s, &middle_low, &middle_high)) return 0;
    for(i = 0; i < 3; ++i)
    {
        double weight = i * 0.5;
        double t_first = first_low + weight * (first_high - first_low);
        double t_middle = middle_low + weight * (middle_high - middle_low);
        double t_last = last_low + weight * (last_high - last_low);
        if(!curve_project(setup, first_s, t_first, &first[i]) ||
           !curve_project(setup, middle_s, t_middle, &middle[i]) ||
           !curve_project(setup, last_s, t_last, &last[i])) return 0;
        maximum_error = fmax(maximum_error, curve_midpoint_error(first[i], middle[i], last[i]));
    }
    /* The GPU divides each band across first-low to last-high. The rotating
     * width basis can make that interior diagonal less accurate than either
     * longitudinal edge, even while the cross-sections remain straight. */
    if(!curve_project(setup, middle_s, (first_low + last_high) * 0.5, &diagonal)) return 0;
    maximum_error = fmax(maximum_error, curve_midpoint_error(first[0], diagonal, last[2]));
    curve_section(setup, first_s, &ignored1, &ignored2, &first_angle);
    curve_section(setup, last_s, &ignored1, &ignored2, &last_angle);
    split = maximum_error > GPU_CURVE_PIXEL_TOLERANCE || fabs(last_angle - first_angle) > CURVE_PI / 36.0;
    if(split)
    {
        if(depth >= CURVE_MAX_DEPTH) return 0;
        return curve_band(setup, first_s, middle_s, depth + 1) &&
               curve_band(setup, middle_s, last_s, depth + 1);
    }
    /* The (s,t) frame has positive determinant. Perimeter order therefore
     * agrees with the ordinary source rectangle before placement reflections. */
    quad[0] = first[0];
    quad[1] = last[0];
    quad[2] = last[2];
    quad[3] = first[2];
    return curve_emit_band(setup, quad);
}

static int curve_compare(const void *left, const void *right)
{
    double a = *(const double *)left, b = *(const double *)right;
    return (a > b) - (a < b);
}

int gpu_curve_build(const gpu_transform_parameters *p, const s_drawmethod *m, gpu_curve_mesh *mesh)
{
    curve_setup setup;
    double breaks[7], minimum, maximum;
    int result, count = 4, i;
    if(!mesh) return GPU_TRANSFORM_FAILED;
    mesh->quad_count = 0;
    result = curve_initialize(p, m, &setup);
    if(result != GPU_TRANSFORM_DRAWN) return result;
    setup.mesh = mesh;
    if(setup.left >= setup.right || setup.top >= setup.bottom) return GPU_TRANSFORM_EMPTY;
    for(i = 0; i < 4; ++i)
    {
        double u = (i & 1 ? 0.5 : -0.5) * p->source_width;
        double v = (i & 2 ? 0.5 : -0.5) * p->source_height;
        breaks[i] = u * setup.bx + v * setup.by;
    }
    qsort(breaks, 4, sizeof(double), curve_compare);
    minimum = breaks[0];
    maximum = breaks[3];
    /* Split the profile's transition points explicitly. Otherwise a narrow
     * bend could hide between samples of a mostly straight source interval. */
    for(i = -1; setup.angle != 0.0 && i <= 1; ++i)
    {
        double point = setup.focus + i * setup.extent * 0.5;
        if(point > minimum && point < maximum) breaks[count++] = point;
    }
    qsort(breaks, count, sizeof(double), curve_compare);
    for(i = 0; i + 1 < count; ++i)
    {
        if(breaks[i + 1] - breaks[i] <= 1e-9) continue;
        if(!curve_interval_valid(&setup, breaks[i], breaks[i + 1]))
            return GPU_TRANSFORM_FAILED;
    }
    for(i = 0; i + 1 < count; ++i)
    {
        if(breaks[i + 1] - breaks[i] <= 1e-9) continue;
        if(!curve_band(&setup, breaks[i], breaks[i + 1], 0))
        {
            mesh->quad_count = 0;
            return GPU_TRANSFORM_FAILED;
        }
    }
    return mesh->quad_count ? GPU_TRANSFORM_DRAWN : GPU_TRANSFORM_EMPTY;
}

void gpu_curve_free(gpu_curve_mesh *mesh)
{
    if(!mesh) return;
    free(mesh->vertices);
    memset(mesh, 0, sizeof(*mesh));
}
