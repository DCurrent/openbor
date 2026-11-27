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
s_savedata savedata;
s_global_sample global_sample_list = {.fall = -1};
float default_level_gravity = -0.1f;
float default_level_maxfallspeed = -6.0f;
float default_level_maxtossspeed = 100.0f;
int autoland;
static s_level stage;
static s_anim animation;
static s_move frame_move;
static s_move *moves[] = {&frame_move};
static unsigned checks, move_y_events, land_events;
static float event_x, event_y;

#define CHECK(condition) do { ++checks; if(!(condition)) { \
    fprintf(stderr, "%s:%d: %s\n", __FILE__, __LINE__, #condition); exit(1); } } while(0)
#define NEAR(actual, expected) CHECK(fabs((double)(actual) - (expected)) < 0.025)

entity *check_platform_below_entity(entity *e) { (void)e; return NULL; }
entity *check_platform_above_entity(entity *e) { (void)e; return NULL; }
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
void check_link_move(float x, float z) { (void)x; (void)z; CHECK(0); }
void ent_unlink(entity *e) { (void)e; }
void execute_inhole_script(entity *e, s_terrain *hole, int index)
{ (void)e; (void)hole; (void)index; CHECK(0); }
int is_frozen(entity *e) { (void)e; return 0; }
int check_edge(entity *e) { (void)e; return EDGE_NONE; }
bool check_bind_override(entity *e, bind_config_t config)
{ (void)e; (void)config; return false; }
bool check_frame_set_drop(entity *e) { (void)e; return false; }
bool check_landframe(entity *e) { (void)e; ++land_events; return false; }
void checkdamageonlanding(entity *e) { (void)e; }
void execute_onblocky_script(entity *e, entity *other)
{ (void)e; (void)other; CHECK(0); }
void common_walkoff(void) { CHECK(0); }
void ent_set_anim(entity *e, animation_id_t animation_id, int resetable)
{ (void)e; (void)animation_id; (void)resetable; CHECK(0); }
entity *spawn(const float x, const float z, const float y, const e_direction direction,
              char *name, const int index, s_model *model)
{ (void)x; (void)z; (void)y; (void)direction; (void)name; (void)index; (void)model; CHECK(0); return NULL; }
void execute_onspawn_script(entity *e) { (void)e; CHECK(0); }
void toss(entity *e, float lift) { (void)e; (void)lift; CHECK(0); }
int sound_play_sample(int sample, unsigned int priority, int left, int right, unsigned int speed)
{ (void)sample; (void)priority; (void)left; (void)right; (void)speed; CHECK(0); return 0; }
void control_rumble(int port, int ratio, int duration)
{ (void)port; (void)ratio; (void)duration; CHECK(0); }
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
    level->gravity = default_level_gravity;
    level->maxfallspeed = default_level_maxfallspeed;
    level->maxtossspeed = default_level_maxtossspeed;
    level->meshmaps = meshmap_store_create();
    CHECK(level->meshmaps != NULL);
    move_y_events = 0;
    land_events = 0;
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

/* Exercise the production movement and gravity hooks consecutively, as the
 * game does. Traversing this very shallow 16-bit exit must never invoke a land
 * callback or clear horizontal speed while the feet retain mesh support. */
static void test_shallow_exit_gravity(void)
{
    const uint16_t samples[] = {32782, 32783, 32768, 32768, 32768, 32768};
    const float velocity_x = -1.6772f * 0.5f;
    const float velocity_z = 12.617609f * 0.5f + 250 * 0.00001f;
    entity e;
    s_meshmap_config config;
    s_meshmap_sample sample;
    s_meshmap *map;
    scene();
    map = flat(0, MESHMAP_AXIS_Y, 7168, 0, 7936, 128, 128, 0);
    config = map->config;
    config.min_height = -128;
    config.max_height = 128;
    CHECK(meshmap_configure(map, &config));
    CHECK(meshmap_set_geometry(map, 2, 3, samples, "native shallow exit"));
    e = actor(7279.8427734375f, 0, 7955.044921875f, 4);
    CHECK(meshmap_sample_world(map, (s_meshmap_vec3){e.position.x, 0, e.position.z}, &sample));
    e.position.y = e.base = (float)sample.position.y;
    e.modeldata.move_config_flags |= MOVE_CONFIG_SUBJECT_TO_GRAVITY;
    animation.move_config_flags |= MOVE_CONFIG_SUBJECT_TO_GRAVITY;
    e.velocity.x = velocity_x;
    e.velocity.z = velocity_z;
    for(unsigned tick = 0; tick < 32; ++tick)
    {
        ++_time;
        e.update_mark = 0;
        e.movex = velocity_x;
        e.movez = velocity_z;
        check_move(&e);
        check_gravity(&e);
        CHECK(e.velocity.x == velocity_x && e.velocity.z == velocity_z);
        CHECK(e.position.y == e.base);
        CHECK(e.meshmap.support_id == 0);
        CHECK(land_events == 0);
        CHECK(self == NULL);
    }
    CHECK(e.position.z > 8064);
    CHECK(e.position.y == 0);
}

/* Canonical contact must not suppress a genuine fall. Ordinary native landing
 * still clears velocity and notifies once; the next grounded tick must retain
 * any newly applied horizontal movement without a second landing. */
static void test_gravity_lands_once(void)
{
    const double floors[] = {0.0, -31.67531677};
    for(unsigned variant = 0; variant < sizeof(floors) / sizeof(floors[0]); ++variant)
    {
        entity e;
        scene();
        flat(7, MESHMAP_AXIS_Y, -10, 0, -10, 20, 20, floors[variant]);
        e = actor(0, (float)(floors[variant] + 3.0), 0, 4);
        e.modeldata.move_config_flags |= MOVE_CONFIG_SUBJECT_TO_GRAVITY;
        animation.move_config_flags |= MOVE_CONFIG_SUBJECT_TO_GRAVITY;
        e.velocity = (s_axis_principal_float){7, -5, 3};
        e.falling = 1;
        ++_time;
        check_gravity(&e);
        CHECK(e.position.y == (float)floors[variant]);
        CHECK(e.position.y == e.base);
        CHECK(e.velocity.x == 0 && e.velocity.y == 0 && e.velocity.z == 0);
        CHECK(e.falling == 0);
        CHECK(e.meshmap.support_id == 7);
        CHECK(land_events == 1);
        e.velocity.x = 7;
        e.velocity.z = 3;
        e.update_mark = 0;
        ++_time;
        check_gravity(&e);
        CHECK(e.velocity.x == 7 && e.velocity.z == 3);
        CHECK(e.position.y == e.base);
        CHECK(land_events == 1);
        CHECK(self == NULL);
    }
}

int main(void)
{
    test_explicit_zero_basemap();
    test_negative_mesh_floor();
    test_direct_movement();
    test_step_movement_event();
    test_legacy_floor_unchanged();
    test_shallow_exit_gravity();
    test_gravity_lands_once();
    meshmap_store_free(level->meshmaps);
    level = NULL;
    printf("meshmap production native hooks: %u outcome checks passed\n", checks);
    return 0;
}
