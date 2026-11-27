/*
 * Native callsite regressions use the production openbor.c functions extracted
 * by the runner, the real entity adapter, and the real terrain core. Only
 * unrelated legacy platform/event services below are replaced by fixtures.
 */
#include "openbor.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#undef printf
#undef malloc
#undef calloc
#undef realloc
#undef free

s_level *level;
entity *self;
uint64_t _time = 1;
s_global_config global_config = {.game_speed = 100};
static s_level stage;
static s_anim animation;
static s_move frame_move;
static s_move *moves[] = {&frame_move};
static unsigned checks, move_y_events;
static float event_x, event_y;

#define CHECK(condition) do { ++checks; if(!(condition)) { \
    fprintf(stderr, "%s:%d: %s\n", __FILE__, __LINE__, #condition); exit(1); } } while(0)
#define NEAR(actual, expected) CHECK(fabs((double)(actual) - (expected)) < 0.025)

entity *check_platform_below_entity(entity *e) { (void)e; return NULL; }
entity *check_platform_below(float x, float z, float y, entity *e)
{ (void)x; (void)z; (void)y; (void)e; return NULL; }
int checkwall_below(float x, float z, float y)
{ (void)x; (void)z; (void)y; return WALL_INDEX_NONE; }
int checkhole(float x, float z) { (void)x; (void)z; return 0; }
int checkhole_in(float x, float z, float y)
{ (void)x; (void)z; (void)y; return 0; }
int checkholeindex_in(float x, float z, float y)
{ (void)x; (void)z; (void)y; return HOLE_INDEX_NONE; }
float get_platform_base(entity *e) { return e->position.y; }
int testplatform(entity *platform, float x, float z, entity *e)
{ (void)platform; (void)x; (void)z; (void)e; return 0; }
void check_gravity(entity *e) { (void)e; CHECK(0); }
void check_link_move(float x, float z) { (void)x; (void)z; CHECK(0); }
void ent_unlink(entity *e) { (void)e; }
void execute_inhole_script(entity *e, s_terrain *hole, int index)
{ (void)e; (void)hole; (void)index; CHECK(0); }
int is_frozen(entity *e) { (void)e; return 0; }
void execute_onmovea_script(entity *e)
{
    ++move_y_events;
    event_x = e->position.x;
    event_y = e->position.y;
}

#include "meshmap_native_hooks.inc"

static void scene(void)
{
    if(level) meshmap_store_free(level->meshmaps);
    memset(&stage, 0, sizeof(stage));
    level = &stage;
    level->meshmaps = meshmap_store_create();
    CHECK(level->meshmaps != NULL);
    move_y_events = 0;
    self = NULL;
}

static entity actor(float x, float y, float z, int height)
{
    entity e;
    memset(&e, 0, sizeof(e));
    memset(&animation, 0, sizeof(animation));
    memset(&frame_move, 0, sizeof(frame_move));
    animation.numframes = 1;
    animation.move = moves;
    e.animation = &animation;
    meshmap_entity_initialize(&e);
    e.position = (s_axis_principal_float){x, y, z};
    e.modeldata.size.x = 2;
    e.modeldata.size.y = height;
    e.base = MESHMAP_ENTITY_VOID_BASE;
    return e;
}

static s_meshmap *flat(uint32_t id, e_meshmap_axis axis, double x, double y,
                       double z, double du, double dv, double elevation)
{
    const uint16_t samples[] = {1, 1, 1, 1};
    s_meshmap *map = meshmap_create(level->meshmaps, id);
    s_meshmap_config config;
    CHECK(map != NULL);
    meshmap_config_default(&config);
    config.axis = axis;
    config.origin = (s_meshmap_vec3){x, y, z};
    config.spacing_u = du;
    config.spacing_v = dv;
    config.min_height = config.max_height = elevation;
    CHECK(meshmap_configure(map, &config));
    CHECK(meshmap_set_geometry(map, 2, 2, samples, "native-hook-fixture"));
    return map;
}

static void test_explicit_zero_basemap(void)
{
    float samples[4] = {0};
    s_basemap basemap = {.position = {-1, -1}, .size = {2, 2}, .map = samples};
    entity e, *platform = NULL;
    scene();
    /* Mesh terrain elsewhere disables the implicit floor. This authored
     * legacy basemap must still provide its explicitly specified Y=0 floor. */
    flat(0, MESHMAP_AXIS_Y, 100, 0, 100, 10, 10, 10);
    level->basemaps = &basemap;
    level->numbasemaps = 1;
    e = actor(0, 0, 0, 4);
    e.modeldata.move_config_flags = MOVE_CONFIG_SUBJECT_TO_BASEMAP;
    CHECK(check_basemap_index(0, 0) == 0);
    NEAR(checkbase(0, 0, 0, &e), 0);
    adjust_base(&e, &platform);
    NEAR(e.base, 0);
    CHECK(e.meshmap.support_id == -1);
    CHECK(check_basemap_index(20, 20) == -1);
}

static void test_negative_mesh_floor(void)
{
    entity e, *platform = NULL;
    scene();
    flat(0, MESHMAP_AXIS_Y, -10, 0, -10, 20, 20, -1000);
    e = actor(0, -1000, 0, 4);
    e.modeldata.move_config_flags = MOVE_CONFIG_SUBJECT_TO_HOLE;
    e.base = -1000;
    e.velocity.x = 7;
    e.velocity.z = 3;
    adjust_base(&e, &platform);
    NEAR(e.base, -1000);
    NEAR(e.position.y, -1000);
    NEAR(e.velocity.x, 7);
    NEAR(e.velocity.z, 3);
    CHECK(e.meshmap.support_id == 0);
    NEAR(meshmap_entity_pit_depth(&e), -1250);

    /* Animation-controlled vertical motion must not activate an unauthored
     * maxbase=0 fallback while the actor is below zero. */
    frame_move.axis.y = 1;
    adjust_base(&e, &platform);
    NEAR(e.position.y, -1000);
    NEAR(e.base, -1000);
    NEAR(e.velocity.x, 7);
}

static void test_direct_movement(void)
{
    entity e, carrier;
    s_meshmap *wall;
    s_meshmap_config config;
    scene();
    wall = flat(0, MESHMAP_AXIS_X, 0, 20, -10, 20, 20, 5);
    config = wall->config;
    config.side = MESHMAP_SIDE_POSITIVE;
    CHECK(meshmap_configure(wall, &config));
    e = actor(0, 5, 0, 4);
    e.movex = 10;
    check_move(&e);
    CHECK(e.position.x < 5 && e.position.x > 4.9);
    NEAR(e.movex, e.position.x);
    CHECK(e.update_mark & UPDATE_MARK_CHECK_MOVE);
    CHECK(self == NULL);

    /* Platform carry has its own direct-movement branch. */
    e = actor(0, 5, 0, 4);
    memset(&carrier, 0, sizeof(carrier));
    carrier.movex = 10;
    carrier.update_mark = UPDATE_MARK_CHECK_MOVE;
    e.landed_on_platform = &carrier;
    check_move(&e);
    CHECK(e.position.x < 5 && e.position.x > 4.9);
    CHECK(self == NULL);

    e = actor(0, 5, 0, 4);
    e.meshmap.enabled = 0;
    e.movex = 10;
    check_move(&e);
    NEAR(e.position.x, 10);
}

static void test_step_movement_event(void)
{
    const uint16_t ramp[] = {1, 65535, 1, 65535};
    s_meshmap *map;
    s_meshmap_config config;
    entity e;
    scene();
    map = flat(0, MESHMAP_AXIS_Y, 0, 0, 0, 10, 10, 0);
    config = map->config;
    config.max_height = 2;
    CHECK(meshmap_configure(map, &config));
    CHECK(meshmap_set_geometry(map, 2, 2, ramp, "native-ramp"));
    e = actor(0, 0, 5, 4);
    meshmap_entity_adjust_base(&e, MESHMAP_ENTITY_VOID_BASE);
    e.movex = 2;
    check_move(&e);
    NEAR(e.position.y, 0.4);
    CHECK(move_y_events == 1);
    NEAR(event_x, 2);
    NEAR(event_y, 0.4);
}

static void test_legacy_floor_unchanged(void)
{
    entity e, *platform = NULL;
    scene();
    e = actor(0, 0, 0, 4);
    e.base = 0;
    adjust_base(&e, &platform);
    NEAR(e.base, 0);
    NEAR(checkbase(0, 0, 0, &e), 0);
    meshmap_store_free(level->meshmaps);
    level->meshmaps = NULL;
    adjust_base(&e, &platform);
    NEAR(e.base, 0);
    NEAR(checkbase(0, 0, 0, &e), 0);

    /* The old hole sentinel still has its original meaning without meshes. */
    e.base = e.position.y = T_MIN_BASEMAP;
    e.velocity.x = 7;
    e.velocity.z = 3;
    e.modeldata.move_config_flags = MOVE_CONFIG_SUBJECT_TO_HOLE;
    adjust_base(&e, &platform);
    NEAR(e.velocity.x, 0);
    NEAR(e.velocity.z, 0);
}

int main(void)
{
    test_explicit_zero_basemap();
    test_negative_mesh_floor();
    test_direct_movement();
    test_step_movement_event();
    test_legacy_floor_unchanged();
    meshmap_store_free(level->meshmaps);
    level = NULL;
    printf("meshmap production native hooks: %u outcome checks passed\n", checks);
    return 0;
}
