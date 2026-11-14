/*
 * OpenBOR - http://www.chronocrash.com
 * All rights reserved. See LICENSE in OpenBOR root for license details.
 *
 * Damon V. Caskey
 * 2026-11-14
 * Exercise production script entry points, constant lookup, and property maps.
 */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include "scriptcommon.h"

#undef malloc
#undef calloc
#undef realloc

const s_drawmethod plainmethod = {
    .config = DRAWMETHOD_CONFIG_ENABLED,
    .alpha = -1, .remap = -1,
    .scalex = 256, .scaley = 256, .xrepeat = 1, .yrepeat = 1,
    .channelr = 255, .channelg = 255, .channelb = 255,
    .affine = {.m00 = 1.0f, .m11 = 1.0f},
    .object_type = OBJECT_TYPE_DRAWMETHOD
};

/* Unrelated dynamic constants are linked but never queried by this fixture. */
int BGHEIGHT;
animation_id_t *animspecials, *animattacks, *animfollows, *animpains;
animation_id_t *animfalls, *animrises, *animriseattacks, *animdies;
animation_id_t *animidles, *animwalks, *animbackwalks, *animups, *animdowns;
animation_id_t *animbackpains, *animbackfalls, *animbackdies, *animbackrises;
animation_id_t *animbackriseattacks, *animblkpains, *animbackblkpains;

enum color_script_api { COLOR_COMMON, COLOR_OBJECT, COLOR_LEGACY };
static ScriptVariant color_arguments[8];
static ScriptVariant *color_argument_ptrs[8];
static s_drawmethod color_object;
static entity color_owner;
static unsigned color_sorted_searches;
static unsigned color_atomic_rejections;

/*
 * Damon V. Caskey
 * 2026-11-14
 * Retain allocator failure detection without linking the game startup path.
 */
void *__wrap_checkAlloc(void *pointer, size_t size, const char *function,
                        const char *file, int line)
{
    (void)size; (void)function; (void)file; (void)line;
    assert(pointer);
    return pointer;
}

int __real_searchList(const char *list[], const char *value, int length);

/*
 * Damon V. Caskey
 * 2026-11-14
 * Validate the actual legacy table before using its production binary search.
 */
int __wrap_searchList(const char *list[], const char *value, int length)
{
    int i;
    for(i = 1; i < length; ++i)
    {
        assert(strcasecmp(list[i - 1], list[i]) < 0);
    }
    ++color_sorted_searches;
    return __real_searchList(list, value, length);
}

/*
 * Damon V. Caskey
 * 2026-11-14
 * Clear argument ownership before preparing another real API invocation.
 */
static void color_script_arguments(void)
{
    int i;
    for(i = 0; i < 8; ++i)
    {
        ScriptVariant_Clear(&color_arguments[i]);
        color_argument_ptrs[i] = &color_arguments[i];
    }
}

/*
 * Damon V. Caskey
 * 2026-11-14
 * Construct one integer script value, including numeric RGB black.
 */
static void color_script_integer(int index, LONG value)
{
    ScriptVariant_ChangeType(&color_arguments[index], VT_INTEGER);
    color_arguments[index].lVal = value;
}

/*
 * Damon V. Caskey
 * 2026-11-14
 * Use the real string cache consumed by legacy names and openborconstant.
 */
static void color_script_string(int index, const char *value)
{
    ScriptVariant_ChangeType(&color_arguments[index], VT_STR);
    color_arguments[index].strVal = StrCache_CreateNewFrom(value);
}

/*
 * Damon V. Caskey
 * 2026-11-14
 * Supply a real drawmethod or entity pointer through the script variant.
 */
static void color_script_pointer(int index, void *value)
{
    ScriptVariant_ChangeType(&color_arguments[index], VT_PTR);
    color_arguments[index].ptrVal = value;
}

/*
 * Damon V. Caskey
 * 2026-11-14
 * Select the documented legacy spelling for the same public color property.
 */
static const char *color_script_name(int property)
{
    static const char *const names[] = {
        "chromakeya", "chromakeyb", "chromakeyc", "chromakeyd", "chromakeye",
        "chromakeyenabled", "substitutea", "substituteb", "substitutec",
        "substituted", "substitutee", "substituteenabled"
    };
    assert(property >= DM_CHROMA_KEY_A && property <= DM_SUBSTITUTE_ENABLED);
    return names[property - DM_CHROMA_KEY_A];
}

/*
 * Damon V. Caskey
 * 2026-11-14
 * Prepare equivalent common, object, or legacy calls without emulating setters.
 */
static void color_script_prepare(enum color_script_api api, int property)
{
    color_script_arguments();
    if(api == COLOR_OBJECT) color_script_pointer(0, &color_object);
    if(api == COLOR_LEGACY) color_script_pointer(0, &color_owner);
    if(api == COLOR_LEGACY) color_script_string(1, color_script_name(property));
    else color_script_integer(1, property);
}

/*
 * Damon V. Caskey
 * 2026-11-14
 * Dispatch only to the production entry point under test.
 */
static HRESULT color_script_set(enum color_script_api api, int count)
{
    ScriptVariant result = {0};
    ScriptVariant *result_pointer = &result;
    HRESULT status;
    if(api == COLOR_COMMON)
        status = openbor_set_draw_method(color_argument_ptrs, &result_pointer, count);
    else if(api == COLOR_OBJECT)
        status = openbor_set_drawmethod_property(color_argument_ptrs, &result_pointer, count);
    else
        status = openbor_changedrawmethod(color_argument_ptrs, &result_pointer, count);
    ScriptVariant_Clear(&result);
    return status;
}

/*
 * Damon V. Caskey
 * 2026-11-14
 * Retrieve an integer through the actual getters and preserve error results.
 */
static HRESULT color_script_get(enum color_script_api api, int count, LONG *value)
{
    ScriptVariant result = {0};
    ScriptVariant *result_pointer = &result;
    HRESULT status;
    if(api == COLOR_COMMON)
        status = openbor_get_draw_method(color_argument_ptrs, &result_pointer, count);
    else if(api == COLOR_OBJECT)
        status = openbor_get_drawmethod_property(
            (const ScriptVariant *const *)color_argument_ptrs, &result_pointer, count);
    else
        status = openbor_getdrawmethod(color_argument_ptrs, &result_pointer, count);
    if(SUCCEEDED(status))
    {
        assert(result_pointer == &result && result.vt == VT_INTEGER);
        *value = result.lVal;
    }
    ScriptVariant_Clear(&result);
    return status;
}

/*
 * Damon V. Caskey
 * 2026-11-14
 * Read the default component or an explicit match, substitute, or enable bit.
 */
static LONG color_script_read(enum color_script_api api, int property, int component)
{
    LONG value = -1;
    color_script_prepare(api, property);
    if(component >= 0) color_script_integer(2, component);
    assert(color_script_get(api, component < 0 ? 2 : 3, &value) == S_OK);
    return value;
}

/*
 * Damon V. Caskey
 * 2026-11-14
 * Record all exposed policy fields before a potentially invalid update.
 */
static s_drawmethod_colors color_script_snapshot(enum color_script_api api)
{
    s_drawmethod_colors state = {0};
    unsigned i;
    for(i = 0; i < DRAWMETHOD_COLOR_SLOTS; ++i)
    {
        state.chroma[i] = (unsigned)color_script_read(api, DM_CHROMA_KEY_A + i, 0);
        state.substitute[i].match = (unsigned)color_script_read(api, DM_SUBSTITUTE_A + i, 0);
        state.substitute[i].replacement = (unsigned)color_script_read(api, DM_SUBSTITUTE_A + i, 1);
    }
    state.chroma_enabled = (unsigned)color_script_read(api, DM_CHROMA_KEY_ENABLED, -1);
    state.substitute_enabled = (unsigned)color_script_read(api, DM_SUBSTITUTE_ENABLED, -1);
    return state;
}

/*
 * Damon V. Caskey
 * 2026-11-14
 * Require a failed update to leave every previously stored color field intact.
 */
static void color_script_rejected(enum color_script_api api, int count,
                                  const s_drawmethod_colors *before)
{
    s_drawmethod_colors after;
    assert(FAILED(color_script_set(api, count)));
    after = color_script_snapshot(api);
    assert(memcmp(before, &after, sizeof(after)) == 0);
    ++color_atomic_rejections;
}

/*
 * Damon V. Caskey
 * 2026-11-14
 * Reset via the engine's existing legacy reset operation, including common state.
 */
static void color_script_reset(enum color_script_api api)
{
    color_script_arguments();
    if(api != COLOR_COMMON) color_script_pointer(0, &color_owner);
    color_script_string(1, "reset");
    color_script_integer(2, 1);
    assert(color_script_set(COLOR_LEGACY, 3) == S_OK);
}

/*
 * Damon V. Caskey
 * 2026-11-14
 * Exercise all five slots, optional components, NULL disabling, and masks.
 */
static void color_script_slots(enum color_script_api api)
{
    static const unsigned matches[5] = {0, 0xFFFFFF, 0x0000FF, 0x00FF00, 0xFF0000};
    static const unsigned replacements[5] = {0x010101, 0xFEFEFE, 0x0101FE, 0x01FE01, 0xFE0101};
    unsigned i;
    color_script_reset(api);
    for(i = 0; i < 5; ++i)
    {
        color_script_prepare(api, DM_SUBSTITUTE_A + i);
        color_script_integer(2, matches[i]);
        color_script_integer(3, replacements[i]);
        assert(color_script_set(api, 4) == S_OK);
        color_script_prepare(api, DM_CHROMA_KEY_A + i);
        color_script_integer(2, matches[i]);
        assert(color_script_set(api, 3) == S_OK);
        assert(color_script_read(api, DM_SUBSTITUTE_A + i, -1) == (LONG)matches[i]);
        assert(color_script_read(api, DM_SUBSTITUTE_A + i, DM_COLOR_SUBSTITUTE) == (LONG)replacements[i]);
        assert(color_script_read(api, DM_SUBSTITUTE_A + i, DM_COLOR_ENABLED) == 1);
        assert(color_script_read(api, DM_CHROMA_KEY_A + i, -1) == (LONG)matches[i]);
        assert(color_script_read(api, DM_CHROMA_KEY_A + i, DM_COLOR_ENABLED) == 1);
    }
    assert(color_script_read(api, DM_SUBSTITUTE_ENABLED, -1) == 31);
    assert(color_script_read(api, DM_CHROMA_KEY_ENABLED, -1) == 31);

    /* NULL means VT_EMPTY; numeric zero above remains a live black key. */
    color_script_prepare(api, DM_SUBSTITUTE_A);
    assert(color_script_set(api, 3) == S_OK);
    color_script_prepare(api, DM_CHROMA_KEY_A);
    assert(color_script_set(api, 3) == S_OK);
    assert(color_script_read(api, DM_SUBSTITUTE_A, 0) == 0);
    assert(color_script_read(api, DM_SUBSTITUTE_A, 1) == 0x010101);
    assert(color_script_read(api, DM_SUBSTITUTE_A, 2) == 0);
    assert(color_script_read(api, DM_CHROMA_KEY_A, 2) == 0);
    color_script_prepare(api, DM_CHROMA_KEY_A);
    color_script_integer(2, 0);
    assert(color_script_set(api, 3) == S_OK);
    assert(color_script_read(api, DM_CHROMA_KEY_A, 2) == 1);

    for(i = 0; i < 2; ++i)
    {
        int property = i ? DM_CHROMA_KEY_ENABLED : DM_SUBSTITUTE_ENABLED;
        color_script_prepare(api, property);
        color_script_integer(2, 0);
        assert(color_script_set(api, 3) == S_OK);
        assert(color_script_read(api, property, -1) == 0);
        color_script_prepare(api, property);
        color_script_integer(2, 31);
        assert(color_script_set(api, 3) == S_OK);
        assert(color_script_read(api, property, -1) == 31);
    }
}

/*
 * Damon V. Caskey
 * 2026-11-14
 * Cover malformed pairs and cross-slot conflicts without partial mutations.
 */
static void color_script_invalid_updates(enum color_script_api api)
{
    static const LONG invalid_colors[] = {-1, 0x1000000};
    static const unsigned conflicts[][2] = {
        {0, 0x020202},               /* Duplicate match. */
        {0x010101, 0x020202},        /* Existing replacement becomes a match. */
        {0xFFFFFF, 0},              /* New replacement is an existing match. */
        {0xFFFFFF, 0xFFFFFF}        /* Self replacement. */
    };
    s_drawmethod_colors before = color_script_snapshot(api);
    unsigned i;
    LONG ignored;
    for(i = 0; i < sizeof(conflicts) / sizeof(conflicts[0]); ++i)
    {
        color_script_prepare(api, DM_SUBSTITUTE_B);
        color_script_integer(2, conflicts[i][0]);
        color_script_integer(3, conflicts[i][1]);
        color_script_rejected(api, 4, &before);
    }
    for(i = 0; i < 2; ++i)
    {
        color_script_prepare(api, DM_SUBSTITUTE_A);
        color_script_integer(2, 0);
        color_script_integer(3, invalid_colors[i]);
        color_script_rejected(api, 4, &before);
        color_script_prepare(api, DM_CHROMA_KEY_A);
        color_script_integer(2, invalid_colors[i]);
        color_script_rejected(api, 3, &before);
    }

    color_script_prepare(api, DM_SUBSTITUTE_A);
    color_script_integer(2, 0); /* A valid match without a replacement is incomplete. */
    color_script_rejected(api, 3, &before);
    color_script_prepare(api, DM_SUBSTITUTE_A);
    color_script_integer(2, 0);
    color_script_integer(3, 0x010101);
    color_script_integer(4, 7);
    color_script_rejected(api, 5, &before);
    color_script_prepare(api, DM_SUBSTITUTE_A);
    color_script_integer(3, 0x010101); /* NULL plus a second value is not disable. */
    color_script_rejected(api, 4, &before);

    color_script_prepare(api, DM_SUBSTITUTE_A);
    color_script_integer(2, 0);
    ScriptVariant_ChangeType(&color_arguments[3], VT_DECIMAL);
    color_arguments[3].dblVal = 1.0;
    color_script_rejected(api, 4, &before);
    color_script_prepare(api, DM_SUBSTITUTE_A);
    color_script_integer(2, 0);
    color_script_string(3, "1");
    color_script_rejected(api, 4, &before);
    color_script_prepare(api, DM_CHROMA_KEY_A);
    color_script_pointer(2, NULL); /* Pointer type is not the NULL() value type. */
    color_script_rejected(api, 3, &before);

    for(i = 0; i < 2; ++i)
    {
        int property = i ? DM_CHROMA_KEY_ENABLED : DM_SUBSTITUTE_ENABLED;
        color_script_prepare(api, property);
        color_script_integer(2, 32);
        color_script_rejected(api, 3, &before);
        color_script_prepare(api, property);
        color_script_integer(2, -1);
        color_script_rejected(api, 3, &before);
    }

    color_script_prepare(api, DM_SUBSTITUTE_A);
    color_script_integer(2, 3);
    assert(FAILED(color_script_get(api, 3, &ignored)));
    color_script_prepare(api, DM_SUBSTITUTE_A);
    ScriptVariant_ChangeType(&color_arguments[2], VT_DECIMAL);
    color_arguments[2].dblVal = 1.0;
    assert(FAILED(color_script_get(api, 3, &ignored)));
    color_script_prepare(api, DM_CHROMA_KEY_A);
    color_script_integer(2, DM_COLOR_SUBSTITUTE);
    assert(FAILED(color_script_get(api, 3, &ignored)));
    color_script_prepare(api, DM_SUBSTITUTE_ENABLED);
    color_script_integer(2, DM_COLOR_ENABLED);
    assert(FAILED(color_script_get(api, 3, &ignored)));
}

/*
 * Damon V. Caskey
 * 2026-11-14
 * Re-enabling retained slots must validate duplicates and cascades together.
 */
static void color_script_reenable(enum color_script_api api)
{
    unsigned cascade;
    for(cascade = 0; cascade < 2; ++cascade)
    {
        s_drawmethod_colors before;
        color_script_reset(api);
        color_script_prepare(api, DM_SUBSTITUTE_A);
        color_script_integer(2, 0); color_script_integer(3, 1);
        assert(color_script_set(api, 4) == S_OK);
        color_script_prepare(api, DM_SUBSTITUTE_ENABLED);
        color_script_integer(2, 0);
        assert(color_script_set(api, 3) == S_OK);
        color_script_prepare(api, DM_SUBSTITUTE_B);
        color_script_integer(2, cascade ? 1 : 0); color_script_integer(3, 2);
        assert(color_script_set(api, 4) == S_OK);
        before = color_script_snapshot(api);
        color_script_prepare(api, DM_SUBSTITUTE_ENABLED);
        color_script_integer(2, 3);
        color_script_rejected(api, 3, &before);
        assert(color_script_read(api, DM_SUBSTITUTE_ENABLED, -1) == 2);
        assert(color_script_read(api, DM_SUBSTITUTE_A, 1) == 1);
    }
}

/*
 * Damon V. Caskey
 * 2026-11-14
 * Keep ordinary properties and existing numeric legacy IDs working unchanged.
 */
static void color_script_ordinary_and_reset(enum color_script_api api)
{
    LONG value;
    s_drawmethod_colors empty = {0}, actual;
    color_script_arguments();
    if(api == COLOR_LEGACY)
    {
        color_script_pointer(0, &color_owner);
        color_script_integer(1, 24); /* Pre-existing numeric scalex ID. */
    }
    else
    {
        if(api == COLOR_OBJECT) color_script_pointer(0, &color_object);
        color_script_integer(1, DRAWMETHOD_PROPERTY_SCALE_X);
    }
    color_script_integer(2, 384);
    assert(color_script_set(api, 3) == S_OK);
    if(api == COLOR_LEGACY) color_script_string(1, "SCALEX");
    assert(color_script_get(api, 2, &value) == S_OK && value == 384);

    color_script_reset(api);
    actual = color_script_snapshot(api);
    assert(memcmp(&actual, &empty, sizeof(actual)) == 0);
    if(api != COLOR_COMMON) assert(color_object.scalex == 256);
}

/*
 * Damon V. Caskey
 * 2026-11-14
 * Verify the actual named constant translator, including long-form aliases.
 */
static void color_script_constants(void)
{
#define COLOR_CONSTANT(name) {#name, name}
    static const struct { const char *name; LONG value; } constants[] = {
        COLOR_CONSTANT(DM_CHROMA_KEY_A), COLOR_CONSTANT(DM_CHROMA_KEY_B),
        COLOR_CONSTANT(DM_CHROMA_KEY_C), COLOR_CONSTANT(DM_CHROMA_KEY_D),
        COLOR_CONSTANT(DM_CHROMA_KEY_E), COLOR_CONSTANT(DM_CHROMA_KEY_ENABLED),
        COLOR_CONSTANT(DM_SUBSTITUTE_A), COLOR_CONSTANT(DM_SUBSTITUTE_B),
        COLOR_CONSTANT(DM_SUBSTITUTE_C), COLOR_CONSTANT(DM_SUBSTITUTE_D),
        COLOR_CONSTANT(DM_SUBSTITUTE_E), COLOR_CONSTANT(DM_SUBSTITUTE_ENABLED),
        COLOR_CONSTANT(DM_COLOR_MATCH), COLOR_CONSTANT(DM_COLOR_SUBSTITUTE),
        COLOR_CONSTANT(DM_COLOR_ENABLED),
        COLOR_CONSTANT(DRAWMETHOD_PROPERTY_CHROMA_KEY_A),
        COLOR_CONSTANT(DRAWMETHOD_PROPERTY_CHROMA_KEY_B),
        COLOR_CONSTANT(DRAWMETHOD_PROPERTY_CHROMA_KEY_C),
        COLOR_CONSTANT(DRAWMETHOD_PROPERTY_CHROMA_KEY_D),
        COLOR_CONSTANT(DRAWMETHOD_PROPERTY_CHROMA_KEY_E),
        COLOR_CONSTANT(DRAWMETHOD_PROPERTY_CHROMA_KEY_ENABLED),
        COLOR_CONSTANT(DRAWMETHOD_PROPERTY_SUBSTITUTE_A),
        COLOR_CONSTANT(DRAWMETHOD_PROPERTY_SUBSTITUTE_B),
        COLOR_CONSTANT(DRAWMETHOD_PROPERTY_SUBSTITUTE_C),
        COLOR_CONSTANT(DRAWMETHOD_PROPERTY_SUBSTITUTE_D),
        COLOR_CONSTANT(DRAWMETHOD_PROPERTY_SUBSTITUTE_E),
        COLOR_CONSTANT(DRAWMETHOD_PROPERTY_SUBSTITUTE_ENABLED)
    };
#undef COLOR_CONSTANT
    unsigned i;
    for(i = 0; i < sizeof(constants) / sizeof(constants[0]); ++i)
    {
        ScriptVariant result = {0};
        ScriptVariant *result_pointer = &result;
        color_script_arguments();
        color_script_string(0, constants[i].name);
        assert(openbor_transconst(color_argument_ptrs, &result_pointer, 1) == S_OK);
        assert(result_pointer == &result && result.vt == VT_INTEGER);
        assert(result.lVal == constants[i].value);
        ScriptVariant_Clear(&result);
    }
}

/*
 * Damon V. Caskey
 * 2026-11-14
 * Protect the shared default and distinguish convenience NULL from object APIs.
 */
static void color_script_object_contract(void)
{
    s_drawmethod before = plainmethod;
    LONG ignored;
    unsigned api;
    for(api = COLOR_COMMON; api <= COLOR_OBJECT; ++api)
    {
        color_script_prepare((enum color_script_api)api, DM_SUBSTITUTE_A);
        color_script_pointer(0, (void *)&plainmethod);
        color_script_integer(2, 0); color_script_integer(3, 1);
        assert(FAILED(color_script_set((enum color_script_api)api, 4)));
        assert(memcmp(&plainmethod, &before, sizeof(before)) == 0);
    }
    color_owner.drawmethod = (s_drawmethod *)&plainmethod;
    color_script_prepare(COLOR_LEGACY, DM_CHROMA_KEY_A);
    color_script_integer(2, 0);
    assert(FAILED(color_script_set(COLOR_LEGACY, 3)));
    assert(memcmp(&plainmethod, &before, sizeof(before)) == 0);
    color_owner.drawmethod = &color_object;

    color_script_prepare(COLOR_COMMON, DM_SUBSTITUTE_A);
    color_script_pointer(0, NULL);
    color_script_integer(2, 0); color_script_integer(3, 1);
    assert(color_script_set(COLOR_COMMON, 4) == S_OK);
    assert(color_script_read(COLOR_COMMON, DM_SUBSTITUTE_A, 1) == 1);
    color_script_prepare(COLOR_OBJECT, DM_SUBSTITUTE_A);
    color_script_pointer(0, NULL);
    color_script_integer(2, 0); color_script_integer(3, 1);
    assert(FAILED(color_script_set(COLOR_OBJECT, 4)));

    /* Generic access checks the actual object's identity before mutation. */
    color_object.object_type = 0;
    color_script_prepare(COLOR_OBJECT, DM_SUBSTITUTE_A);
    color_script_integer(2, 0); color_script_integer(3, 1);
    assert(FAILED(color_script_set(COLOR_OBJECT, 4)));
    color_script_prepare(COLOR_OBJECT, DM_SUBSTITUTE_A);
    assert(FAILED(color_script_get(COLOR_OBJECT, 2, &ignored)));
    color_object = plainmethod;
}

/*
 * Damon V. Caskey
 * 2026-11-14
 * Run the same behavioral contract through all three script-facing APIs.
 */
int main(void)
{
    unsigned api;
    assert(sizeof(void *) == 8);
    color_object = plainmethod;
    color_owner.drawmethod = &color_object;
    color_script_arguments();
    color_script_constants();
    for(api = COLOR_COMMON; api <= COLOR_LEGACY; ++api)
    {
        color_script_slots((enum color_script_api)api);
        color_script_invalid_updates((enum color_script_api)api);
        color_script_reenable((enum color_script_api)api);
        color_script_ordinary_and_reset((enum color_script_api)api);
    }
    color_script_object_contract();
    assert(color_sorted_searches > 0 && color_atomic_rejections == 60);
    color_script_arguments();
    StrCache_Clear();
    printf("drawmethod color script checks passed: 27 constants, 3 production APIs, "
           "%u atomic rejections; legacy tables sorted\n", color_atomic_rejections);
    return 0;
}
