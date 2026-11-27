/*
 * Damon V. Caskey - 2026-11-27
 * Render real meshmaps with the production backend. Independent screen-space
 * rectangles and ray/plane intersections predict pixels; expected results do
 * not call meshmap sampling, projection, or triangulation helpers.
 */
#define main meshmap_shared_resample_main
#include "test_gpu_resample.c"
#undef main
#include "meshmap.h"
#include "meshmap_render.h"
#include <limits.h>

static unsigned meshmap_images_checked, meshmap_pixels_checked;

static void meshmap_test_artwork_free(void *pointer)
{
    s_screen *screen = pointer;
    gpu_release_screen(screen);
    free(screen);
}

static s_screen *meshmap_test_artwork(int indexed, unsigned tint)
{
    s_screen *screen = resample_screen(4, 4, indexed ? PIXEL_x8 : PIXEL_32, 0);
    unsigned index;
    for(index = 0; index < 16; ++index)
    {
        unsigned color = ((index % 4) * 53u + 17u) |
                         (((index / 4) * 47u + 21u) << 8) | (0x91u << 16);
        color ^= tint;
        if(index == 5) color = 0; /* Black artwork remains visible terrain. */
        if(indexed)
        {
            screen->data[index] = (unsigned char)index;
            ((unsigned *)screen->palette)[index] = color;
        }
        else ((unsigned *)screen->data)[index] = color;
    }
    return screen;
}

static unsigned meshmap_test_art_pixel(const s_screen *screen, unsigned x, unsigned y)
{
    unsigned index = y * (unsigned)screen->width + x;
    return screen->pixelformat == PIXEL_x8
        ? ((unsigned *)screen->palette)[screen->data[index]]
        : ((unsigned *)screen->data)[index];
}

static s_meshmap *meshmap_test_map(s_meshmap_store *store, e_meshmap_axis axis,
                                  int indexed, int hole)
{
    s_meshmap *map = meshmap_create(store, 0);
    s_meshmap_config config;
    uint16_t samples[16];
    unsigned index;
    assert(map);
    for(index = 0; index < 16; ++index) samples[index] = 1;
    if(hole) samples[0] = 0;
    meshmap_config_default(&config);
    config.axis = axis;
    config.spacing_u = config.spacing_v = 8;
    config.min_height = config.max_height = 0;
    config.layer = 12;
    assert(meshmap_configure(map, &config));
    assert(meshmap_set_geometry(map, 4, 4, samples, "test-grid"));
    assert(meshmap_set_artwork(map, meshmap_test_artwork(indexed, 0), meshmap_test_artwork_free, "test-art"));
    return map;
}

static void meshmap_test_axis_view(s_meshmap_view *view, e_meshmap_axis axis)
{
    meshmap_view_orthographic(view, 8, 8);
    if(axis == MESHMAP_AXIS_Z)
    {
        view->projection[6] = 0;
    }
    else if(axis == MESHMAP_AXIS_X)
    {
        view->projection[0] = 0;
        view->projection[2] = 1;
        view->projection[6] = 0;
    }
}

static void meshmap_test_flat_axes(void)
{
    int axis, indexed;
    for(axis = MESHMAP_AXIS_X; axis <= MESHMAP_AXIS_Z; ++axis)
    {
        for(indexed = 0; indexed <= 1; ++indexed)
        {
            s_meshmap_store *store = meshmap_store_create();
            s_meshmap *map = meshmap_test_map(store, (e_meshmap_axis)axis, indexed, 1);
            s_meshmap_config config = map->config;
            s_screen *target = resample_screen(64, 64, PIXEL_32, RESAMPLE_BACKGROUND);
            s_meshmap_view view;
            unsigned x, y;
            unsigned long readbacks = gpu_gl_get_stats()->readbacks;
            /* Scripted views can draw without an automatic scene copy. */
            config.flags &= ~MESHMAP_DRAW;
            assert(meshmap_configure(map, &config));
            meshmap_test_axis_view(&view, (e_meshmap_axis)axis);
            assert(meshmap_draw(store, 0, target, &view, NULL));
            assert(gpu_gl_get_stats()->readbacks == readbacks);
            gpu_sync_cpu(target);
            for(y = 0; y < 64; ++y)
                for(x = 0; x < 64; ++x)
                {
                    unsigned expected = RESAMPLE_BACKGROUND;
                    if(x >= 8 && x < 32 && y >= 8 && y < 32 && !(x < 16 && y < 16))
                        expected = meshmap_test_art_pixel(map->artwork,
                                      (unsigned)((x - 8 + 0.5) * 4 / 24),
                                      (unsigned)((y - 8 + 0.5) * 4 / 24));
                    if((((unsigned *)target->data)[y * 64 + x] & 0xffffffu) != expected)
                    {
                        fprintf(stderr, "Flat mismatch axis=%d indexed=%d x=%u y=%u got=%06x expected=%06x\n",
                                axis, indexed, x, y, ((unsigned *)target->data)[y * 64 + x] & 0xffffffu, expected);
                        assert(0);
                    }
                    ++meshmap_pixels_checked;
                }
            ++meshmap_images_checked;
            meshmap_test_artwork_free(target);
            meshmap_store_free(store);
        }
    }
}

static void meshmap_test_perspective(int crossing_near)
{
    s_meshmap_store *store = meshmap_store_create();
    s_meshmap *map = meshmap_create(store, 0);
    s_meshmap_config config;
    s_meshmap_view view;
    uint16_t samples[4] = {1, 1, 1, 1};
    s_screen *target = resample_screen(64, 64, PIXEL_32, RESAMPLE_BACKGROUND);
    double half_width = crossing_near ? 1 : 8;
    double z_start = crossing_near ? -1 : 4;
    double z_span = crossing_near ? 4 : 16;
    double focal = crossing_near ? 8 : 20;
    double camera_height = crossing_near ? 1 : 8;
    unsigned x, y;
    unsigned long readbacks = gpu_gl_get_stats()->readbacks;
    meshmap_config_default(&config);
    config.axis = MESHMAP_AXIS_Y;
    config.min_height = config.max_height = 0;
    config.origin.x = -half_width;
    config.origin.z = z_start;
    config.spacing_u = half_width * 2;
    config.spacing_v = z_span;
    assert(map && meshmap_configure(map, &config));
    assert(meshmap_set_geometry(map, 2, 2, samples, "perspective"));
    assert(meshmap_set_artwork(map, meshmap_test_artwork(0, 0), meshmap_test_artwork_free, "art"));
    memset(&view, 0, sizeof(view));
    view.projection[0] = focal;
    view.projection[2] = 32;
    view.projection[5] = -focal;
    view.projection[6] = 32;
    view.projection[7] = focal * camera_height;
    view.projection[10] = 1;
    view.near_w = crossing_near ? 0.5 : 0.0001;
    assert(meshmap_draw(store, 0, target, &view, NULL));
    assert(gpu_gl_get_stats()->readbacks == readbacks);
    gpu_sync_cpu(target);
    for(y = 0; y < 64; ++y)
    {
        for(x = 0; x < 64; ++x)
        {
            unsigned expected = RESAMPLE_BACKGROUND;
            double dy = y + 0.5 - 32;
            if(dy > 0)
            {
                double world_z = focal * camera_height / dy;
                double world_x = (x + 0.5 - 32) * world_z / focal;
                if(world_z >= fmax(z_start, view.near_w) && world_z < z_start + z_span &&
                   world_x >= -half_width && world_x < half_width)
                {
                    double u = (world_x + half_width) / (half_width * 2) * 4;
                    double v = (world_z - z_start) / z_span * 4;
                    /* Skip only mathematically exact texel boundaries, where
                     * the GPU's permitted float rounding can choose either
                     * neighbor. All covered geometry pixels remain checked. */
                    if(fabs(u - nearbyint(u)) < 0.00001 || fabs(v - nearbyint(v)) < 0.00001) continue;
                    expected = meshmap_test_art_pixel(map->artwork, (unsigned)u, (unsigned)v);
                }
            }
            if((((unsigned *)target->data)[y * 64 + x] & 0xffffffu) != expected)
            {
                fprintf(stderr, "Perspective mismatch near=%d x=%u y=%u got=%06x expected=%06x\n",
                        crossing_near, x, y, ((unsigned *)target->data)[y * 64 + x] & 0xffffffu, expected);
                assert(0);
            }
            ++meshmap_pixels_checked;
        }
    }
    ++meshmap_images_checked;
    meshmap_test_artwork_free(target);
    meshmap_store_free(store);
}

static void meshmap_test_queue_lifetime(void)
{
    s_meshmap_store *store = meshmap_store_create();
    s_meshmap *map = meshmap_test_map(store, MESHMAP_AXIS_Y, 0, 0);
    s_meshmap_config config;
    s_screen *target = resample_screen(64, 64, PIXEL_32, RESAMPLE_BACKGROUND);
    unsigned index;
    spriteq_unlock();
    spriteq_clear();
    meshmap_queue_native(store, 8, 8);
    assert(meshmap_remove(store, 0));
    spriteq_draw(target, 0, INT_MIN, INT_MAX, 0, 0);
    gpu_sync_cpu(target);
    for(index = 0; index < 64 * 64; ++index)
        assert(((unsigned *)target->data)[index] == RESAMPLE_BACKGROUND);
    spriteq_clear();
    map = meshmap_test_map(store, MESHMAP_AXIS_Y, 0, 0);
    meshmap_queue_native(store, 8, 8);
    config = map->config;
    config.flags &= ~MESHMAP_DRAW;
    assert(meshmap_configure(map, &config));
    spriteq_draw(target, 0, INT_MIN, INT_MAX, 0, 0);
    gpu_sync_cpu(target);
    for(index = 0; index < 64 * 64; ++index)
        assert(((unsigned *)target->data)[index] == RESAMPLE_BACKGROUND);
    spriteq_clear();
    config.flags |= MESHMAP_DRAW;
    assert(meshmap_configure(map, &config));
    meshmap_queue_native(store, 8, 8);
    assert(meshmap_set_artwork(map, meshmap_test_artwork(1, 0x112233), meshmap_test_artwork_free, "replacement"));
    spriteq_draw(target, 0, INT_MIN, INT_MAX, 0, 0);
    gpu_sync_cpu(target);
    assert((((unsigned *)target->data)[9 * 64 + 9] & 0xffffffu) == meshmap_test_art_pixel(map->artwork, 0, 0));
    spriteq_clear();
    meshmap_test_artwork_free(target);
    meshmap_store_free(store);
}

static void meshmap_test_rejected_projection(void)
{
    s_meshmap_store *store = meshmap_store_create();
    s_meshmap *map = meshmap_test_map(store, MESHMAP_AXIS_Y, 0, 0);
    s_screen *target = resample_screen(64, 64, PIXEL_32, RESAMPLE_BACKGROUND);
    s_meshmap_view view;
    unsigned index;
    meshmap_view_orthographic(&view, 8, 8);
    view.projection[0] = NAN;
    assert(!meshmap_draw(store, 0, target, &view, NULL));
    gpu_sync_cpu(target);
    for(index = 0; index < 64 * 64; ++index)
        assert(((unsigned *)target->data)[index] == RESAMPLE_BACKGROUND);
    assert(map->samples[0] == 1);
    meshmap_test_artwork_free(target);
    meshmap_store_free(store);
}

int main(void)
{
    SDL_Window *window;
    SDL_GLContext context;
    if(SDL_Init(SDL_INIT_VIDEO) != 0)
    {
        fprintf(stderr, "Meshmap test video initialization failed: %s\n", SDL_GetError());
        return 77;
    }
    gpu_test_context_attributes();
    window = SDL_CreateWindow("OpenBOR meshmap tests", 0, 0, 64, 64,
                              SDL_WINDOW_OPENGL | SDL_WINDOW_HIDDEN);
    context = window ? SDL_GL_CreateContext(window) : NULL;
    if(!context)
    {
        fprintf(stderr, "Meshmap test GL context unavailable: %s\n", SDL_GetError());
        if(window) SDL_DestroyWindow(window);
        SDL_Quit();
        return 77;
    }
    assert(LoadGLFunctions() && gpu_gl_init());
    meshmap_test_flat_axes();
    meshmap_test_perspective(0);
    meshmap_test_perspective(1);
    meshmap_test_queue_lifetime();
    meshmap_test_rejected_projection();
    assert(gpu_gl_get_stats()->fallback_draws == 0 && gpu_gl_get_stats()->mesh_draws >= 9);
    meshmap_render_shutdown();
    gpu_gl_shutdown();
    assert(gpu_gl_init());
    meshmap_test_perspective(1);
    meshmap_render_shutdown();
    gpu_gl_shutdown();
    SDL_GL_DeleteContext(context);
    SDL_DestroyWindow(window);
    SDL_Quit();
    printf("Meshmap rendering passed: %u independent images, %u pixels; three axes, holes, indexed/RGB textures, perspective and near clipping, explicit/queued visibility, queued replacement/removal, and renderer reinitialization.\n",
           meshmap_images_checked, meshmap_pixels_checked);
    return 0;
}
