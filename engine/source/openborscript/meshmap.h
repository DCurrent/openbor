/*
 * OpenBOR - http://www.chronocrash.com
 * Copyright (c) OpenBOR Team. All rights reserved. See LICENSE.
 *
 * Script-facing terrain properties. A collection is obtained from the level;
 * individual maps are addressed by ID so removing a map cannot leave a script
 * holding a borrowed pointer to the released map record.
 */
#ifndef OPENBOR_SCRIPT_MESHMAP_H
#define OPENBOR_SCRIPT_MESHMAP_H

#include <stddef.h>
#include "../gamelib/meshmap.h"

typedef enum e_meshmap_properties
{
    MESHMAP_PROPERTY_EXISTS,
    MESHMAP_PROPERTY_ID,
    MESHMAP_PROPERTY_AREA,
    MESHMAP_PROPERTY_SECTOR,
    MESHMAP_PROPERTY_ROOM,
    MESHMAP_PROPERTY_INDEX,
    MESHMAP_PROPERTY_ENABLED,
    MESHMAP_PROPERTY_AXIS,
    MESHMAP_PROPERTY_SIDE,
    MESHMAP_PROPERTY_ORIGIN_X,
    MESHMAP_PROPERTY_ORIGIN_Y,
    MESHMAP_PROPERTY_ORIGIN_Z,
    MESHMAP_PROPERTY_MINIMUM,
    MESHMAP_PROPERTY_MAXIMUM,
    MESHMAP_PROPERTY_SPACING_U,
    MESHMAP_PROPERTY_SPACING_V,
    MESHMAP_PROPERTY_SLOPE_LIMIT,
    MESHMAP_PROPERTY_FLAGS,
    MESHMAP_PROPERTY_SUPPORT_GROUP,
    MESHMAP_PROPERTY_LAYER,
    MESHMAP_PROPERTY_COLLIDE,
    MESHMAP_PROPERTY_DRAW,
    MESHMAP_PROPERTY_WALKABLE,
    MESHMAP_PROPERTY_ONE_WAY,
    MESHMAP_PROPERTY_DROP_THROUGH,
    MESHMAP_PROPERTY_WIDTH,
    MESHMAP_PROPERTY_HEIGHT,
    MESHMAP_PROPERTY_SAMPLE,
    MESHMAP_PROPERTY_GEOMETRY,
    MESHMAP_PROPERTY_ARTWORK,
    MESHMAP_PROPERTY_REVISION,
    MESHMAP_PROPERTY_GENERATION,
    MESHMAP_PROPERTY_PRESENT_CELLS,
    MESHMAP_PROPERTY_GATE_ENABLED,
    MESHMAP_PROPERTY_GATE_MODE,
    MESHMAP_PROPERTY_MIN_X,
    MESHMAP_PROPERTY_MIN_Y,
    MESHMAP_PROPERTY_MIN_Z,
    MESHMAP_PROPERTY_MAX_X,
    MESHMAP_PROPERTY_MAX_Y,
    MESHMAP_PROPERTY_MAX_Z,
    MESHMAP_PROPERTY_END
} e_meshmap_properties;

typedef enum e_meshmap_bank_properties
{
    MESHMAP_BANK_PROPERTY_EXISTS,
    MESHMAP_BANK_PROPERTY_DEPTH,
    MESHMAP_BANK_PROPERTY_PRESENT_MASK,
    MESHMAP_BANK_PROPERTY_ACTIVE_MASK,
    MESHMAP_BANK_PROPERTY_CHILD_COUNT,
    MESHMAP_BANK_PROPERTY_GATE_ENABLED,
    MESHMAP_BANK_PROPERTY_GATE_MODE,
    MESHMAP_BANK_PROPERTY_MIN_X,
    MESHMAP_BANK_PROPERTY_MIN_Y,
    MESHMAP_BANK_PROPERTY_MIN_Z,
    MESHMAP_BANK_PROPERTY_MAX_X,
    MESHMAP_BANK_PROPERTY_MAX_Y,
    MESHMAP_BANK_PROPERTY_MAX_Z,
    MESHMAP_BANK_PROPERTY_END
} e_meshmap_bank_properties;

HRESULT openbor_get_meshmap_property(ScriptVariant **varlist, ScriptVariant **pretvar, int paramCount);
HRESULT openbor_set_meshmap_property(ScriptVariant **varlist, ScriptVariant **pretvar, int paramCount);
HRESULT openbor_get_meshmap_bank_property(ScriptVariant **varlist, ScriptVariant **pretvar, int paramCount);
HRESULT openbor_set_meshmap_bank_property(ScriptVariant **varlist, ScriptVariant **pretvar, int paramCount);
HRESULT openbor_draw_meshmap(ScriptVariant **varlist, ScriptVariant **pretvar, int paramCount);
HRESULT openbor_draw_meshmap_projected(ScriptVariant **varlist, ScriptVariant **pretvar, int paramCount);

/* The native loader and script property setters both reach the same core
 * constructors/validators. The loader supplies its tokens directly; this
 * function reports a useful error for load_level() to attach to its line. */
int meshmap_level_command(int command, size_t count, const char *const *arguments,
                          char *error, size_t error_capacity);

#endif
