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

/* Follow the real constant translation path before calling a script setter.
 * Named values describe behavior flags; addresses remain ordinary integers. */
static void named_constant(unsigned index, const char *name)
{
    string(index, name);
    assert(mapstrings_transconst(argument_pointers + index, 1));
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
    named_constant(0, name);
    assert(SUCCEEDED(ScriptVariant_Unsigned64Value(&arguments[0], &actual)));
    assert(actual == value);
    ++checks;
}

static void expect_native(s_meshmap_authoring_context *context, int success,
                          int command, size_t count, const char *const *tokens)
{
    char error[512];
    int actual = meshmap_level_command(context, command, count, tokens, error, sizeof(error));
    if(!!actual != !!success)
        fprintf(stderr, "%s: expected %s, received %s (%s)\n", tokens[0],
                success ? "success" : "failure", actual ? "success" : "failure", error);
    assert(!!actual == !!success);
    assert(success || error[0]);
    ++checks;
}

static void test_level_and_masks(void)
{
    s_meshmap_store *store;
    ScriptVariant saved = {{.ptrVal = NULL}, VT_EMPTY};
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
    ScriptVariant_Copy(&saved, &returned);
    integer(3, MESHMAP_BANK_PROPERTY_CHILD_ACTIVE);
    integer(4, 0); integer(5, 0);
    expect(1, openbor_set_meshmap_bank_property(argument_pointers, &result, 6));
    assert(meshmap_is_active(store, MESHMAP_ID_MAX) && !meshmap_is_active(store, 0));

    /* Opaque snapshots must retain slot 63 exactly through a read/clear/write
     * round trip. Creators select slots by integer index and do not need to
     * know the numeric representation of these returned masks. */
    integer(3, MESHMAP_BANK_PROPERTY_ACTIVE_MASK);
    expect(1, openbor_get_meshmap_bank_property(argument_pointers, &result, 4));
    assert(returned.vt == VT_UINTEGER64 && returned.ullVal == (UINT64_C(1) << 63));
    ScriptVariant_Copy(&arguments[6], &returned);
    named_constant(4, "MESHMAP_BANK_MASK_NONE");
    expect(1, openbor_set_meshmap_bank_property(argument_pointers, &result, 5));
    assert(!store->root.active_mask);
    ScriptVariant_Copy(&arguments[4], &arguments[6]);
    expect(1, openbor_set_meshmap_bank_property(argument_pointers, &result, 5));
    assert(store->root.active_mask == (UINT64_C(1) << 63));
    ScriptVariant_Copy(&arguments[4], &saved);
    assert(arguments[4].vt == VT_UINTEGER64 && arguments[4].ullVal == (UINT64_C(1) << 63 | 1));
    expect(1, openbor_set_meshmap_bank_property(argument_pointers, &result, 5));
    ScriptVariant_Clear(&saved);
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

/* Behavioral masks use named constants and the VM's ordinary bitwise OR.
 * The integer slot indices are deliberately absent from this name registry. */
static void test_symbolic_script_values(void)
{
    s_meshmap *map = meshmap_get(level->meshmaps, 0);
    assert(map && map->config.flags == MESHMAP_FLAGS_DEFAULT);
    check_constant("MESHMAP_FLAGS_NONE", MESHMAP_FLAGS_NONE);
    check_constant("MESHMAP_FLAGS_DEFAULT", MESHMAP_FLAGS_DEFAULT);
    check_constant("MESHMAP_FLAGS_ALL", MESHMAP_FLAGS_ALL);
    check_constant("MESHMAP_BANK_MASK_NONE", 0);
    check_constant("MESHMAP_BANK_PROPERTY_CHILD_PRESENT", MESHMAP_BANK_PROPERTY_CHILD_PRESENT);
    check_constant("MESHMAP_BANK_PROPERTY_CHILD_ACTIVE", MESHMAP_BANK_PROPERTY_CHILD_ACTIVE);
    map_arguments(0, MESHMAP_PROPERTY_FLAGS);
    named_constant(3, "MESHMAP_COLLIDE");
    named_constant(4, "MESHMAP_WALKABLE");
    ScriptVariant_Copy(&arguments[3], ScriptVariant_Bit_Or(&arguments[3], &arguments[4]));
    expect(1, openbor_set_meshmap_property(argument_pointers, &result, 4));
    assert(map->config.flags == (MESHMAP_COLLIDE | MESHMAP_WALKABLE));
    named_constant(3, "MESHMAP_FLAGS_NONE");
    expect(1, openbor_set_meshmap_property(argument_pointers, &result, 4));
    assert(map->config.flags == MESHMAP_FLAGS_NONE);
    named_constant(3, "MESHMAP_FLAGS_DEFAULT");
    expect(1, openbor_set_meshmap_property(argument_pointers, &result, 4));
    assert(map->config.flags == MESHMAP_FLAGS_DEFAULT);
}

/* Indexed activation is the ordinary script interface: slot numbers are
 * addresses, while enabled remains a Boolean. Its result describes this
 * bank's immediate child, independently of any disabled ancestor. */
static void test_indexed_bank_properties(void)
{
    s_meshmap_store *store = level->meshmaps;
    const uint64_t present = store->root.present_mask;
    bank_arguments(0, 0, MESHMAP_BANK_PROPERTY_CHILD_PRESENT);
    integer(4, 63);
    expect(1, openbor_get_meshmap_bank_property(argument_pointers, &result, 5));
    assert(returned.vt == VT_INTEGER && returned.lVal == 1);
    integer(4, 5);
    expect(1, openbor_get_meshmap_bank_property(argument_pointers, &result, 5));
    assert(returned.lVal == 0);
    integer(3, MESHMAP_BANK_PROPERTY_CHILD_ACTIVE);
    expect(1, openbor_get_meshmap_bank_property(argument_pointers, &result, 5));
    assert(returned.lVal == 0);
    integer(4, 63);
    expect(1, openbor_get_meshmap_bank_property(argument_pointers, &result, 5));
    assert(returned.lVal == 1);

    /* Wrong arity, invalid indices, non-Boolean values and absent children
     * must fail before changing the bank's previously active selection. */
    expect(0, openbor_get_meshmap_bank_property(argument_pointers, &result, 4));
    expect(0, openbor_get_meshmap_bank_property(argument_pointers, &result, 6));
    integer(4, -1);
    expect(0, openbor_get_meshmap_bank_property(argument_pointers, &result, 5));
    integer(4, 64);
    expect(0, openbor_get_meshmap_bank_property(argument_pointers, &result, 5));
    decimal(4, 0.5);
    expect(0, openbor_get_meshmap_bank_property(argument_pointers, &result, 5));
    integer(3, MESHMAP_BANK_PROPERTY_DEPTH); integer(4, 0);
    expect(0, openbor_get_meshmap_bank_property(argument_pointers, &result, 5));
    integer(3, MESHMAP_BANK_PROPERTY_CHILD_ACTIVE);
    integer(4, 0); integer(5, 63);
    expect(0, openbor_set_meshmap_bank_property(argument_pointers, &result, 5));
    expect(0, openbor_set_meshmap_bank_property(argument_pointers, &result, 7));
    integer(4, 2);
    expect(0, openbor_set_meshmap_bank_property(argument_pointers, &result, 6));
    decimal(4, 0.0);
    expect(0, openbor_set_meshmap_bank_property(argument_pointers, &result, 6));
    integer(4, 1); integer(5, 5);
    expect(0, openbor_set_meshmap_bank_property(argument_pointers, &result, 6));
    integer(5, 64);
    expect(0, openbor_set_meshmap_bank_property(argument_pointers, &result, 6));
    integer(5, -1);
    expect(0, openbor_set_meshmap_bank_property(argument_pointers, &result, 6));
    decimal(5, 63.0);
    expect(0, openbor_set_meshmap_bank_property(argument_pointers, &result, 6));
    integer(3, MESHMAP_BANK_PROPERTY_CHILD_PRESENT); integer(5, 63);
    expect(0, openbor_set_meshmap_bank_property(argument_pointers, &result, 6));
    integer(3, MESHMAP_BANK_PROPERTY_ACTIVE_MASK);
    named_constant(4, "MESHMAP_BANK_MASK_NONE");
    expect(0, openbor_set_meshmap_bank_property(argument_pointers, &result, 6));
    assert(store->root.active_mask == present && store->root.present_mask == present);

    bank_arguments(0, 0, MESHMAP_BANK_PROPERTY_CHILD_ACTIVE);
    integer(4, 0); integer(5, 63);
    expect(1, openbor_set_meshmap_bank_property(argument_pointers, &result, 6));
    assert(!meshmap_is_active(store, MESHMAP_ID_MAX) && meshmap_is_active(store, 0));
    bank_arguments(3, MESHMAP_ID_MAX, MESHMAP_BANK_PROPERTY_CHILD_ACTIVE);
    integer(4, 1); integer(5, 63);
    expect(1, openbor_set_meshmap_bank_property(argument_pointers, &result, 6));
    integer(4, 63);
    expect(1, openbor_get_meshmap_bank_property(argument_pointers, &result, 5));
    assert(returned.lVal == 1);
    map_arguments(MESHMAP_ID_MAX, MESHMAP_PROPERTY_ENABLED);
    expect(1, openbor_get_meshmap_property(argument_pointers, &result, 3));
    assert(returned.lVal == 0);
    bank_arguments(0, 0, MESHMAP_BANK_PROPERTY_CHILD_ACTIVE);
    integer(4, 1); integer(5, 63);
    expect(1, openbor_set_meshmap_bank_property(argument_pointers, &result, 6));
    assert(meshmap_is_active(store, MESHMAP_ID_MAX));

    /* Whole-mask snapshots remain opaque exact values. NONE clears them, and
     * the returned PRESENT_MASK can reactivate all existing children without
     * requiring the creator to calculate any bit pattern. */
    bank_arguments(0, 0, MESHMAP_BANK_PROPERTY_ACTIVE_MASK);
    named_constant(4, "MESHMAP_BANK_MASK_NONE");
    expect(1, openbor_set_meshmap_bank_property(argument_pointers, &result, 5));
    assert(!store->root.active_mask && store->root.present_mask == present);
    integer(3, MESHMAP_BANK_PROPERTY_PRESENT_MASK);
    expect(1, openbor_get_meshmap_bank_property(argument_pointers, &result, 4));
    ScriptVariant_Copy(&arguments[4], &returned);
    integer(3, MESHMAP_BANK_PROPERTY_ACTIVE_MASK);
    expect(1, openbor_set_meshmap_bank_property(argument_pointers, &result, 5));
    assert(store->root.active_mask == present);
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
    s_meshmap_authoring_context context = {0};
    char geometry[1024], artwork[1024];
    const char *select[] = {"meshmap_index", "01", "02", "03", "08"};
    const char *create[] = {"meshmap_geometry", "z", "10", "20", "30", "-40", "60", geometry};
    const char *texture[] = {"meshmap_texture", artwork};
    const char *area[] = {"meshmap_area", "-100", "-200", "-300", "400", "500", "600", "1"};
    const char *spacing[] = {"meshmap_spacing", "4", "8"};
    const char *flags[] = {"meshmap_flags", "all"};
    const char *rules[] = {"meshmap_rules", "1", "4294967295", "60", "-10"};
    const char *mask[] = {"meshmap_mask", "0", "slots", "63", "0", "1"};
    s_meshmap *map;
    s_meshmap_bank *bank;
    uint32_t id;
    uint64_t generation;
    size_t old_count;
    snprintf(geometry, sizeof(geometry), "%s/geometry.png", directory);
    snprintf(artwork, sizeof(artwork), "%s/rgb.png", directory);
    assert(meshmap_pack_id(1, 2, 3, 8, &id));
    expect_native(&context, 1, CMD_LEVEL_MESHMAP_INDEX, 5, select);
    assert(context.id == id && !meshmap_get(level->meshmaps, id));
    expect_native(&context, 1, CMD_LEVEL_MESHMAP_AREA, 8, area);
    assert(context.id == id && !meshmap_get(level->meshmaps, id));

    /* Settings may precede image loading. Their first successful assignment
     * creates a default leaf; subsequent declarations preserve those settings. */
    expect_native(&context, 1, CMD_LEVEL_MESHMAP_SPACING, 3, spacing);
    map = meshmap_get(level->meshmaps, id);
    assert(map && !map->samples && map->config.flags == MESHMAP_FLAGS_DEFAULT);
    generation = map->generation;
    expect_native(&context, 1, CMD_LEVEL_MESHMAP_FLAGS, 2, flags);
    expect_native(&context, 1, CMD_LEVEL_MESHMAP_RULES, 5, rules);
    expect_native(&context, 1, CMD_LEVEL_MESHMAP_ACTIVE, 2,
        (const char *const[]){"meshmap_active", "0"});
    expect_native(&context, 1, CMD_LEVEL_MESHMAP_GEOMETRY, 8, create);
    expect_native(&context, 1, CMD_LEVEL_MESHMAP_TEXTURE, 2, texture);
    assert(map->artwork && !strcmp(map->artwork_path, artwork));
    assert(meshmap_get(level->meshmaps, id) == map && map->generation == generation);
    assert(map->config.axis == MESHMAP_AXIS_Z && map->config.spacing_u == 4 && map->config.spacing_v == 8);
    assert(map->config.flags == MESHMAP_FLAGS_ALL && map->config.side == MESHMAP_SIDE_POSITIVE
           && map->config.support_group == MESHMAP_GROUP_NONE
           && map->config.slope_limit == 60 && map->config.layer == -10);
    assert(!meshmap_is_active(level->meshmaps, id));
    bank = meshmap_bank_get(level->meshmaps, 1, id, 0);
    assert(bank && bank->gate.mode == MESHMAP_GATE_BODY && bank->gate.bounds.min.y == -200.0);
    expect_native(&context, 1, CMD_LEVEL_MESHMAP_MASK, 6, mask);
    assert(level->meshmaps->root.active_mask == UINT64_C(0x8000000000000003));
    old_count = level->meshmaps->map_count;
    expect_native(&context, 1, CMD_LEVEL_MESHMAP_GEOMETRY, 8, create);
    assert(level->meshmaps->map_count == old_count && meshmap_get(level->meshmaps, id) == map);
    assert(map->generation == generation && !meshmap_is_active(level->meshmaps, id));
    expect_native(&context, 0, CMD_LEVEL_MESHMAP_AREA, 7,
        (const char *const[]){"meshmap_area", "2", "0", "0", "1", "1", "1"});
    expect_native(&context, 0, CMD_LEVEL_MESHMAP_SPACING, 3,
        (const char *const[]){"meshmap_spacing", "nan", "1"});
    assert(map->config.spacing_u == 4 && context.id == id);
    bank_arguments(1, id, MESHMAP_BANK_PROPERTY_EXISTS);
    integer(4, 0);
    expect(1, openbor_set_meshmap_bank_property(argument_pointers, &result, 5));
    assert(!meshmap_get(level->meshmaps, id) && !meshmap_bank_get(level->meshmaps, 1, id, 0));
    assert(level->meshmaps->map_count == old_count - 1 && !meshmap_store_uses_body_gates(level->meshmaps));
}

/* Native authoring replaces complete selections only after every name has
 * been validated. Keep these independent of PNG loading so parser failures
 * cannot be mistaken for an image or filesystem failure. */
static void test_native_symbolic_flags_and_rules(void)
{
    s_meshmap_authoring_context context = {0};
    s_meshmap *map = meshmap_get(level->meshmaps, 0);
    const char *flags[] = {"meshmap_flags", "CoLlIdE", "DRAW", "walkable", "one_way", "drop_through"};
    const char *invalid_flags[] = {"0", "7", "0x1f", "unknown", "collide|draw", "collide,draw"};
    const char *invalid_sides[] = {"-1", "2", "7", "0x1f", "unknown"};
    unsigned char saved[sizeof(map->config)];
    uint64_t revision;
    size_t i;
    assert(map);
    expect_native(&context, 1, CMD_LEVEL_MESHMAP_FLAGS, 6, flags);
    assert(map->config.flags == MESHMAP_FLAGS_ALL);
    expect_native(&context, 1, CMD_LEVEL_MESHMAP_FLAGS, 2,
        (const char *const[]){"meshmap_flags", "NONE"});
    assert(map->config.flags == MESHMAP_FLAGS_NONE);
    expect_native(&context, 1, CMD_LEVEL_MESHMAP_FLAGS, 2,
        (const char *const[]){"meshmap_flags", "DeFaUlT"});
    assert(map->config.flags == MESHMAP_FLAGS_DEFAULT);
    expect_native(&context, 1, CMD_LEVEL_MESHMAP_FLAGS, 2,
        (const char *const[]){"meshmap_flags", "ALL"});
    assert(map->config.flags == MESHMAP_FLAGS_ALL);

    /* NONE contributes zero, rather than acting as a reset in the middle of
     * a command. Repeated names are idempotent, and the previous ALL value
     * must not leak into this new two-flag selection. */
    expect_native(&context, 1, CMD_LEVEL_MESHMAP_FLAGS, 5,
        (const char *const[]){"meshmap_flags", "collide", "none", "walkable", "collide"});
    assert(map->config.flags == (MESHMAP_COLLIDE | MESHMAP_WALKABLE));
    memcpy(saved, &map->config, sizeof(saved));
    revision = map->revision;
    for(i = 0; i < sizeof(invalid_flags) / sizeof(invalid_flags[0]); ++i)
    {
        /* Place the invalid name after a valid name to expose partial writes. */
        expect_native(&context, 0, CMD_LEVEL_MESHMAP_FLAGS, 3,
            (const char *const[]){"meshmap_flags", "draw", invalid_flags[i]});
        assert(!memcmp(saved, &map->config, sizeof(saved)) && map->revision == revision);
    }
    expect_native(&context, 0, CMD_LEVEL_MESHMAP_FLAGS, 1,
        (const char *const[]){"meshmap_flags"});
    assert(!memcmp(saved, &map->config, sizeof(saved)) && map->revision == revision);

    /* Side, support group, slope and layer are scalar values, not masks.
     * They stay numeric when behavioral flags move into their own command. */
    expect_native(&context, 1, CMD_LEVEL_MESHMAP_RULES, 5,
        (const char *const[]){"meshmap_rules", "1", "42", "60", "-10"});
    assert(map->config.side == MESHMAP_SIDE_POSITIVE && map->config.support_group == 42
           && map->config.slope_limit == 60 && map->config.layer == -10);
    assert(map->config.flags == (MESHMAP_COLLIDE | MESHMAP_WALKABLE));
    expect_native(&context, 1, CMD_LEVEL_MESHMAP_RULES, 2,
        (const char *const[]){"meshmap_rules", "0"});
    assert(map->config.side == MESHMAP_SIDE_NEGATIVE && map->config.support_group == 42
           && map->config.slope_limit == 60 && map->config.layer == -10);
    expect_native(&context, 1, CMD_LEVEL_MESHMAP_RULES, 3,
        (const char *const[]){"meshmap_rules", "1", "4294967295"});
    assert(map->config.support_group == MESHMAP_GROUP_NONE);
    memcpy(saved, &map->config, sizeof(saved));
    revision = map->revision;
    for(i = 0; i < sizeof(invalid_sides) / sizeof(invalid_sides[0]); ++i)
    {
        expect_native(&context, 0, CMD_LEVEL_MESHMAP_RULES, 3,
            (const char *const[]){"meshmap_rules", invalid_sides[i], "1"});
        assert(!memcmp(saved, &map->config, sizeof(saved)) && map->revision == revision);
    }
    expect_native(&context, 0, CMD_LEVEL_MESHMAP_RULES, 6,
        (const char *const[]){"meshmap_rules", "0", "1", "30", "5", "9"});
    assert(!memcmp(saved, &map->config, sizeof(saved)) && map->revision == revision);
    expect_native(&context, 0, CMD_LEVEL_MESHMAP_RULES, 5,
        (const char *const[]){"meshmap_rules", "0", "13", "nan", "5"});
    assert(!memcmp(saved, &map->config, sizeof(saved)) && map->revision == revision);
    expect_native(&context, 0, CMD_LEVEL_MESHMAP_RULES, 5,
        (const char *const[]){"meshmap_rules", "0", "13", "30", "bad-layer"});
    assert(!memcmp(saved, &map->config, sizeof(saved)) && map->revision == revision);
}

static void test_native_symbolic_bank_masks(void)
{
    s_meshmap_authoring_context context = {0};
    s_level scratch_stage = {0};
    s_level *previous_level = level;
    s_meshmap_bank *root;
    const uint64_t sparse = (UINT64_C(1) << 63) | 1;
    const char *invalid_slots[] = {"1", "64", "-1", "63junk", "0.5", "slot_63", "all", "none", "unknown"};
    const char *invalid_selectors[] = {"0", "3", "0x8000000000000001", "slot_63", "unknown"};
    const char *many[3 + MESHMAP_BANK_SLOTS];
    char indices[MESHMAP_BANK_SLOTS][4];
    uint32_t id;
    size_t i;

    /* Use a separate collection so a bank containing all 64 children cannot
     * hide the sparse-bank ALL case or affect later entity/renderer tests. */
    level = &scratch_stage;
    assert((level->meshmaps = meshmap_store_create()));
    assert(meshmap_create(level->meshmaps, 0));
    assert(meshmap_create(level->meshmaps, MESHMAP_ID_MAX));
    root = &level->meshmaps->root;
    assert(root->present_mask == sparse && root->active_mask == sparse);
    expect_native(&context, 1, CMD_LEVEL_MESHMAP_MASK, 3,
        (const char *const[]){"meshmap_mask", "0", "NoNe"});
    assert(!root->active_mask && root->present_mask == sparse);
    expect_native(&context, 1, CMD_LEVEL_MESHMAP_MASK, 3,
        (const char *const[]){"meshmap_mask", "0", "ALL"});
    assert(root->active_mask == sparse);
    expect_native(&context, 1, CMD_LEVEL_MESHMAP_MASK, 5,
        (const char *const[]){"meshmap_mask", "0", "SlOtS", "63", "63"});
    assert(root->active_mask == (UINT64_C(1) << 63));
    for(i = 0; i < sizeof(invalid_slots) / sizeof(invalid_slots[0]); ++i)
    {
        /* Index 1 is valid but has no child. No failure may first enable
         * index 0 or erase the previously active high slot. */
        expect_native(&context, 0, CMD_LEVEL_MESHMAP_MASK, 5,
            (const char *const[]){"meshmap_mask", "0", "slots", "0", invalid_slots[i]});
        assert(root->active_mask == (UINT64_C(1) << 63) && root->present_mask == sparse);
    }
    for(i = 0; i < sizeof(invalid_selectors) / sizeof(invalid_selectors[0]); ++i)
    {
        /* The slots selector prevents an old packed numeric mask from being
         * silently interpreted as a different child selection. */
        expect_native(&context, 0, CMD_LEVEL_MESHMAP_MASK, 3,
            (const char *const[]){"meshmap_mask", "0", invalid_selectors[i]});
        assert(root->active_mask == (UINT64_C(1) << 63) && root->present_mask == sparse);
    }
    expect_native(&context, 0, CMD_LEVEL_MESHMAP_MASK, 2,
        (const char *const[]){"meshmap_mask", "0"});
    expect_native(&context, 0, CMD_LEVEL_MESHMAP_MASK, 3,
        (const char *const[]){"meshmap_mask", "0", "slots"});
    expect_native(&context, 0, CMD_LEVEL_MESHMAP_MASK, 4,
        (const char *const[]){"meshmap_mask", "0", "all", "0"});
    expect_native(&context, 0, CMD_LEVEL_MESHMAP_MASK, 4,
        (const char *const[]){"meshmap_mask", "0", "none", "63"});
    expect_native(&context, 0, CMD_LEVEL_MESHMAP_MASK, 4,
        (const char *const[]){"meshmap_mask", "0", "none", "all"});
    assert(root->active_mask == (UINT64_C(1) << 63));

    /* The full 64-slot selection must reach the parser. Its count is now 67:
     * command, depth, slots selector, then the ordinary numeric indices. */
    many[0] = "meshmap_mask"; many[1] = "0"; many[2] = "slots";
    for(i = 0; i < MESHMAP_BANK_SLOTS; ++i)
    {
        if(i && i != 63)
        {
            assert(meshmap_pack_id((unsigned)i, 0, 0, 0, &id));
            assert(meshmap_create(level->meshmaps, id));
        }
        snprintf(indices[i], sizeof(indices[i]), "%u", (unsigned)i);
        many[3 + i] = indices[i];
    }
    expect_native(&context, 1, CMD_LEVEL_MESHMAP_MASK, 3,
        (const char *const[]){"meshmap_mask", "0", "none"});
    expect_native(&context, 1, CMD_LEVEL_MESHMAP_MASK, 3 + MESHMAP_BANK_SLOTS, many);
    assert(root->active_mask == UINT64_MAX && root->present_mask == UINT64_MAX);
    meshmap_store_free(level->meshmaps);
    level = previous_level;
}

/* Selection belongs to an explicit level-loading context, never to a global
 * runtime target. Interleave contexts, visit ancestors, and start another
 * level so a hidden static selector cannot accidentally satisfy these checks. */
static void test_native_authoring_contexts(void)
{
    s_level first_stage = {0}, next_stage = {0};
    s_level *previous_level = level;
    s_meshmap_authoring_context first = {0}, second = {0}, fresh = {0};
    const char *selection[] = {"meshmap_index", "1", "2", "3", "4"};
    const char *invalid[] = {"-1", "64", "bad", "1.5"};
    s_meshmap *selected, *zero;
    uint32_t first_id, ancestor_id;
    size_t i, slot;
    level = &first_stage;
    assert(meshmap_pack_id(1, 2, 3, 4, &first_id));
    expect_native(&first, 1, CMD_LEVEL_MESHMAP_INDEX, 5, selection);
    assert(first.id == first_id && !level->meshmaps);
    for(slot = 1; slot < 5; ++slot)
    {
        const char *saved = selection[slot];
        for(i = 0; i < sizeof(invalid) / sizeof(invalid[0]); ++i)
        {
            selection[slot] = invalid[i];
            expect_native(&first, 0, CMD_LEVEL_MESHMAP_INDEX, 5, selection);
            assert(first.id == first_id && !level->meshmaps);
        }
        selection[slot] = saved;
    }
    expect_native(&first, 0, CMD_LEVEL_MESHMAP_INDEX, 4, selection);
    expect_native(&first, 0, CMD_LEVEL_MESHMAP_INDEX, 6,
        (const char *const[]){"meshmap_index", "1", "2", "3", "4", "5"});
    assert(first.id == first_id && !level->meshmaps);
    expect_native(&first, 1, CMD_LEVEL_MESHMAP_INDEX, 5,
        (const char *const[]){"meshmap_index", "63", "63", "63", "63"});
    assert(first.id == MESHMAP_ID_MAX && !level->meshmaps);
    expect_native(&first, 1, CMD_LEVEL_MESHMAP_INDEX, 5, selection);

    /* Invalid settings must not create a default leaf as a side effect. */
    expect_native(&first, 0, CMD_LEVEL_MESHMAP_SPACING, 3,
        (const char *const[]){"meshmap_spacing", "0", "1"});
    expect_native(&first, 0, CMD_LEVEL_MESHMAP_FLAGS, 2,
        (const char *const[]){"meshmap_flags", "typo"});
    expect_native(&first, 0, CMD_LEVEL_MESHMAP_RULES, 2,
        (const char *const[]){"meshmap_rules", "2"});
    expect_native(&first, 0, CMD_LEVEL_MESHMAP_ACTIVE, 2,
        (const char *const[]){"meshmap_active", "2"});
    expect_native(&first, 0, CMD_LEVEL_MESHMAP_BOUNDS, 7,
        (const char *const[]){"meshmap_bounds", "2", "0", "0", "1", "1", "1"});
    assert(first.id == first_id && (!level->meshmaps || !level->meshmaps->map_count));

    expect_native(&first, 1, CMD_LEVEL_MESHMAP_RULES, 5,
        (const char *const[]){"meshmap_rules", "1", "42", "35", "12"});
    selected = meshmap_get(level->meshmaps, first_id);
    assert(selected && !selected->samples && selected->config.flags == MESHMAP_FLAGS_DEFAULT);
    expect_native(&second, 1, CMD_LEVEL_MESHMAP_FLAGS, 2,
        (const char *const[]){"meshmap_flags", "draw"});
    zero = meshmap_get(level->meshmaps, 0);
    assert(zero && zero != selected && zero->config.flags == MESHMAP_DRAW);
    assert(first.id == first_id && second.id == 0 && level->meshmaps->map_count == 2);
    expect_native(&first, 1, CMD_LEVEL_MESHMAP_INDEX, 5,
        (const char *const[]){"meshmap_index", "0", "0", "0", "0"});
    expect_native(&first, 1, CMD_LEVEL_MESHMAP_SPACING, 3,
        (const char *const[]){"meshmap_spacing", "8", "16"});
    assert(zero->config.spacing_u == 8 && zero->config.flags == MESHMAP_DRAW);
    expect_native(&first, 1, CMD_LEVEL_MESHMAP_INDEX, 5, selection);
    expect_native(&first, 1, CMD_LEVEL_MESHMAP_FLAGS, 2,
        (const char *const[]){"meshmap_flags", "walkable"});
    assert(selected->config.spacing_u == 1 && selected->config.side == MESHMAP_SIDE_POSITIVE
           && selected->config.support_group == 42 && selected->config.layer == 12);

    /* Runtime script addressing stays explicit and does not consult or alter
     * either authoring context, even when their selected indices differ. */
    map_arguments(0, MESHMAP_PROPERTY_LAYER);
    integer(3, 77);
    expect(1, openbor_set_meshmap_property(argument_pointers, &result, 4));
    assert(zero->config.layer == 77 && selected->config.layer == 12);
    assert(first.id == first_id && second.id == 0);

    assert(meshmap_pack_id(2, 3, 4, 5, &ancestor_id));
    expect_native(&first, 1, CMD_LEVEL_MESHMAP_INDEX, 5,
        (const char *const[]){"meshmap_index", "2", "3", "4", "5"});
    expect_native(&first, 1, CMD_LEVEL_MESHMAP_AREA, 7,
        (const char *const[]){"meshmap_area", "-100", "-10", "-10", "100", "10", "10"});
    expect_native(&first, 1, CMD_LEVEL_MESHMAP_SECTOR, 7,
        (const char *const[]){"meshmap_sector", "-200", "-10", "-10", "200", "10", "10"});
    expect_native(&first, 1, CMD_LEVEL_MESHMAP_ROOM, 7,
        (const char *const[]){"meshmap_room", "-300", "-10", "-10", "300", "10", "10"});
    expect_native(&first, 1, CMD_LEVEL_MESHMAP_BANK, 8,
        (const char *const[]){"meshmap_bank", "2", "-250", "-10", "-10", "250", "10", "10"});
    expect_native(&first, 1, CMD_LEVEL_MESHMAP_BANK, 8,
        (const char *const[]){"meshmap_bank", "0", "-400", "-10", "-10", "400", "10", "10"});
    assert(first.id == ancestor_id && !meshmap_get(level->meshmaps, ancestor_id));
    assert(meshmap_bank_get(level->meshmaps, 1, ancestor_id, 0)->gate.bounds.min.x == -100);
    assert(meshmap_bank_get(level->meshmaps, 2, ancestor_id, 0)->gate.bounds.min.x == -250);
    assert(meshmap_bank_get(level->meshmaps, 3, ancestor_id, 0)->gate.bounds.min.x == -300);
    assert(level->meshmaps->root.gate.bounds.min.x == -400);
    expect_native(&first, 1, CMD_LEVEL_MESHMAP_ACTIVE, 2,
        (const char *const[]){"meshmap_active", "1"});
    expect_native(&first, 1, CMD_LEVEL_MESHMAP_MASK, 4,
        (const char *const[]){"meshmap_mask", "1", "slots", "3"});
    expect_native(&first, 1, CMD_LEVEL_MESHMAP_MASK, 4,
        (const char *const[]){"meshmap_mask", "2", "slots", "4"});
    expect_native(&first, 1, CMD_LEVEL_MESHMAP_MASK, 3,
        (const char *const[]){"meshmap_mask", "3", "none"});
    expect_native(&first, 1, CMD_LEVEL_MESHMAP_FLAGS, 2,
        (const char *const[]){"meshmap_flags", "collide"});
    assert(!meshmap_is_active(level->meshmaps, ancestor_id));
    expect_native(&first, 1, CMD_LEVEL_MESHMAP_MASK, 3,
        (const char *const[]){"meshmap_mask", "3", "all"});
    assert(first.id == ancestor_id && meshmap_is_active(level->meshmaps, ancestor_id));

    /* A new loader context always starts at zero, regardless of where the
     * previous level stopped. Existing contexts remain untouched. */
    level = &next_stage;
    expect_native(&fresh, 1, CMD_LEVEL_MESHMAP_FLAGS, 2,
        (const char *const[]){"meshmap_flags", "collide"});
    assert(fresh.id == 0 && level->meshmaps->map_count == 1);
    assert(meshmap_get(level->meshmaps, 0) && !meshmap_get(level->meshmaps, ancestor_id));
    assert(first.id == ancestor_id && second.id == 0);
    meshmap_store_free(next_stage.meshmaps);
    meshmap_store_free(first_stage.meshmaps);
    level = previous_level;
}

/* Failed combined definitions must preserve both the map record and the data
 * behind its sample pointer. Track every ancestor's selection as well: keeping
 * a record alive is insufficient if a failed load accidentally reactivates it. */
static void expect_resource_unchanged(s_meshmap_authoring_context *context,
                                      s_meshmap *map, int command, size_t count,
                                      const char *const *tokens)
{
    s_meshmap before;
    uint16_t samples[9];
    uint64_t active[4];
    size_t maps = level->meshmaps->map_count;
    unsigned depth;
    assert(map->width == 3 && map->height == 3);
    memcpy(&before, map, sizeof(before));
    memcpy(samples, map->samples, sizeof(samples));
    for(depth = 0; depth < 4; ++depth)
        active[depth] = meshmap_bank_get(level->meshmaps, depth, map->id, 0)->active_mask;
    expect_native(context, 0, command, count, tokens);
    assert(context->id == before.id && meshmap_get(level->meshmaps, before.id) == map);
    assert(!memcmp(&before, map, sizeof(before)) && !memcmp(samples, map->samples, sizeof(samples)));
    assert(level->meshmaps->map_count == maps);
    for(depth = 0; depth < 4; ++depth)
        assert(meshmap_bank_get(level->meshmaps, depth, map->id, 0)->active_mask == active[depth]);
}

static void test_native_definition_transactions(const char *directory)
{
    s_level scratch_stage = {0};
    s_level *previous_level = level;
    s_meshmap_authoring_context context = {0};
    char geometry[1024], indexed[1024], rgb[1024], missing[1024];
    const char *axes[] = {"x", "y", "z"};
    s_meshmap *map, *other;
    s_meshmap_gate original_gate;
    void *original_artwork;
    char *original_artwork_path;
    uint16_t *original_samples;
    uint8_t *original_cells;
    char *original_geometry_path;
    s_meshmap_config original_config;
    uint64_t generation, revision;
    size_t count;
    unsigned axis, releases;
    snprintf(geometry, sizeof(geometry), "%s/geometry.png", directory);
    snprintf(indexed, sizeof(indexed), "%s/indexed.png", directory);
    snprintf(rgb, sizeof(rgb), "%s/rgb.png", directory);
    snprintf(missing, sizeof(missing), "%s/does-not-exist.png", directory);
    level = &scratch_stage;
    expect_native(&context, 1, CMD_LEVEL_MESHMAP_INDEX, 5,
        (const char *const[]){"meshmap_index", "63", "63", "63", "63"});
    assert(context.id == MESHMAP_ID_MAX && !level->meshmaps);
    expect_native(&context, 1, CMD_LEVEL_MESHMAP_BOUNDS, 8,
        (const char *const[]){"meshmap_bounds", "-10", "-20", "-30", "100", "200", "300", "1"});
    map = meshmap_get(level->meshmaps, MESHMAP_ID_MAX);
    assert(map && !map->samples && map->config.flags == MESHMAP_FLAGS_DEFAULT);
    memcpy(&original_gate, &map->gate, sizeof(original_gate));
    expect_native(&context, 1, CMD_LEVEL_MESHMAP_SPACING, 3,
        (const char *const[]){"meshmap_spacing", "4", "8"});
    expect_native(&context, 1, CMD_LEVEL_MESHMAP_FLAGS, 2,
        (const char *const[]){"meshmap_flags", "all"});
    expect_native(&context, 1, CMD_LEVEL_MESHMAP_RULES, 5,
        (const char *const[]){"meshmap_rules", "1", "7", "30", "-12"});
    expect_native(&context, 1, CMD_LEVEL_MESHMAP_ACTIVE, 2,
        (const char *const[]){"meshmap_active", "0"});
    expect_native(&context, 1, CMD_LEVEL_MESHMAP_MASK, 3,
        (const char *const[]){"meshmap_mask", "0", "none"});
    generation = map->generation;
    expect_native(&context, 1, CMD_LEVEL_MESHMAP_TEXTURE, 2,
        (const char *const[]){"meshmap_texture", indexed});
    assert(map->artwork && !map->samples && !strcmp(map->artwork_path, indexed));
    revision = map->revision;
    expect_native(&context, 1, CMD_LEVEL_MESHMAP_GEOMETRY, 8,
        (const char *const[]){"meshmap_geometry", "y", "10", "20", "30", "-5", "15", geometry});
    assert(meshmap_get(level->meshmaps, MESHMAP_ID_MAX) == map && map->generation == generation);
    assert(map->revision == revision + 1 && map->width == 3 && map->height == 3);
    assert(map->config.spacing_u == 4 && map->config.spacing_v == 8
           && map->config.flags == MESHMAP_FLAGS_ALL && map->config.side == MESHMAP_SIDE_POSITIVE
           && map->config.support_group == 7 && map->config.slope_limit == 30 && map->config.layer == -12);
    assert(!memcmp(&original_gate, &map->gate, sizeof(original_gate))
           && meshmap_store_uses_body_gates(level->meshmaps));
    assert(!level->meshmaps->root.active_mask && !meshmap_is_active(level->meshmaps, map->id));
    original_artwork = map->artwork;
    original_artwork_path = map->artwork_path;
    assert(original_artwork && !strcmp(original_artwork_path, indexed));

    /* Every orientation works at the highest packed address. Replacing a
     * geometry changes its explicit fields, while texture, prior
     * settings, gates, generation and activation remain attached to the map. */
    for(axis = 0; axis < 3; ++axis)
    {
        revision = map->revision;
        expect_native(&context, 1, CMD_LEVEL_MESHMAP_GEOMETRY, 8,
            (const char *const[]){"meshmap_geometry", axes[axis], "11", "22", "33", "-6", "16", geometry});
        assert(map->config.axis == (e_meshmap_axis)axis && map->revision == revision + 1);
        assert(map->config.origin.x == 11 && map->config.origin.y == 22 && map->config.origin.z == 33
               && map->config.min_height == -6 && map->config.max_height == 16);
        assert(map->artwork == original_artwork && map->artwork_path == original_artwork_path);
        assert(map->generation == generation && map->config.spacing_u == 4
               && map->config.flags == MESHMAP_FLAGS_ALL && map->config.side == MESHMAP_SIDE_POSITIVE);
        assert(!memcmp(&original_gate, &map->gate, sizeof(original_gate))
               && !level->meshmaps->root.active_mask);
    }

    expect_native(&context, 1, CMD_LEVEL_MESHMAP_INDEX, 5,
        (const char *const[]){"meshmap_index", "0", "0", "0", "0"});
    expect_native(&context, 1, CMD_LEVEL_MESHMAP_GEOMETRY, 8,
        (const char *const[]){"meshmap_geometry", "z", "0", "0", "0", "0", "1", geometry});
    other = meshmap_get(level->meshmaps, 0);
    assert(other && other != map && !other->artwork && other->config.flags == MESHMAP_FLAGS_DEFAULT);
    assert(other->config.spacing_u == 1 && other->config.side == MESHMAP_SIDE_NEGATIVE
           && !other->gate.enabled && meshmap_is_active(level->meshmaps, 0));
    revision = map->revision;
    expect_native(&context, 1, CMD_LEVEL_MESHMAP_INDEX, 5,
        (const char *const[]){"meshmap_index", "63", "63", "63", "63"});
    assert(map->revision == revision && map->artwork == original_artwork && map->generation == generation);

    /* Validate the two resources independently. Malformed declarations
     * and failed loads must preserve existing resources and settings. */
    expect_resource_unchanged(&context, map, CMD_LEVEL_MESHMAP_GEOMETRY, 8,
        (const char *const[]){"meshmap_geometry", "x", "90", "80", "70", "0", "50", rgb});
    expect_resource_unchanged(&context, map, CMD_LEVEL_MESHMAP_GEOMETRY, 9,
        (const char *const[]){"meshmap_geometry", "x", "90", "80", "70", "0", "50", geometry, indexed});
    expect_resource_unchanged(&context, map, CMD_LEVEL_MESHMAP_GEOMETRY, 8,
        (const char *const[]){"meshmap_geometry", "x", "90", "80", "70", "50", "0", geometry});
    expect_resource_unchanged(&context, map, CMD_LEVEL_MESHMAP_TEXTURE, 2,
        (const char *const[]){"meshmap_texture", geometry});
    expect_resource_unchanged(&context, map, CMD_LEVEL_MESHMAP_TEXTURE, 2,
        (const char *const[]){"meshmap_texture", missing});
    expect_resource_unchanged(&context, map, CMD_LEVEL_MESHMAP_TEXTURE, 1,
        (const char *const[]){"meshmap_texture"});
    expect_resource_unchanged(&context, map, CMD_LEVEL_MESHMAP_TEXTURE, 3,
        (const char *const[]){"meshmap_texture", indexed, rgb});

    original_samples = map->samples;
    original_cells = map->cells;
    original_geometry_path = map->geometry_path;
    memcpy(&original_config, &map->config, sizeof(original_config));
    releases = released_screens;
    revision = map->revision;
    expect_native(&context, 1, CMD_LEVEL_MESHMAP_TEXTURE, 2,
        (const char *const[]){"meshmap_texture", rgb});
    assert(map->artwork && !strcmp(map->artwork_path, rgb));
    assert(released_screens == releases + 1 && map->revision == revision + 1);
    assert(map->samples == original_samples && map->cells == original_cells
           && map->geometry_path == original_geometry_path
           && !memcmp(&original_config, &map->config, sizeof(original_config)));
    releases = released_screens;
    revision = map->revision;
    expect_native(&context, 1, CMD_LEVEL_MESHMAP_TEXTURE, 2,
        (const char *const[]){"meshmap_texture", "NoNe"});
    assert(!map->artwork && (!map->artwork_path || !map->artwork_path[0]));
    assert(released_screens == releases + 1 && map->generation == generation
           && map->revision == revision + 1);
    assert(map->samples == original_samples && map->cells == original_cells
           && map->geometry_path == original_geometry_path
           && !memcmp(&original_config, &map->config, sizeof(original_config)));
    assert(!memcmp(&original_gate, &map->gate, sizeof(original_gate))
           && !meshmap_is_active(level->meshmaps, MESHMAP_ID_MAX));

    expect_native(&context, 1, CMD_LEVEL_MESHMAP_INDEX, 5,
        (const char *const[]){"meshmap_index", "0", "0", "0", "1"});
    count = level->meshmaps->map_count;
    expect_native(&context, 0, CMD_LEVEL_MESHMAP_TEXTURE, 2,
        (const char *const[]){"meshmap_texture", missing});
    assert(context.id == 1 && !meshmap_get(level->meshmaps, 1) && level->meshmaps->map_count == count);
    expect_native(&context, 0, CMD_LEVEL_MESHMAP_GEOMETRY, 8,
        (const char *const[]){"meshmap_geometry", "y", "0", "0", "0", "0", "1", missing});
    assert(context.id == 1 && !meshmap_get(level->meshmaps, 1) && level->meshmaps->map_count == count);

    /* Texture alone is sufficient to create a map. Later geometry
     * assignment preserves the original texture allocation and identity. */
    expect_native(&context, 1, CMD_LEVEL_MESHMAP_TEXTURE, 2,
        (const char *const[]){"meshmap_texture", indexed});
    other = meshmap_get(level->meshmaps, 1);
    assert(other && other->artwork && !other->samples && level->meshmaps->map_count == count + 1);
    original_artwork = other->artwork;
    original_artwork_path = other->artwork_path;
    generation = other->generation;
    expect_native(&context, 1, CMD_LEVEL_MESHMAP_GEOMETRY, 8,
        (const char *const[]){"meshmap_geometry", "y", "0", "0", "0", "0", "1", geometry});
    assert(meshmap_get(level->meshmaps, 1) == other && other->samples
           && other->generation == generation && other->artwork == original_artwork
           && other->artwork_path == original_artwork_path);
    assert(!open_files);
    meshmap_store_free(level->meshmaps);
    level = previous_level;
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
    test_symbolic_script_values();
    test_indexed_bank_properties();
    test_geometry_and_artwork(argv[1]);
    test_native_commands(argv[1]);
    test_native_symbolic_flags_and_rules();
    test_native_symbolic_bank_masks();
    test_native_authoring_contexts();
    test_native_definition_transactions(argv[1]);
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
    printf("meshmap script/native authoring checks passed: %u assertions, per-load selection, settings before geometry, independent geometry/texture transactions, symbolic flags, indexed bank activation, samples/holes, entity settings and draw arguments\n", checks);
    return 0;
}
