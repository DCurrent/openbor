/*
 * OpenBOR - http://www.chronocrash.com
 * All rights reserved. See LICENSE in OpenBOR root for license details.
 *
 * Damon V. Caskey
 * 2026-11-14
 * Exercise production script entry points, constant lookup, and property maps.
 * 2026-11-15: cover static resamplers through object, common and named APIs.
 */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <math.h>
#include "scriptcommon.h"
#include "drawmethod_curve.h"

#undef malloc
#undef calloc
#undef realloc

const s_drawmethod plainmethod = {
    .config = DRAWMETHOD_CONFIG_ENABLED,
    .resample = DC_RESAMPLE_NEAREST,
    .alpha = -1, .remap = -1,
    .scalex = 256, .scaley = 256, .xrepeat = 1, .yrepeat = 1,
    .channelr = 255, .channelg = 255, .channelb = 255,
    .affine = {.m00 = 1.0f, .m11 = 1.0f},
    .curve = DRAWMETHOD_CURVE_INITIALIZER,
    .object_type = OBJECT_TYPE_DRAWMETHOD
};

/* Unrelated dynamic constants are linked but never queried by this fixture. */
int BGHEIGHT;
animation_id_t *animspecials, *animattacks, *animfollows, *animpains;
animation_id_t *animfalls, *animrises, *animriseattacks, *animdies;
animation_id_t *animidles, *animwalks, *animbackwalks, *animups, *animdowns;
animation_id_t *animbackpains, *animbackfalls, *animbackdies, *animbackrises;
animation_id_t *animbackriseattacks, *animblkpains, *animbackblkpains;

enum color_script_api {
    COLOR_COMMON, COLOR_OBJECT, COLOR_LEGACY,
    COLOR_COMMON_OBJECT, COLOR_LEGACY_COMMON
};
#define DRAWMETHOD_SCRIPT_TEST_ARGUMENTS 16
static ScriptVariant color_arguments[DRAWMETHOD_SCRIPT_TEST_ARGUMENTS];
static ScriptVariant *color_argument_ptrs[DRAWMETHOD_SCRIPT_TEST_ARGUMENTS];
static s_drawmethod color_object;
static entity color_owner;
static unsigned color_sorted_searches;
static unsigned color_atomic_rejections;
static unsigned resample_atomic_rejections;
static unsigned drawmethod_constant_checks;
static unsigned curve_atomic_rejections;

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
    for(i = 0; i < DRAWMETHOD_SCRIPT_TEST_ARGUMENTS; ++i)
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
 * 2026-11-15: include the independent scalar resample control.
 */
static const char *color_script_name(int property)
{
    static const char *const curve_names[] = {
        "curve", "curveenabled", "curveangle", "curveaxis", "curveplane",
        "curvefocus", "curveextent", "curveperspective", "curvetilt",
        "curveprofile", "curvescrollx", "curvescrolly", "curvewrap"
    };
    static const char *const names[] = {
        "chromakeya", "chromakeyb", "chromakeyc", "chromakeyd", "chromakeye",
        "chromakeyenabled", "substitutea", "substituteb", "substitutec",
        "substituted", "substitutee", "substituteenabled"
    };
    if(property == DM_RESAMPLE)
    {
        return "resample";
    }
    if(property >= DM_CURVE && property <= DM_CURVE_WRAP)
    {
        return curve_names[property - DM_CURVE];
    }
    assert(property >= DM_CHROMA_KEY_A && property <= DM_SUBSTITUTE_ENABLED);
    return names[property - DM_CHROMA_KEY_A];
}

/*
 * Damon V. Caskey
 * 2026-11-14
 * Prepare equivalent common, object, or legacy calls without emulating setters.
 * 2026-11-15: distinguish convenience-object and named-common access paths.
 */
static void color_script_prepare(enum color_script_api api, int property)
{
    color_script_arguments();
    if(api == COLOR_OBJECT || api == COLOR_COMMON_OBJECT)
    {
        color_script_pointer(0, &color_object);
    }
    if(api == COLOR_LEGACY)
    {
        color_script_pointer(0, &color_owner);
    }
    if(api == COLOR_LEGACY || api == COLOR_LEGACY_COMMON)
    {
        color_script_string(1, color_script_name(property));
    }
    else
    {
        color_script_integer(1, property);
    }
}

/*
 * Damon V. Caskey
 * 2026-11-14
 * Dispatch only to the production entry point under test.
 * 2026-11-15: use the same entry points for all five ownership/access paths.
 */
static HRESULT color_script_set(enum color_script_api api, int count)
{
    ScriptVariant result = {0};
    ScriptVariant *result_pointer = &result;
    HRESULT status;
    if(api == COLOR_COMMON || api == COLOR_COMMON_OBJECT)
    {
        status = openbor_set_draw_method(color_argument_ptrs, &result_pointer, count);
    }
    else if(api == COLOR_OBJECT)
    {
        status = openbor_set_drawmethod_property(color_argument_ptrs, &result_pointer, count);
    }
    else
    {
        status = openbor_changedrawmethod(color_argument_ptrs, &result_pointer, count);
    }
    ScriptVariant_Clear(&result);
    return status;
}

/*
 * Damon V. Caskey
 * 2026-11-14
 * Retrieve an integer through the actual getters and preserve error results.
 * 2026-11-15: cover convenience-object and named-common reads as well.
 */
static HRESULT color_script_get(enum color_script_api api, int count, LONG *value)
{
    ScriptVariant result = {0};
    ScriptVariant *result_pointer = &result;
    HRESULT status;
    if(api == COLOR_COMMON || api == COLOR_COMMON_OBJECT)
    {
        status = openbor_get_draw_method(color_argument_ptrs, &result_pointer, count);
    }
    else if(api == COLOR_OBJECT)
    {
        status = openbor_get_drawmethod_property(
            (const ScriptVariant *const *)color_argument_ptrs, &result_pointer, count);
    }
    else
    {
        status = openbor_getdrawmethod(color_argument_ptrs, &result_pointer, count);
    }
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
 * 2026-11-15: both global access forms address the same common method.
 */
static void color_script_reset(enum color_script_api api)
{
    color_script_arguments();
    if(api != COLOR_COMMON && api != COLOR_LEGACY_COMMON)
    {
        color_script_pointer(0, &color_owner);
    }
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
 * 2026-11-15: include every public built-in sampler and its property aliases.
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
        COLOR_CONSTANT(DRAWMETHOD_PROPERTY_SUBSTITUTE_ENABLED),
        COLOR_CONSTANT(DM_RESAMPLE),
        COLOR_CONSTANT(DRAWMETHOD_PROPERTY_RESAMPLE),
        COLOR_CONSTANT(DC_RESAMPLE_NEAREST),
        COLOR_CONSTANT(DC_RESAMPLE_BILINEAR),
        COLOR_CONSTANT(DC_RESAMPLE_BICUBIC_CATMULL_ROM),
        COLOR_CONSTANT(DC_RESAMPLE_BICUBIC_MITCHELL),
        COLOR_CONSTANT(DC_RESAMPLE_BICUBIC_BSPLINE),
        COLOR_CONSTANT(DC_RESAMPLE_LANCZOS_2),
        COLOR_CONSTANT(DC_RESAMPLE_LANCZOS_3),
        COLOR_CONSTANT(DC_RESAMPLE_MOVIE),
        COLOR_CONSTANT(DM_CURVE),
        COLOR_CONSTANT(DRAWMETHOD_PROPERTY_CURVE),
        COLOR_CONSTANT(DM_CURVE_ENABLED),
        COLOR_CONSTANT(DRAWMETHOD_PROPERTY_CURVE_ENABLED),
        COLOR_CONSTANT(DM_CURVE_ANGLE),
        COLOR_CONSTANT(DRAWMETHOD_PROPERTY_CURVE_ANGLE),
        COLOR_CONSTANT(DM_CURVE_AXIS),
        COLOR_CONSTANT(DRAWMETHOD_PROPERTY_CURVE_AXIS),
        COLOR_CONSTANT(DM_CURVE_PLANE),
        COLOR_CONSTANT(DRAWMETHOD_PROPERTY_CURVE_PLANE),
        COLOR_CONSTANT(DM_CURVE_FOCUS),
        COLOR_CONSTANT(DRAWMETHOD_PROPERTY_CURVE_FOCUS),
        COLOR_CONSTANT(DM_CURVE_EXTENT),
        COLOR_CONSTANT(DRAWMETHOD_PROPERTY_CURVE_EXTENT),
        COLOR_CONSTANT(DM_CURVE_PERSPECTIVE),
        COLOR_CONSTANT(DRAWMETHOD_PROPERTY_CURVE_PERSPECTIVE),
        COLOR_CONSTANT(DM_CURVE_TILT),
        COLOR_CONSTANT(DRAWMETHOD_PROPERTY_CURVE_TILT),
        COLOR_CONSTANT(DM_CURVE_PROFILE),
        COLOR_CONSTANT(DRAWMETHOD_PROPERTY_CURVE_PROFILE),
        COLOR_CONSTANT(DM_CURVE_SCROLL_X),
        COLOR_CONSTANT(DRAWMETHOD_PROPERTY_CURVE_SCROLL_X),
        COLOR_CONSTANT(DM_CURVE_SCROLL_Y),
        COLOR_CONSTANT(DRAWMETHOD_PROPERTY_CURVE_SCROLL_Y),
        COLOR_CONSTANT(DM_CURVE_WRAP),
        COLOR_CONSTANT(DRAWMETHOD_PROPERTY_CURVE_WRAP),
        COLOR_CONSTANT(DRAWMETHOD_CONFIG_CURVE),
        COLOR_CONSTANT(DC_CURVE_PROFILE_UNIFORM),
        COLOR_CONSTANT(DC_CURVE_PROFILE_SMOOTH),
        COLOR_CONSTANT(DC_CURVE_WRAP_CLIP),
        COLOR_CONSTANT(DC_CURVE_WRAP_REPEAT)
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
        ++drawmethod_constant_checks;
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
 * 2026-11-15
 * Round-trip all built-in kernels and verify reset restores explicit nearest.
 * This selector belongs to the drawmethod, independent of its transform fields.
 */
static void resample_script_modes(enum color_script_api api)
{
    int mode;
    color_script_reset(api);
    assert(color_script_read(api, DM_RESAMPLE, -1) == DC_RESAMPLE_NEAREST);
    for(mode = DC_RESAMPLE_NEAREST; mode < DC_RESAMPLE_END; ++mode)
    {
        color_script_prepare(api, DM_RESAMPLE);
        color_script_integer(2, mode);
        assert(color_script_set(api, 3) == S_OK);
        assert(color_script_read(api, DM_RESAMPLE, -1) == mode);
    }
    color_script_prepare(api, DM_RESAMPLE);
    color_script_integer(2, DC_RESAMPLE_MOVIE);
    assert(color_script_set(api, 3) == S_OK);
    assert(color_script_read(api, DM_RESAMPLE, -1) == DC_RESAMPLE_BILINEAR);
    color_script_reset(api);
    assert(color_script_read(api, DM_RESAMPLE, -1) == DC_RESAMPLE_NEAREST);
}

/*
 * Damon V. Caskey
 * 2026-11-15
 * Failed sampler updates must retain the sampler and all existing color rules.
 * Compare the entire object too, including transform and object identity fields.
 */
static void resample_script_rejected(enum color_script_api api, int count,
                                     const s_drawmethod *before,
                                     const s_drawmethod_colors *colors_before)
{
    s_drawmethod_colors colors_after;
    assert(FAILED(color_script_set(api, count)));
    assert(memcmp(before, &color_object, sizeof(*before)) == 0);
    assert(color_script_read(api, DM_RESAMPLE, -1) == DC_RESAMPLE_LANCZOS_3);
    colors_after = color_script_snapshot(api);
    assert(memcmp(colors_before, &colors_after, sizeof(colors_after)) == 0);
    ++resample_atomic_rejections;
}

/*
 * Damon V. Caskey
 * 2026-11-15
 * Reject range violations, coercible values, alternate variant types and
 * malformed argument counts through each real production setter.
 */
static void resample_script_invalid_updates(enum color_script_api api)
{
    static const LONG invalid_modes[] = {-1, DC_RESAMPLE_END, INT32_MIN, INT32_MAX};
    static const DOUBLE invalid_decimals[] = {0.0, 1.0, 1.5, NAN, INFINITY};
    static const char *const invalid_strings[] = {"1", "DC_RESAMPLE_BILINEAR"};
    s_drawmethod before;
    s_drawmethod_colors colors_before;
    unsigned i;
    LONG ignored;

    color_script_prepare(api, DM_RESAMPLE);
    color_script_integer(2, DC_RESAMPLE_LANCZOS_3);
    assert(color_script_set(api, 3) == S_OK);
    color_script_prepare(api, DM_CHROMA_KEY_A);
    color_script_integer(2, 0x123456);
    assert(color_script_set(api, 3) == S_OK);
    memcpy(&before, &color_object, sizeof(before));
    colors_before = color_script_snapshot(api);

    for(i = 0; i < sizeof(invalid_modes) / sizeof(invalid_modes[0]); ++i)
    {
        color_script_prepare(api, DM_RESAMPLE);
        color_script_integer(2, invalid_modes[i]);
        resample_script_rejected(api, 3, &before, &colors_before);
    }
    for(i = 0; i < sizeof(invalid_decimals) / sizeof(invalid_decimals[0]); ++i)
    {
        color_script_prepare(api, DM_RESAMPLE);
        ScriptVariant_ChangeType(&color_arguments[2], VT_DECIMAL);
        color_arguments[2].dblVal = invalid_decimals[i];
        resample_script_rejected(api, 3, &before, &colors_before);
    }
    for(i = 0; i < sizeof(invalid_strings) / sizeof(invalid_strings[0]); ++i)
    {
        color_script_prepare(api, DM_RESAMPLE);
        color_script_string(2, invalid_strings[i]);
        resample_script_rejected(api, 3, &before, &colors_before);
    }

    color_script_prepare(api, DM_RESAMPLE); /* NULL() cannot disable a sampler. */
    resample_script_rejected(api, 3, &before, &colors_before);
    color_script_prepare(api, DM_RESAMPLE);
    color_script_pointer(2, NULL);
    resample_script_rejected(api, 3, &before, &colors_before);
    color_script_prepare(api, DM_RESAMPLE);
    color_script_pointer(2, &color_object);
    resample_script_rejected(api, 3, &before, &colors_before);

    /* Match the existing property contract: selectors use VT_INTEGER exactly. */
    color_script_prepare(api, DM_RESAMPLE);
    ScriptVariant_ChangeType(&color_arguments[2], VT_INTEGER64);
    color_arguments[2].llVal = DC_RESAMPLE_BILINEAR;
    resample_script_rejected(api, 3, &before, &colors_before);
    color_script_prepare(api, DM_RESAMPLE);
    ScriptVariant_ChangeType(&color_arguments[2], VT_UINTEGER64);
    color_arguments[2].ullVal = DC_RESAMPLE_BILINEAR;
    resample_script_rejected(api, 3, &before, &colors_before);

    color_script_prepare(api, DM_RESAMPLE);
    resample_script_rejected(api, 2, &before, &colors_before);
    color_script_prepare(api, DM_RESAMPLE);
    color_script_integer(2, DC_RESAMPLE_BILINEAR);
    color_script_integer(3, 0);
    resample_script_rejected(api, 4, &before, &colors_before);
    color_script_prepare(api, DM_RESAMPLE);
    color_script_integer(2, DC_RESAMPLE_BILINEAR);
    color_script_integer(3, 0);
    color_script_integer(4, 0);
    resample_script_rejected(api, 5, &before, &colors_before);

    color_script_prepare(api, DM_RESAMPLE);
    color_script_integer(2, DM_COLOR_MATCH);
    assert(FAILED(color_script_get(api, 3, &ignored)));
    assert(color_script_read(api, DM_RESAMPLE, -1) == DC_RESAMPLE_LANCZOS_3);
}

/*
 * Damon V. Caskey
 * 2026-11-15
 * Exercise the real copy entry point and retain a sampler after its source
 * changes. Copying the shared default restores nearest without aliasing it.
 */
static void resample_script_copy(void)
{
    s_drawmethod copied = plainmethod;
    s_drawmethod before;
    ScriptVariant result = {0};
    ScriptVariant *result_pointer = &result;
    color_script_prepare(COLOR_OBJECT, DM_RESAMPLE);
    color_script_integer(2, DC_RESAMPLE_BICUBIC_MITCHELL);
    assert(color_script_set(COLOR_OBJECT, 3) == S_OK);
    memcpy(&before, &color_object, sizeof(before));

    color_script_arguments();
    color_script_pointer(0, &copied);
    color_script_pointer(1, &color_object);
    assert(openbor_copy_drawmethod(color_argument_ptrs, &result_pointer, 2) == S_OK);
    assert(memcmp(&before, &copied, sizeof(copied)) == 0);
    color_script_prepare(COLOR_OBJECT, DM_RESAMPLE);
    color_script_integer(2, DC_RESAMPLE_NEAREST);
    assert(color_script_set(COLOR_OBJECT, 3) == S_OK);
    assert(copied.resample == DC_RESAMPLE_BICUBIC_MITCHELL);

    color_script_arguments();
    color_script_pointer(0, &copied);
    color_script_pointer(1, (void *)&plainmethod);
    assert(openbor_copy_drawmethod(color_argument_ptrs, &result_pointer, 2) == S_OK);
    assert(copied.resample == DC_RESAMPLE_NEAREST);
    assert(memcmp(&copied, &plainmethod, sizeof(copied)) == 0);
    ScriptVariant_Clear(&result);
}

/*
 * Damon V. Caskey
 * 2026-11-15
 * Protect the shared default and invalid object pointers using sampler calls,
 * then verify convenience and legacy global access share one stored selector.
 */
static void resample_script_object_contract(void)
{
    s_drawmethod before;
    LONG ignored;
    unsigned api;
    memcpy(&before, &plainmethod, sizeof(before));
    for(api = COLOR_COMMON; api <= COLOR_LEGACY; ++api)
    {
        color_script_prepare((enum color_script_api)api, DM_RESAMPLE);
        if(api == COLOR_LEGACY)
        {
            color_owner.drawmethod = (s_drawmethod *)&plainmethod;
        }
        else
        {
            color_script_pointer(0, (void *)&plainmethod);
        }
        assert(color_script_get((enum color_script_api)api, 2, &ignored) == S_OK);
        assert(ignored == DC_RESAMPLE_NEAREST);
        color_script_integer(2, DC_RESAMPLE_BILINEAR);
        assert(FAILED(color_script_set((enum color_script_api)api, 3)));
        assert(memcmp(&before, &plainmethod, sizeof(before)) == 0);
    }
    color_owner.drawmethod = &color_object;

    color_script_prepare(COLOR_OBJECT, DM_RESAMPLE);
    color_script_pointer(0, NULL);
    color_script_integer(2, DC_RESAMPLE_BILINEAR);
    assert(FAILED(color_script_set(COLOR_OBJECT, 3)));
    assert(FAILED(color_script_get(COLOR_OBJECT, 2, &ignored)));
    color_object.object_type = 0;
    color_script_prepare(COLOR_OBJECT, DM_RESAMPLE);
    color_script_integer(2, DC_RESAMPLE_BILINEAR);
    assert(FAILED(color_script_set(COLOR_OBJECT, 3)));
    assert(FAILED(color_script_get(COLOR_OBJECT, 2, &ignored)));
    color_object = plainmethod;

    color_script_prepare(COLOR_COMMON, DM_RESAMPLE);
    color_script_pointer(0, NULL);
    color_script_integer(2, DC_RESAMPLE_BICUBIC_BSPLINE);
    assert(color_script_set(COLOR_COMMON, 3) == S_OK);
    assert(color_script_read(COLOR_LEGACY_COMMON, DM_RESAMPLE, -1) == DC_RESAMPLE_BICUBIC_BSPLINE);
    color_script_prepare(COLOR_LEGACY_COMMON, DM_RESAMPLE);
    color_script_integer(2, DC_RESAMPLE_LANCZOS_2);
    assert(color_script_set(COLOR_LEGACY_COMMON, 3) == S_OK);
    assert(color_script_read(COLOR_COMMON, DM_RESAMPLE, -1) == DC_RESAMPLE_LANCZOS_2);
    color_script_reset(COLOR_LEGACY_COMMON);
    assert(color_script_read(COLOR_COMMON, DM_RESAMPLE, -1) == DC_RESAMPLE_NEAREST);
}

/*
 * Damon V. Caskey
 * 2026-11-20
 * Exercise the complete curve call through the actual script entry points.
 * The values deliberately cover fractional focus, depth, tilt and scrolling.
 */
static const DOUBLE curve_script_values[DRAWMETHOD_CURVE_ARGUMENTS] = {
    -72.5, 90.0, 0.0, 0.35, 0.6, 4.0, -20.0, 1.0, 17.25, -3.75, 1.0
};

static void curve_script_decimal(int index, DOUBLE value)
{
    ScriptVariant_ChangeType(&color_arguments[index], VT_DECIMAL);
    color_arguments[index].dblVal = value;
}

static void curve_script_prepare_full(enum color_script_api api)
{
    int i;
    color_script_prepare(api, DM_CURVE);
    for(i = 0; i < DRAWMETHOD_CURVE_ARGUMENTS; ++i)
    {
        if(i == DM_CURVE_PROFILE - DM_CURVE_ANGLE ||
           i == DM_CURVE_WRAP - DM_CURVE_ANGLE)
        {
            color_script_integer(i + 2, (LONG)curve_script_values[i]);
        }
        else { curve_script_decimal(i + 2, curve_script_values[i]); }
    }
}

static HRESULT curve_script_get(enum color_script_api api, int count, DOUBLE *value)
{
    ScriptVariant result = {0};
    ScriptVariant *result_pointer = &result;
    HRESULT status;
    if(api == COLOR_COMMON || api == COLOR_COMMON_OBJECT)
    {
        status = openbor_get_draw_method(color_argument_ptrs, &result_pointer, count);
    }
    else if(api == COLOR_OBJECT)
    {
        status = openbor_get_drawmethod_property(
            (const ScriptVariant *const *)color_argument_ptrs, &result_pointer, count);
    }
    else
    {
        status = openbor_getdrawmethod(color_argument_ptrs, &result_pointer, count);
    }
    if(SUCCEEDED(status))
    {
        assert(result_pointer == &result);
        assert(result.vt == VT_INTEGER || result.vt == VT_DECIMAL);
        *value = result.vt == VT_INTEGER ? result.lVal : result.dblVal;
    }
    ScriptVariant_Clear(&result);
    return status;
}

static DOUBLE curve_script_read(enum color_script_api api, int property)
{
    DOUBLE value = 0;
    color_script_prepare(api, property);
    assert(curve_script_get(api, 2, &value) == S_OK);
    return value;
}

static void curve_script_snapshot(enum color_script_api api, DOUBLE *values)
{
    int property;
    for(property = DM_CURVE; property <= DM_CURVE_WRAP; ++property)
    {
        values[property - DM_CURVE] = curve_script_read(api, property);
    }
}

static void curve_script_roundtrip(enum color_script_api api)
{
    static const DOUBLE defaults[DRAWMETHOD_CURVE_ARGUMENTS] = {
        0, 0, 90, 0.5, 1, 0, 0, 0, 0, 0, 0
    };
    int i;
    color_script_reset(api);
    assert(curve_script_read(api, DM_CURVE) == 0);
    for(i = 0; i < DRAWMETHOD_CURVE_ARGUMENTS; ++i)
    {
        assert(curve_script_read(api, DM_CURVE_ANGLE + i) == defaults[i]);
    }
    curve_script_prepare_full(api);
    assert(color_script_set(api, DRAWMETHOD_CURVE_ARGUMENTS + 2) == S_OK);
    assert(curve_script_read(api, DM_CURVE_ENABLED) == 1);
    for(i = 0; i < DRAWMETHOD_CURVE_ARGUMENTS; ++i)
    {
        assert(fabs(curve_script_read(api, DM_CURVE_ANGLE + i) -
                    curve_script_values[i]) < 0.000001);
    }

    color_script_prepare(api, DM_CURVE_ENABLED);
    color_script_integer(2, 0);
    assert(color_script_set(api, 3) == S_OK);
    assert(curve_script_read(api, DM_CURVE) == 0);
    for(i = 0; i < DRAWMETHOD_CURVE_ARGUMENTS; ++i)
    {
        assert(fabs(curve_script_read(api, DM_CURVE_ANGLE + i) -
                    curve_script_values[i]) < 0.000001);
    }
    color_script_prepare(api, DM_CURVE_ENABLED);
    color_script_integer(2, 1);
    assert(color_script_set(api, 3) == S_OK);
    assert(curve_script_read(api, DM_CURVE) == 1);

    /* Individual fields share the full setter's validation and auto-enable. */
    for(i = 0; i < DRAWMETHOD_CURVE_ARGUMENTS; ++i)
    {
        color_script_prepare(api, DM_CURVE_ENABLED);
        color_script_integer(2, 0);
        assert(color_script_set(api, 3) == S_OK);
        color_script_prepare(api, DM_CURVE_ANGLE + i);
        if(i == DM_CURVE_PROFILE - DM_CURVE_ANGLE ||
           i == DM_CURVE_WRAP - DM_CURVE_ANGLE)
        {
            color_script_integer(2, (LONG)defaults[i]);
        }
        else { curve_script_decimal(2, defaults[i]); }
        assert(color_script_set(api, 3) == S_OK);
        assert(curve_script_read(api, DM_CURVE_ANGLE + i) == defaults[i]);
        assert(curve_script_read(api, DM_CURVE_ENABLED) == 1);
    }
    color_script_reset(api);
    assert(curve_script_read(api, DM_CURVE_ENABLED) == 0);

    /* The non-binary lower bound must remain valid after a script getter. */
    color_script_prepare(api, DM_CURVE_EXTENT);
    curve_script_decimal(2, 0.01);
    assert(color_script_set(api, 3) == S_OK);
    {
        DOUBLE extent = curve_script_read(api, DM_CURVE_EXTENT);
        color_script_prepare(api, DM_CURVE_EXTENT);
        curve_script_decimal(2, extent);
        assert(color_script_set(api, 3) == S_OK);
    }
    color_script_reset(api);
}

static void curve_script_rejected(enum color_script_api api, int count,
                                   const DOUBLE *before)
{
    DOUBLE after[DM_CURVE_WRAP - DM_CURVE + 1];
    s_drawmethod object_before;
    memcpy(&object_before, &color_object, sizeof(object_before));
    assert(FAILED(color_script_set(api, count)));
    assert(memcmp(&object_before, &color_object, sizeof(object_before)) == 0);
    curve_script_snapshot(api, after);
    assert(memcmp(before, after, sizeof(after)) == 0);
    assert(color_script_read(api, DM_RESAMPLE, -1) == DC_RESAMPLE_LANCZOS_3);
    ++curve_atomic_rejections;
}

static void curve_script_invalid(enum color_script_api api)
{
    static const DOUBLE invalid_low[DRAWMETHOD_CURVE_ARGUMENTS] = {
        -180.000001, -360.000001, -360.000001, -4.000001, 0.009999999,
        -0.000001, -89.000001, -1, -10000000.000001, -10000000.000001, -1
    };
    static const DOUBLE invalid_high[DRAWMETHOD_CURVE_ARGUMENTS] = {
        180.000001, 360.000001, 360.000001, 5.000001, 16.000001,
        64.000001, 89.000001, 2, 10000000.000001, 10000000.000001, 2
    };
    static const DOUBLE nonfinite[] = {NAN, INFINITY, -INFINITY};
    DOUBLE before[DM_CURVE_WRAP - DM_CURVE + 1], ignored;
    int i, n;
    color_script_reset(api);
    curve_script_prepare_full(api);
    assert(color_script_set(api, DRAWMETHOD_CURVE_ARGUMENTS + 2) == S_OK);
    color_script_prepare(api, DM_RESAMPLE);
    color_script_integer(2, DC_RESAMPLE_LANCZOS_3);
    assert(color_script_set(api, 3) == S_OK);
    curve_script_snapshot(api, before);

    for(i = 0; i < DRAWMETHOD_CURVE_ARGUMENTS; ++i)
    {
        int property = DM_CURVE_ANGLE + i;
        int integer = property == DM_CURVE_PROFILE || property == DM_CURVE_WRAP;
        for(n = 0; n < 2; ++n)
        {
            DOUBLE invalid = n ? invalid_high[i] : invalid_low[i];
            color_script_prepare(api, property);
            if(integer) { color_script_integer(2, (LONG)invalid); }
            else { curve_script_decimal(2, invalid); }
            curve_script_rejected(api, 3, before);

            /* A bad late field must not commit any earlier atomic arguments. */
            curve_script_prepare_full(api);
            curve_script_decimal(2, 123.0);
            if(integer) { color_script_integer(i + 2, (LONG)invalid); }
            else { curve_script_decimal(i + 2, invalid); }
            curve_script_rejected(api, DRAWMETHOD_CURVE_ARGUMENTS + 2, before);
        }
        for(n = 0; n < 3; ++n)
        {
            color_script_prepare(api, property);
            curve_script_decimal(2, nonfinite[n]);
            curve_script_rejected(api, 3, before);
        }
        color_script_prepare(api, property);
        color_script_string(2, "1");
        curve_script_rejected(api, 3, before);
        color_script_prepare(api, property);
        curve_script_rejected(api, 3, before); /* VT_EMPTY */
        color_script_prepare(api, property);
        color_script_pointer(2, &color_object);
        curve_script_rejected(api, 3, before);
        color_script_prepare(api, property);
        curve_script_decimal(2, 1.0);
        color_script_integer(3, 0);
        curve_script_rejected(api, 4, before);
        if(integer)
        {
            color_script_prepare(api, property);
            curve_script_decimal(2, 1.0);
            curve_script_rejected(api, 3, before);
        }
        color_script_prepare(api, property);
        color_script_integer(2, 0);
        assert(FAILED(curve_script_get(api, 3, &ignored)));
    }
    curve_script_prepare_full(api);
    curve_script_rejected(api, DRAWMETHOD_CURVE_ARGUMENTS + 1, before);
    curve_script_prepare_full(api);
    color_script_integer(DRAWMETHOD_CURVE_ARGUMENTS + 2, 0);
    curve_script_rejected(api, DRAWMETHOD_CURVE_ARGUMENTS + 3, before);
    color_script_prepare(api, DM_CURVE_ENABLED);
    color_script_integer(2, 2);
    curve_script_rejected(api, 3, before);
    color_script_prepare(api, DM_CURVE_ENABLED);
    curve_script_decimal(2, 1.0);
    curve_script_rejected(api, 3, before);
}

static void curve_script_copy_and_default(void)
{
    s_drawmethod copied = plainmethod, before;
    ScriptVariant result = {0}, *result_pointer = &result;
    unsigned api;
    DOUBLE value;
    color_script_reset(COLOR_OBJECT);
    curve_script_prepare_full(COLOR_OBJECT);
    assert(color_script_set(COLOR_OBJECT, DRAWMETHOD_CURVE_ARGUMENTS + 2) == S_OK);
    before = color_object;
    color_script_arguments();
    color_script_pointer(0, &copied);
    color_script_pointer(1, &color_object);
    assert(openbor_copy_drawmethod(color_argument_ptrs, &result_pointer, 2) == S_OK);
    assert(memcmp(&copied, &before, sizeof(copied)) == 0);
    color_script_prepare(COLOR_OBJECT, DM_CURVE_FOCUS);
    curve_script_decimal(2, 0.8);
    assert(color_script_set(COLOR_OBJECT, 3) == S_OK);
    assert(copied.curve.focus == before.curve.focus);

    before = plainmethod;
    for(api = COLOR_COMMON; api <= COLOR_LEGACY; ++api)
    {
        curve_script_prepare_full((enum color_script_api)api);
        if(api == COLOR_LEGACY) { color_owner.drawmethod = (s_drawmethod *)&plainmethod; }
        else { color_script_pointer(0, (void *)&plainmethod); }
        assert(FAILED(color_script_set((enum color_script_api)api,
                                       DRAWMETHOD_CURVE_ARGUMENTS + 2)));
        assert(memcmp(&plainmethod, &before, sizeof(before)) == 0);
    }
    color_owner.drawmethod = NULL;
    color_script_prepare(COLOR_LEGACY, DM_CURVE_ENABLED);
    color_script_integer(2, 0);
    assert(FAILED(color_script_set(COLOR_LEGACY, 3)));
    assert(FAILED(curve_script_get(COLOR_LEGACY, 2, &value)));
    color_owner.drawmethod = &color_object;
    ScriptVariant_Clear(&result);

    /* Disabling also permits recovery from an invalid direct native edit. */
    color_object.curve.extent = 0;
    color_script_prepare(COLOR_OBJECT, DM_CURVE_ENABLED);
    color_script_integer(2, 0);
    assert(color_script_set(COLOR_OBJECT, 3) == S_OK);
    color_script_prepare(COLOR_OBJECT, DM_CURVE_ENABLED);
    color_script_integer(2, 1);
    assert(FAILED(color_script_set(COLOR_OBJECT, 3)));
    color_script_reset(COLOR_OBJECT);
    assert(drawmethod_curve_valid(&color_object.curve));
}

/*
 * Damon V. Caskey
 * 2026-11-14
 * Run the same behavioral contract through all three script-facing APIs.
 * 2026-11-15: exercise all five resample access paths and fixed enum values.
 */
int main(void)
{
    unsigned api;
    assert(sizeof(void *) == 8);
    assert(DC_RESAMPLE_NEAREST == 0 && DC_RESAMPLE_BILINEAR == 1);
    assert(DC_RESAMPLE_BICUBIC_CATMULL_ROM == 2 && DC_RESAMPLE_BICUBIC_MITCHELL == 3);
    assert(DC_RESAMPLE_BICUBIC_BSPLINE == 4 && DC_RESAMPLE_LANCZOS_2 == 5);
    assert(DC_RESAMPLE_LANCZOS_3 == 6 && DC_RESAMPLE_END == 7);
    assert(DC_RESAMPLE_MOVIE == DC_RESAMPLE_BILINEAR);
    assert(DM_RESAMPLE == DRAWMETHOD_PROPERTY_RESAMPLE && DM_RESAMPLE == 45);
    assert(DM_CURVE == DM_RESAMPLE + 1);
    assert(DM_CURVE_WRAP - DM_CURVE_ANGLE + 1 == DRAWMETHOD_CURVE_ARGUMENTS);
    assert(DRAWMETHOD_CONFIG_CURVE == (1 << 7));
    assert(drawmethod_curve_valid(&plainmethod.curve));
    assert(plainmethod.resample == DC_RESAMPLE_NEAREST);
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
    for(api = COLOR_COMMON; api <= COLOR_LEGACY_COMMON; ++api)
    {
        resample_script_modes((enum color_script_api)api);
        resample_script_invalid_updates((enum color_script_api)api);
    }
    resample_script_copy();
    resample_script_object_contract();
    for(api = COLOR_COMMON; api <= COLOR_LEGACY_COMMON; ++api)
    {
        curve_script_roundtrip((enum color_script_api)api);
        curve_script_invalid((enum color_script_api)api);
    }
    curve_script_copy_and_default();
    assert(color_sorted_searches > 0 && color_atomic_rejections == 60);
    assert(resample_atomic_rejections == 95 && drawmethod_constant_checks == 68);
    assert(curve_atomic_rejections >= 600);
    color_script_arguments();
    StrCache_Clear();
    printf("drawmethod color/resample/curve script checks passed: %u constants, "
           "3 production APIs, 5 resample access paths, %u color and %u resample "
           "and %u curve atomic rejections; copy/default/reset verified; legacy tables sorted\n",
           drawmethod_constant_checks, color_atomic_rejections, resample_atomic_rejections,
           curve_atomic_rejections);
    return 0;
}
