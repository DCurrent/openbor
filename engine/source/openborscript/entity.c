/*
 * OpenBOR - http://www.chronocrash.com
 * -----------------------------------------------------------------------
 * All rights reserved. See LICENSE in OpenBOR root for license details.
 *
 * Copyright (c) 2004 - 2018 OpenBOR Team
 */

 #include "scriptcommon.h"

// Use string property argument to find an
// integer property constant and populate
// varlist->lval.
int mapstrings_entity_property(ScriptVariant **varlist, int paramCount)
{
    #define ARG_MINIMUM     2   // Minimum number of arguments allowed in varlist.
    #define ARG_PROPERTY    1   // Varlist element carrying which property is requested.

    char *propname = NULL;  // Placeholder for string property name from varlist.
    int prop;               // Placeholder for integer constant located by string.

    static const char *proplist[] =
    {
        "ai_target_entity",
        "animation_animating",
        "animation_animation",
        "animation_collection",
        "animation_frame",
        "arrow_on",
        "attacking",
        "attack_id_incoming",
        "attack_id_outgoing",
        "autokill",
        "binding",
        "blink",
        "blocking",
        "boss",
        "charging",
        "colorset_default",
        "colorset_dying_health_1",
        "colorset_dying_health_2",
        "colorset_dying_index_1",
        "colorset_dying_index_2",
        "colorset_table",
        "colorset_time",
        "combo_step",
        "combo_time",
        "damage_on_landing",
        "deduct_ammo",
        "energy_status",
        "player_index",
        "position_base_alternate",
        "position_base_default",
        "position_coordinates",
        "position_direction",
        "projectile_prime",
        "spawn_type",
    };

    // If the minimum argument count
    // was not passed, then there is
    // nothing to map. Return true - we'll
    // catch the mistake in property access
    // functions.
    if(paramCount < ARG_MINIMUM)
    {
        return 1;
    }

    // See macro - will return 0 on fail.
    MAPSTRINGS(varlist[ARG_PROPERTY], proplist, _ENTITY_END,
               "\n\n Error: '%s' is not a known entity property.\n");


    // If we made it this far everything should be OK.
    return 1;

    #undef ARG_MINIMUM
    #undef ARG_PROPERTY
}


// Caskey, Damon  V.
// 2018-04-02
//
// Return an entity property. Requires
// an entity handle and property name to
// access.
HRESULT openbor_get_entity_property(ScriptVariant **varlist , ScriptVariant **pretvar, int paramCount)
{
    #define SELF_NAME       "openbor_get_entity_property(void handle, char property)"
    #define ARG_MINIMUM     2   // Minimum required arguments.
    #define ARG_HANDLE      0   // Handle (pointer to property structure).
    #define ARG_PROPERTY    1   // Property to access.

    entity                  *handle     = NULL; // Property handle.
    e_entity_properties    property    = 0;    // Property argument.

    // Clear pass by reference argument used to send
    // property data back to calling script.     .
    ScriptVariant_Clear(*pretvar);

    // Map string property name to a
    // matching integer constant.
    mapstrings_binding(varlist, paramCount);

    // Verify arguments. There should at least
    // be a pointer for the property handle and an integer
    // to determine which property constant is accessed.
    if(paramCount < ARG_MINIMUM
       || varlist[ARG_HANDLE]->vt != VT_PTR
       || varlist[ARG_PROPERTY]->vt != VT_INTEGER)
    {
        *pretvar = NULL;
        goto error_local;
    }
    else
    {
        // Populate local vars for readability.
        handle      = (entity *)varlist[ARG_HANDLE]->ptrVal;
        property    = (LONG)varlist[ARG_PROPERTY]->lVal;
    }

    switch(property)
    {
        case _ENTITY_AI_TARGET_ENTITY:

            if(handle->custom_target)
            {
                ScriptVariant_ChangeType(*pretvar, VT_PTR);
                (*pretvar)->ptrVal = (VOID *)handle->custom_target;
            }

            break;

        case _ENTITY_ANIMATION_ANIMATING:

            ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
            (*pretvar)->lVal = (LONG)handle->animating;

            break;

        case _ENTITY_ANIMATION_ANIMATION:

            ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
            (*pretvar)->lVal = (LONG)handle->animnum;

            break;

        case _ENTITY_ANIMATION_COLLECTION:

            // Verify entity has an animation collection
            // before getting the pointer. All entities
            // should, but there might be rare special
            // cases depending on future development.
            if(handle->animation)
            {
                ScriptVariant_ChangeType(*pretvar, VT_PTR);
                (*pretvar)->ptrVal = (VOID *)handle->animation;
            }

            break;

        case _ENTITY_ANIMATION_FRAME:

            ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
            (*pretvar)->lVal = (LONG)handle->animpos;

            break;

        case _ENTITY_ARROW_ON:

            ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
            (*pretvar)->lVal = (LONG)handle->arrowon;

            break;

        case _ENTITY_ATTACKING:

            ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
            (*pretvar)->lVal = (LONG)handle->attacking;

            break;

        case _ENTITY_ATTACK_ID_INCOMING:

            ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
            (*pretvar)->lVal = (LONG)handle->attack_id_incoming;

            break;

        case _ENTITY_ATTACK_ID_OUTGOING:

            ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
            (*pretvar)->lVal = (LONG)handle->attack_id_outgoing;

            break;

        case _ENTITY_AUTOKILL:

            ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
            (*pretvar)->lVal = (LONG)handle->autokill;

            break;

        case _ENTITY_BINDING:

            ScriptVariant_ChangeType(*pretvar, VT_PTR);
            (*pretvar)->ptrVal = (VOID *)&handle->binding;

            break;

        case _ENTITY_BLINK:

            ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
            (*pretvar)->lVal = (LONG)handle->blink;

            break;

        case _ENTITY_BLOCKING:

            ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
            (*pretvar)->lVal = (LONG)handle->blocking;

            break;

        case _ENTITY_BOSS:

            ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
            (*pretvar)->lVal = (LONG)handle->boss;

            break;

        case _ENTITY_CHARGING:

            ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
            (*pretvar)->lVal = (LONG)handle->charging;

            break;

        case _ENTITY_COLORSET_DEFAULT:

            ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
            (*pretvar)->lVal = (LONG)handle->map;

            break;

        case _ENTITY_COLORSET_DYING_HEALTH_1:

            ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
            (*pretvar)->lVal = (LONG)handle->per1;

            break;

        case _ENTITY_COLORSET_DYING_HEALTH_2:

            ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
            (*pretvar)->lVal = (LONG)handle->per2;

            break;

        case _ENTITY_COLORSET_DYING_INDEX_1:

            ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
            (*pretvar)->lVal = (LONG)handle->dying;

            break;

        case _ENTITY_COLORSET_DYING_INDEX_2:

            ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
            (*pretvar)->lVal = (LONG)handle->dying2;

            break;

        case _ENTITY_COLORSET_TABLE:

            ScriptVariant_ChangeType(*pretvar, VT_PTR);
            (*pretvar)->ptrVal = (VOID *)(handle->colourmap);

            break;

        case _ENTITY_COLORSET_TIME:

            ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
            (*pretvar)->lVal = (LONG)handle->maptime;

            break;

        case _ENTITY_COMBO_STEP:

            ScriptVariant_ChangeType(*pretvar, VT_PTR);
            (*pretvar)->ptrVal = (VOID *)&handle->combostep;

            break;

        case _ENTITY_COMBO_TIME:

            ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
            (*pretvar)->lVal = (LONG)handle->combotime;

            break;

        case _ENTITY_DAMAGE_ON_LANDING:

            ScriptVariant_ChangeType(*pretvar, VT_PTR);
            (*pretvar)->ptrVal = (VOID *)&handle->damage_on_landing;

            break;

        case _ENTITY_DEDUCT_AMMO:

            ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
            (*pretvar)->lVal = (LONG)handle->deduct_ammo;

            break;

        case _ENTITY_ENERGY_STATUS:

            ScriptVariant_ChangeType(*pretvar, VT_PTR);
            (*pretvar)->ptrVal = (VOID *)&handle->energy_status;

            break;

        case _ENTITY_PLAYER_INDEX:

            ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
            (*pretvar)->lVal = (LONG)handle->playerindex;

            break;

        case _ENTITY_POSITION_BASE_ALTERNATE:

            ScriptVariant_ChangeType(*pretvar, VT_DECIMAL);
            (*pretvar)->dblVal = (DOUBLE)handle->altbase;

            break;

        case _ENTITY_POSITION_BASE_DEFAULT:

            ScriptVariant_ChangeType(*pretvar, VT_DECIMAL);
            (*pretvar)->dblVal = (DOUBLE)handle->base;

            break;

        case _ENTITY_POSITION_COORDINATES:

            ScriptVariant_ChangeType(*pretvar, VT_PTR);
            (*pretvar)->ptrVal = (VOID *)&handle->position;

            break;

        case _ENTITY_POSITION_DIRECTION:

            ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
            (*pretvar)->lVal = (LONG)handle->direction;

            break;

        case _ENTITY_PROJECTILE_PRIME:

            ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
            (*pretvar)->lVal = (LONG)handle->projectile_prime;

            break;

        case _ENTITY_SPAWN_TYPE:

            ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
            (*pretvar)->lVal = (LONG)handle->spawntype;

            break;

        default:

            printf("Unsupported property.\n");
            goto error_local;

            break;
    }

    return S_OK;

    error_local:

    printf("You must provide a valid handle and property name: " SELF_NAME "\n");
    *pretvar = NULL;

    return E_FAIL;

    #undef SELF_NAME
    #undef ARG_MINIMUM
    #undef ARG_HANDLE
    #undef ARG_INDEX
}

// Caskey, Damon  V.
// 2018-04-03
//
// Mutate an entity property. Requires
// the entity handle, a string property
// name, and new value.
HRESULT openbor_set_entity_property(ScriptVariant **varlist, ScriptVariant **pretvar, int paramCount)
{
    #define SELF_NAME           "openbor_set_entity_property(void handle, char property, value)"
    #define ARG_MINIMUM         3   // Minimum required arguments.
    #define ARG_HANDLE          0   // Handle (pointer to property structure).
    #define ARG_PROPERTY        1   // Property to access.
    #define ARG_VALUE           2   // New value to apply.

    int                 result      = S_OK; // Success or error?
    entity              *handle     = NULL; // Property handle.
    e_entity_properties property    = 0;    // Property to access.

    // Value carriers to apply on properties after
    // taken from argument.
    LONG    temp_int;
    DOUBLE  temp_float;

    // Map string property name to a
    // matching integer constant.
    mapstrings_binding(varlist, paramCount);

    // Verify incoming arguments. There should at least
    // be a pointer for the property handle and an integer
    // to determine which property is accessed.
    if(paramCount < ARG_MINIMUM
       || varlist[ARG_HANDLE]->vt != VT_PTR
       || varlist[ARG_PROPERTY]->vt != VT_INTEGER)
    {
        *pretvar = NULL;
        goto error_local;
    }

    // Populate local handle and property vars.
    handle      = (entity *)varlist[ARG_HANDLE]->ptrVal;
    property    = (LONG)varlist[ARG_PROPERTY]->lVal;

    // Which property to modify?
    switch(property)
    {

        case _ENTITY_AI_TARGET_ENTITY:

            handle->custom_target = (entity *)varlist[ARG_VALUE]->ptrVal;

            break;

        case _ENTITY_ANIMATION_ANIMATING:

            if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
            {
                handle->animating = temp_int;
            }

            break;

        case _ENTITY_ANIMATION_ANIMATION:

            if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
            {
                handle->animnum = temp_int;
            }

            break;

        case _ENTITY_ANIMATION_COLLECTION:

            //handle->animation = (s_anim **)varlist[ARG_VALUE]->ptrVal;

            break;

        case _ENTITY_ANIMATION_FRAME:

            if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
            {
                handle->animpos = temp_int;
            }

            break;

        case _ENTITY_ARROW_ON:

            if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
            {
                handle->arrowon = temp_int;
            }

            break;

        case _ENTITY_ATTACKING:

            if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
            {
                handle->attacking = temp_int;
            }

            break;

        case _ENTITY_ATTACK_ID_INCOMING:

            if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
            {
                handle->attack_id_incoming = temp_int;
            }

            break;

        case _ENTITY_ATTACK_ID_OUTGOING:

            if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
            {
                handle->attack_id_outgoing = temp_int;
            }

            break;

        case _ENTITY_AUTOKILL:

            if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
            {
                handle->autokill = temp_int;
            }

            break;

        case _ENTITY_BINDING:

            // Read only.

            break;

        case _ENTITY_BLINK:

            if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
            {
                handle->blink = temp_int;
            }

            break;

        case _ENTITY_BLOCKING:

            if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
            {
                handle->blocking = temp_int;
            }

            break;

        case _ENTITY_BOSS:

            if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
            {
                handle->boss = temp_int;
            }

            break;

        case _ENTITY_CHARGING:

            if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
            {
                handle->charging = temp_int;
            }

            break;

        case _ENTITY_COLORSET_DEFAULT:

            if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
            {
                handle->map = temp_int;
            }

            break;

        case _ENTITY_COLORSET_DYING_HEALTH_1:

            if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
            {
                handle->per1 = temp_int;
            }

            break;

        case _ENTITY_COLORSET_DYING_HEALTH_2:

            if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
            {
                handle->per2 = temp_int;
            }

            break;

        case _ENTITY_COLORSET_DYING_INDEX_1:

            if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
            {
                handle->dying = temp_int;
            }

            break;

        case _ENTITY_COLORSET_DYING_INDEX_2:

            if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
            {
                handle->dying2 = temp_int;
            }

            break;

        case _ENTITY_COLORSET_TABLE:

            // Read only.

            break;

        case _ENTITY_COLORSET_TIME:

            if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
            {
                handle->maptime = temp_int;
            }

            break;

        case _ENTITY_COMBO_STEP:

            // Read only.

            break;

        case _ENTITY_COMBO_TIME:

            if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
            {
                handle->combotime = temp_int;
            }

            break;

        case _ENTITY_DAMAGE_ON_LANDING:

            // Read only.

            break;

        case _ENTITY_DEDUCT_AMMO:

            if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
            {
                handle->deduct_ammo = temp_int;
            }

            break;

        case _ENTITY_ENERGY_STATUS:

            // Read only.

            break;

        case _ENTITY_PLAYER_INDEX:

            if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
            {
                handle->playerindex = temp_int;
            }

            break;

        case _ENTITY_POSITION_BASE_ALTERNATE:

            if(SUCCEEDED(ScriptVariant_DecimalValue(varlist[ARG_VALUE], &temp_float)))
            {
                handle->altbase = temp_float;
            }

            break;

        case _ENTITY_POSITION_BASE_DEFAULT:

            if(SUCCEEDED(ScriptVariant_DecimalValue(varlist[ARG_VALUE], &temp_float)))
            {
                handle->base = temp_float;
            }

            break;

        case _ENTITY_POSITION_COORDINATES:

            // Read only.

            break;

        case _ENTITY_POSITION_DIRECTION:

            if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
            {
                handle->direction = temp_int;
            }

            break;

        case _ENTITY_PROJECTILE_PRIME:

            if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
            {
                handle->projectile_prime = temp_int;
            }

            break;

        case _ENTITY_SPAWN_TYPE:

            if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
            {
                handle->spawntype = temp_int;
            }

            break;

        default:

            printf("Unsupported property.\n");
            goto error_local;

            break;
    }

    return result;

    // Error trapping.
    error_local:

    printf("You must provide a valid handle, property, and new value: " SELF_NAME "\n");

    result = E_FAIL;
    return result;

    #undef SELF_NAME
    #undef ARG_MINIMUM
    #undef ARG_HANDLE
    #undef ARG_PROPERTY
    #undef ARG_VALUE
}
