/*
 * Damon V. Caskey
 * 2026-11-20
 * Real desktop/GLES curve draws checked against analytic circle intersections
 * and the independent reconstruction/material oracle. No production geometry
 * helper is used to predict a pixel. Explicit verification is the only reader.
 */
#define main gpu_resample_shared_main
#include "test_gpu_resample.c"
#undef main

static unsigned curve_images, curve_pixels, curve_maximum_error;

static s_drawmethod curve_method(const resample_source *source)
{
    s_drawmethod method = plainmethod;
    method.config |= DRAWMETHOD_CONFIG_CURVE;
    method.curve = (s_drawmethod_curve)DRAWMETHOD_CURVE_INITIALIZER;
    method.centerx = source->width / 2 - source->centerx;
    method.centery = source->height / 2 - source->centery;
    return method;
}

static void curve_source(resample_source *source, int height, int indexed, int masked)
{
    int x, y;
    memset(source, 0, sizeof(*source));
    source->width = 16;
    source->height = height;
    source->indexed = indexed;
    source->masked = masked;
    for(y = 0; y < height; ++y)
        for(x = 0; x < 16; ++x)
        {
            int i = y * 16 + x;
            unsigned color = (20u + 12u*x) | ((30u + 10u*y) << 8) |
                             ((40u + 5u*x + 4u*y) << 16);
            source->rgb[i] = color;
            source->indices[i] = (unsigned char)(i == 255 ? 255 : i + 1);
            source->palette[source->indices[i]] = color;
            source->opacity[i] = (unsigned char)(40 + (x*11 + y*3)%216);
        }
}

/* Analytic cylinder projection: a ray intersects the circle whose center is
 * one radius behind the central tangent. Bisection selects its visible arc,
 * independently of the renderer's mesh subdivision and clipping. */
static long double curve_circle_x(long double s, long double radius, long double focal)
{
    long double depth = radius * (1 - cosl(s / radius));
    return radius * sinl(s / radius) / (focal > 0 ? 1 + depth / focal : 1);
}

/* Returns -1 only in a narrow silhouette/nearest-texel ambiguity band. That
 * band belongs to the bounded geometric approximation, not material error.
 * Flat tests below check every pixel, including all boundaries, exactly. */
static int curve_inverse(const resample_source *source, const s_drawmethod *method,
                         int x, int y, long double *u, long double *v)
{
    long double px = x + 0.5L - 32, py = y + 0.5L - 32;
    long double sx, sy, radius, angle = method->curve.angle * RESAMPLE_PI / 180;
    long double margin = angle == 0 ? 0 : 0.16L;
    if(method->clipw > 0 && method->cliph > 0 &&
       (x < method->clipx || x >= method->clipx + method->clipw ||
        y < method->clipy || y >= method->clipy + method->cliph)) return 0;
    if(method->config & DRAWMETHOD_CONFIG_AFFINE)
    {
        long double a = method->affine.m00, b = method->affine.m01;
        long double c = method->affine.m10, d = method->affine.m11;
        long double determinant = a*d-b*c;
        px -= method->affine.tx; py -= method->affine.ty;
        sx = (d*px-b*py)/determinant;
        sy = (a*py-c*px)/determinant;
    }
    else
    {
        sx = px / (method->scalex / 256.0L);
        sy = py / (method->scaley / 256.0L);
    }
    if(method->config & DRAWMETHOD_CONFIG_FLIP_X) sx = -sx;
    if(method->config & DRAWMETHOD_CONFIG_FLIP_Y) sy = -sy;
    if(angle != 0 && method->curve.plane == 0)
    {
        radius = source->width / angle;
        assert(radius > source->height / 2);
        *u = source->width / 2.0L + radius * atan2l(sx, radius-sy);
        *v = source->height / 2.0L + radius - hypotl(sx, radius-sy);
    }
    else if(angle != 0)
    {
        long double low = -source->width/2.0L, high = -low, s, w;
        long double focal = method->curve.perspective * source->width;
        long double left, right;
        int iteration;
        radius = source->width / angle;
        left = curve_circle_x(low, radius, focal);
        right = curve_circle_x(high, radius, focal);
        if(sx < left-margin || sx > right+margin) return 0;
        if(sx < left+margin || sx > right-margin) return -1;
        for(iteration = 0; iteration < 70; ++iteration)
        {
            s = (low+high)/2;
            if(curve_circle_x(s, radius, focal) < sx) low = s;
            else high = s;
        }
        s = (low+high)/2;
        w = focal > 0 ? 1 + radius*(1-cosl(s/radius))/focal : 1;
        *u = source->width/2.0L + s;
        *v = source->height/2.0L + sy*w;
    }
    else
    {
        *u = source->width/2.0L + sx;
        *v = source->height/2.0L + sy;
    }
    if(*u < -margin || *u >= source->width+margin ||
       *v < -margin || *v >= source->height+margin) return 0;
    if(*u < margin || *u >= source->width-margin ||
       *v < margin || *v >= source->height-margin) return margin == 0 ? 0 : -1;
    *u += method->curve.scrollx;
    *v += method->curve.scrolly;
    if(!method->curve.wrap)
    {
        if(*u < -margin || *u >= source->width+margin ||
           *v < -margin || *v >= source->height+margin) return 0;
        if(*u < margin || *u >= source->width-margin ||
           *v < margin || *v >= source->height-margin) return margin == 0 ? 0 : -1;
    }
    if(angle != 0 && method->resample == DC_RESAMPLE_NEAREST &&
       (fabsl(*u-roundl(*u)) < 0.12L || fabsl(*v-roundl(*v)) < 0.12L)) return -1;
    *u -= 0.5L; *v -= 0.5L;
    return 1;
}

static void curve_verify(const resample_source *source, s_screen *target,
                         const s_drawmethod *method, int tolerance, const char *label)
{
    int x, y, channel, failures = 0, checked = 0, covered = 0;
    gpu_sync_cpu(target);
    for(y = 0; y < target->height; ++y)
        for(x = 0; x < target->width; ++x)
        {
            long double u = 0, v = 0;
            int mapping = curve_inverse(source, method, x, y, &u, &v);
            unsigned expected = RESAMPLE_BACKGROUND;
            unsigned actual = ((unsigned *)target->data)[y*target->width+x] & 0xffffffu;
            int mismatch = 0;
            if(mapping < 0) continue;
            if(mapping)
            {
                expected = resample_material(source, method,
                    resample_reconstruct(source, method, u, v, method->curve.wrap), expected);
                ++covered;
            }
            for(channel = 0; channel < 3; ++channel)
            {
                unsigned a = (actual >> (channel*8)) & 255u;
                unsigned b = (expected >> (channel*8)) & 255u;
                unsigned error = a > b ? a-b : b-a;
                if(error > curve_maximum_error) curve_maximum_error = error;
                if(error > (unsigned)tolerance) mismatch = 1;
            }
            if(mismatch && failures++ < 8)
                fprintf(stderr, "%s sampler %d (%d,%d): expected %06x, actual %06x, UV %.5Lf %.5Lf\n",
                        label, method->resample, x, y, expected, actual, u, v);
            ++checked;
        }
    assert(covered > 30 && checked > 3000 && failures == 0);
    ++curve_images; curve_pixels += checked;
}

static void curve_draw_verify(const resample_source *source, s_screen *target,
                              const s_drawmethod *method, int tolerance, const char *label)
{
    gpu_renderer_stats before;
    assert(gpu_draw_box(target, 0, 0, 64, 64, RESAMPLE_BACKGROUND, &plainmethod));
    before = *gpu_gl_get_stats();
    if(source->sprite) assert(gpu_draw_sprite(target, source->sprite, 32, 32, method));
    else assert(gpu_draw_screen(target, source->screen, 32, 32, method));
    assert(gpu_gl_get_stats()->readbacks == before.readbacks);
    assert(gpu_gl_get_stats()->fallback_draws == before.fallback_draws);
    assert(gpu_gl_get_stats()->curve_draws == before.curve_draws + 1);
    curve_verify(source, target, method, tolerance, label);
}

static void curve_test_identity_and_scroll(void)
{
    int mode, flip, indexed;
    s_screen *target = resample_screen(64, 64, PIXEL_32, 0);
    for(indexed = 0; indexed < 2; ++indexed)
    {
        resample_source source;
        s_drawmethod method;
        curve_source(&source, 12, indexed, 0);
        resample_prepare_source(&source, indexed);
        method = curve_method(&source);
        method.scalex = method.scaley = 512;
        for(mode = DC_RESAMPLE_NEAREST; mode < DC_RESAMPLE_END; ++mode)
            for(flip = 0; flip < 4; ++flip)
            {
                method.resample = (e_drawmethod_resample)mode;
                method.config &= ~(DRAWMETHOD_CONFIG_FLIP_X | DRAWMETHOD_CONFIG_FLIP_Y);
                if(flip & 1) method.config |= DRAWMETHOD_CONFIG_FLIP_X;
                if(flip & 2) method.config |= DRAWMETHOD_CONFIG_FLIP_Y;
                resample_compare(&source, target, &method, 32, 32, "flat curve identity", mode == 0 ? 0 : 1);
            }
        method.config &= ~(DRAWMETHOD_CONFIG_FLIP_X | DRAWMETHOD_CONFIG_FLIP_Y);
        method.curve.scrollx = -3.125f; method.curve.scrolly = 15.125f;
        method.curve.wrap = DC_CURVE_WRAP_REPEAT;
        for(mode = DC_RESAMPLE_NEAREST; mode < DC_RESAMPLE_END; ++mode)
        {
            method.resample = (e_drawmethod_resample)mode;
            curve_draw_verify(&source, target, &method, mode == 0 ? 0 : 1, "negative repeat phase");
        }
        method.curve.wrap = DC_CURVE_WRAP_CLIP;
        method.curve.scrolly = -1.125f;
        curve_draw_verify(&source, target, &method, 1, "non-repeating source boundary");
        method.config &= ~DRAWMETHOD_CONFIG_CURVE;
        method.curve.angle = NAN;
        resample_compare(&source, target, &method, 32, 32, "disabled invalid curve is inert", 1);
        resample_release_source(&source);
    }
    resample_release_screen(target);
}

static void curve_test_analytic_surfaces(void)
{
    const float angles[] = {110, 110, -85, 80};
    int shape, mode;
    s_screen *target = resample_screen(64, 64, PIXEL_32, 0);
    for(shape = 0; shape < 4; ++shape)
    {
        resample_source source;
        s_drawmethod method;
        curve_source(&source, shape == 3 ? 6 : 12, 0, 0);
        resample_prepare_source(&source, 0);
        method = curve_method(&source);
        method.curve.angle = angles[shape];
        method.curve.plane = shape == 3 ? 0 : 90;
        method.curve.perspective = shape == 1 || shape == 2 ? 1.25f : 0;
        method.scalex = method.scaley = 768;
        for(mode = DC_RESAMPLE_NEAREST; mode < DC_RESAMPLE_END; ++mode)
        {
            method.resample = (e_drawmethod_resample)mode;
            curve_draw_verify(&source, target, &method, mode == 0 ? 0 : 2,
                              shape == 3 ? "constant-width road" : "analytic cylinder");
        }
        method.config |= DRAWMETHOD_CONFIG_AFFINE | DRAWMETHOD_CONFIG_FLIP_X;
        method.affine = (affine_transform){2.5f, .375f, -.25f, 2.75f, .125f, -.375f};
        method.resample = DC_RESAMPLE_BILINEAR;
        curve_draw_verify(&source, target, &method, 2, "curve then affine reflection");
        method.clipx = 24; method.clipy = 23; method.clipw = 16; method.cliph = 17;
        curve_draw_verify(&source, target, &method, 2, "curve target-space clip");
        resample_release_source(&source);
    }
    resample_release_screen(target);
}

static void curve_test_materials_and_cache(void)
{
    int material, masked;
    s_screen *target = resample_screen(64, 64, PIXEL_32, 0);
    for(masked = 0; masked < 2; ++masked)
    {
        resample_source source;
        s_drawmethod method;
        uint64_t builds, uploads;
        curve_source(&source, 12, 1, masked);
        source.indices[4*16+7] = 0;
        resample_prepare_source(&source, 1);
        method = curve_method(&source);
        method.curve.angle = 100;
        method.curve.perspective = 2;
        method.scalex = method.scaley = 768;
        curve_draw_verify(&source, target, &method, 0, "indexed curve and mask");
        builds = gpu_gl_get_stats()->curve_mesh_builds;
        uploads = gpu_gl_get_stats()->uploads;
        for(material = 0; material < 7; ++material)
        {
            s_drawmethod changed = method;
            if(material < 6) changed.alpha = material + 1;
            else
            {
                changed.tintmode = 2; changed.tintcolor = 0x90b0d0;
                changed.colors.chroma_enabled = 1;
                changed.colors.chroma[0] = source.palette[20];
                changed.colors.substitute_enabled = 1;
                changed.colors.substitute[0] = (s_color_substitute){0x382c2d, 0x204060};
            }
            curve_draw_verify(&source, target, &changed, 0, "curve material arithmetic");
            assert(gpu_gl_get_stats()->curve_mesh_builds == builds);
            assert(gpu_gl_get_stats()->uploads == uploads);
        }
        method.curve.scrollx = 3;
        method.curve.wrap = 1;
        curve_draw_verify(&source, target, &method, 0, "phase keeps cached geometry");
        assert(gpu_gl_get_stats()->curve_mesh_builds == builds);
        method.clipx = 24 + masked; method.clipy = 24; method.clipw = method.cliph = 16;
        curve_draw_verify(&source, target, &method, 0, "clip changes cached geometry");
        assert(gpu_gl_get_stats()->curve_mesh_builds == builds + 1);
        resample_release_source(&source);
    }
    resample_release_screen(target);
}

static void curve_test_queue_and_composition(void)
{
    resample_source source;
    s_screen *target = resample_screen(64, 64, PIXEL_32, 0);
    s_screen *composed = resample_screen(64, 64, PIXEL_32, 0);
    s_drawmethod method, captured, outer;
    gpu_renderer_stats before;
    int x, y;
    curve_source(&source, 12, 0, 0);
    source.screen = resample_screen(16, 12, PIXEL_32, 0);
    for(y = 0; y < 12; ++y)
        for(x = 0; x < 16; ++x)
            assert(gpu_draw_box(source.screen, x, y, 1, 1, source.rgb[y*16+x], &plainmethod));
    method = curve_method(&source);
    method.curve.angle = 105; method.curve.perspective = 1.5f;
    method.scalex = method.scaley = 768;
    method.resample = DC_RESAMPLE_BILINEAR;
    captured = method;
    spriteq_unlock(); spriteq_clear();
    spriteq_add_screen(32, 32, 7, source.screen, &method, 0);
    method.curve.angle = -150; method.curve.perspective = 0;
    assert(gpu_draw_box(target, 0, 0, 64, 64, RESAMPLE_BACKGROUND, &plainmethod));
    before = *gpu_gl_get_stats();
    spriteq_draw(target, 0, 7, 7, 0, 0);
    assert(gpu_gl_get_stats()->uploads == before.uploads);
    assert(gpu_gl_get_stats()->readbacks == before.readbacks);
    for(x = 0; x < 16*12; ++x) assert(((unsigned *)source.screen->data)[x] == 0);
    /* The captured first-pass surface feeds a second curved draw before any
     * verification readback. Both source shadows must still be stale here. */
    for(x = 0; x < 64*64; ++x) assert(((unsigned *)target->data)[x] == 0);
    outer = plainmethod;
    outer.config |= DRAWMETHOD_CONFIG_CURVE;
    outer.curve = (s_drawmethod_curve)DRAWMETHOD_CURVE_INITIALIZER;
    outer.curve.angle = -70; outer.curve.axis = 90;
    outer.curve.perspective = 2;
    outer.centerx = outer.centery = 32;
    outer.resample = DC_RESAMPLE_BILINEAR;
    assert(gpu_draw_box(composed, 0, 0, 64, 64, RESAMPLE_BACKGROUND, &plainmethod));
    before = *gpu_gl_get_stats();
    assert(gpu_draw_screen(composed, target, 32, 32, &outer));
    assert(gpu_gl_get_stats()->curve_draws == before.curve_draws + 1);
    assert(gpu_gl_get_stats()->uploads == before.uploads);
    assert(gpu_gl_get_stats()->readbacks == before.readbacks);
    curve_verify(&source, target, &captured, 2, "queued inline curve snapshot");
    gpu_sync_cpu(composed);
    assert((((unsigned *)composed->data)[32*64+32] & 0xffffffu) != RESAMPLE_BACKGROUND);
    assert(memcmp(composed->data, target->data, 64*64*sizeof(unsigned)) != 0);
    spriteq_clear();
    resample_release_source(&source);
    resample_release_screen(target);
    resample_release_screen(composed);
}

static void curve_test_self_copy_and_rejection(void)
{
    resample_source source;
    s_screen *target = resample_screen(64, 64, PIXEL_32, 0);
    s_screen *reference = resample_screen(64, 64, PIXEL_32, 0);
    s_screen *separate = resample_screen(64, 64, PIXEL_32, 0);
    s_drawmethod method, plain = plainmethod;
    gpu_renderer_stats before;
    unsigned snapshot[4096];
    curve_source(&source, 12, 0, 0);
    resample_prepare_source(&source, 0);
    assert(gpu_draw_box(target, 0, 0, 64, 64, RESAMPLE_BACKGROUND, &plain));
    assert(gpu_draw_screen(target, source.screen, 24, 26, &plain));
    assert(gpu_copy_screen(reference, target));
    assert(gpu_copy_screen(separate, target));
    method = plainmethod;
    method.config |= DRAWMETHOD_CONFIG_CURVE;
    method.curve = (s_drawmethod_curve)DRAWMETHOD_CURVE_INITIALIZER;
    method.curve.angle = 85; method.curve.perspective = 2;
    method.centerx = method.centery = 32;
    method.resample = DC_RESAMPLE_BICUBIC_MITCHELL;
    before = *gpu_gl_get_stats();
    assert(gpu_draw_screen(target, target, 32, 32, &method));
    assert(gpu_gl_get_stats()->destination_copies == before.destination_copies + 1);
    assert(gpu_gl_get_stats()->uploads == before.uploads);
    assert(gpu_gl_get_stats()->readbacks == before.readbacks);
    /* Rendering from an independent GPU copy must match the overlapping draw
     * byte for byte. The first operation cannot sample its own writes. */
    assert(gpu_draw_screen(separate, reference, 32, 32, &method));
    gpu_sync_cpu(target); gpu_sync_cpu(separate);
    assert(memcmp(target->data, separate->data, sizeof(snapshot)) == 0);
    memcpy(snapshot, target->data, sizeof(snapshot));
    before = *gpu_gl_get_stats();
    method.curve.angle = NAN;
    assert(!gpu_draw_screen(target, source.screen, 32, 32, &method));
    method.curve.angle = 170; method.curve.plane = 0; method.curve.extent = .01f;
    assert(!gpu_draw_screen(target, source.screen, 32, 32, &method));
    assert(gpu_gl_get_stats()->draws == before.draws);
    assert(gpu_gl_get_stats()->readbacks == before.readbacks);
    /* This legacy counter records rejected draws; no software renderer is
     * linked into this executable and no destination pixels were changed. */
    assert(gpu_gl_get_stats()->fallback_draws == before.fallback_draws + 2);
    gpu_sync_cpu(target);
    assert(memcmp(snapshot, target->data, sizeof(snapshot)) == 0);
    resample_release_source(&source);
    resample_release_screen(target);
    resample_release_screen(reference);
    resample_release_screen(separate);
}

int main(void)
{
    SDL_Window *window;
    SDL_GLContext context;
    if(SDL_Init(SDL_INIT_VIDEO) != 0) return 77;
    gpu_test_context_attributes();
    window = SDL_CreateWindow("OpenBOR curve tests", 0, 0, 64, 64,
                              SDL_WINDOW_OPENGL | SDL_WINDOW_HIDDEN);
    context = window ? SDL_GL_CreateContext(window) : NULL;
    if(!context)
    {
        if(window) SDL_DestroyWindow(window);
        SDL_Quit(); return 77;
    }
    assert(LoadGLFunctions() && gpu_gl_init());
    curve_test_identity_and_scroll();
    curve_test_analytic_surfaces();
    curve_test_materials_and_cache();
    curve_test_queue_and_composition();
    assert(gpu_gl_get_stats()->fallback_draws == 0);
    curve_test_self_copy_and_rejection();
    /* Retiring a used auxiliary stream and cache must permit clean reuse. */
    gpu_gl_shutdown();
    assert(gpu_gl_init());
    curve_test_analytic_surfaces();
    gpu_gl_shutdown();
    SDL_GL_DeleteContext(context); SDL_DestroyWindow(window); SDL_Quit();
    printf("GPU curves passed: %u analytic/phase/material images, %u checked pixels, maximum byte error %u; %u complete flat images; GPU-only composition, snapshots, cache, rejection and stream recreation.\n",
           curve_images, curve_pixels, curve_maximum_error, resample_images_checked);
    return 0;
}
