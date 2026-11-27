/*
 * Exercise production script/level authoring against real PNG files. Only the
 * archive transport, screen disposal, draw backend and entity drop callback are
 * replaced: their own terrain/renderer tests cover those subsystems separately.
 */
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "scriptcommon.h"
#include "commands.h"
#include "../engine/source/gamelib/meshmap_render.h"

#undef malloc
#undef calloc
#undef realloc

s_level *level;
char packfile[MAX_FILENAME_LEN] = "";
int BGHEIGHT;
animation_id_t *animspecials, *animattacks, *animfollows, *animpains;
animation_id_t *animfalls, *animrises, *animriseattacks, *animdies;
animation_id_t *animidles, *animwalks, *animbackwalks, *animups, *animdowns;
animation_id_t *animbackpains, *animbackfalls, *animbackdies, *animbackrises;
animation_id_t *animbackriseattacks, *animblkpains, *animbackblkpains;

static FILE *files[16];
static unsigned open_files, released_screens, draw_calls, drop_calls;
static unsigned checks;
static s_meshmap_view submitted_view;
static ScriptVariant arguments[20], returned;
static ScriptVariant *argument_pointers[20], *result = &returned;

void *__wrap_checkAlloc(void *pointer, size_t size, const char *function, const char *file, int line)
{
    (void)size; (void)function; (void)file; (void)line;
    assert(pointer);
    return pointer;
}

int openpackfile(const char *filename, const char *archive)
{
    unsigned i;
    (void)archive;
    for(i = 0; i < 16; ++i)
    {
        if(!files[i])
        {
            files[i] = fopen(filename, "rb");
            if(!files[i]) return -1;
            ++open_files;
            return (int)i;
        }
    }
    return -1;
}

int readpackfile(int handle, void *buffer, int length)
{
    assert(handle >= 0 && handle < 16 && files[handle]);
    return (int)fread(buffer, 1, (size_t)length, files[handle]);
}

int closepackfile(int handle)
{
    assert(handle >= 0 && handle < 16 && files[handle]);
    fclose(files[handle]);
    files[handle] = NULL;
    --open_files;
    return 0;
}

void freescreen(s_screen **screen)
{
    if(screen && *screen)
    {
        assert((*screen)->magic == screen_magic);
        free(*screen);
        *screen = NULL;
        ++released_screens;
    }
}

void meshmap_view_orthographic(s_meshmap_view *view, double offset_x, double offset_y)
{
    memset(view, 0, sizeof(*view));
    view->projection[0] = 1.0;
    view->projection[3] = offset_x;
    view->projection[5] = -1.0;
    view->projection[6] = 1.0;
    view->projection[7] = offset_y;
    view->projection[11] = 1.0;
    view->near_w = 0.0001;
}

int meshmap_draw(const s_meshmap_store *store, uint32_t id, s_screen *screen,
                const s_meshmap_view *view, const s_drawmethod *method)
{
    assert(meshmap_get_const(store, id));
    assert(screen && screen->magic == screen_magic);
    assert(!method || method->object_type == OBJECT_TYPE_DRAWMETHOD);
    submitted_view = *view;
    ++draw_calls;
    return 1;
}

int meshmap_entity_drop(entity *actor)
{
    ++drop_calls;
    return actor->meshmap.support_id >= 0;
}

static void prepare(void)
{
    unsigned i;
    for(i = 0; i < 20; ++i)
    {
        ScriptVariant_Clear(&arguments[i]);
        argument_pointers[i] = &arguments[i];
    }
    ScriptVariant_Clear(&returned);
    result = &returned;
}

static void integer(unsigned index, int value)
{
    ScriptVariant_ChangeType(&arguments[index], VT_INTEGER);
    arguments[index].lVal = value;
}

static void wide(unsigned index, uint64_t value)
{
    ScriptVariant_ChangeType(&arguments[index], VT_UINTEGER64);
    arguments[index].ullVal = value;
}

static void decimal(unsigned index, double value)
{
    ScriptVariant_ChangeType(&arguments[index], VT_DECIMAL);
    arguments[index].dblVal = value;
}

static void pointer(unsigned index, void *value)
{
    ScriptVariant_ChangeType(&arguments[index], VT_PTR);
    arguments[index].ptrVal = value;
}

static void string(unsigned index, const char *value)
{
    ScriptVariant_ChangeType(&arguments[index], VT_STR);
    arguments[index].strVal = StrCache_CreateNewFrom(value);
}

static void map_arguments(uint32_t id, int property)
{
    prepare();
    pointer(0, level->meshmaps);
    integer(1, (int)id);
    integer(2, property);
}

static void bank_arguments(unsigned depth, uint32_t prefix, int property)
{
    prepare();
    pointer(0, level->meshmaps);
    integer(1, (int)depth);
    integer(2, (int)prefix);
    integer(3, property);
}

static void expect(int success, HRESULT actual)
{
    assert(success ? SUCCEEDED(actual) : FAILED(actual));
    ++checks;
}

static void check_constant(const char *name, uint64_t value)
{
    uint64_t actual;
    prepare();
    string(0, name);
    assert(mapstrings_transconst(argument_pointers, 1));
    assert(SUCCEEDED(ScriptVariant_Unsigned64Value(&arguments[0], &actual)));
    assert(actual == value);
    ++checks;
}

static void test_level_and_masks(void)
{
    s_meshmap_store *store;
    prepare();
    integer(0, 0);
    integer(1, LEVEL_PROPERTY_MESHMAP_COUNT);
    expect(1, openbor_get_level_property(argument_pointers, &result, 2));
    assert(returned.lVal == 0 && !level->meshmaps);
    integer(1, LEVEL_PROPERTY_MESHMAP_COLLECTION);
    expect(1, openbor_get_level_property(argument_pointers, &result, 2));
    assert(returned.vt == VT_PTR && returned.ptrVal == level->meshmaps);
    store = level->meshmaps;
    assert(store && store->map_count == 0 && store->bank_count == 1);
    map_arguments(0, MESHMAP_PROPERTY_EXISTS);
    expect(1, openbor_get_meshmap_property(argument_pointers, &result, 3));
    assert(returned.lVal == 0);
    integer(3, 1);
    expect(1, openbor_set_meshmap_property(argument_pointers, &result, 4));
    map_arguments(MESHMAP_ID_MAX, MESHMAP_PROPERTY_EXISTS);
    integer(3, 1);
    expect(1, openbor_set_meshmap_property(argument_pointers, &result, 4));
    assert(store->map_count == 2);
    bank_arguments(0, 0, MESHMAP_BANK_PROPERTY_ACTIVE_MASK);
    expect(1, openbor_get_meshmap_bank_property(argument_pointers, &result, 4));
    assert(returned.vt == VT_UINTEGER64 && returned.ullVal == (UINT64_C(1) << 63 | 1));
    wide(4, UINT64_C(1) << 63);
    expect(1, openbor_set_meshmap_bank_property(argument_pointers, &result, 5));
    assert(meshmap_is_active(store, MESHMAP_ID_MAX) && !meshmap_is_active(store, 0));
    wide(4, UINT64_C(1) << 5);
    expect(0, openbor_set_meshmap_bank_property(argument_pointers, &result, 5));
    assert(store->root.active_mask == (UINT64_C(1) << 63));
    wide(4, UINT64_C(1) << 63 | 1);
    expect(1, openbor_set_meshmap_bank_property(argument_pointers, &result, 5));
    decimal(4, 9223372036854775808.0);
    expect(0, openbor_set_meshmap_bank_property(argument_pointers, &result, 5));
    map_arguments(0, MESHMAP_PROPERTY_EXISTS);
    pointer(0, (void *)(uintptr_t)1);
    expect(0, openbor_get_meshmap_property(argument_pointers, &result, 3));
    prepare();
    expect(0, openbor_get_meshmap_property(argument_pointers, &result, 0));
    check_constant("MESHMAP_ID_MAX", MESHMAP_ID_MAX);
    check_constant("MESHMAP_GROUP_NONE", MESHMAP_GROUP_NONE);
    check_constant("MESHMAP_PROPERTY_SAMPLE", MESHMAP_PROPERTY_SAMPLE);
    check_constant("MESHMAP_BANK_PROPERTY_ACTIVE_MASK", MESHMAP_BANK_PROPERTY_ACTIVE_MASK);
    check_constant("ENTITY_PROPERTY_MESHMAP_DROP_REQUEST", ENTITY_PROPERTY_MESHMAP_DROP_REQUEST);
}

static void test_geometry_and_artwork(const char *directory)
{
    char geometry[1024], indexed[1024], rgb[1024];
    s_meshmap *map = meshmap_get(level->meshmaps, 0);
    s_meshmap_sample sample;
    uint64_t revision;
    snprintf(geometry, sizeof(geometry), "%s/geometry.png", directory);
    snprintf(indexed, sizeof(indexed), "%s/indexed.png", directory);
    snprintf(rgb, sizeof(rgb), "%s/rgb.png", directory);
    map_arguments(0, MESHMAP_PROPERTY_GEOMETRY);
    string(3, geometry);
    expect(1, openbor_set_meshmap_property(argument_pointers, &result, 4));
    assert(map->width == 3 && map->height == 3 && map->present_cells == 4);
    map_arguments(0, MESHMAP_PROPERTY_MINIMUM);
    decimal(3, -20.0);
    expect(1, openbor_set_meshmap_property(argument_pointers, &result, 4));
    map_arguments(0, MESHMAP_PROPERTY_MAXIMUM);
    decimal(3, 20.0);
    expect(1, openbor_set_meshmap_property(argument_pointers, &result, 4));
    assert(meshmap_decode_height(map, 1) == -20.0);
    assert(meshmap_decode_height(map, 32768) == 0.0);
    assert(meshmap_decode_height(map, 65535) == 20.0);
    map_arguments(0, MESHMAP_PROPERTY_SAMPLE);
    integer(3, 1); integer(4, 1);
    expect(1, openbor_get_meshmap_property(argument_pointers, &result, 5));
    assert(returned.lVal == 32768);
    integer(3, 0); integer(4, 1); integer(5, 1);
    expect(1, openbor_set_meshmap_property(argument_pointers, &result, 6));
    assert(map->present_cells == 0 && !meshmap_sample_local(map, 0.5, 0.5, &sample));
    integer(3, 32768);
    expect(1, openbor_set_meshmap_property(argument_pointers, &result, 6));
    assert(map->present_cells == 4);
    revision = map->revision;
    integer(3, 65536);
    expect(0, openbor_set_meshmap_property(argument_pointers, &result, 6));
    assert(map->revision == revision && map->samples[4] == 32768);
    map_arguments(0, MESHMAP_PROPERTY_SPACING_U);
    decimal(3, 0.0);
    expect(0, openbor_set_meshmap_property(argument_pointers, &result, 4));
    assert(map->config.spacing_u == 1.0 && map->revision == revision);
    map_arguments(0, MESHMAP_PROPERTY_AXIS);
    integer(3, 3);
    expect(0, openbor_set_meshmap_property(argument_pointers, &result, 4));
    assert(map->config.axis == MESHMAP_AXIS_Y);
    integer(3, MESHMAP_AXIS_X);
    expect(1, openbor_set_meshmap_property(argument_pointers, &result, 4));
    assert(map->config.axis == MESHMAP_AXIS_X);
    map_arguments(0, MESHMAP_PROPERTY_GEOMETRY);
    string(3, rgb);
    revision = map->revision;
    expect(0, openbor_set_meshmap_property(argument_pointers, &result, 4));
    assert(map->revision == revision && !strcmp(map->geometry_path, geometry));
    map_arguments(0, MESHMAP_PROPERTY_ARTWORK);
    string(3, indexed);
    expect(1, openbor_set_meshmap_property(argument_pointers, &result, 4));
    assert(((s_screen *)map->artwork)->width == 5 && ((s_screen *)map->artwork)->pixelformat == PIXEL_x8);
    string(3, rgb);
    expect(1, openbor_set_meshmap_property(argument_pointers, &result, 4));
    assert(released_screens == 1 && ((s_screen *)map->artwork)->pixelformat == PIXEL_32);
    string(3, geometry);
    expect(0, openbor_set_meshmap_property(argument_pointers, &result, 4));
    assert(((s_screen *)map->artwork)->pixelformat == PIXEL_32);
    string(3, "");
    expect(1, openbor_set_meshmap_property(argument_pointers, &result, 4));
    assert(!map->artwork && map->samples && released_screens == 2);
    assert(!open_files);
}

static void test_native_commands(const char *directory)
{
    char geometry[1024], artwork[1024], error[512];
    const char *create[] = {"meshmap", "01", "02", "03", "08", "z", "10", "20", "30", "-40", "60", geometry, artwork};
    const char *area[] = {"meshmaparea", "1", "-100", "-200", "-300", "400", "500", "600", "1"};
    const char *spacing[] = {"meshmapspacing", "1", "2", "3", "8", "4", "8"};
    const char *rules[] = {"meshmaprules", "1", "2", "3", "8", "0x1f", "1", "0xffffffff", "60", "-10"};
    const char *mask[] = {"meshmapmask", "0", "0", "0x8000000000000003"};
    s_meshmap *map;
    s_meshmap_bank *bank;
    uint32_t id;
    size_t old_count;
    snprintf(geometry, sizeof(geometry), "%s/geometry.png", directory);
    snprintf(artwork, sizeof(artwork), "%s/rgb.png", directory);
    assert(meshmap_level_command(CMD_LEVEL_MESHMAPAREA, 9, area, error, sizeof(error)));
    assert(meshmap_level_command(CMD_LEVEL_MESHMAP, 13, create, error, sizeof(error)));
    assert(meshmap_pack_id(1, 2, 3, 8, &id));
    assert((map = meshmap_get(level->meshmaps, id)) && map->config.axis == MESHMAP_AXIS_Z);
    bank = meshmap_bank_get(level->meshmaps, 1, id, 0);
    assert(bank && bank->gate.mode == MESHMAP_GATE_BODY && bank->gate.bounds.min.y == -200.0);
    assert(meshmap_level_command(CMD_LEVEL_MESHMAPSPACING, 7, spacing, error, sizeof(error)));
    assert(map->config.spacing_u == 4 && map->config.spacing_v == 8);
    assert(meshmap_level_command(CMD_LEVEL_MESHMAPRULES, 10, rules, error, sizeof(error)));
    assert(map->config.flags == MESHMAP_FLAGS_ALL && map->config.side == MESHMAP_SIDE_POSITIVE
           && map->config.slope_limit == 60 && map->config.layer == -10);
    assert(meshmap_level_command(CMD_LEVEL_MESHMAPMASK, 4, mask, error, sizeof(error)));
    assert(level->meshmaps->root.active_mask == UINT64_C(0x8000000000000003));
    old_count = level->meshmaps->map_count;
    assert(!meshmap_level_command(CMD_LEVEL_MESHMAP, 13, create, error, sizeof(error)));
    assert(level->meshmaps->map_count == old_count && meshmap_get(level->meshmaps, id) == map);
    assert(!meshmap_level_command(CMD_LEVEL_MESHMAPAREA, 8,
           (const char *const[]){"meshmaparea", "64", "0", "0", "0", "1", "1", "1"}, error, sizeof(error)));
    assert(!meshmap_level_command(CMD_LEVEL_MESHMAPSPACING, 7,
           (const char *const[]){"meshmapspacing", "1", "2", "3", "8", "nan", "1"}, error, sizeof(error)));
    assert(map->config.spacing_u == 4);
    bank_arguments(1, id, MESHMAP_BANK_PROPERTY_EXISTS);
    integer(4, 0);
    expect(1, openbor_set_meshmap_bank_property(argument_pointers, &result, 5));
    assert(!meshmap_get(level->meshmaps, id) && !meshmap_bank_get(level->meshmaps, 1, id, 0));
    assert(level->meshmaps->map_count == old_count - 1 && !meshmap_store_uses_body_gates(level->meshmaps));
    checks += 12;
}

static void test_entity_and_draw(void)
{
    entity actor = {0};
    s_screen target = {0};
    actor.object_type = OBJECT_TYPE_ENTITY;
    actor.meshmap.support_id = 0;
    actor.meshmap.support_group = UINT32_MAX;
    actor.meshmap.drop_id = -1;
    prepare(); pointer(0, &actor); integer(1, ENTITY_PROPERTY_MESHMAP_MAX_SLOPE); decimal(2, 30.0);
    expect(1, openbor_set_entity_property(argument_pointers, &result, 3));
    assert(actor.meshmap.max_slope_degrees == 30.0f);
    decimal(2, NAN);
    expect(0, openbor_set_entity_property(argument_pointers, &result, 3));
    assert(actor.meshmap.max_slope_degrees == 30.0f);
    integer(1, ENTITY_PROPERTY_MESHMAP_SUPPORT_GROUP);
    expect(1, openbor_get_entity_property((const ScriptVariant *const *)argument_pointers, &result, 2));
    assert(returned.vt == VT_UINTEGER64 && returned.ullVal == UINT32_MAX);
    integer(1, ENTITY_PROPERTY_MESHMAP_DROP_REQUEST); integer(2, 1);
    expect(1, openbor_set_entity_property(argument_pointers, &result, 3));
    assert(returned.lVal == 1 && drop_calls == 1);
    decimal(2, 0.75);
    expect(0, openbor_set_entity_property(argument_pointers, &result, 3));
    assert(drop_calls == 1);
    integer(1, ENTITY_PROPERTY_MESHMAP_SUPPORT_ID);
    expect(0, openbor_set_entity_property(argument_pointers, &result, 3));
    assert(actor.meshmap.support_id == 0);
    prepare();
    expect(0, openbor_get_entity_property((const ScriptVariant *const *)argument_pointers, &result, 0));
    target.magic = screen_magic; target.width = 320; target.height = 240; target.pixelformat = PIXEL_32;
    prepare(); pointer(0, level->meshmaps); integer(1, 0); pointer(2, &target); pointer(3, NULL);
    decimal(4, -12.5); decimal(5, 20.0);
    expect(1, openbor_draw_meshmap(argument_pointers, &result, 6));
    assert(draw_calls == 1 && submitted_view.projection[3] == -12.5 && submitted_view.projection[7] == 20.0);
    for(unsigned i = 0; i < 12; ++i) decimal(4 + i, (double)i + 0.25);
    decimal(16, 0.05);
    expect(1, openbor_draw_meshmap_projected(argument_pointers, &result, 17));
    assert(draw_calls == 2 && submitted_view.projection[11] == 11.25 && submitted_view.near_w == 0.05);
    decimal(8, INFINITY);
    expect(0, openbor_draw_meshmap_projected(argument_pointers, &result, 17));
    assert(draw_calls == 2);
}

/* World probes are checked against an analytic plane in all three map
 * orientations. Their optional coordinates are world positions, not image
 * indices; holes remain distinct from a genuine world-zero surface. */
static void test_world_surface_properties(void)
{
    const uint16_t pixels[4] = {1, 65535, 1, 65535};
    s_meshmap *map = meshmap_create(level->meshmaps, 62);
    s_meshmap_config config;
    const int properties[3] = {MESHMAP_PROPERTY_SURFACE_X,
        MESHMAP_PROPERTY_SURFACE_Y, MESHMAP_PROPERTY_SURFACE_Z};
    const double expected[3] = {97.5, 17.5, 297.5};
    assert(map);
    meshmap_config_default(&config);
    config.origin = (s_meshmap_vec3){100, 20, 300};
    config.min_height = -10;
    config.max_height = 10;
    config.spacing_u = 4;
    config.spacing_v = 8;
    assert(meshmap_set_geometry(map, 2, 2, pixels, "analytic plane"));
    for(unsigned axis = 0; axis < 3; ++axis)
    {
        config.axis = (e_meshmap_axis)axis;
        assert(meshmap_configure(map, &config));
        map_arguments(62, properties[axis]);
        decimal(3, 101.5); decimal(4, 18); decimal(5, 301.5);
        expect(1, openbor_get_meshmap_property(argument_pointers, &result, 6));
        assert(returned.vt == VT_DECIMAL && fabs(returned.dblVal - expected[axis]) < 1e-10);
    }
    config.axis = MESHMAP_AXIS_Y;
    assert(meshmap_configure(map, &config));
    integer(2, MESHMAP_PROPERTY_SURFACE_NORMAL_X);
    expect(1, openbor_get_meshmap_property(argument_pointers, &result, 6));
    assert(fabs(returned.dblVal + 5 / sqrt(26)) < 1e-10);
    integer(2, MESHMAP_PROPERTY_SURFACE_NORMAL_Y);
    expect(1, openbor_get_meshmap_property(argument_pointers, &result, 6));
    assert(fabs(returned.dblVal - 1 / sqrt(26)) < 1e-10);
    integer(2, MESHMAP_PROPERTY_SURFACE_PRESENT);
    expect(1, openbor_get_meshmap_property(argument_pointers, &result, 6));
    assert(returned.vt == VT_INTEGER && returned.lVal == 1);
    assert(meshmap_set_active(level->meshmaps, 62, 0));
    expect(1, openbor_get_meshmap_property(argument_pointers, &result, 6));
    assert(returned.lVal == 1); /* Resource inspection is independent of admission. */
    assert(meshmap_set_sample(map, 0, 0, 0));
    expect(1, openbor_get_meshmap_property(argument_pointers, &result, 6));
    assert(returned.lVal == 0);
    integer(2, MESHMAP_PROPERTY_SURFACE_Y);
    expect(1, openbor_get_meshmap_property(argument_pointers, &result, 6));
    assert(returned.vt == VT_EMPTY);
    expect(0, openbor_get_meshmap_property(argument_pointers, &result, 5));
    decimal(3, INFINITY);
    expect(0, openbor_get_meshmap_property(argument_pointers, &result, 6));
    integer(3, 1);
    expect(0, openbor_set_meshmap_property(argument_pointers, &result, 4));
    assert(meshmap_remove(level->meshmaps, 62));
    check_constant("MESHMAP_PROPERTY_SURFACE_PRESENT", MESHMAP_PROPERTY_SURFACE_PRESENT);
    check_constant("MESHMAP_PROPERTY_SURFACE_Y", MESHMAP_PROPERTY_SURFACE_Y);
    check_constant("MESHMAP_PROPERTY_SURFACE_NORMAL_X", MESHMAP_PROPERTY_SURFACE_NORMAL_X);
}

int main(int argc, char **argv)
{
    s_level stage = {0};
    assert(argc == 2);
    level = &stage;
    test_level_and_masks();
    test_geometry_and_artwork(argv[1]);
    test_native_commands(argv[1]);
    test_entity_and_draw();
    test_world_surface_properties();
    bank_arguments(0, 0, MESHMAP_BANK_PROPERTY_EXISTS);
    integer(4, 0);
    expect(1, openbor_set_meshmap_bank_property(argument_pointers, &result, 5));
    assert(stage.meshmaps && stage.meshmaps->map_count == 0 && stage.meshmaps->bank_count == 1);
    meshmap_store_free(stage.meshmaps);
    stage.meshmaps = NULL;
    prepare();
    StrCache_Clear();
    assert(open_files == 0);
    printf("meshmap script/native authoring checks passed: %u assertions, exact 64-bit masks, real PNG transactions, samples/holes, native banks, entity settings and draw arguments\n", checks);
    return 0;
}
