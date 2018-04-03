/*
 * OpenBOR - http://www.chronocrash.com
 * -----------------------------------------------------------------------
 * All rights reserved. See LICENSE in OpenBOR root for license details.
 *
 * Copyright (c) 2004 - 2018 OpenBOR Team
 */

// Entity Properties
// 2018-04-02
// Caskey, Damon V.

typedef enum
{
    _entity_animation_animating,
    _entity_animation_animation,
    _entity_animation_collection,
    _entity_animation_frame,
    _entity_arrow_on,
    _entity_attacking,
    _entity_attack_id,
    _entity_position_alternate_base,
    _entity_position_base,
    _entity_position_direction,
    _entity_position_x,
    _entity_position_y,
    _entity_position_z,
    _entity_the_end,

} e_entity_properties;

HRESULT openbor_get_entity_property(ScriptVariant **varlist, ScriptVariant **pretvar, int paramCount);
HRESULT openbor_set_entity_property(ScriptVariant **varlist, ScriptVariant **pretvar, int paramCount);

int mapstrings_entity_property(ScriptVariant **varlist, int paramCount);
