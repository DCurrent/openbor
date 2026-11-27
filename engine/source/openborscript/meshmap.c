/*
 * OpenBOR - http://www.chronocrash.com
 * Copyright (c) OpenBOR Team. All rights reserved. See LICENSE.
 *
 * Meshmap authoring has two front ends: level-sheet commands and scripts.
 * Both use the numerical core's validating operations. In particular, script
 * access never exposes writable allocation pointers or a half-loaded image.
 */
#include "scriptcommon.h"
#include "../gamelib/meshmap.h"
#include "../gamelib/meshmap_image.h"
#include "../gamelib/meshmap_render.h"
#include "commands.h"

#include <errno.h>
#include <limits.h>
#include <math.h>
#include <stddef.h>

typedef enum meshmap_field_type
{
    MESHMAP_FIELD_INT,
    MESHMAP_FIELD_UINT,
    MESHMAP_FIELD_DOUBLE
} meshmap_field_type;

typedef struct meshmap_property_info
{
    int property;
    const char *name;
    size_t offset;
    meshmap_field_type type;
} meshmap_property_info;

/* The table follows the entity-property design, but the numerical terrain core
 * deliberately uses doubles. The common legacy VT_DECIMAL accessor reads a
 * float, so these fields need the correctly sized accessors below. */
#define MAP_FIELD(property, member, kind) \
    { property, #property, offsetof(s_meshmap_config, member), kind }
static const meshmap_property_info meshmap_config_properties[] = {
    MAP_FIELD(MESHMAP_PROPERTY_AXIS, axis, MESHMAP_FIELD_INT),
    MAP_FIELD(MESHMAP_PROPERTY_SIDE, side, MESHMAP_FIELD_INT),
    MAP_FIELD(MESHMAP_PROPERTY_ORIGIN_X, origin.x, MESHMAP_FIELD_DOUBLE),
    MAP_FIELD(MESHMAP_PROPERTY_ORIGIN_Y, origin.y, MESHMAP_FIELD_DOUBLE),
    MAP_FIELD(MESHMAP_PROPERTY_ORIGIN_Z, origin.z, MESHMAP_FIELD_DOUBLE),
    MAP_FIELD(MESHMAP_PROPERTY_MINIMUM, min_height, MESHMAP_FIELD_DOUBLE),
    MAP_FIELD(MESHMAP_PROPERTY_MAXIMUM, max_height, MESHMAP_FIELD_DOUBLE),
    MAP_FIELD(MESHMAP_PROPERTY_SPACING_U, spacing_u, MESHMAP_FIELD_DOUBLE),
    MAP_FIELD(MESHMAP_PROPERTY_SPACING_V, spacing_v, MESHMAP_FIELD_DOUBLE),
    MAP_FIELD(MESHMAP_PROPERTY_SLOPE_LIMIT, slope_limit, MESHMAP_FIELD_DOUBLE),
    MAP_FIELD(MESHMAP_PROPERTY_FLAGS, flags, MESHMAP_FIELD_UINT),
    MAP_FIELD(MESHMAP_PROPERTY_SUPPORT_GROUP, support_group, MESHMAP_FIELD_UINT),
    MAP_FIELD(MESHMAP_PROPERTY_LAYER, layer, MESHMAP_FIELD_INT)
};
#undef MAP_FIELD

/* Map and bank gates have the same field layout and contiguous property order.
 * Sharing this table also keeps their validation and numeric precision equal. */
#define GATE_FIELD(member, kind) { 0, #member, offsetof(s_meshmap_gate, member), kind }
static const meshmap_property_info meshmap_gate_properties[] = {
    GATE_FIELD(enabled, MESHMAP_FIELD_INT),
    GATE_FIELD(mode, MESHMAP_FIELD_INT),
    GATE_FIELD(bounds.min.x, MESHMAP_FIELD_DOUBLE),
    GATE_FIELD(bounds.min.y, MESHMAP_FIELD_DOUBLE),
    GATE_FIELD(bounds.min.z, MESHMAP_FIELD_DOUBLE),
    GATE_FIELD(bounds.max.x, MESHMAP_FIELD_DOUBLE),
    GATE_FIELD(bounds.max.y, MESHMAP_FIELD_DOUBLE),
    GATE_FIELD(bounds.max.z, MESHMAP_FIELD_DOUBLE)
};
#undef GATE_FIELD

static HRESULT meshmap_script_error(const char *function, const char *message)
{
    printf("\nScript error: %s: %s\n", function, message);
    return E_FAIL;
}

static void meshmap_return_int(ScriptVariant *result, int value)
{
    ScriptVariant_ChangeType(result, VT_INTEGER);
    result->lVal = value;
}

static void meshmap_return_uint64(ScriptVariant *result, uint64_t value)
{
    ScriptVariant_ChangeType(result, VT_UINTEGER64);
    result->ullVal = value;
}

static void meshmap_return_double(ScriptVariant *result, double value)
{
    ScriptVariant_ChangeType(result, VT_DECIMAL);
    result->dblVal = value;
}

static void meshmap_return_string(ScriptVariant *result, const char *value)
{
    ScriptVariant_ChangeType(result, VT_STR);
    result->strVal = StrCache_CreateNewFrom(value ? value : "");
}

static int meshmap_script_uint(ScriptVariant *argument, uint64_t maximum, uint64_t *value)
{
    /* IDs, indices and masks must arrive as integers. Passing a decimal through
     * a floating conversion could silently truncate an index or lose bit 63. */
    return argument && (argument->vt & VT_INTANY)
        && SUCCEEDED(ScriptVariant_Unsigned64Value(argument, value))
        && *value <= maximum;
}

static int meshmap_script_int(ScriptVariant *argument, int *value)
{
    int64_t number;
    if(!argument || !(argument->vt & VT_INTANY)
       || FAILED(ScriptVariant_Integer64Value(argument, &number))
       || number < INT_MIN || number > INT_MAX)
    {
        return 0;
    }
    *value = (int)number;
    return 1;
}

static int meshmap_script_number(ScriptVariant *argument, double *value)
{
    return argument && SUCCEEDED(ScriptVariant_DecimalValue(argument, value))
        && isfinite(*value);
}

static int meshmap_script_bool(ScriptVariant *argument, int *value)
{
    return meshmap_script_int(argument, value) && (*value == 0 || *value == 1);
}

static s_meshmap_store *meshmap_script_store(ScriptVariant *argument)
{
    /* Collection pointers are borrowed from the currently loaded level. Compare
     * identity before dereferencing: a handle retained after level unload must
     * fail cleanly instead of being treated as a new collection structure. */
    if(!argument || argument->vt != VT_PTR || !level
       || !level->meshmaps || argument->ptrVal != level->meshmaps)
    {
        return NULL;
    }
    return level->meshmaps;
}

static const meshmap_property_info *meshmap_config_property(int property)
{
    size_t i;
    for(i = 0; i < sizeof(meshmap_config_properties) / sizeof(meshmap_config_properties[0]); ++i)
    {
        if(meshmap_config_properties[i].property == property)
        {
            return &meshmap_config_properties[i];
        }
    }
    return NULL;
}

static void meshmap_field_get(const void *object, const meshmap_property_info *info,
                              ScriptVariant *result)
{
    const unsigned char *field = (const unsigned char *)object + info->offset;
    int signed_value;
    uint32_t unsigned_value;
    double decimal_value;
    switch(info->type)
    {
    case MESHMAP_FIELD_INT:
        memcpy(&signed_value, field, sizeof(signed_value));
        meshmap_return_int(result, signed_value);
        break;
    case MESHMAP_FIELD_UINT:
        memcpy(&unsigned_value, field, sizeof(unsigned_value));
        meshmap_return_uint64(result, unsigned_value);
        break;
    case MESHMAP_FIELD_DOUBLE:
        memcpy(&decimal_value, field, sizeof(decimal_value));
        meshmap_return_double(result, decimal_value);
        break;
    }
}

static int meshmap_field_set(void *object, const meshmap_property_info *info,
                             ScriptVariant *value)
{
    unsigned char *field = (unsigned char *)object + info->offset;
    int signed_value;
    uint64_t wide_value;
    uint32_t unsigned_value;
    double decimal_value;
    switch(info->type)
    {
    case MESHMAP_FIELD_INT:
        if(!meshmap_script_int(value, &signed_value)) return 0;
        memcpy(field, &signed_value, sizeof(signed_value));
        return 1;
    case MESHMAP_FIELD_UINT:
        if(!meshmap_script_uint(value, UINT32_MAX, &wide_value)) return 0;
        unsigned_value = (uint32_t)wide_value;
        memcpy(field, &unsigned_value, sizeof(unsigned_value));
        return 1;
    case MESHMAP_FIELD_DOUBLE:
        if(!meshmap_script_number(value, &decimal_value)) return 0;
        memcpy(field, &decimal_value, sizeof(decimal_value));
        return 1;
    }
    return 0;
}

static uint32_t meshmap_property_flag(int property)
{
    switch(property)
    {
    case MESHMAP_PROPERTY_COLLIDE: return MESHMAP_COLLIDE;
    case MESHMAP_PROPERTY_DRAW: return MESHMAP_DRAW;
    case MESHMAP_PROPERTY_WALKABLE: return MESHMAP_WALKABLE;
    case MESHMAP_PROPERTY_ONE_WAY: return MESHMAP_ONE_WAY;
    case MESHMAP_PROPERTY_DROP_THROUGH: return MESHMAP_DROP_THROUGH;
    default: return 0;
    }
}

/*
 * Read a map by its level-local ID. SAMPLE adds column and row arguments;
 * dimensions and samples remain native integers rather than image colors.
 * SURFACE_* instead adds a world XYZ probe. Native interpolation owns these
 * results so cameras, rider attitude and scenery need no second height field.
 */
HRESULT openbor_get_meshmap_property(ScriptVariant **varlist, ScriptVariant **pretvar, int paramCount)
{
    const char *function = "get_meshmap_property(collection, id, property [, column, row | world_x, world_y, world_z])";
    s_meshmap_store *store;
    const s_meshmap *map;
    const meshmap_property_info *info;
    uint64_t id, column, row;
    uint32_t flag;
    int property;
    ScriptVariant_Clear(*pretvar);
    if(paramCount < 3 || !(store = meshmap_script_store(varlist[0]))
       || !meshmap_script_uint(varlist[1], MESHMAP_ID_MAX, &id)
       || !meshmap_script_int(varlist[2], &property))
    {
        return meshmap_script_error(function, "expected a current level collection, valid map ID and property constant");
    }
    map = meshmap_get_const(store, (uint32_t)id);
    if(property == MESHMAP_PROPERTY_EXISTS)
    {
        meshmap_return_int(*pretvar, map != NULL);
        return S_OK;
    }
    if(!map) return meshmap_script_error(function, "map does not exist");
    if(property >= MESHMAP_PROPERTY_SURFACE_PRESENT && property <= MESHMAP_PROPERTY_SURFACE_TRIANGLE)
    {
        s_meshmap_vec3 point;
        s_meshmap_sample sample;
        int present;
        if(paramCount != 6 || !meshmap_script_number(varlist[3], &point.x)
           || !meshmap_script_number(varlist[4], &point.y)
           || !meshmap_script_number(varlist[5], &point.z))
            return meshmap_script_error(function, "surface properties require finite world X, Y and Z");
        /* This is a numerical resource probe, independent of entity admission
         * gates and active masks. Missing cells return PRESENT=0 and NULL for
         * other fields, keeping an actual zero-height surface distinguishable.
         * The mapped coordinate is replaced by the sampled surface position. */
        present = meshmap_sample_world(map, point, &sample);
        if(property == MESHMAP_PROPERTY_SURFACE_PRESENT)
            meshmap_return_int(*pretvar, present);
        else if(present)
        {
            switch(property)
            {
            case MESHMAP_PROPERTY_SURFACE_X: meshmap_return_double(*pretvar, sample.position.x); break;
            case MESHMAP_PROPERTY_SURFACE_Y: meshmap_return_double(*pretvar, sample.position.y); break;
            case MESHMAP_PROPERTY_SURFACE_Z: meshmap_return_double(*pretvar, sample.position.z); break;
            case MESHMAP_PROPERTY_SURFACE_NORMAL_X: meshmap_return_double(*pretvar, sample.normal.x); break;
            case MESHMAP_PROPERTY_SURFACE_NORMAL_Y: meshmap_return_double(*pretvar, sample.normal.y); break;
            case MESHMAP_PROPERTY_SURFACE_NORMAL_Z: meshmap_return_double(*pretvar, sample.normal.z); break;
            case MESHMAP_PROPERTY_SURFACE_COLUMN: meshmap_return_uint64(*pretvar, sample.column); break;
            case MESHMAP_PROPERTY_SURFACE_ROW: meshmap_return_uint64(*pretvar, sample.row); break;
            case MESHMAP_PROPERTY_SURFACE_TRIANGLE: meshmap_return_uint64(*pretvar, sample.triangle); break;
            default: break;
            }
        }
        return S_OK;
    }
    info = meshmap_config_property(property);
    if(info)
    {
        meshmap_field_get(&map->config, info, *pretvar);
        return S_OK;
    }
    if(property >= MESHMAP_PROPERTY_GATE_ENABLED && property <= MESHMAP_PROPERTY_MAX_Z)
    {
        meshmap_field_get(&map->gate, &meshmap_gate_properties[property - MESHMAP_PROPERTY_GATE_ENABLED], *pretvar);
        return S_OK;
    }
    flag = meshmap_property_flag(property);
    if(flag)
    {
        meshmap_return_int(*pretvar, (map->config.flags & flag) != 0);
        return S_OK;
    }
    switch(property)
    {
    case MESHMAP_PROPERTY_ID: meshmap_return_int(*pretvar, (int)map->id); break;
    case MESHMAP_PROPERTY_AREA: meshmap_return_int(*pretvar, meshmap_id_slot(map->id, 0)); break;
    case MESHMAP_PROPERTY_SECTOR: meshmap_return_int(*pretvar, meshmap_id_slot(map->id, 1)); break;
    case MESHMAP_PROPERTY_ROOM: meshmap_return_int(*pretvar, meshmap_id_slot(map->id, 2)); break;
    case MESHMAP_PROPERTY_INDEX: meshmap_return_int(*pretvar, meshmap_id_slot(map->id, 3)); break;
    case MESHMAP_PROPERTY_ENABLED: meshmap_return_int(*pretvar, meshmap_is_active(store, map->id)); break;
    case MESHMAP_PROPERTY_WIDTH: meshmap_return_uint64(*pretvar, map->width); break;
    case MESHMAP_PROPERTY_HEIGHT: meshmap_return_uint64(*pretvar, map->height); break;
    case MESHMAP_PROPERTY_PRESENT_CELLS: meshmap_return_uint64(*pretvar, map->present_cells); break;
    case MESHMAP_PROPERTY_REVISION: meshmap_return_uint64(*pretvar, map->revision); break;
    case MESHMAP_PROPERTY_GENERATION: meshmap_return_uint64(*pretvar, map->generation); break;
    case MESHMAP_PROPERTY_GEOMETRY: meshmap_return_string(*pretvar, map->geometry_path); break;
    case MESHMAP_PROPERTY_ARTWORK: meshmap_return_string(*pretvar, map->artwork_path); break;
    case MESHMAP_PROPERTY_SAMPLE:
        if(paramCount != 5 || !map->samples
           || !meshmap_script_uint(varlist[3], UINT32_MAX, &column)
           || !meshmap_script_uint(varlist[4], UINT32_MAX, &row)
           || column >= map->width || row >= map->height)
            return meshmap_script_error(function, "sample column or row is outside the geometry image");
        meshmap_return_int(*pretvar, map->samples[row * map->width + column]);
        break;
    case PROPERTY_ACCESS_DUMP:
        printf("\nMeshmap %u: axis=%d, size=%u x %u, present cells=%zu, revision=%llu\n",
               map->id, map->config.axis, map->width, map->height, map->present_cells,
               (unsigned long long)map->revision);
        for(size_t i = 0; i < sizeof(meshmap_config_properties) / sizeof(meshmap_config_properties[0]); ++i)
        {
            printf("  %d: %s (read/write)\n", meshmap_config_properties[i].property,
                   meshmap_config_properties[i].name);
        }
        break;
    default: return meshmap_script_error(function, "unknown meshmap property");
    }
    return S_OK;
}

/*
 * Validate edits in a temporary descriptor and publish them through the core.
 * This is why setting a spacing to zero or replacing an invalid PNG leaves the
 * previous terrain usable by both the collision and rendering paths.
 */
HRESULT openbor_set_meshmap_property(ScriptVariant **varlist, ScriptVariant **pretvar, int paramCount)
{
    const char *function = "set_meshmap_property(collection, id, property, value [, column, row])";
    s_meshmap_store *store;
    s_meshmap *map;
    s_meshmap_config config;
    s_meshmap_gate gate;
    const meshmap_property_info *info;
    uint64_t id, column, row, sample;
    uint32_t flag;
    int property, enabled, ok = 0;
    char error[512] = {0};
    ScriptVariant_Clear(*pretvar);
    if(paramCount < 4 || !(store = meshmap_script_store(varlist[0]))
       || !meshmap_script_uint(varlist[1], MESHMAP_ID_MAX, &id)
       || !meshmap_script_int(varlist[2], &property))
        return meshmap_script_error(function, "expected a current level collection, valid map ID, property and value");
    map = meshmap_get(store, (uint32_t)id);
    if(property == MESHMAP_PROPERTY_EXISTS)
    {
        if(!meshmap_script_bool(varlist[3], &enabled))
            return meshmap_script_error(function, "EXISTS expects integer 0 or 1");
        ok = enabled ? (map != NULL || meshmap_create(store, (uint32_t)id) != NULL)
                     : (map == NULL || meshmap_remove(store, (uint32_t)id));
    }
    else
    {
        if(!map) return meshmap_script_error(function, "create the map before assigning its properties");
        config = map->config;
        info = meshmap_config_property(property);
        flag = meshmap_property_flag(property);
        if(info)
        {
            ok = meshmap_field_set(&config, info, varlist[3]) && meshmap_configure(map, &config);
        }
        else if(property >= MESHMAP_PROPERTY_GATE_ENABLED && property <= MESHMAP_PROPERTY_MAX_Z)
        {
            gate = map->gate;
            ok = meshmap_field_set(&gate, &meshmap_gate_properties[property - MESHMAP_PROPERTY_GATE_ENABLED], varlist[3])
                && meshmap_set_gate(map, &gate);
        }
        else if(flag)
        {
            if(meshmap_script_bool(varlist[3], &enabled))
            {
                config.flags = enabled ? config.flags | flag : config.flags & ~flag;
                ok = meshmap_configure(map, &config);
            }
        }
        else switch(property)
        {
        case MESHMAP_PROPERTY_ENABLED:
            ok = meshmap_script_bool(varlist[3], &enabled) && meshmap_set_active(store, map->id, enabled);
            break;
        case MESHMAP_PROPERTY_SAMPLE:
            ok = paramCount == 6
                && meshmap_script_uint(varlist[3], UINT16_MAX, &sample)
                && meshmap_script_uint(varlist[4], UINT32_MAX, &column)
                && meshmap_script_uint(varlist[5], UINT32_MAX, &row)
                && meshmap_set_sample(map, (uint32_t)column, (uint32_t)row, (uint16_t)sample);
            break;
        case MESHMAP_PROPERTY_GEOMETRY:
            if(varlist[3]->vt == VT_STR)
                ok = meshmap_image_replace_geometry(map, StrCache_Get(varlist[3]->strVal), packfile, error, sizeof(error));
            break;
        case MESHMAP_PROPERTY_ARTWORK:
            if(varlist[3]->vt == VT_STR)
                ok = meshmap_image_replace_artwork(map, StrCache_Get(varlist[3]->strVal), packfile, error, sizeof(error));
            break;
        default: return meshmap_script_error(function, "property is read-only or unknown");
        }
    }
    if(!ok) return meshmap_script_error(function, error[0] ? error : "invalid value or allocation failure; previous map retained");
    meshmap_return_int(*pretvar, 1);
    return S_OK;
}

static unsigned meshmap_mask_count(uint64_t mask)
{
    unsigned count = 0;
    while(mask) { mask &= mask - 1; ++count; }
    return count;
}

HRESULT openbor_get_meshmap_bank_property(ScriptVariant **varlist, ScriptVariant **pretvar, int paramCount)
{
    const char *function = "get_meshmap_bank_property(collection, depth, prefix, property)";
    s_meshmap_store *store;
    s_meshmap_bank *bank;
    uint64_t depth, prefix;
    int property;
    ScriptVariant_Clear(*pretvar);
    if(paramCount != 4 || !(store = meshmap_script_store(varlist[0]))
       || !meshmap_script_uint(varlist[1], 3, &depth)
       || !meshmap_script_uint(varlist[2], MESHMAP_ID_MAX, &prefix)
       || !meshmap_script_int(varlist[3], &property))
        return meshmap_script_error(function, "expected collection, depth 0..3, packed prefix and property");
    bank = meshmap_bank_get(store, (unsigned)depth, (uint32_t)prefix, 0);
    if(property == MESHMAP_BANK_PROPERTY_EXISTS)
    {
        meshmap_return_int(*pretvar, bank != NULL);
        return S_OK;
    }
    if(!bank) return meshmap_script_error(function, "bank does not exist");
    if(property >= MESHMAP_BANK_PROPERTY_GATE_ENABLED && property <= MESHMAP_BANK_PROPERTY_MAX_Z)
    {
        meshmap_field_get(&bank->gate, &meshmap_gate_properties[property - MESHMAP_BANK_PROPERTY_GATE_ENABLED], *pretvar);
        return S_OK;
    }
    switch(property)
    {
    case MESHMAP_BANK_PROPERTY_DEPTH: meshmap_return_int(*pretvar, bank->depth); break;
    case MESHMAP_BANK_PROPERTY_PRESENT_MASK: meshmap_return_uint64(*pretvar, bank->present_mask); break;
    case MESHMAP_BANK_PROPERTY_ACTIVE_MASK: meshmap_return_uint64(*pretvar, bank->active_mask); break;
    case MESHMAP_BANK_PROPERTY_CHILD_COUNT: meshmap_return_int(*pretvar, meshmap_mask_count(bank->present_mask)); break;
    case PROPERTY_ACCESS_DUMP:
        printf("\nMeshmap bank depth %u: present=0x%016llx active=0x%016llx\n", bank->depth,
               (unsigned long long)bank->present_mask, (unsigned long long)bank->active_mask);
        break;
    default: return meshmap_script_error(function, "unknown bank property");
    }
    return S_OK;
}

HRESULT openbor_set_meshmap_bank_property(ScriptVariant **varlist, ScriptVariant **pretvar, int paramCount)
{
    const char *function = "set_meshmap_bank_property(collection, depth, prefix, property, value)";
    s_meshmap_store *store;
    s_meshmap_bank *bank;
    s_meshmap_gate gate;
    uint64_t depth, prefix, mask;
    int property, enabled, ok = 0;
    ScriptVariant_Clear(*pretvar);
    if(paramCount != 5 || !(store = meshmap_script_store(varlist[0]))
       || !meshmap_script_uint(varlist[1], 3, &depth)
       || !meshmap_script_uint(varlist[2], MESHMAP_ID_MAX, &prefix)
       || !meshmap_script_int(varlist[3], &property))
        return meshmap_script_error(function, "expected collection, depth 0..3, packed prefix, property and value");
    bank = meshmap_bank_get(store, (unsigned)depth, (uint32_t)prefix, 0);
    if(property == MESHMAP_BANK_PROPERTY_EXISTS)
    {
        if(!meshmap_script_bool(varlist[4], &enabled))
            return meshmap_script_error(function, "EXISTS expects integer 0 or 1");
        ok = enabled ? (bank != NULL || meshmap_bank_get(store, (unsigned)depth, (uint32_t)prefix, 1) != NULL)
                     : (!bank || meshmap_bank_remove(store, (unsigned)depth, (uint32_t)prefix));
    }
    else
    {
        if(!bank) return meshmap_script_error(function, "create the bank before assigning its properties");
        if(property == MESHMAP_BANK_PROPERTY_ACTIVE_MASK)
            ok = meshmap_script_uint(varlist[4], UINT64_MAX, &mask) && meshmap_bank_set_mask(bank, mask);
        else if(property >= MESHMAP_BANK_PROPERTY_GATE_ENABLED && property <= MESHMAP_BANK_PROPERTY_MAX_Z)
        {
            gate = bank->gate;
            ok = meshmap_field_set(&gate, &meshmap_gate_properties[property - MESHMAP_BANK_PROPERTY_GATE_ENABLED], varlist[4])
                && meshmap_bank_set_gate(bank, &gate);
        }
        else return meshmap_script_error(function, "property is read-only or unknown");
    }
    if(!ok) return meshmap_script_error(function, "invalid bank value or allocation failure; previous bank retained");
    meshmap_return_int(*pretvar, 1);
    return S_OK;
}

/*
 * A view is a three-row projection: screen-X numerator, screen-Y numerator,
 * and perspective denominator W. Native clipping and triangle submission keep
 * projected terrain out of the per-vertex script loop. Passing an ordinary
 * screen handle also lets the caller draw into an existing split-screen view.
 */
static HRESULT meshmap_script_draw(ScriptVariant **arguments, ScriptVariant **result,
                                    int count, int projected)
{
    const char *function = projected ? "draw_meshmap_projected" : "draw_meshmap";
    s_meshmap_store *store;
    s_meshmap_view view;
    s_screen *target;
    const s_drawmethod *method;
    uint64_t id;
    double offset_x = 0.0, offset_y = 0.0;
    int i;
    ScriptVariant_Clear(*result);
    if(count < 4 || !(store = meshmap_script_store(arguments[0]))
       || !meshmap_script_uint(arguments[1], MESHMAP_ID_MAX, &id)
       || arguments[2]->vt != VT_PTR || !arguments[2]->ptrVal)
        return meshmap_script_error(function, "expected collection, map ID, destination screen and drawmethod (or NULL)");
    target = arguments[2]->ptrVal;
    if(target->magic != screen_magic)
        return meshmap_script_error(function, "destination pointer is not a screen");
    if(arguments[3]->vt != VT_PTR && arguments[3]->vt != VT_EMPTY
       && !(arguments[3]->vt == VT_INTEGER && arguments[3]->lVal == 0))
        return meshmap_script_error(function, "drawmethod must be a drawmethod handle or NULL");
    method = arguments[3]->vt == VT_PTR ? arguments[3]->ptrVal : NULL;
    if(method && method->object_type != OBJECT_TYPE_DRAWMETHOD)
        return meshmap_script_error(function, "drawmethod pointer has the wrong object type");
    meshmap_view_orthographic(&view, 0.0, 0.0);
    if(projected)
    {
        if(count != 16 && count != 17)
            return meshmap_script_error(function, "expected 12 projection coefficients and optional positive near-W");
        for(i = 0; i < 12; ++i)
        {
            if(!meshmap_script_number(arguments[4 + i], &view.projection[i]))
                return meshmap_script_error(function, "projection coefficients must be finite numbers");
        }
        if(count == 17 && (!meshmap_script_number(arguments[16], &view.near_w) || view.near_w <= 0.0))
            return meshmap_script_error(function, "near-W must be a finite positive number");
    }
    else
    {
        if(count != 4 && count != 6)
            return meshmap_script_error(function, "optional screen offsets require both X and Y");
        if(count == 6 && (!meshmap_script_number(arguments[4], &offset_x)
                         || !meshmap_script_number(arguments[5], &offset_y)))
            return meshmap_script_error(function, "screen offsets must be finite numbers");
        meshmap_view_orthographic(&view, offset_x, offset_y);
    }
    if(!meshmap_get_const(store, (uint32_t)id))
        return meshmap_script_error(function, "map does not exist");
    meshmap_return_int(*result, meshmap_draw(store, (uint32_t)id, target, &view, method));
    return S_OK;
}

HRESULT openbor_draw_meshmap(ScriptVariant **varlist, ScriptVariant **pretvar, int paramCount)
{
    return meshmap_script_draw(varlist, pretvar, paramCount, 0);
}

HRESULT openbor_draw_meshmap_projected(ScriptVariant **varlist, ScriptVariant **pretvar, int paramCount)
{
    return meshmap_script_draw(varlist, pretvar, paramCount, 1);
}

static int meshmap_text_error(char *error, size_t capacity, const char *message)
{
    if(error && capacity) snprintf(error, capacity, "%s", message);
    return 0;
}

static int meshmap_text_uint(const char *text, uint64_t maximum, uint64_t *value)
{
    char *end;
    const char *digits;
    unsigned long long parsed;
    if(!text || !text[0] || text[0] == '-') return 0;
    digits = text[0] == '+' ? text + 1 : text;
    errno = 0;
    parsed = strtoull(text, &end, digits[0] == '0' && (digits[1] == 'x' || digits[1] == 'X') ? 16 : 10);
    if(errno || end == text || *end || parsed > maximum) return 0;
    *value = (uint64_t)parsed;
    return 1;
}

static int meshmap_text_int(const char *text, int *value)
{
    char *end;
    const char *digits;
    long parsed;
    if(!text || !text[0]) return 0;
    digits = text[0] == '+' || text[0] == '-' ? text + 1 : text;
    errno = 0;
    parsed = strtol(text, &end, digits[0] == '0' && (digits[1] == 'x' || digits[1] == 'X') ? 16 : 10);
    if(errno || end == text || *end || parsed < INT_MIN || parsed > INT_MAX) return 0;
    *value = (int)parsed;
    return 1;
}

static int meshmap_text_number(const char *text, double *value)
{
    char *end;
    if(!text || !text[0]) return 0;
    errno = 0;
    *value = strtod(text, &end);
    return !errno && end != text && !*end && isfinite(*value);
}

static int meshmap_text_axis(const char *text, e_meshmap_axis *axis)
{
    uint64_t number;
    if(!strcmp(text, "x") || !strcmp(text, "X")) *axis = MESHMAP_AXIS_X;
    else if(!strcmp(text, "y") || !strcmp(text, "Y")) *axis = MESHMAP_AXIS_Y;
    else if(!strcmp(text, "z") || !strcmp(text, "Z")) *axis = MESHMAP_AXIS_Z;
    else if(meshmap_text_uint(text, 2, &number)) *axis = (e_meshmap_axis)number;
    else return 0;
    return 1;
}

static int meshmap_text_address(const char *const *arguments, unsigned slots, uint32_t *id)
{
    uint64_t address[4] = {0, 0, 0, 0};
    unsigned i;
    for(i = 0; i < slots; ++i)
        if(!meshmap_text_uint(arguments[i], 63, &address[i])) return 0;
    return meshmap_pack_id((unsigned)address[0], (unsigned)address[1],
                           (unsigned)address[2], (unsigned)address[3], id);
}

static int meshmap_text_gate(const char *const *arguments, int has_mode, s_meshmap_gate *gate)
{
    uint64_t mode = MESHMAP_GATE_POSITION;
    memset(gate, 0, sizeof(*gate));
    gate->enabled = 1;
    if(!meshmap_text_number(arguments[0], &gate->bounds.min.x)
       || !meshmap_text_number(arguments[1], &gate->bounds.min.y)
       || !meshmap_text_number(arguments[2], &gate->bounds.min.z)
       || !meshmap_text_number(arguments[3], &gate->bounds.max.x)
       || !meshmap_text_number(arguments[4], &gate->bounds.max.y)
       || !meshmap_text_number(arguments[5], &gate->bounds.max.z)
       || (has_mode && !meshmap_text_uint(arguments[6], MESHMAP_GATE_BODY, &mode)))
        return 0;
    gate->mode = (e_meshmap_gate_mode)mode;
    return meshmap_gate_valid(gate);
}

/*
 * Parse complete declarations before allocating anything. Bank commands define
 * authored applicability ranges, while meshmapbounds sets the optional leaf
 * range. None of these values are inferred from movement or recomputed from an
 * image. Creating a map may allocate ancestors, but their default gates remain
 * unrestricted until the creator supplies a bank command.
 */
int meshmap_level_command(int command, size_t count, const char *const *arguments,
                          char *error, size_t error_capacity)
{
    s_meshmap_store *store;
    s_meshmap *map;
    s_meshmap_bank *bank;
    s_meshmap_config config;
    s_meshmap_gate gate;
    uint32_t id = 0;
    uint64_t value, depth = 0, prefix = 0, mask;
    unsigned address_slots = 0;
    size_t first_bound;
    int enabled;
    if(error && error_capacity) error[0] = '\0';
    if(!level || !arguments || !count)
        return meshmap_text_error(error, error_capacity, "meshmap command requires a loaded level");
    if(!level->meshmaps && !(level->meshmaps = meshmap_store_create()))
        return meshmap_text_error(error, error_capacity, "out of memory creating the meshmap collection");
    store = level->meshmaps;

    switch(command)
    {
    case CMD_LEVEL_MESHMAP:
        if((count != 12 && count != 13) || !meshmap_text_address(arguments + 1, 4, &id))
            return meshmap_text_error(error, error_capacity,
                "meshmap expects area sector room index axis x y z minimum maximum geometry_png [artwork_png]");
        meshmap_config_default(&config);
        if(!meshmap_text_axis(arguments[5], &config.axis)
           || !meshmap_text_number(arguments[6], &config.origin.x)
           || !meshmap_text_number(arguments[7], &config.origin.y)
           || !meshmap_text_number(arguments[8], &config.origin.z)
           || !meshmap_text_number(arguments[9], &config.min_height)
           || !meshmap_text_number(arguments[10], &config.max_height)
           || !meshmap_config_valid(&config))
            return meshmap_text_error(error, error_capacity, "meshmap has an invalid axis, origin or displacement range");
        if(meshmap_get(store, id))
            return meshmap_text_error(error, error_capacity, "meshmap address is already occupied");
        if(!(map = meshmap_create(store, id)))
            return meshmap_text_error(error, error_capacity, "out of memory creating the meshmap");
        if(!meshmap_configure(map, &config)
           || !meshmap_image_replace_geometry(map, arguments[11], packfile, error, error_capacity)
           || (count == 13 && strcmp(arguments[12], "none")
               && !meshmap_image_replace_artwork(map, arguments[12], packfile, error, error_capacity)))
        {
            meshmap_remove(store, id);
            if(error && error_capacity && !error[0])
                meshmap_text_error(error, error_capacity, "invalid meshmap resources");
            return 0;
        }
        return 1;

    case CMD_LEVEL_MESHMAPSPACING:
    case CMD_LEVEL_MESHMAPRULES:
    case CMD_LEVEL_MESHMAPACTIVE:
    case CMD_LEVEL_MESHMAPBOUNDS:
        if(count < 6 || !meshmap_text_address(arguments + 1, 4, &id)
           || !(map = meshmap_get(store, id)))
            return meshmap_text_error(error, error_capacity, "meshmap setting needs an existing area/sector/room/index address");
        config = map->config;
        if(command == CMD_LEVEL_MESHMAPSPACING)
        {
            if(count != 7 || !meshmap_text_number(arguments[5], &config.spacing_u)
               || !meshmap_text_number(arguments[6], &config.spacing_v)
               || !meshmap_configure(map, &config))
                return meshmap_text_error(error, error_capacity, "meshmapspacing expects positive finite U and V spacing");
            return 1;
        }
        if(command == CMD_LEVEL_MESHMAPACTIVE)
        {
            if(count != 6 || !meshmap_text_int(arguments[5], &enabled)
               || (enabled != 0 && enabled != 1) || !meshmap_set_active(store, id, enabled))
                return meshmap_text_error(error, error_capacity, "meshmapactive expects integer 0 or 1");
            return 1;
        }
        if(command == CMD_LEVEL_MESHMAPBOUNDS)
        {
            if((count != 11 && count != 12) || !meshmap_text_gate(arguments + 5, count == 12, &gate)
               || !meshmap_set_gate(map, &gate))
                return meshmap_text_error(error, error_capacity, "meshmapbounds expects min_x min_y min_z max_x max_y max_z [gate_mode]");
            return 1;
        }
        if(count < 7 || count > 10 || !meshmap_text_uint(arguments[5], MESHMAP_FLAGS_ALL, &value))
            return meshmap_text_error(error, error_capacity, "meshmaprules expects flags side [support_group] [slope_limit] [layer]");
        config.flags = (uint32_t)value;
        if(!meshmap_text_uint(arguments[6], MESHMAP_SIDE_POSITIVE, &value))
            return meshmap_text_error(error, error_capacity, "meshmaprules side must be 0 (negative) or 1 (positive)");
        config.side = (e_meshmap_side)value;
        if(count > 7)
        {
            if(!meshmap_text_uint(arguments[7], UINT32_MAX, &value))
                return meshmap_text_error(error, error_capacity, "invalid meshmap support group");
            config.support_group = (uint32_t)value;
        }
        if((count > 8 && !meshmap_text_number(arguments[8], &config.slope_limit))
           || (count > 9 && !meshmap_text_int(arguments[9], &config.layer))
           || !meshmap_configure(map, &config))
            return meshmap_text_error(error, error_capacity, "invalid meshmap collision/rendering rule value");
        return 1;

    case CMD_LEVEL_MESHMAPAREA: address_slots = 1; break;
    case CMD_LEVEL_MESHMAPSECTOR: address_slots = 2; break;
    case CMD_LEVEL_MESHMAPROOM: address_slots = 3; break;
    case CMD_LEVEL_MESHMAPBANK:
        if((count != 9 && count != 10)
           || !meshmap_text_uint(arguments[1], 3, &depth)
           || !meshmap_text_uint(arguments[2], MESHMAP_ID_MAX, &prefix)
           || !meshmap_text_gate(arguments + 3, count == 10, &gate))
            return meshmap_text_error(error, error_capacity, "meshmapbank expects depth packed_prefix min_x min_y min_z max_x max_y max_z [gate_mode]");
        bank = meshmap_bank_get(store, (unsigned)depth, (uint32_t)prefix, 1);
        if(!bank || !meshmap_bank_set_gate(bank, &gate))
            return meshmap_text_error(error, error_capacity, "could not configure meshmap bank");
        return 1;
    case CMD_LEVEL_MESHMAPMASK:
        if(count != 4 || !meshmap_text_uint(arguments[1], 3, &depth)
           || !meshmap_text_uint(arguments[2], MESHMAP_ID_MAX, &prefix)
           || !meshmap_text_uint(arguments[3], UINT64_MAX, &mask)
           || !(bank = meshmap_bank_get(store, (unsigned)depth, (uint32_t)prefix, 0))
           || !meshmap_bank_set_mask(bank, mask))
            return meshmap_text_error(error, error_capacity, "meshmapmask expects an existing depth/prefix and a 64-bit mask containing only existing children");
        return 1;
    default:
        return meshmap_text_error(error, error_capacity, "unknown meshmap command");
    }

    first_bound = 1 + address_slots;
    if((count != first_bound + 6 && count != first_bound + 7)
       || !meshmap_text_address(arguments + 1, address_slots, &id)
       || !meshmap_text_gate(arguments + first_bound, count == first_bound + 7, &gate)
       || !(bank = meshmap_bank_get(store, address_slots, id, 1))
       || !meshmap_bank_set_gate(bank, &gate))
        return meshmap_text_error(error, error_capacity, "bank command expects its address followed by min_x min_y min_z max_x max_y max_z [gate_mode]");
    return 1;
}
