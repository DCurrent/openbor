/*
 * Damon V. Caskey
 * 2026-11-20
 * Independent geometric checks for native surface bending. Circular results
 * are compared with closed-form geometry; smooth results use a separate
 * high-resolution integration oracle. No pixel rasterizer is linked here.
 */
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#include "gpu_curve.h"
#include "drawmethod_curve.h"

#define TEST_PI 3.1415926535897932384626433832795

static unsigned checks;

static s_drawmethod curve_method(void)
{
    s_drawmethod method;
    memset(&method, 0, sizeof(method));
    method.config = DRAWMETHOD_CONFIG_ENABLED | DRAWMETHOD_CONFIG_CURVE;
    method.scalex = method.scaley = 256;
    method.xrepeat = method.yrepeat = 1;
    method.affine.m00 = method.affine.m11 = 1.0f;
    method.curve = (s_drawmethod_curve)DRAWMETHOD_CURVE_INITIALIZER;
    return method;
}

static gpu_transform_parameters parameters(void)
{
    gpu_transform_parameters p = {128, 64, 1024, 768, 400, 300, 64, 32, 1};
    return p;
}

static void near_value(double value, double expected, double tolerance)
{
    ++checks;
    if(fabs(value - expected) > tolerance)
    {
        fprintf(stderr, "curve geometry: expected %.12f, got %.12f (tolerance %.9f)\n",
                expected, value, tolerance);
        assert(0);
    }
}

static gpu_curve_vertex mapped(const gpu_transform_parameters *p, const s_drawmethod *m,
                               double u, double v)
{
    gpu_curve_vertex vertex;
    assert(gpu_curve_map_point(p, m, u, v, &vertex) == GPU_TRANSFORM_DRAWN);
    return vertex;
}

static void bounded_mesh(const gpu_transform_parameters *p, const s_drawmethod *m,
                          const gpu_curve_mesh *mesh)
{
    size_t i;
    double left = 0, top = 0, right = p->target_width, bottom = p->target_height;
    if(m->clipw > 0 && m->cliph > 0)
    {
        left = fmax(left, m->clipx);
        top = fmax(top, m->clipy);
        right = fmin(right, (double)m->clipx + m->clipw);
        bottom = fmin(bottom, (double)m->clipy + m->cliph);
    }
    assert(mesh->quad_count <= GPU_CURVE_MAX_QUADS);
    for(i = 0; i < mesh->quad_count * 4; ++i)
    {
        const gpu_curve_vertex *v = &mesh->vertices[i];
        assert(isfinite(v->x) && isfinite(v->y) && isfinite(v->u) && isfinite(v->v));
        assert(isfinite(v->w) && v->w > 0.0f);
        assert(v->x >= left - 0.001 && v->x <= right + 0.001);
        assert(v->y >= top - 0.001 && v->y <= bottom + 0.001);
    }
}

static void test_identity_and_anchor(void)
{
    gpu_transform_parameters p = parameters();
    s_drawmethod m = curve_method();
    gpu_curve_mesh mesh = {0};
    gpu_curve_vertex v;
    gpu_curve_vertex *storage;
    assert(gpu_curve_build(&p, &m, &mesh) == GPU_TRANSFORM_DRAWN);
    assert(mesh.quad_count == 1);
    near_value(mesh.vertices[0].x, 336, 0.00001);
    near_value(mesh.vertices[0].y, 268, 0.00001);
    near_value(mesh.vertices[2].x, 464, 0.00001);
    near_value(mesh.vertices[2].y, 332, 0.00001);
    storage = mesh.vertices;
    assert(gpu_curve_build(&p, &m, &mesh) == GPU_TRANSFORM_DRAWN);
    assert(mesh.vertices == storage);
    v = mapped(&p, &m, 17.25, 42.75);
    near_value(v.x, 353.25, 0.00001);
    near_value(v.y, 310.75, 0.00001);
    near_value(v.w, 1, 0.0);

    p.centerx = 23;
    p.centery = 19;
    m.centerx = 4;
    m.centery = -3;
    m.curve.angle = 120;
    m.curve.axis = 37;
    m.curve.plane = 68;
    m.curve.focus = 0.83f;
    m.curve.extent = 0.7f;
    m.curve.perspective = 2;
    m.curve.tilt = 23;
    v = mapped(&p, &m, 27, 16);
    near_value(v.x, p.x, 0.00001);
    near_value(v.y, p.y, 0.00001);
    near_value(v.w, 1, 0.00001);
    gpu_curve_free(&mesh);
    assert(!mesh.vertices && !mesh.quad_capacity && !mesh.quad_count);
}

static void test_circular_projection(void)
{
    gpu_transform_parameters p = parameters();
    s_drawmethod m = curve_method();
    gpu_curve_vertex v, first;
    double angle, radius, theta, x, z, w;
    p.source_width = 100;
    p.source_height = 80;
    p.centerx = 50;
    p.centery = 40;
    m.curve.angle = 120;
    angle = 120 * TEST_PI / 180;
    radius = 100 / angle;
    theta = angle * 0.25;
    x = radius * sin(theta);
    z = radius * (1 - cos(theta));
    v = mapped(&p, &m, 75, 20);
    near_value(v.x, p.x + x, 0.00004);
    near_value(v.y, p.y - 20, 0.00004);
    m.curve.perspective = 2;
    w = 1 + z / 200;
    v = mapped(&p, &m, 75, 20);
    near_value(v.x, p.x + x / w, 0.00004);
    near_value(v.y, p.y - 20 / w, 0.00004);
    near_value(v.w, w, 0.000001);
    first = v;
    m.curve.scrollx = -137;
    m.curve.scrolly = 53.25f;
    m.curve.wrap = 1;
    v = mapped(&p, &m, 75, 20);
    near_value(v.x, first.x, 0.0);
    near_value(v.y, first.y, 0.0);
    near_value(v.u, -62, 0.0);
    near_value(v.v, 73.25, 0.0);
}

/* Deliberately independent midpoint integration of tangent angle over source
 * distance. The large sample count makes the oracle error much smaller than the
 * final float-vertex quantization, including intervals with straight tails. */
static void smooth_oracle(double s, double focus, double extent, double angle,
                           double *along, double *depth)
{
    const int samples = 200000;
    double step = (s - focus) / samples;
    double x = focus, z = 0;
    int i;
    for(i = 0; i < samples; ++i)
    {
        double at = focus + (i + 0.5) * step;
        double t = fmax(0.0, fmin(1.0, (at - focus) / extent + 0.5));
        double theta = angle * (t * t * (3 - 2 * t) - 0.5);
        x += step * cos(theta);
        z += step * sin(theta);
    }
    *along = x;
    *depth = z;
}

static void test_smooth_profile_and_focus(void)
{
    gpu_transform_parameters p = parameters();
    s_drawmethod m = curve_method();
    gpu_curve_vertex v, moved;
    double center_x, center_z, source_x, source_z;
    double span, focus, extent, angle, w;
    int i;
    m.curve.profile = DC_CURVE_PROFILE_SMOOTH;
    m.curve.angle = 150;
    m.curve.extent = 0.45f;
    m.curve.focus = 0.35f;
    m.curve.perspective = 3;
    span = p.source_width;
    focus = (m.curve.focus - 0.5) * span;
    extent = m.curve.extent * span;
    angle = m.curve.angle * TEST_PI / 180;
    smooth_oracle(0, focus, extent, angle, &center_x, &center_z);
    for(i = 0; i <= 8; ++i)
    {
        double u = p.source_width * i / 8.0;
        smooth_oracle(u - p.source_width * 0.5, focus, extent, angle, &source_x, &source_z);
        w = 1 + (source_z - center_z) / (3 * p.source_width);
        v = mapped(&p, &m, u, p.centery);
        near_value(v.x, p.x + (source_x - center_x) / w, 0.00008);
        near_value(v.y, p.y, 0.00002);
        near_value(v.w, w, 0.000001);
    }
    v = mapped(&p, &m, 12, 20);
    m.curve.focus = 0.7f;
    moved = mapped(&p, &m, 12, 20);
    assert(fabs(v.x - moved.x) + fabs(v.y - moved.y) > 1);
    moved = mapped(&p, &m, p.centerx, p.centery);
    near_value(moved.x, p.x, 0.00001);
    near_value(moved.y, p.y, 0.00001);
}

static void test_planar_width_and_transform(void)
{
    gpu_transform_parameters p = parameters();
    s_drawmethod m = curve_method();
    gpu_curve_mesh mesh = {0};
    gpu_curve_vertex left, right, original, reflected;
    double dx, dy;
    p.source_width = 40;
    p.source_height = 200;
    p.centerx = 20;
    p.centery = 100;
    m.curve.axis = 90;
    m.curve.plane = 0;
    m.curve.angle = 60;
    left = mapped(&p, &m, 0, 65);
    right = mapped(&p, &m, 40, 65);
    near_value(hypot(right.x - left.x, right.y - left.y), 40, 0.00006);
    assert(gpu_curve_build(&p, &m, &mesh) == GPU_TRANSFORM_DRAWN);
    bounded_mesh(&p, &m, &mesh);
    original = mapped(&p, &m, 15, 40);
    m.config |= DRAWMETHOD_CONFIG_FLIP_X;
    reflected = mapped(&p, &m, 15, 40);
    near_value(reflected.x, 2 * p.x - original.x, 0.00005);
    near_value(reflected.y, original.y, 0.00005);
    assert(gpu_curve_build(&p, &m, &mesh) == GPU_TRANSFORM_DRAWN);
    m.config &= ~DRAWMETHOD_CONFIG_FLIP_X;
    m.config |= DRAWMETHOD_CONFIG_AFFINE;
    m.affine.m00 = -1.5f;
    m.affine.m01 = 0.25f;
    m.affine.m10 = 0.4f;
    m.affine.m11 = 1.2f;
    m.affine.tx = 3;
    m.affine.ty = -7;
    dx = original.x - p.x;
    dy = original.y - p.y;
    reflected = mapped(&p, &m, 15, 40);
    near_value(reflected.x, p.x + 3 - 1.5 * dx + 0.25 * dy, 0.00006);
    near_value(reflected.y, p.y - 7 + 0.4 * dx + 1.2 * dy, 0.00006);
    assert(gpu_curve_build(&p, &m, &mesh) == GPU_TRANSFORM_DRAWN);
    m.curve.angle = 180;
    m.curve.extent = 0.1f;
    assert(gpu_curve_build(&p, &m, &mesh) == GPU_TRANSFORM_FAILED);
    assert(mesh.quad_count == 0);
    gpu_curve_free(&mesh);
}

/* For an oblique source, the inner edge is sloped in bend coordinates. Its
 * smooth-profile curvature maximum falls between the usual subdivision
 * samples; the independent dense Jacobian check below distinguishes a valid
 * tight turn from a narrow local fold without depending on mesh topology. */
static void test_oblique_smooth_fold(void)
{
    gpu_transform_parameters p = parameters();
    s_drawmethod m = curve_method();
    gpu_curve_mesh mesh = {0};
    double axis = 15 * TEST_PI / 180;
    double bx = cos(axis), by = sin(axis);
    double span = bx * p.source_width + by * p.source_height;
    double minimum = 1.0;
    int i;
    m.curve.axis = 15;
    m.curve.plane = 0;
    m.curve.extent = 0.7f;
    m.curve.profile = DC_CURVE_PROFILE_SMOOTH;
    m.curve.angle = 109.1482f;
    for(i = 0; i <= 100000; ++i)
    {
        double s = span * (i / 100000.0 - 0.5);
        double position = s / (m.curve.extent * span) + 0.5;
        double inner = fmin((p.source_width * 0.5 + s * bx) / by,
                            (p.source_height * 0.5 - s * by) / bx);
        if(position >= 0.0 && position <= 1.0)
        {
            double curvature = (m.curve.angle * TEST_PI / 180) /
                               (m.curve.extent * span) * 6 * position * (1 - position);
            minimum = fmin(minimum, 1 - curvature * inner);
        }
    }
    assert(minimum < -5e-7);
    assert(gpu_curve_build(&p, &m, &mesh) == GPU_TRANSFORM_FAILED);
    assert(!mesh.quad_count);
    m.curve.angle = 109.1480f;
    assert(gpu_curve_build(&p, &m, &mesh) == GPU_TRANSFORM_DRAWN);
    bounded_mesh(&p, &m, &mesh);
    gpu_curve_free(&mesh);
}

static int texture_at(const gpu_curve_vertex *a, const gpu_curve_vertex *b,
                       const gpu_curve_vertex *c, double x, double y, double *u, double *v)
{
    double denominator = (b->y - c->y) * (a->x - c->x) + (c->x - b->x) * (a->y - c->y);
    double la, lb, lc, inverse_w;
    if(fabs(denominator) < 1e-12) return 0;
    la = ((b->y - c->y) * (x - c->x) + (c->x - b->x) * (y - c->y)) / denominator;
    lb = ((c->y - a->y) * (x - c->x) + (a->x - c->x) * (y - c->y)) / denominator;
    lc = 1 - la - lb;
    if(la < -1e-6 || lb < -1e-6 || lc < -1e-6) return 0;
    inverse_w = la / a->w + lb / b->w + lc / c->w;
    *u = (la * a->u / a->w + lb * b->u / b->w + lc * c->u / c->w) / inverse_w;
    *v = (la * a->v / a->w + lb * b->v / b->w + lc * c->v / c->w) / inverse_w;
    return 1;
}

static void test_clipping_and_perspective_sampling(void)
{
    gpu_transform_parameters p = parameters();
    s_drawmethod m = curve_method();
    gpu_curve_mesh mesh = {0};
    gpu_curve_vertex point, untouched = {11, 22, 33, 44, 55};
    double u = 0, v = 0;
    size_t i;
    int found = 0;
    m.curve.tilt = 60;
    m.curve.perspective = 0.1f;
    point = untouched;
    assert(gpu_curve_map_point(&p, &m, 0, 32, &point) == GPU_TRANSFORM_EMPTY);
    assert(!memcmp(&point, &untouched, sizeof(point)));
    assert(gpu_curve_build(&p, &m, &mesh) == GPU_TRANSFORM_DRAWN);
    bounded_mesh(&p, &m, &mesh);
    point = mapped(&p, &m, 100, 20);
    for(i = 0; i < mesh.quad_count && !found; ++i)
    {
        const gpu_curve_vertex *q = mesh.vertices + i * 4;
        found = texture_at(q, q + 1, q + 2, point.x, point.y, &u, &v) ||
                texture_at(q, q + 2, q + 3, point.x, point.y, &u, &v);
    }
    assert(found);
    near_value(u, 100, 0.0005);
    near_value(v, 20, 0.0005);
    m.clipx = 380;
    m.clipy = 290;
    m.clipw = 30;
    m.cliph = 30;
    assert(gpu_curve_build(&p, &m, &mesh) == GPU_TRANSFORM_DRAWN);
    bounded_mesh(&p, &m, &mesh);
    m.curve.tilt = 0;
    m.curve.perspective = 0;
    p.x = 5000;
    assert(gpu_curve_build(&p, &m, &mesh) == GPU_TRANSFORM_EMPTY);
    assert(mesh.quad_count == 0);
    gpu_curve_free(&mesh);
}

/* Damon V. Caskey - 2026-11-21
 * The 16k finite race plane exposed cancellation at a nearly cardinal source
 * corner. Its low/high cross-section bounds differed by 1.88e-7 even though
 * one endpoint was within 1e-12 source texel of the actual corner. Keep the
 * recorded camera and exercise both sides of each cardinal at other sizes. */
static void check_near_cardinal_ground(int size, double x, double z, double yaw)
{
    gpu_transform_parameters p = {size, size, 1920, 1080, 960, 540, 0, 0, 1};
    s_drawmethod m = curve_method();
    gpu_curve_mesh mesh = {0};
    double sine = sin(yaw * TEST_PI / 180), cosine = cos(yaw * TEST_PI / 180);
    double pitch = 8 * TEST_PI / 180, height = 135, focal = 960;
    double distance = height / sin(pitch), look = height / tan(pitch);
    double aim_x = x + look * sine, aim_z = z + look * cosine;
    double scale = focal / distance, depth = 200 * cos(pitch) + height * sin(pitch);
    double source_x = x + 200 * sine + 60 * cosine;
    double source_z = z + 200 * cosine - 60 * sine;
    gpu_curve_vertex point;
    size_t vertex;
    m.config |= DRAWMETHOD_CONFIG_AFFINE;
    m.centerx = (int)floor(aim_x);
    m.centery = (int)floor(aim_z);
    m.curve.axis = (float)(90 - yaw);
    m.curve.tilt = 82;
    m.curve.perspective = (float)(distance / size);
    m.curve.scrollx = (float)(aim_x - m.centerx);
    m.curve.scrolly = (float)(aim_z - m.centery);
    m.affine = (affine_transform){(float)(scale * cosine), (float)(-scale * sine),
                                  (float)(-scale * sine), (float)(-scale * cosine), 0, 0};
    assert(gpu_curve_build(&p, &m, &mesh) == GPU_TRANSFORM_DRAWN);
    assert(mesh.quad_count < 20);
    bounded_mesh(&p, &m, &mesh);
    for(vertex = 0; vertex < mesh.quad_count * 4; ++vertex)
    {
        const gpu_curve_vertex *v = &mesh.vertices[vertex];
        assert(v->u >= -0.001 && v->u <= size + 1.001);
        assert(v->v >= -0.001 && v->v <= size + 1.001);
    }
    point = mapped(&p, &m, source_x - m.curve.scrollx, source_z - m.curve.scrolly);
    near_value(point.x, 960 + focal * 60 / depth, 0.001);
    near_value(point.y, 540 - focal * (200 * sin(pitch) - height * cos(pitch)) / depth, 0.001);
    near_value(point.u, source_x, 0.002);
    near_value(point.v, source_z, 0.002);
    gpu_curve_free(&mesh);
}

static void test_near_cardinal_source_corners(void)
{
    static const int sizes[] = {512, 4096, 8192, 16384, 32768};
    static const double offsets[] = {0, -0.000283085193004, 0.000283085193004,
                                     -1e-6, 1e-6, -1e-9, 1e-9, -1e-12, 1e-12};
    size_t size, offset;
    int cardinal;
    check_near_cardinal_ground(16384, 8219.9881675797642, 13491.278131234938,
                              90.000283085193004);
    for(size = 0; size < sizeof(sizes) / sizeof(sizes[0]); ++size)
        for(cardinal = -180; cardinal <= 180; cardinal += 90)
            for(offset = 0; offset < sizeof(offsets) / sizeof(offsets[0]); ++offset)
                check_near_cardinal_ground(sizes[size], sizes[size] * 0.5,
                                          sizes[size] * 0.5, cardinal + offsets[offset]);
}

static void test_adaptive_accuracy_and_shared_edges(void)
{
    gpu_transform_parameters p = parameters();
    s_drawmethod m = curve_method();
    gpu_curve_mesh mesh = {0};
    size_t i;
    int side, sample;
    p.source_width = 512;
    p.source_height = 128;
    p.centerx = 256;
    p.centery = 64;
    p.x = p.y = 2500;
    p.target_width = p.target_height = 5000;
    m.scalex = m.scaley = 768;
    m.curve.angle = 140;
    m.curve.perspective = 2;
    m.curve.profile = DC_CURVE_PROFILE_SMOOTH;
    assert(gpu_curve_build(&p, &m, &mesh) == GPU_TRANSFORM_DRAWN);
    assert(mesh.quad_count > 8 && mesh.quad_count < 512);
    for(i = 0; i < mesh.quad_count; ++i)
    {
        const gpu_curve_vertex *quad = mesh.vertices + i * 4;
        for(side = 0; side < 2; ++side)
        {
            const gpu_curve_vertex *a = quad + (side ? 3 : 0);
            const gpu_curve_vertex *b = quad + (side ? 2 : 1);
            for(sample = 1; sample <= 3; ++sample)
            {
                double t = sample * 0.25;
                double w = (1 - t) * a->w + t * b->w;
                double x = ((1 - t) * a->x * a->w + t * b->x * b->w) / w;
                double y = ((1 - t) * a->y * a->w + t * b->y * b->w) / w;
                gpu_curve_vertex exact = mapped(&p, &m, (1 - t) * a->u + t * b->u,
                                                (1 - t) * a->v + t * b->v);
                assert(hypot(exact.x - x, exact.y - y) <= GPU_CURVE_PIXEL_TOLERANCE + 0.002);
            }
        }
        if(i + 1 < mesh.quad_count)
        {
            const gpu_curve_vertex *next = quad + 4;
            assert(!memcmp(quad + 1, next, sizeof(*next)));
            assert(!memcmp(quad + 2, next + 3, sizeof(*next)));
        }
    }
    bounded_mesh(&p, &m, &mesh);
    gpu_curve_free(&mesh);
}

static void test_invalid_and_oblique_sources(void)
{
    gpu_transform_parameters p = parameters();
    s_drawmethod m = curve_method();
    gpu_curve_mesh mesh = {0};
    int axis;
    m.curve.angle = 95;
    m.curve.extent = 0.7f;
    for(axis = -180; axis <= 180; axis += 15)
    {
        m.curve.axis = axis;
        assert(gpu_curve_build(&p, &m, &mesh) == GPU_TRANSFORM_DRAWN);
        bounded_mesh(&p, &m, &mesh);
    }
    m.curve.angle = NAN;
    assert(gpu_curve_build(&p, &m, &mesh) == GPU_TRANSFORM_FAILED && !mesh.quad_count);
    m = curve_method();
    m.curve.extent = 0;
    assert(gpu_curve_build(&p, &m, &mesh) == GPU_TRANSFORM_FAILED && !mesh.quad_count);
    m = curve_method();
    m.config |= DRAWMETHOD_CONFIG_AFFINE;
    m.affine.m00 = INFINITY;
    assert(gpu_curve_build(&p, &m, &mesh) == GPU_TRANSFORM_FAILED && !mesh.quad_count);
    m.affine.m00 = 0;
    assert(gpu_curve_build(&p, &m, &mesh) == GPU_TRANSFORM_EMPTY && !mesh.quad_count);
    p.source_width = 0;
    assert(gpu_curve_build(&p, &m, &mesh) == GPU_TRANSFORM_FAILED && !mesh.quad_count);
    gpu_curve_free(&mesh);
}

/* The rotating width basis makes a ruled band non-affine across its diagonal.
 * Check the actual triangle interior as well as the section boundaries so a
 * wide road cannot hide interpolation error in an otherwise accurate edge. */
static void test_adaptive_interior_accuracy(void)
{
    gpu_transform_parameters p = parameters();
    s_drawmethod m = curve_method();
    gpu_curve_mesh mesh = {0};
    static const double weights[][3] = {
        {0.5, 0.0, 0.5}, {0.25, 0.0, 0.75}, {0.75, 0.0, 0.25},
        {1.0 / 3, 1.0 / 3, 1.0 / 3}
    };
    size_t q, sample;
    int plane, triangle, vertex;
    p.source_width = 128;
    p.source_height = 512;
    p.centerx = 64;
    p.centery = 256;
    p.x = p.y = 2500;
    p.target_width = p.target_height = 5000;
    m.scalex = m.scaley = 768;
    m.curve.axis = 90;
    m.curve.angle = 140;
    m.curve.perspective = 2;
    m.curve.profile = DC_CURVE_PROFILE_SMOOTH;
    for(plane = 0; plane <= 90; plane += 45)
    {
        m.curve.plane = plane;
        assert(gpu_curve_build(&p, &m, &mesh) == GPU_TRANSFORM_DRAWN);
        for(q = 0; q < mesh.quad_count; ++q)
            for(triangle = 0; triangle < 2; ++triangle)
                for(sample = 0; sample < sizeof(weights) / sizeof(weights[0]); ++sample)
                {
                    const gpu_curve_vertex *quad = mesh.vertices + q * 4;
                    double x = 0, y = 0, w = 0, u = 0, v = 0;
                    gpu_curve_vertex exact;
                    for(vertex = 0; vertex < 3; ++vertex)
                    {
                        int index = vertex == 0 ? 0 : vertex + triangle;
                        const gpu_curve_vertex *point = quad + index;
                        double weight = weights[sample][vertex];
                        x += weight * point->x * point->w;
                        y += weight * point->y * point->w;
                        w += weight * point->w;
                        u += weight * point->u;
                        v += weight * point->v;
                    }
                    exact = mapped(&p, &m, u, v);
                    assert(hypot(exact.x - x / w, exact.y - y / w) <=
                           GPU_CURVE_PIXEL_TOLERANCE + 0.002);
                }
        bounded_mesh(&p, &m, &mesh);
    }
    gpu_curve_free(&mesh);
}

int main(void)
{
    test_identity_and_anchor();
    test_circular_projection();
    test_smooth_profile_and_focus();
    test_planar_width_and_transform();
    test_oblique_smooth_fold();
    test_clipping_and_perspective_sampling();
    test_near_cardinal_source_corners();
    test_adaptive_accuracy_and_shared_edges();
    test_adaptive_interior_accuracy();
    test_invalid_and_oblique_sources();
    printf("GPU curve geometry: %u independent numeric checks; identity, circular/smooth profiles, focus, width, projection, clipping, near-cardinal source corners, reflections and shared edges passed.\n", checks);
    return 0;
}
