/* Exercise the production entity adapter against real sparse terrain stores.
 * These cases describe movement outcomes, rather than repeating its algorithm. */
#include "openbor.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#undef malloc
#undef calloc
#undef realloc
#undef free

s_level *level;
s_global_config global_config = {.game_speed = 100};
static s_level stage;
static unsigned assertions;
#define REQUIRE(condition) do { ++assertions; if(!(condition)) { \
    fprintf(stderr, "%s:%d: %s\n", __FILE__, __LINE__, #condition); exit(1); } } while(0)
#define NEAR(actual, expected) REQUIRE(fabs((double)(actual) - (expected)) < 0.025)

static void scene(void)
{
    if(level) meshmap_store_free(level->meshmaps);
    memset(&stage, 0, sizeof(stage));
    level = &stage;
    level->meshmaps = meshmap_store_create();
    REQUIRE(level->meshmaps != NULL);
}

static entity actor(float x, float y, float z, int height)
{
    entity e;
    memset(&e, 0, sizeof(e));
    meshmap_entity_initialize(&e);
    e.position = (s_axis_principal_float){x, y, z};
    e.modeldata.size.x = 2;
    e.modeldata.size.y = height;
    e.base = MESHMAP_ENTITY_VOID_BASE;
    return e;
}

static s_meshmap *flat(uint32_t id, e_meshmap_axis axis, double x, double y,
                       double z, double spacing_u, double spacing_v, double displacement)
{
    const uint16_t pixels[4] = {1, 1, 1, 1};
    s_meshmap *map = meshmap_create(level->meshmaps, id);
    s_meshmap_config config;
    REQUIRE(map != NULL);
    meshmap_config_default(&config);
    config.axis = axis;
    config.origin = (s_meshmap_vec3){x, y, z};
    config.spacing_u = spacing_u;
    config.spacing_v = spacing_v;
    config.min_height = config.max_height = displacement;
    REQUIRE(meshmap_configure(map, &config));
    REQUIRE(meshmap_set_geometry(map, 2, 2, pixels, "fixture"));
    return map;
}

static void move(entity *e, float dx, float dz)
{
    meshmap_entity_move(e, &dx, &dz);
    e->position.x += dx;
    e->position.z += dz;
}

static void test_ground_and_holes(void)
{
    entity e;
    s_meshmap *map;
    scene();
    map = flat(0, MESHMAP_AXIS_Y, -10, 0, -10, 20, 20, -1200);
    e = actor(0, -1200, 0, 4);
    meshmap_entity_adjust_base(&e, MESHMAP_ENTITY_VOID_BASE);
    NEAR(e.base, -1200);
    REQUIRE(e.meshmap.support_id == 0);
    NEAR(meshmap_entity_pit_depth(&e), -1450);
    e.position.y = -1100;
    e.velocity.y = -200;
    NEAR(meshmap_entity_vertical(&e, -1300), -1200);
    REQUIRE(e.meshmap.support_id == 0);
    REQUIRE(meshmap_set_sample(map, 0, 0, 0));
    e.position.y = -1200;
    meshmap_entity_adjust_base(&e, MESHMAP_ENTITY_VOID_BASE);
    REQUIRE(e.base == MESHMAP_ENTITY_VOID_BASE);
    REQUIRE(e.meshmap.support_id == -1);
    NEAR(meshmap_entity_vertical(&e, -1210), -1210);
}

static void test_slopes(void)
{
    const uint16_t samples[4] = {1, 65535, 1, 65535};
    entity e;
    s_meshmap *map;
    s_meshmap_config config;
    scene();
    map = flat(0, MESHMAP_AXIS_Y, 0, 0, 0, 10, 10, 0);
    config = map->config;
    config.max_height = 2;
    REQUIRE(meshmap_configure(map, &config));
    REQUIRE(meshmap_set_geometry(map, 2, 2, samples, "ramp"));
    e = actor(0, 0, 5, 3);
    meshmap_entity_adjust_base(&e, MESHMAP_ENTITY_VOID_BASE);
    move(&e, 2, 0);
    NEAR(e.position.x, 2);
    NEAR(e.position.y, 0.4);
    REQUIRE(e.meshmap.support_id == 0);

    config.max_height = 20;
    REQUIRE(meshmap_configure(map, &config));
    e = actor(0, 0, 5, 3);
    e.base = 0;
    move(&e, 2, 0);
    REQUIRE(e.position.x < 0.05);
    NEAR(e.position.y, 0);
}

/* Gravity's resting tolerance can finish a fall just before the feet cross
 * a mesh. Support must identify the receiver on that first clamped tick,
 * while a nearby higher legacy floor must keep its independent identity. */
static void test_landing_support_identity(void)
{
    const double floor_y = -31.67531677;
    entity e;
    s_meshmap *map;
    scene();
    map = flat(7, MESHMAP_AXIS_Y, -10, 0, -10, 20, 20, floor_y);
    e = actor(0, (float)(floor_y + 3.0), 0, 4);
    e.velocity.y = -2.95f;
    meshmap_entity_adjust_base(&e, MESHMAP_ENTITY_VOID_BASE);
    REQUIRE(e.base == (float)floor_y);
    REQUIRE(e.meshmap.support_id == -1);
    e.position.y = meshmap_entity_vertical(&e, (float)(floor_y + 0.05));
    REQUIRE(e.position.y > e.base && e.position.y - e.base < 0.1f);
    REQUIRE(e.meshmap.support_id == -1);

    /* check_gravity accepts this resting gap and clamps to the selected base. */
    e.position.y = e.base;
    meshmap_entity_land(&e);
    REQUIRE(e.meshmap.support_id == 7);
    REQUIRE(e.meshmap.support_generation == map->generation);
    REQUIRE(e.meshmap.support_y == e.base);

    /* Even a legacy floor within the contact skin remains distinct. Reusing
     * the actor also proves a former mesh support cannot leak into this land. */
    e.position.y = e.base = (float)(floor_y + 0.005);
    meshmap_entity_land(&e);
    REQUIRE(e.meshmap.support_id == -1);
    REQUIRE(e.meshmap.support_generation == 0);
    REQUIRE(e.position.y == (float)(floor_y + 0.005));
    REQUIRE(e.base == e.position.y);
}

/* Entity positions are floats, while terrain calculations are doubles. At
 * large world coordinates an unrounded candidate can produce a different
 * support Y from the next base refresh. Native gravity treats that mismatch
 * as a new landing and stops horizontal velocity even on a gentle slope. */
static void test_large_world_resting_contact(void)
{
    const uint16_t samples[4] = {32768, 35000, 32790, 35080};
    entity e;
    s_meshmap *map;
    s_meshmap_config config;
    s_meshmap_sample sample;
    scene();
    map = flat(0, MESHMAP_AXIS_Y, 16384, 0, 0, 16384, 16384, 0);
    config = map->config;
    config.min_height = -32;
    config.max_height = 32;
    REQUIRE(meshmap_configure(map, &config));
    REQUIRE(meshmap_set_geometry(map, 2, 2, samples, "large-world shallow plane"));
    e = actor(18761.123f, 0, 4817.456f, 22);
    REQUIRE(meshmap_sample_world(map, (s_meshmap_vec3){e.position.x, 0, e.position.z}, &sample));
    e.position.y = (float)sample.position.y;
    meshmap_entity_adjust_base(&e, MESHMAP_ENTITY_VOID_BASE);
    for(unsigned tick = 0; tick < 400; ++tick)
    {
        const float direction = (tick / 100) % 2 ? 1.0f : -1.0f;
        const float old_x = e.position.x;
        const float requested_x = direction * 6.29321f;
        const float requested_z = direction * 0.21789f;
        float dx = requested_x, dz = requested_z;
        meshmap_entity_move(&e, &dx, &dz);
        REQUIRE(dx == requested_x && dz == requested_z);
        e.position.x += dx;
        e.position.z += dz;
        REQUIRE(fabs(e.position.x - old_x) > 6.29);
        REQUIRE(e.position.y == e.base);
        meshmap_entity_adjust_base(&e, MESHMAP_ENTITY_VOID_BASE);
        REQUIRE(e.meshmap.support_id == 0);
        REQUIRE(e.position.y == e.base);
    }
}

/* This is the final shallow cell and first flat cell of a 16-bit banked road.
 * A ray's interpolated contact can retain a signed rounding residue at Y=0.
 * Stored support must agree exactly with the next base query, otherwise native
 * gravity treats continued grounded movement as a fresh landing. */
static void test_shallow_exit_resting_contact(void)
{
    const uint16_t samples[] = {32782, 32783, 32768, 32768, 32768, 32768};
    const double origins[] = {0.0, 0.00390637, -31.67531677};
    for(unsigned variant = 0; variant < sizeof(origins) / sizeof(origins[0]); ++variant)
    {
        entity e;
        s_meshmap_config config;
        s_meshmap_sample sample;
        s_meshmap *map;
        scene();
        map = flat(0, MESHMAP_AXIS_Y, 7168, origins[variant], 7936, 128, 128, 0);
        config = map->config;
        config.min_height = -128;
        config.max_height = 128;
        REQUIRE(meshmap_configure(map, &config));
        REQUIRE(meshmap_set_geometry(map, 2, 3, samples, "shallow road to flat ground"));
        e = actor(7279.8427734375f, 0, 7955.044921875f, 4);
        REQUIRE(meshmap_sample_world(map, (s_meshmap_vec3){e.position.x, 0, e.position.z}, &sample));
        e.position.y = e.base = (float)sample.position.y;
        meshmap_entity_adjust_base(&e, MESHMAP_ENTITY_VOID_BASE);
        for(unsigned tick = 0; tick < 32; ++tick)
        {
            const float requested_x = -1.6772f * 0.5f;
            const float requested_z = 12.617609f * 0.5f + 250 * 0.00001f;
            float dx = requested_x, dz = requested_z;
            meshmap_entity_move(&e, &dx, &dz);
            REQUIRE(dx == requested_x && dz == requested_z);
            e.position.x += dx;
            e.position.z += dz;
            REQUIRE(meshmap_sample_world(map, (s_meshmap_vec3){e.position.x, 0, e.position.z}, &sample));
            REQUIRE(e.position.y == (float)sample.position.y);
            meshmap_entity_adjust_base(&e, MESHMAP_ENTITY_VOID_BASE);
            REQUIRE(e.meshmap.support_id == 0);
            REQUIRE(e.position.y == e.base);
        }
        REQUIRE(e.position.z > 8064);
        REQUIRE(e.position.y == (float)origins[variant]);
    }
}

/* A genuine downward crossing must choose the same stored surface as a later
 * resting query, including a zero-height floor and fractional negative floors.
 * The return value is used directly as the entity's Y by native gravity. */
static void test_vertical_contact_height(void)
{
    const double floors[] = {0.0, 0.00390637, -31.67531677};
    for(unsigned variant = 0; variant < sizeof(floors) / sizeof(floors[0]); ++variant)
    {
        entity e;
        s_meshmap *map;
        scene();
        map = flat(7, MESHMAP_AXIS_Y, -10, 0, -10, 20, 20, floors[variant]);
        for(unsigned fall = 1; fall <= 16; ++fall)
        {
            e = actor(0, (float)(floors[variant] + fall * 0.031), 0, 4);
            e.velocity.y = -3;
            e.position.y = meshmap_entity_vertical(&e, (float)(floors[variant] - fall * 0.179));
            REQUIRE(e.position.y == (float)floors[variant]);
            REQUIRE(e.base == e.position.y);
            REQUIRE(e.meshmap.support_id == 7);
            REQUIRE(e.meshmap.support_generation == map->generation);
            meshmap_entity_adjust_base(&e, MESHMAP_ENTITY_VOID_BASE);
            REQUIRE(e.base == e.position.y);
            meshmap_entity_land(&e);
            REQUIRE(e.meshmap.support_id == 7);
        }
    }
}

static void test_side_platforms(void)
{
    entity e;
    s_meshmap *map;
    s_meshmap_config config;
    scene();
    map = flat(5, MESHMAP_AXIS_Z, 0, 20, 0, 10, 10, 1);
    e = actor(5, 25, 0, 8);
    e.velocity.y = -30;
    NEAR(meshmap_entity_vertical(&e, 0), 20);
    NEAR(e.base, 20);
    REQUIRE(e.meshmap.support_id == 5);

    e = actor(5, 0, 0, 8);
    e.velocity.y = 5;
    NEAR(meshmap_entity_vertical(&e, 5), 2);
    NEAR(e.velocity.y, 0);
    config = map->config;
    config.flags |= MESHMAP_ONE_WAY | MESHMAP_DROP_THROUGH;
    REQUIRE(meshmap_configure(map, &config));
    e.velocity.y = 5;
    NEAR(meshmap_entity_vertical(&e, 25), 25);
    e.position.y = 25;
    e.velocity.y = -30;
    NEAR(meshmap_entity_vertical(&e, 0), 20);
}

static void test_steps_and_intervening_wall(void)
{
    entity e;
    scene();
    flat(0, MESHMAP_AXIS_Y, 0, 0, -10, 30, 20, 0);
    flat(1, MESHMAP_AXIS_Z, 10, 5, 0, 10, 10, 1);
    e = actor(8, 0, 0, 3);
    meshmap_entity_adjust_base(&e, MESHMAP_ENTITY_VOID_BASE);
    move(&e, 4, 0);
    REQUIRE(e.position.x < 10);
    e = actor(8, 0, 0, 3);
    e.meshmap.max_step_height = 6;
    meshmap_entity_adjust_base(&e, MESHMAP_ENTITY_VOID_BASE);
    move(&e, 4, 0);
    NEAR(e.position.x, 12);
    NEAR(e.position.y, 5);

    /* The destination floor is clear, but reaching it crosses a tall wall. */
    scene();
    flat(0, MESHMAP_AXIS_Y, -5, 0, -10, 30, 20, 0);
    flat(1, MESHMAP_AXIS_Z, 5, 10, 0, 1, 10, 1);
    e = actor(0, 0, 0, 20);
    meshmap_entity_adjust_base(&e, MESHMAP_ENTITY_VOID_BASE);
    move(&e, 10, 0);
    REQUIRE(e.position.x < 5);

    /* A beam can fall strictly between the actor's feet and head. */
    scene();
    flat(0, MESHMAP_AXIS_Y, -5, 0, -10, 30, 20, 0);
    flat(1, MESHMAP_AXIS_Z, 5, 8, 0, 1, 2, 1);
    e = actor(0, 0, 0, 20);
    meshmap_entity_adjust_base(&e, MESHMAP_ENTITY_VOID_BASE);
    move(&e, 10, 0);
    REQUIRE(e.position.x < 5);
}

static void test_x_wall_and_gates(void)
{
    s_meshmap *map;
    s_meshmap_config config;
    s_meshmap_gate gate = {0};
    entity e;
    scene();
    map = flat(0, MESHMAP_AXIS_X, 0, 20, -10, 20, 20, 5);
    config = map->config;
    config.side = MESHMAP_SIDE_POSITIVE;
    REQUIRE(meshmap_configure(map, &config));
    e = actor(0, 5, 0, 4);
    move(&e, 10, 0);
    REQUIRE(e.position.x < 5 && e.position.x > 4.9);

    gate.enabled = 1;
    gate.bounds.min = (s_meshmap_vec3){4, -100, -100};
    gate.bounds.max = (s_meshmap_vec3){6, 100, 100};
    gate.mode = MESHMAP_GATE_POSITION;
    REQUIRE(meshmap_bank_set_gate(&level->meshmaps->root, &gate));
    e = actor(0, 5, 0, 4);
    move(&e, 10, 0);
    NEAR(e.position.x, 10); /* Current point is outside the authored bank. */
    gate.mode = MESHMAP_GATE_BODY;
    REQUIRE(meshmap_bank_set_gate(&level->meshmaps->root, &gate));
    e = actor(0, 5, 0, 4);
    e.modeldata.size.x = 10;
    move(&e, 10, 0);
    REQUIRE(e.position.x < 5 && e.position.x > 4.9);
}

static void test_drop_through(void)
{
    entity e;
    s_meshmap *map;
    s_meshmap_config config;
    scene();
    map = flat(3, MESHMAP_AXIS_Y, -10, 0, -10, 20, 20, 10);
    config = map->config;
    config.flags |= MESHMAP_ONE_WAY | MESHMAP_DROP_THROUGH;
    config.support_group = 17;
    REQUIRE(meshmap_configure(map, &config));
    /* The lower ungrouped map ID equals the upper explicit group number. */
    flat(17, MESHMAP_AXIS_Y, -10, 0, -10, 20, 20, 0);
    e = actor(0, 10, 0, 2);
    meshmap_entity_adjust_base(&e, MESHMAP_ENTITY_VOID_BASE);
    REQUIRE(e.meshmap.support_id == 3);
    REQUIRE(meshmap_entity_drop(&e));
    REQUIRE(e.meshmap.drop_group == 17);
    NEAR(meshmap_entity_vertical(&e, -5), 0);
    REQUIRE(e.meshmap.support_id == 17);
    e.position.y = 0;
    e.velocity.y = 0;
    meshmap_entity_adjust_base(&e, MESHMAP_ENTITY_VOID_BASE);
    REQUIRE(e.meshmap.drop_id == -1);
    REQUIRE(!meshmap_entity_drop(&e));
}

static void test_hypothetical_move_origin(void)
{
    entity e;
    s_meshmap *wall;
    s_meshmap_config config;
    s_meshmap_gate gate = {0};
    scene();
    flat(0, MESHMAP_AXIS_Y, -5, 0, -10, 30, 20, 0);
    wall = flat(1, MESHMAP_AXIS_Z, 5, 20, 0, 1, 20, 1);
    e = actor(0, 0, 0, 4);
    meshmap_entity_adjust_base(&e, MESHMAP_ENTITY_VOID_BASE);
    /* The actor cannot cross the wall, but an A* edge wholly beyond it is
     * clear. Testing that edge must not trace from the actor's real origin. */
    REQUIRE(!meshmap_entity_test_move(&e, 0, 0, 12, 0));
    REQUIRE(meshmap_entity_test_move(&e, 10, 0, 12, 0));
    NEAR(e.position.x, 0);
    NEAR(e.position.y, 0);
    REQUIRE(e.meshmap.support_id == 0);

    /* Bank admission also belongs to the hypothetical start. The actor's
     * actual position is outside this gate; the requested edge enters a wall
     * in an eligible bank and must still be rejected. */
    config = wall->config;
    config.origin.x = 11;
    REQUIRE(meshmap_configure(wall, &config));
    gate.enabled = 1;
    gate.bounds.min = (s_meshmap_vec3){9, -100, -100};
    gate.bounds.max = (s_meshmap_vec3){13, 100, 100};
    REQUIRE(meshmap_bank_set_gate(&level->meshmaps->root, &gate));
    REQUIRE(!meshmap_entity_test_move(&e, 10, 0, 12, 0));
    NEAR(e.position.x, 0);
    REQUIRE(e.meshmap.support_id == 0);
}

static void test_curves_edges_and_headroom(void)
{
    const uint16_t hill[6] = {1, 65535, 1, 1, 65535, 1};
    const uint16_t edge[6] = {1, 1, 0, 1, 1, 0};
    entity e;
    s_meshmap *map;
    s_meshmap_config config;
    scene();
    map = flat(0, MESHMAP_AXIS_Y, 0, 0, 0, 5, 10, 0);
    config = map->config;
    config.max_height = 2;
    REQUIRE(meshmap_configure(map, &config));
    REQUIRE(meshmap_set_geometry(map, 3, 2, hill, "hill"));
    e = actor(0, 0, 5, 3);
    e.meshmap.max_step_height = 0;
    meshmap_entity_adjust_base(&e, MESHMAP_ENTITY_VOID_BASE);
    move(&e, 10, 0);
    NEAR(e.position.x, 10);
    NEAR(e.position.y, 0);
    REQUIRE(e.meshmap.support_id == 0);

    /* Without top contacts in lateral queries, the straight path would cut
     * through this one-way crest and pass beneath a beam. Following the crest
     * raises the body into the beam and must stop the move. */
    config.flags |= MESHMAP_ONE_WAY;
    REQUIRE(meshmap_configure(map, &config));
    flat(1, MESHMAP_AXIS_Z, 4, 6, 5, 2, 2, 1);
    e = actor(0, 0, 5, 3);
    e.meshmap.max_step_height = 0;
    meshmap_entity_adjust_base(&e, MESHMAP_ENTITY_VOID_BASE);
    move(&e, 10, 0);
    REQUIRE(e.position.x < 4);
    REQUIRE(e.position.y > 0);
    REQUIRE(meshmap_remove(level->meshmaps, 1));

    /* Missing cells release support instead of leaving a remembered floor. */
    config.max_height = 0;
    REQUIRE(meshmap_configure(map, &config));
    REQUIRE(meshmap_set_geometry(map, 3, 2, edge, "edge"));
    e = actor(4, 0, 5, 3);
    meshmap_entity_adjust_base(&e, MESHMAP_ENTITY_VOID_BASE);
    move(&e, 4, 0);
    NEAR(e.position.x, 8);
    REQUIRE(e.meshmap.support_id == -1);
    REQUIRE(e.base == MESHMAP_ENTITY_VOID_BASE);

    /* Enough leg clearance is insufficient when a step puts the head/body
     * into a ceiling. Test the actual route as well as its final support. */
    scene();
    flat(0, MESHMAP_AXIS_Y, 0, 0, -10, 30, 20, 0);
    flat(1, MESHMAP_AXIS_Z, 10, 5, 0, 10, 10, 1);
    flat(2, MESHMAP_AXIS_Z, 6, 7, 0, 10, 1, 1);
    e = actor(8, 0, 0, 3);
    e.meshmap.max_step_height = 6;
    meshmap_entity_adjust_base(&e, MESHMAP_ENTITY_VOID_BASE);
    move(&e, 4, 0);
    REQUIRE(e.position.x < 10);
    NEAR(e.position.y, 0);
}

static void test_movement_planes_and_one_way_ramps(void)
{
    const uint16_t ramp[4] = {1, 65535, 1, 65535};
    entity e;
    s_meshmap *map;
    s_meshmap_config config;
    scene();
    map = flat(0, MESHMAP_AXIS_Y, 0, 0, 0, 10, 10, 0);
    config = map->config;
    config.max_height = 2;
    config.flags |= MESHMAP_ONE_WAY;
    REQUIRE(meshmap_configure(map, &config));
    REQUIRE(meshmap_set_geometry(map, 2, 2, ramp, "one-way ramp"));
    e = actor(0, 0, 5, 3);
    e.meshmap.max_step_height = 0;
    meshmap_entity_adjust_base(&e, MESHMAP_ENTITY_VOID_BASE);
    move(&e, 10, 0);
    NEAR(e.position.x, 10);
    NEAR(e.position.y, 2);
    move(&e, -10, 0);
    NEAR(e.position.y, 0);

    e = actor(2, 15, 5, 3);
    e.meshmap.movement_plane = MESHMAP_AXIS_Y;
    e.velocity.y = -4;
    meshmap_entity_adjust_base(&e, MESHMAP_ENTITY_VOID_BASE);
    NEAR(e.base, 15);
    NEAR(e.velocity.y, 0);
    NEAR(meshmap_entity_vertical(&e, -50), 15);
    move(&e, 1, 1);
    NEAR(e.position.x, 3);
    NEAR(e.position.y, 15);
    NEAR(e.position.z, 6);
    e.meshmap.movement_plane = MESHMAP_AXIS_Z;
    move(&e, 1, 1);
    NEAR(e.position.x, 4);
    NEAR(e.position.z, 6);
    e.meshmap.movement_plane = MESHMAP_AXIS_X;
    move(&e, 1, 1);
    NEAR(e.position.x, 4);
    NEAR(e.position.z, 7);
}

static void test_disconnected_support(void)
{
    const uint16_t islands[14] = {
        65535, 65535, 0, 0, 1, 1, 1,
        65535, 65535, 0, 0, 1, 1, 1
    };
    entity e;
    s_meshmap *map;
    s_meshmap_config config;
    scene();
    map = flat(0, MESHMAP_AXIS_Y, 0, 0, 0, 2, 4, 0);
    config = map->config;
    config.max_height = 8;
    REQUIRE(meshmap_configure(map, &config));
    REQUIRE(meshmap_set_geometry(map, 7, 2, islands, "disconnected islands"));
    e = actor(1, 8, 2, 3);
    meshmap_entity_adjust_base(&e, MESHMAP_ENTITY_VOID_BASE);
    move(&e, 10, 0);
    NEAR(e.position.x, 11);
    NEAR(e.position.y, 8); /* Walk off, then fall through normal gravity. */
    REQUIRE(e.meshmap.support_id == -1);
    REQUIRE(e.base == MESHMAP_ENTITY_VOID_BASE);
}

int main(void)
{
    test_ground_and_holes();
    test_slopes();
    test_landing_support_identity();
    test_large_world_resting_contact();
    test_shallow_exit_resting_contact();
    test_vertical_contact_height();
    test_side_platforms();
    test_steps_and_intervening_wall();
    test_x_wall_and_gates();
    test_drop_through();
    test_curves_edges_and_headroom();
    test_movement_planes_and_one_way_ramps();
    test_disconnected_support();
    test_hypothetical_move_origin();
    meshmap_store_free(level->meshmaps);
    level = NULL;
    printf("meshmap entity: %u outcome checks passed\n", assertions);
    return 0;
}
