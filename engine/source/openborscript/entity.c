/*
 * OpenBOR - http://www.chronocrash.com
 * -----------------------------------------------------------------------
 * All rights reserved. See LICENSE in OpenBOR root for license details.
 *
 * Copyright (c) 2004 - 2018 OpenBOR Team
 */

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
        "boomerang_loop",
        "boss",
        "charging",
        "colorset_default",
        "colorset_dying_1",
        "colorset_dying_2",
        "colorset_table",
        "colorset_time",
        "position_alternate_base",
        "position_base",
        "position_direction",
        "position_x",
        "position_y",
        "position_z",
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
    MAPSTRINGS(varlist[ARG_PROPERTY], proplist, _entity_the_end,
               "Property name '%s' is not supported by entities.\n");


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
        case _entity_animation_animating:

            ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
            (*pretvar)->lVal = (LONG)handle->animating;

            break;

        case _entity_animation_animation:

            ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
            (*pretvar)->lVal = (LONG)handle->animnum;

            break;

        case _entity_animation_collection:

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

        case _entity_animation_frame:

            ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
            (*pretvar)->lVal = (LONG)handle->animpos;

            break;

        case _entity_arrow_on:

            ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
            (*pretvar)->lVal = (LONG)handle->arrowon;

            break;

        case _entity_attacking:

            ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
            (*pretvar)->lVal = (LONG)handle->attacking;

            break;

        case _entity_attack_id_incoming:

            ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
            (*pretvar)->lVal = (LONG)handle->attack_id_incoming;

            break;

        case _entity_attack_id_outgoing:

            ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
            (*pretvar)->lVal = (LONG)handle->attack_id_outgoing;

            break;

        case _entity_autokill:

            ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
            (*pretvar)->lVal = (LONG)handle->autokill;

            break;

        case _entity_binding:

            ScriptVariant_ChangeType(*pretvar, VT_PTR);
            (*pretvar)->ptrVal = (VOID *)&ent->binding;

            break;

        case _entity_blink:

            ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
            (*pretvar)->lVal = (LONG)handle->blink;

            break;

        case _entity_blocking:

            ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
            (*pretvar)->lVal = (LONG)handle->blocking;

            break;

        case _entity_boomerang_loop:

            ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
            (*pretvar)->lVal = (LONG)handle->boomerang_loop;

            break;

        case _entity_boss:

            ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
            (*pretvar)->lVal = (LONG)handle->boss;

            break;

        case _entity_charging:

            ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
            (*pretvar)->lVal = (LONG)handle->charging;

            break;

        case _entity_colorset_default:

            ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
            (*pretvar)->lVal = (LONG)handle->map;

            break;

        case _entity_colorset_dying_1:

            ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
            (*pretvar)->lVal = (LONG)handle->dying;

            break;

        case _entity_colorset_dying_2:

            ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
            (*pretvar)->lVal = (LONG)handle->dying2;

            break;

        case _entity_colorset_table:

            ScriptVariant_ChangeType(*pretvar, VT_PTR);
            (*pretvar)->ptrVal = (VOID *)(ent->colourmap);

            break;

        case _entity_colorset_time:

            ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
            (*pretvar)->lVal = (LONG)handle->maptime;

            break;

        case _entity_position_alternate_base:

            ScriptVariant_ChangeType(*pretvar, VT_DECIMAL);
            (*pretvar)->dblVal = (DOUBLE)handle->altbase;

            break;

        case _entity_position_base:

            ScriptVariant_ChangeType(*pretvar, VT_DECIMAL);
            (*pretvar)->dblVal = (DOUBLE)handle->base;

            break;

        case _entity_position_direction:

            ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
            (*pretvar)->lVal = (LONG)handle->direction;

            break;

        case _entity_position_x:

            ScriptVariant_ChangeType(*pretvar, VT_DECIMAL);
            (*pretvar)->dblVal = (DOUBLE)handle->position.x;

            break;

        case _entity_position_y:

            ScriptVariant_ChangeType(*pretvar, VT_DECIMAL);
            (*pretvar)->dblVal = (DOUBLE)handle->position.y;

            break;

        case _entity_position_z:

            ScriptVariant_ChangeType(*pretvar, VT_DECIMAL);
            (*pretvar)->dblVal = (DOUBLE)handle->position.z;

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
    int temp_int;
    int temp_float;

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

        case _entity_animation_animating:

            if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
            {
                handle->animating = temp_int;
            }

            break;

        case _entity_animation_animation:

            if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
            {
                handle->animnum = temp_int;
            }

            break;

        case _entity_animation_collection:

            handle->ent = (s_anim **)varlist[ARG_VALUE]->ptrVal;

            break;

        case _entity_animation_frame:

            if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
            {
                handle->animpos = temp_int;
            }

            break;

        case _entity_arrow_on:

            if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
            {
                handle->arrowon = temp_int;
            }

            break;

        case _entity_attacking:

            if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
            {
                handle->attacking = temp_int;
            }

            break;

        case _entity_attack_id_incoming:

            if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
            {
                handle->attack_id_incoming = temp_int;
            }

            break;

        case _entity_attack_id_outgoing:

            if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
            {
                handle->attack_id_outgoing = temp_int;
            }

            break;

        case _entity_autokill:

            if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
            {
                handle->autokill = temp_int;
            }

            break;

        case _entity_binding:

            // Read only.

            break;

        case _entity_blink:

            if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
            {
                handle->blink = temp_int;
            }

            break;

        case _entity_blocking:

            if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
            {
                handle->blocking = temp_int;
            }

            break;

        case _entity_boomerang_loop:

            if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
            {
                handle->boomerang_loop = temp_int;
            }

            break;

        case _entity_boss:

            if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
            {
                handle->boss = temp_int;
            }

            break;

        case _entity_charging:

            if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
            {
                handle->charging = temp_int;
            }

            break;

        case _entity_colorset_default:

            if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
            {
                handle->map = temp_int;
            }

            break;

        case _entity_colorset_dying_1:

            if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
            {
                handle->dying = temp_int;
            }

            break;

        case _entity_colorset_dying_2:

            if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
            {
                handle->dying2 = temp_int;
            }

            break;

        case _entity_colorset_table:

            // Read only.

            break;

        case _entity_position_alternate_base:

            if(SUCCEEDED(ScriptVariant_DecimalValue(varlist[ARG_VALUE], &temp_float)))
            {
                handle->altbase = temp_float;
            }

            break;

        case _entity_position_base:

            if(SUCCEEDED(ScriptVariant_DecimalValue(varlist[ARG_VALUE], &temp_float)))
            {
                handle->base = temp_float;
            }

            break;

        case _entity_position_direction:

            if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
            {
                handle->direction = temp_int;
            }

            break;

        case _entity_position_x:

            if(SUCCEEDED(ScriptVariant_DecimalValue(varlist[ARG_VALUE], &temp_float)))
            {
                handle->position.x = temp_float;
            }

            break;

        case _entity_position_y:

            if(SUCCEEDED(ScriptVariant_DecimalValue(varlist[ARG_VALUE], &temp_float)))
            {
                handle->position.y = temp_float;
            }

            break;

        case _entity_position_z:

            if(SUCCEEDED(ScriptVariant_DecimalValue(varlist[ARG_VALUE], &temp_float)))
            {
                handle->position.z = temp_float;
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
