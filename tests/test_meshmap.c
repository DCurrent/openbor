/*
 * Damon V. Caskey - 2026-11-26.
 * Pure native mesh-map behavior, without an image decoder or graphics context.
 * The fixtures specify physical surfaces and solid regions independently of
 * the engine adapter, so successful rendering cannot hide a collision error.
 */
#include "meshmap.h"

#include <float.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static unsigned checks;
static unsigned failures;

#define CHECK(condition) do { checks++; if(!(condition)) { failures++; \
    fprintf(stderr, "%s:%d: %s\n", __FILE__, __LINE__, #condition); } } while(0)
#define NEAR(actual, expected) CHECK(fabs((actual) - (expected)) < 1.0e-8)

static s_meshmap_vec3 point(double x, double y, double z)
{
    s_meshmap_vec3 result = {x, y, z};
    return result;
}

static s_meshmap *flat(s_meshmap_store *store, uint32_t id, e_meshmap_axis axis,
                        unsigned width, unsigned height, double elevation)
{
    s_meshmap *map = meshmap_create(store, id);
    s_meshmap_config config;
    uint16_t *samples = malloc((size_t)width * height * sizeof(*samples));
    size_t index;
    CHECK(map && samples);
    if(!map || !samples)
    {
        free(samples);
        return NULL;
    }
    meshmap_config_default(&config);
    config.axis = axis;
    config.spacing_u = config.spacing_v = 10.0;
    config.min_height = config.max_height = elevation;
    CHECK(meshmap_configure(map, &config));
    for(index = 0; index < (size_t)width * height; index++)
    {
        samples[index] = 1;
    }
    CHECK(meshmap_set_geometry(map, width, height, samples, "test/geometry.png"));
    free(samples);
    return map;
}

static int count_map(const s_meshmap *map, void *context)
{
    unsigned *count = context;
    CHECK(map != NULL);
    (*count)++;
    return 1;
}

static void test_sparse_banks(void)
{
    s_meshmap_store *store = meshmap_store_create();
    s_meshmap *first, *last;
    s_meshmap_bank *area, *room;
    uint32_t id = 99;
    uint64_t generation;
    unsigned count;
    CHECK(store != NULL);
    CHECK(store->bank_count == 1 && store->map_count == 0);
    CHECK(meshmap_pack_id(0, 0, 0, 0, &id) && id == 0);
    CHECK(meshmap_pack_id(63, 63, 63, 63, &id) && id == MESHMAP_ID_MAX);
    CHECK(!meshmap_pack_id(64, 0, 0, 0, &id));
    CHECK(!meshmap_pack_id(0, 64, 0, 0, &id));
    CHECK(!meshmap_pack_id(0, 0, 64, 0, &id));
    CHECK(!meshmap_pack_id(0, 0, 0, 64, &id));
    CHECK(meshmap_id_slot(MESHMAP_ID_MAX, 0) == 63);
    CHECK(meshmap_id_slot(MESHMAP_ID_MAX, 3) == 63);
    CHECK(meshmap_id_slot(MESHMAP_ID_NONE, 0) == 64);
    first = meshmap_create(store, 0);
    last = meshmap_create(store, MESHMAP_ID_MAX);
    CHECK(first && last && first != last);
    CHECK(store->bank_count == 7 && store->map_count == 2);
    CHECK(meshmap_get(store, 0) == first);
    CHECK(meshmap_get_const(store, MESHMAP_ID_MAX) == last);
    CHECK(!meshmap_get(store, MESHMAP_ID_NONE));
    CHECK(!meshmap_create(store, MESHMAP_ID_MAX + 1));
    CHECK(meshmap_create(store, 0) == first && store->map_count == 2);
    CHECK(meshmap_is_active(store, 0) && meshmap_is_active(store, MESHMAP_ID_MAX));
    CHECK(store->root.present_mask == (UINT64_C(1) | (UINT64_C(1) << 63)));
    CHECK(!meshmap_bank_set_mask(&store->root, UINT64_MAX));
    CHECK(meshmap_bank_set_slot(&store->root, 63, 0));
    CHECK(!meshmap_is_active(store, MESHMAP_ID_MAX) && meshmap_get(store, MESHMAP_ID_MAX) == last);
    count = 0;
    CHECK(meshmap_foreach(store, NULL, count_map, &count) && count == 1);
    CHECK(meshmap_bank_set_mask(&store->root, store->root.present_mask));
    CHECK(meshmap_set_active(store, 0, 0));
    CHECK(!meshmap_is_active(store, 0) && meshmap_is_active(store, MESHMAP_ID_MAX));
    CHECK(meshmap_set_active(store, 0, 1));
    CHECK(!meshmap_set_active(store, 1, 1));
    room = meshmap_bank_get(store, 3, MESHMAP_ID_MAX, 0);
    CHECK(room && room->present_mask == (UINT64_C(1) << 63));
    area = meshmap_bank_get(store, 1, MESHMAP_ID_MAX, 0);
    CHECK(area && area->depth == 1);
    generation = first->generation;
    CHECK(meshmap_remove(store, 0));
    CHECK(!meshmap_remove(store, 0));
    CHECK(store->bank_count == 7 && store->map_count == 1);
    first = meshmap_create(store, 0);
    CHECK(first && first->generation != generation);
    CHECK(meshmap_bank_remove(store, 1, MESHMAP_ID_MAX));
    CHECK(!meshmap_get(store, MESHMAP_ID_MAX) && meshmap_get(store, 0) == first);
    CHECK(store->bank_count == 4 && store->map_count == 1);
    generation = first->generation;
    CHECK(meshmap_bank_remove(store, 0, 0));
    CHECK(store->bank_count == 1 && store->map_count == 0);
    CHECK(store->root.active_mask == 0 && store->root.present_mask == 0);
    first = meshmap_create(store, 0);
    CHECK(first && first->generation != generation);
    CHECK(meshmap_bank_get(store, 2, 1u << 18, 1));
    CHECK(store->bank_count == 6);
    CHECK(meshmap_bank_remove(store, 1, 1u << 18));
    CHECK(store->bank_count == 4);
    meshmap_store_free(store);
}

static unsigned art_releases;
static void release_art(void *artwork)
{
    art_releases++;
    free(artwork);
}

static void test_resource_transactions(void)
{
    s_meshmap_store *store = meshmap_store_create();
    s_meshmap *map = meshmap_create(store, 0);
    s_meshmap_config config, old;
    uint16_t samples[16];
    uint16_t *old_samples;
    uint64_t revision;
    unsigned index;
    int *art1 = malloc(sizeof(*art1)), *art2 = malloc(sizeof(*art2));
    CHECK(art1 && art2);
    for(index = 0; index < 16; index++)
    {
        samples[index] = 1;
    }
    CHECK(meshmap_set_geometry(map, 4, 4, samples, "original.png"));
    CHECK(map->cell_count == 9 && map->present_cells == 9);
    samples[0] = 0;
    CHECK(map->samples[0] == 1);
    CHECK(meshmap_set_sample(map, 1, 1, 0));
    CHECK(map->present_cells == 5);
    CHECK(!meshmap_cell_present(map, 0, 0) && !meshmap_cell_present(map, 1, 1));
    CHECK(meshmap_cell_present(map, 2, 2));
    CHECK(meshmap_set_sample(map, 1, 1, 65535));
    CHECK(map->present_cells == 9);
    revision = map->revision;
    old_samples = map->samples;
    CHECK(!meshmap_set_geometry(map, 1, 4, samples, "bad.png"));
    CHECK(!meshmap_set_geometry(map, 4, 4, NULL, "bad.png"));
    CHECK(!meshmap_set_geometry(map, UINT32_MAX, UINT32_MAX, samples, "bad.png"));
    CHECK(!meshmap_set_sample(map, 4, 0, 1));
    CHECK(map->revision == revision && map->samples == old_samples);
    CHECK(strcmp(map->geometry_path, "original.png") == 0);
    CHECK(meshmap_set_geometry(map, 4, 4, map->samples, map->geometry_path));
    CHECK(map->present_cells == 9 && strcmp(map->geometry_path, "original.png") == 0);
    config = old = map->config;
    revision = map->revision;
    config.spacing_u = -1;
    CHECK(!meshmap_configure(map, &config));
    config = old;
    config.min_height = 2;
    config.max_height = 1;
    CHECK(!meshmap_configure(map, &config));
    config = old;
    config.origin.x = NAN;
    CHECK(!meshmap_configure(map, &config));
    config = old;
    config.min_height = -DBL_MAX;
    config.max_height = DBL_MAX;
    CHECK(!meshmap_configure(map, &config));
    config = old;
    config.spacing_u = DBL_MAX;
    CHECK(!meshmap_configure(map, &config));
    CHECK(memcmp(&map->config, &old, sizeof(old)) == 0 && map->revision == revision);
    config = old;
    config.min_height = -100;
    config.max_height = 100;
    CHECK(meshmap_configure(map, &config));
    NEAR(meshmap_decode_height(map, 1), -100.0);
    NEAR(meshmap_decode_height(map, 32768), 0.0);
    NEAR(meshmap_decode_height(map, 65535), 100.0);
    CHECK(isnan(meshmap_decode_height(map, 0)));
    config.min_height = config.max_height = -9;
    CHECK(meshmap_configure(map, &config));
    NEAR(meshmap_decode_height(map, 65535), -9.0);
    art_releases = 0;
    CHECK(meshmap_set_artwork(map, art1, release_art, "first.png"));
    CHECK(meshmap_set_artwork(map, art1, release_art, map->artwork_path));
    CHECK(art_releases == 0 && strcmp(map->artwork_path, "first.png") == 0);
    CHECK(meshmap_set_artwork(map, art2, release_art, "second.png"));
    CHECK(art_releases == 1 && map->artwork == art2);
    CHECK(meshmap_set_geometry(map, 0, 0, NULL, NULL));
    CHECK(!map->samples && !map->cells && !map->geometry_path && map->width == 0 && map->present_cells == 0);
    CHECK(map->artwork == art2);
    meshmap_store_free(store);
    CHECK(art_releases == 2);
}

static void test_triangle_geometry(void)
{
    s_meshmap_store *store = meshmap_store_create();
    s_meshmap *map = meshmap_create(store, 0);
    s_meshmap_config config;
    s_meshmap_sample sample;
    s_meshmap_triangle triangle;
    s_meshmap_vertex vertex;
    uint16_t samples[4] = {1, 11, 21, 61};
    unsigned axis, side, tri;
    meshmap_config_default(&config);
    config.origin = point(100, 200, 300);
    config.min_height = 0;
    config.max_height = 65534;
    config.spacing_u = config.spacing_v = 10;
    CHECK(meshmap_configure(map, &config));
    CHECK(meshmap_set_geometry(map, 2, 2, samples, NULL));
    CHECK(meshmap_sample_local(map, 7.5, 2.5, &sample));
    NEAR(sample.height, 20.0);
    NEAR(sample.position.x, 107.5);
    NEAR(sample.position.y, 220.0);
    NEAR(sample.position.z, 302.5);
    CHECK(sample.triangle == 0);
    NEAR(sample.normal.x, -1.0 / sqrt(27.0));
    NEAR(sample.normal.y, 1.0 / sqrt(27.0));
    NEAR(sample.normal.z, -5.0 / sqrt(27.0));
    CHECK(meshmap_sample_local(map, 2.5, 7.5, &sample));
    NEAR(sample.height, 25.0);
    CHECK(sample.triangle == 1);
    CHECK(meshmap_sample_local(map, 10, 10, &sample));
    NEAR(sample.height, 60.0);
    CHECK(!meshmap_sample_local(map, -0.1, 5, &sample));
    CHECK(!meshmap_sample_local(map, 5, 10.1, &sample));
    for(axis = 0; axis < 3; axis++)
    {
        config.axis = (e_meshmap_axis)axis;
        for(side = 0; side < 2; side++)
        {
            config.side = (e_meshmap_side)side;
            CHECK(meshmap_configure(map, &config));
            CHECK(meshmap_vertex(map, 1, 1, &vertex));
            NEAR(vertex.u, 1.0);
            NEAR(vertex.v, 1.0);
            if(axis == MESHMAP_AXIS_Y)
            {
                NEAR(vertex.position.x, 110.0);
                NEAR(vertex.position.y, 260.0);
                NEAR(vertex.position.z, 310.0);
            }
            else if(axis == MESHMAP_AXIS_Z)
            {
                NEAR(vertex.position.x, 110.0);
                NEAR(vertex.position.y, 190.0);
                NEAR(vertex.position.z, 360.0);
            }
            else
            {
                NEAR(vertex.position.x, 160.0);
                NEAR(vertex.position.y, 190.0);
                NEAR(vertex.position.z, 310.0);
            }
            for(tri = 0; tri < 2; tri++)
            {
                s_meshmap_vec3 a, b, cross;
                double norm;
                CHECK(meshmap_triangle(map, 0, 0, tri, &triangle));
                a = point(triangle.vertex[1].position.x - triangle.vertex[0].position.x,
                    triangle.vertex[1].position.y - triangle.vertex[0].position.y,
                    triangle.vertex[1].position.z - triangle.vertex[0].position.z);
                b = point(triangle.vertex[2].position.x - triangle.vertex[0].position.x,
                    triangle.vertex[2].position.y - triangle.vertex[0].position.y,
                    triangle.vertex[2].position.z - triangle.vertex[0].position.z);
                cross = point(a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x);
                CHECK(cross.x * triangle.normal.x + cross.y * triangle.normal.y + cross.z * triangle.normal.z > 0);
                norm = triangle.normal.x * triangle.normal.x + triangle.normal.y * triangle.normal.y
                    + triangle.normal.z * triangle.normal.z;
                NEAR(norm, 1.0);
            }
        }
    }
    CHECK(meshmap_set_sample(map, 1, 1, 0));
    CHECK(!meshmap_triangle(map, 0, 0, 0, &triangle));
    CHECK(!meshmap_triangle(map, 0, 0, 1, &triangle));
    CHECK(!meshmap_sample_local(map, 1, 1, &sample));
    meshmap_store_free(store);
}

static void test_gates(void)
{
    s_meshmap_store *store = meshmap_store_create();
    s_meshmap *map = flat(store, 0, MESHMAP_AXIS_Y, 4, 4, 5);
    s_meshmap_bank *bank = meshmap_bank_get(store, 1, 0, 0);
    s_meshmap_gate gate = {0};
    s_meshmap_query query;
    s_meshmap_hit hit;
    s_meshmap_vec3 start = point(15, 20, 15), end = point(15, -10, 15);
    gate.bounds.min = point(0, 0, 0);
    gate.bounds.max = point(10, 10, 10);
    gate.enabled = 1;
    CHECK(meshmap_bank_set_gate(bank, &gate));
    meshmap_query_default(&query, point(15, 5, 5));
    CHECK(meshmap_trace(store, &query, start, end, &hit) == 0);
    /* Current position activates the whole bank. The range is not silently
     * reused as a clip box for this independently placed collision ray. */
    query.position = point(5, 5, 5);
    CHECK(meshmap_trace(store, &query, start, end, &hit) == 1);
    NEAR(hit.position.y, 5.0);
    CHECK(!meshmap_store_uses_body_gates(store));
    gate.mode = MESHMAP_GATE_BODY;
    CHECK(meshmap_bank_set_gate(bank, &gate));
    CHECK(meshmap_store_uses_body_gates(store) && store->body_gate_count == 1);
    query.position = point(15, 5, 5);
    query.body.min = point(9, 0, 0);
    query.body.max = point(16, 10, 10);
    CHECK(meshmap_trace(store, &query, start, end, &hit) == 1);
    query.body.min.x = 10.01;
    CHECK(meshmap_trace(store, &query, start, end, &hit) == 0);
    gate.enabled = 0;
    gate.bounds.min.x = 100;
    CHECK(meshmap_bank_set_gate(bank, &gate));
    CHECK(!meshmap_store_uses_body_gates(store));
    gate.enabled = 1;
    CHECK(!meshmap_bank_set_gate(bank, &gate));
    CHECK(!bank->gate.enabled && bank->gate.bounds.min.x == 100);
    gate.bounds.max.x = 200;
    CHECK(meshmap_set_gate(map, &gate));
    CHECK(store->body_gate_count == 1);
    CHECK(meshmap_bank_set_gate(&store->root, &gate));
    CHECK(store->body_gate_count == 2);
    CHECK(meshmap_bank_remove(store, 0, 0));
    CHECK(!meshmap_store_uses_body_gates(store) && store->body_gate_count == 0);
    meshmap_store_free(store);
}

static void test_solid_faces(void)
{
    s_meshmap_store *store = meshmap_store_create();
    s_meshmap *map = flat(store, 0, MESHMAP_AXIS_Y, 3, 3, 5);
    s_meshmap_query query;
    s_meshmap_hit hit;
    s_meshmap_config config;
    meshmap_query_default(&query, point(7, 20, 8));
    CHECK(meshmap_trace_axis(store, &query, point(7, 20, 8), MESHMAP_AXIS_Y, -10, &hit) == 1);
    NEAR(hit.position.y, 5);
    NEAR(hit.fraction, 0.5);
    NEAR(hit.normal.y, 1);
    CHECK(hit.entering && hit.walkable && !hit.boundary && hit.id == 0);
    CHECK(hit.support_group == MESHMAP_GROUP_NONE);
    CHECK(meshmap_trace_axis(store, &query, point(7, 0, 8), MESHMAP_AXIS_Y, 20, &hit) == 1);
    NEAR(hit.position.y, 5);
    CHECK(!hit.entering);
    query.flags |= MESHMAP_QUERY_ENTER_ONLY;
    CHECK(meshmap_trace_axis(store, &query, point(7, 0, 8), MESHMAP_AXIS_Y, 20, &hit) == 0);
    query.flags = MESHMAP_QUERY_EXIT_ONLY;
    CHECK(meshmap_trace_axis(store, &query, point(7, 20, 8), MESHMAP_AXIS_Y, 0, &hit) == 0);
    query.flags = MESHMAP_QUERY_INCLUDE_ONE_WAY;
    CHECK(meshmap_trace(store, &query, point(2, 0, 2), point(18, 0, 18), &hit) == 0);
    CHECK(meshmap_trace(store, &query, point(2, 0, 8), point(25, 0, 8), &hit) == 1);
    NEAR(hit.position.x, 20);
    NEAR(hit.normal.x, 1);
    CHECK(!hit.entering && hit.boundary);
    CHECK(meshmap_trace(store, &query, point(-5, 0, 8), point(25, 0, 8), &hit) == 1);
    NEAR(hit.position.x, 0);
    NEAR(hit.normal.x, -1);
    CHECK(hit.entering && !hit.walkable && hit.boundary);
    CHECK(meshmap_contains_point(map, point(5, -1000, 5)));
    CHECK(!meshmap_contains_point(map, point(5, 6, 5)));
    CHECK(!meshmap_contains_point(map, point(25, 0, 5)));
    CHECK(meshmap_trace(store, &query, point(5, 6, 5), point(15, 6, 5), &hit) == 0);
    config = map->config;
    config.side = MESHMAP_SIDE_POSITIVE;
    CHECK(meshmap_configure(map, &config));
    CHECK(meshmap_trace_axis(store, &query, point(7, 0, 8), MESHMAP_AXIS_Y, 20, &hit) == 1);
    NEAR(hit.normal.y, -1);
    CHECK(hit.entering && !hit.walkable);
    CHECK(meshmap_contains_point(map, point(5, 1000, 5)));
    CHECK(!meshmap_contains_point(map, point(5, 4, 5)));
    meshmap_store_free(store);
}

static void test_side_map_platforms(void)
{
    unsigned axis;
    for(axis = 0; axis < 3; axis += 2)
    {
        s_meshmap_store *store = meshmap_store_create();
        s_meshmap *map = flat(store, 0, (e_meshmap_axis)axis, 5, 5, 2);
        s_meshmap_config config = map->config;
        s_meshmap_query query;
        s_meshmap_hit hit;
        s_meshmap_vec3 start = axis == MESHMAP_AXIS_Z ? point(20, 50, 0) : point(0, 50, 20);
        config.origin.y = 40;
        CHECK(meshmap_configure(map, &config));
        meshmap_query_default(&query, start);
        query.movement_plane = (int)axis;
        CHECK(meshmap_trace_axis(store, &query, start, MESHMAP_AXIS_Y, 20, &hit) == 1);
        NEAR(hit.position.y, 40);
        NEAR(hit.normal.y, 1);
        CHECK(hit.entering && hit.walkable && hit.boundary);
        start.y = -10;
        CHECK(meshmap_trace_axis(store, &query, start, MESHMAP_AXIS_Y, 20, &hit) == 1);
        NEAR(hit.position.y, 0);
        NEAR(hit.normal.y, -1);
        CHECK(hit.entering && !hit.walkable && hit.boundary);
        if(axis == MESHMAP_AXIS_Z)
        {
            start.z = 3;
        }
        else
        {
            start.x = 3;
        }
        CHECK(meshmap_trace_axis(store, &query, start, MESHMAP_AXIS_Y, 50, &hit) == 0);
        if(axis == MESHMAP_AXIS_Z)
        {
            start.z = 0;
        }
        else
        {
            start.x = 0;
        }
        /* Removing one central vertex opens four cells. The hole's lower edge
         * is a floor and its upper edge is a ceiling in this platform slice. */
        CHECK(meshmap_set_sample(map, 2, 2, 0));
        start.y = 25;
        query.flags |= MESHMAP_QUERY_ENTER_ONLY;
        CHECK(meshmap_trace_axis(store, &query, start, MESHMAP_AXIS_Y, -5, &hit) == 1);
        NEAR(hit.position.y, 10);
        NEAR(hit.normal.y, 1);
        CHECK(hit.walkable && hit.boundary);
        CHECK(meshmap_trace_axis(store, &query, start, MESHMAP_AXIS_Y, 45, &hit) == 1);
        NEAR(hit.position.y, 30);
        NEAR(hit.normal.y, -1);
        CHECK(!hit.walkable && hit.boundary);
        CHECK(!meshmap_contains_point(map, start));
        start.y = 35;
        CHECK(meshmap_contains_point(map, start));
        config.flags |= MESHMAP_ONE_WAY;
        CHECK(meshmap_configure(map, &config));
        start.y = 25;
        CHECK(meshmap_trace_axis(store, &query, start, MESHMAP_AXIS_Y, 45, &hit) == 0);
        CHECK(meshmap_trace_axis(store, &query, start, MESHMAP_AXIS_Y, -5, &hit) == 1);
        NEAR(hit.position.y, 10);
        start.y = -5;
        CHECK(meshmap_trace_axis(store, &query, start, MESHMAP_AXIS_Y, 45, &hit) == 0);
        query.flags &= ~MESHMAP_QUERY_INCLUDE_ONE_WAY;
        start.y = 25;
        CHECK(meshmap_trace_axis(store, &query, start, MESHMAP_AXIS_Y, -5, &hit) == 0);
        meshmap_store_free(store);
    }
}

static void test_slope_projection(void)
{
    unsigned axis;
    for(axis = 0; axis < 3; axis += 2)
    {
        s_meshmap_store *store = meshmap_store_create();
        s_meshmap *map = meshmap_create(store, 0);
        s_meshmap_config config;
        s_meshmap_query query;
        s_meshmap_hit hit;
        uint16_t samples[4] = {1, 32768, 32768, 65535};
        s_meshmap_vec3 start = axis == MESHMAP_AXIS_Z ? point(5, 9, 0) : point(0, 9, 5);
        meshmap_config_default(&config);
        config.axis = (e_meshmap_axis)axis;
        config.origin.y = 10;
        config.spacing_u = config.spacing_v = 10;
        config.min_height = -10;
        config.max_height = 10;
        CHECK(meshmap_configure(map, &config));
        CHECK(meshmap_set_geometry(map, 2, 2, samples, NULL));
        meshmap_query_default(&query, start);
        CHECK(meshmap_trace_axis(store, &query, start, MESHMAP_AXIS_Y, 1, &hit) == 1);
        NEAR(hit.position.y, 5);
        CHECK(!hit.walkable);
        query.movement_plane = (int)axis;
        CHECK(meshmap_trace_axis(store, &query, start, MESHMAP_AXIS_Y, 1, &hit) == 1);
        CHECK(hit.walkable);
        NEAR(hit.normal.y, 1.0 / sqrt(3.0));
        query.slope_limit = 44;
        CHECK(meshmap_trace_axis(store, &query, start, MESHMAP_AXIS_Y, 1, &hit) == 1);
        CHECK(!hit.walkable);
        query.flags |= MESHMAP_QUERY_SUPPORT_ONLY;
        CHECK(meshmap_trace_axis(store, &query, start, MESHMAP_AXIS_Y, 1, &hit) == 0);
        query.slope_limit = 90;
        config.slope_limit = 44;
        CHECK(meshmap_configure(map, &config));
        CHECK(meshmap_trace_axis(store, &query, start, MESHMAP_AXIS_Y, 1, &hit) == 0);
        meshmap_store_free(store);
    }
}

static void test_union_and_drop(void)
{
    s_meshmap_store *store = meshmap_store_create();
    s_meshmap *low = flat(store, 0, MESHMAP_AXIS_Y, 2, 2, 5);
    s_meshmap *high = flat(store, 1, MESHMAP_AXIS_Y, 2, 2, 10);
    s_meshmap_config config = high->config;
    s_meshmap_query query;
    s_meshmap_hit hit;
    uint64_t old_generation;
    config.origin.x = 5;
    CHECK(meshmap_configure(high, &config));
    meshmap_query_default(&query, point(7, 20, 5));
    CHECK(meshmap_trace_axis(store, &query, point(7, 20, 5), MESHMAP_AXIS_Y, 0, &hit) == 1);
    NEAR(hit.position.y, 10);
    CHECK(hit.id == 1);
    CHECK(meshmap_trace_axis(store, &query, point(7, 0, 5), MESHMAP_AXIS_Y, 20, &hit) == 1);
    NEAR(hit.position.y, 10);
    CHECK(!hit.entering);
    CHECK(meshmap_trace_axis(store, &query, point(2, 0, 5), MESHMAP_AXIS_X, 20, &hit) == 1);
    NEAR(hit.position.x, 15);
    config.origin.x = 10;
    CHECK(meshmap_configure(high, &config));
    CHECK(meshmap_trace_axis(store, &query, point(2, 0, 5), MESHMAP_AXIS_X, 18, &hit) == 0);
    config.origin.x = 10.01;
    CHECK(meshmap_configure(high, &config));
    CHECK(meshmap_trace_axis(store, &query, point(2, 0, 5), MESHMAP_AXIS_X, 18, &hit) == 1);
    NEAR(hit.position.x, 10);
    config.origin.x = 0;
    config.flags |= MESHMAP_ONE_WAY | MESHMAP_DROP_THROUGH;
    config.support_group = 0; /* Explicit group zero is distinct from map ID zero. */
    CHECK(meshmap_configure(high, &config));
    query.flags |= MESHMAP_QUERY_IGNORE_DROP_GROUP | MESHMAP_QUERY_ENTER_ONLY;
    query.ignore_group = 0;
    CHECK(meshmap_trace_axis(store, &query, point(7, 20, 5), MESHMAP_AXIS_Y, 0, &hit) == 1);
    NEAR(hit.position.y, 5);
    CHECK(hit.id == 0 && hit.support_group == MESHMAP_GROUP_NONE);
    query.ignore_group = MESHMAP_GROUP_NONE;
    query.ignore_id = 0;
    query.ignore_generation = low->generation;
    config = low->config;
    config.flags |= MESHMAP_ONE_WAY | MESHMAP_DROP_THROUGH;
    CHECK(meshmap_configure(low, &config));
    CHECK(meshmap_trace_axis(store, &query, point(7, 8, 5), MESHMAP_AXIS_Y, 0, &hit) == 0);
    old_generation = low->generation;
    CHECK(meshmap_remove(store, 0));
    low = flat(store, 0, MESHMAP_AXIS_Y, 2, 2, 5);
    config = low->config;
    config.flags |= MESHMAP_ONE_WAY | MESHMAP_DROP_THROUGH;
    CHECK(meshmap_configure(low, &config));
    CHECK(low->generation != old_generation);
    CHECK(meshmap_trace_axis(store, &query, point(7, 8, 5), MESHMAP_AXIS_Y, 0, &hit) == 1);
    NEAR(hit.position.y, 5);
    query.ignore_id = 1;
    query.ignore_generation = high->generation;
    CHECK(meshmap_trace_axis(store, &query, point(7, 20, 5), MESHMAP_AXIS_Y, 0, &hit) == 1);
    NEAR(hit.position.y, 10); /* Ungrouped identity does not suppress a grouped map. */
    config = high->config;
    config.flags &= ~MESHMAP_DROP_THROUGH;
    CHECK(meshmap_configure(high, &config));
    query.ignore_group = 0;
    CHECK(meshmap_trace_axis(store, &query, point(7, 20, 5), MESHMAP_AXIS_Y, 0, &hit) == 1);
    NEAR(hit.position.y, 10);
    meshmap_store_free(store);
}

static uint32_t random_state = 0x721517u;
static double random_unit(void)
{
    random_state = random_state * 1664525u + 1013904223u;
    return (double)(random_state >> 8) / 16777216.0;
}

static void test_independent_height_rays(void)
{
    s_meshmap_store *store = meshmap_store_create();
    s_meshmap *map = meshmap_create(store, 0);
    s_meshmap_config config;
    s_meshmap_query query;
    s_meshmap_hit hit;
    uint16_t samples[81];
    unsigned i, axis;
    meshmap_config_default(&config);
    config.spacing_u = 3;
    config.spacing_v = 7;
    config.min_height = -40;
    config.max_height = 60;
    config.origin = point(100, 200, 300);
    for(i = 0; i < 81; i++)
    {
        samples[i] = (uint16_t)(1 + (unsigned)(random_unit() * 65535));
    }
    CHECK(meshmap_set_geometry(map, 9, 9, samples, NULL));
    for(axis = 0; axis < 3; axis++)
    {
        config.axis = (e_meshmap_axis)axis;
        CHECK(meshmap_configure(map, &config));
        for(i = 0; i < 200; i++)
        {
            double u = random_unit() * 8, v = random_unit() * 8;
            unsigned col = (unsigned)u, row = (unsigned)v;
            double fu = u - col, fv = v - row, expected;
            double a = -40 + (samples[row * 9 + col] - 1.0) / 65534.0 * 100;
            double b = -40 + (samples[row * 9 + col + 1] - 1.0) / 65534.0 * 100;
            double c = -40 + (samples[(row + 1) * 9 + col + 1] - 1.0) / 65534.0 * 100;
            double d = -40 + (samples[(row + 1) * 9 + col] - 1.0) / 65534.0 * 100;
            s_meshmap_vec3 start;
            double endpoint;
            /* Independent barycentric weights on the specified TL-BR split. */
            if(fu >= fv)
            {
                expected = (1 - fu) * a + (fu - fv) * b + fv * c;
            }
            else
            {
                expected = (1 - fv) * a + fu * c + (fv - fu) * d;
            }
            if(axis == MESHMAP_AXIS_Y)
            {
                start = point(100 + u * 3, 300, 300 + v * 7);
                endpoint = 100;
                expected += 200;
            }
            else if(axis == MESHMAP_AXIS_Z)
            {
                start = point(100 + u * 3, 200 - v * 7, 400);
                endpoint = 200;
                expected += 300;
            }
            else
            {
                start = point(200, 200 - v * 7, 300 + u * 3);
                endpoint = 0;
                expected += 100;
            }
            meshmap_query_default(&query, start);
            CHECK(meshmap_trace_axis(store, &query, start, (e_meshmap_axis)axis, endpoint, &hit) == 1);
            NEAR(axis == MESHMAP_AXIS_X ? hit.position.x : axis == MESHMAP_AXIS_Y ? hit.position.y : hit.position.z,
                expected);
            CHECK(hit.entering && !hit.boundary);
        }
    }
    meshmap_store_free(store);
}

static void test_grid_edge_traversal(void)
{
    s_meshmap_store *store = meshmap_store_create();
    s_meshmap *map = flat(store, 0, MESHMAP_AXIS_Y, 33, 33, 5);
    s_meshmap_query query;
    s_meshmap_hit hit;
    s_meshmap_sample sample;
    meshmap_query_default(&query, point(1, 0, 1));
    CHECK(meshmap_trace(store, &query, point(1, 0, 1), point(319, 0, 319), &hit) == 0);
    CHECK(meshmap_trace(store, &query, point(319, 0, 319), point(1, 0, 1), &hit) == 0);
    CHECK(meshmap_trace(store, &query, point(1, 0, 10), point(321, 0, 10), &hit) == 1);
    NEAR(hit.position.x, 320);
    CHECK(meshmap_trace(store, &query, point(319, 0, 10), point(-1, 0, 10), &hit) == 1);
    NEAR(hit.position.x, 0);
    CHECK(meshmap_set_sample(map, 1, 0, 0));
    CHECK(meshmap_sample_local(map, 5, 10, &sample));
    CHECK(meshmap_trace_axis(store, &query, point(5, 20, 10), MESHMAP_AXIS_Y, -20, &hit) == 1);
    NEAR(hit.position.y, 5);
    CHECK(meshmap_trace(store, &query, point(-5, 0, -5), point(0, 0, 0), &hit) == 0);
    CHECK(meshmap_trace(store, &query, point(-5, 6, -5), point(325, 6, 325), &hit) == 0);
    CHECK(meshmap_trace(store, &query, point(1, 0, 1), point(1, 0, 1), &hit) == 0);
    CHECK(meshmap_set_sample(map, 1, 0, 1));
    CHECK(meshmap_set_sample(map, 16, 16, 0));
    CHECK(meshmap_trace(store, &query, point(1, 0, 1), point(319, 0, 319), &hit) == 1);
    NEAR(hit.position.x, 150);
    NEAR(hit.position.z, 150);
    CHECK(!hit.entering && hit.boundary == 1);
    CHECK(fabs(hit.normal.x) == 1.0 || fabs(hit.normal.z) == 1.0);
    query.flags |= MESHMAP_QUERY_ENTER_ONLY;
    CHECK(meshmap_trace(store, &query, point(1, 0, 1), point(319, 0, 319), &hit) == 1);
    NEAR(hit.position.x, 170);
    NEAR(hit.position.z, 170);
    CHECK(hit.entering && hit.boundary == 1);
    query.flags &= ~MESHMAP_QUERY_ENTER_ONLY;
    /* The neighboring present row still owns the closed upper edge of the
     * hole. A ray along that exact grid line must not see a phantom gap. */
    CHECK(meshmap_trace(store, &query, point(1, 0, 150), point(319, 0, 150), &hit) == 0);
    meshmap_store_free(store);
}

static void test_cached_minimum_y(void)
{
    s_meshmap_store *store = meshmap_store_create();
    s_meshmap *ground, *side;
    s_meshmap_config config;
    s_meshmap_gate gate = {0};
    NEAR(meshmap_store_minimum_y(store, 123), 123);
    CHECK(!store->minimum_y_dirty && !store->minimum_y_valid);
    ground = flat(store, 0, MESHMAP_AXIS_Y, 2, 2, -3000);
    CHECK(store->minimum_y_dirty);
    NEAR(meshmap_store_minimum_y(store, 123), -3000);
    CHECK(!store->minimum_y_dirty);
    side = flat(store, 1u << 18, MESHMAP_AXIS_Z, 2, 3, 0);
    config = side->config;
    config.origin.y = -4000;
    CHECK(meshmap_configure(side, &config));
    NEAR(meshmap_store_minimum_y(store, 123), -4020);
    gate.enabled = 1;
    gate.bounds.min = point(100, 100, 100);
    gate.bounds.max = point(200, 200, 200);
    CHECK(meshmap_set_gate(side, &gate));
    CHECK(!store->minimum_y_dirty);
    NEAR(meshmap_store_minimum_y(store, 123), -4020);
    CHECK(meshmap_bank_set_slot(&store->root, 1, 0));
    NEAR(meshmap_store_minimum_y(store, 123), -3000);
    CHECK(meshmap_bank_set_slot(&store->root, 1, 1));
    NEAR(meshmap_store_minimum_y(store, 123), -4020);
    config.flags &= ~MESHMAP_COLLIDE;
    CHECK(meshmap_configure(side, &config));
    NEAR(meshmap_store_minimum_y(store, 123), -3000);
    CHECK(meshmap_set_sample(ground, 0, 0, 0));
    NEAR(meshmap_store_minimum_y(store, 123), 123);
    NEAR(meshmap_store_minimum_y(store, -99), -99);
    CHECK(meshmap_set_sample(ground, 0, 0, 1));
    NEAR(meshmap_store_minimum_y(store, 123), -3000);
    CHECK(meshmap_remove(store, 0));
    NEAR(meshmap_store_minimum_y(store, 123), 123);
    meshmap_store_free(store);
}

static void test_vertical_body_sweeps(void)
{
    s_meshmap_store *store = meshmap_store_create();
    s_meshmap *map = flat(store, 0, MESHMAP_AXIS_Z, 2, 2, 0);
    s_meshmap_config config = map->config;
    s_meshmap_query query;
    s_meshmap_hit hit;
    s_meshmap_gate gate = {0};
    s_meshmap_vec3 start = point(0, 0, 0), end = point(30, 0, 0);
    config.origin = point(10, 10, 0);
    config.spacing_u = config.spacing_v = 2;
    CHECK(meshmap_configure(map, &config));
    meshmap_query_default(&query, start);
    query.flags |= MESHMAP_QUERY_ENTER_ONLY;
    CHECK(meshmap_trace(store, &query, start, end, &hit) == 0);
    CHECK(meshmap_trace(store, &query, point(0, 20, 0), point(30, 20, 0), &hit) == 0);
    CHECK(meshmap_trace_body(store, &query, start, end, 20, &hit) == 1);
    NEAR(hit.position.x, 10);
    NEAR(hit.fraction, 1.0 / 3.0);
    NEAR(hit.normal.x, -1);
    CHECK(hit.position.y >= 8 && hit.position.y <= 10);
    NEAR(hit.body_offset, hit.position.y);
    CHECK(hit.entering && !hit.walkable);
    CHECK(meshmap_trace_body(store, &query, start, end, 0, &hit) == 0);
    CHECK(meshmap_trace_body(store, &query, start, end, -1, &hit) == 0);
    /* Body collision does not change a POSITION bank into a body/sweep gate. */
    gate.enabled = 1;
    gate.bounds.min = point(-1, -1, -1);
    gate.bounds.max = point(1, 1, 1);
    CHECK(meshmap_bank_set_gate(&store->root, &gate));
    CHECK(meshmap_trace_body(store, &query, start, end, 20, &hit) == 1);
    query.position = point(30, 0, 0);
    query.body.min = point(-10, -10, -10);
    query.body.max = point(40, 40, 40);
    CHECK(meshmap_trace_body(store, &query, start, end, 20, &hit) == 0);
    CHECK(meshmap_bank_remove(store, 0, 0));
    map = flat(store, 0, MESHMAP_AXIS_Y, 2, 2, 5);
    config = map->config;
    config.origin = point(10, 0, -10);
    config.spacing_u = 2;
    config.spacing_v = 20;
    CHECK(meshmap_configure(map, &config));
    meshmap_query_default(&query, start);
    query.flags |= MESHMAP_QUERY_ENTER_ONLY;
    CHECK(meshmap_trace_body(store, &query, start, end, 20, &hit) == 1);
    NEAR(hit.position.x, 10);
    CHECK(meshmap_trace_body(store, &query, point(0, 6, 0), point(30, 6, 0), 20, &hit) == 0);
    /* Endpoint floor height cannot excuse an intervening tall obstacle. */
    config.min_height = config.max_height = 30;
    CHECK(meshmap_configure(map, &config));
    CHECK(meshmap_trace_body(store, &query, point(0, 6, 0), point(30, 6, 0), 20, &hit) == 1);
    NEAR(hit.position.x, 10);
    CHECK(meshmap_bank_remove(store, 0, 0));
    map = flat(store, 0, MESHMAP_AXIS_Y, 4, 4, 5);
    CHECK(meshmap_trace_body(store, &query, point(5, 5.01, 5), point(25, 5.01, 25), 20, &hit) == 0);
    CHECK(meshmap_trace_body(store, &query, point(5, 20, 5), point(5, 0, 5), 20, &hit) == 1);
    NEAR(hit.position.y, 5);
    NEAR(hit.body_offset, 0);
    NEAR(hit.normal.y, 1);
    config = map->config;
    config.min_height = config.max_height = 30;
    config.side = MESHMAP_SIDE_POSITIVE;
    CHECK(meshmap_configure(map, &config));
    CHECK(meshmap_trace_body(store, &query, point(5, 0, 5), point(5, 20, 5), 20, &hit) == 1);
    NEAR(hit.position.y, 30);
    NEAR(hit.body_offset, 20);
    NEAR(hit.fraction, 0.5);
    NEAR(hit.normal.y, -1);
    CHECK(!hit.walkable);
    CHECK(meshmap_bank_remove(store, 0, 0));
    map = flat(store, 0, MESHMAP_AXIS_Z, 2, 3, 0);
    {
        uint16_t ridge[6] = {1, 1, 65535, 65535, 1, 1};
        config = map->config;
        config.origin.y = 20;
        config.min_height = 0;
        config.max_height = 10;
        CHECK(meshmap_configure(map, &config));
        CHECK(meshmap_set_geometry(map, 2, 3, ridge, NULL));
        CHECK(meshmap_trace_body(store, &query, point(5, 0, 15), point(5, 0, -5), 20, &hit) == 1);
        NEAR(hit.position.z, 10);
        NEAR(hit.position.y, 10);
        NEAR(hit.body_offset, 10);
        NEAR(hit.normal.z, 1);
        NEAR(hit.normal.y, 0);
        CHECK(hit.entering && !hit.walkable && !hit.boundary);
    }
    meshmap_store_free(store);
}

int main(void)
{
    test_sparse_banks();
    test_resource_transactions();
    test_triangle_geometry();
    test_gates();
    test_solid_faces();
    test_side_map_platforms();
    test_slope_projection();
    test_union_and_drop();
    test_independent_height_rays();
    test_grid_edge_traversal();
    test_cached_minimum_y();
    test_vertical_body_sweeps();
    printf("meshmap core: %u checks, %u failures\n", checks, failures);
    return failures ? EXIT_FAILURE : EXIT_SUCCESS;
}
