/*
 * OpenBOR - http://www.chronocrash.com
 * -----------------------------------------------------------------------
 * All rights reserved. See LICENSE in OpenBOR root for license details.
 *
 * Copyright (c)  OpenBOR Team
 */

// Attack Properties
// 2017-04-26
// Caskey, Damon V.
//
// Access to attack and attack collision properties.

#include "scriptcommon.h"

// Attack specific properties.
// Caskey, Damon V.
// 2016-10-20

// get_attack_collection(void handle, int frame)
//
// Get item handle for an animation frame. When
// multiple items per frame support is implemented,
// this will return the collection of items. It is therefore
// imperative this function NOT be used to access a single
// item handle directly. You should instead use
// get<item>instance(), and pass it the result from
// this function.
HRESULT openbor_get_attack_collection(ScriptVariant **varlist, ScriptVariant **pretvar, int paramCount)
{
    #define SELF_NAME       "get_attack_collection(void handle, int frame)"
    #define ARG_MINIMUM     2   // Minimum required arguments.
    #define ARG_OBJECT      0   // Handle (pointer to property structure).
    #define ARG_FRAME       1   // Frame to access.


    int         result      = S_OK;     // Success or error?
    s_attack    **handle     = NULL;    // Property handle.
    int         frame       = 0;        // Property argument.

    // Clear pass by reference argument used to send
    // property data back to calling script.     .
    ScriptVariant_Clear(*pretvar);

    // Verify incoming arguments. There should at least
    // be a pointer for the property handle and an integer
    // to determine which frame is accessed.
    if(paramCount < ARG_MINIMUM
       || varlist[ARG_OBJECT]->vt != VT_PTR
       || varlist[ARG_FRAME]->vt != VT_INTEGER)
    {
        *pretvar = NULL;
        goto error_local;
    }
    else
    {
        handle  = (s_attack **)varlist[ARG_OBJECT]->ptrVal;
        frame   = (LONG)varlist[ARG_FRAME]->lVal;
    }

    // If this frame has property, send value back to user.
    if(handle[frame])
    {
        ScriptVariant_ChangeType(*pretvar, VT_PTR);
        (*pretvar)->ptrVal = handle[frame];
    }

    return result;

    // Error trapping.
    error_local:

    printf("You must provide a valid handle and frame: " SELF_NAME "\n");

    result = E_FAIL;
    return result;

    #undef SELF_NAME
    #undef ARG_MINIMUM
    #undef ARG_OBJECT
    #undef ARG_FRAME
}

// get_attack_instance(void handle, int index)
//
// Get single item handle from item collection.
// As of 2016-10-30, this function is a placeholder and
// simply returns the handle given. It is in place for future
// support of multiple instances per frame.
HRESULT openbor_get_attack_instance(ScriptVariant **varlist, ScriptVariant **pretvar, int paramCount)
{
    #define SELF_NAME       "get_attack_instance(void handle, int index)"
    #define ARG_MINIMUM     2   // Minimum required arguments.
    #define ARG_OBJECT      0   // Handle (pointer to property structure).
    #define ARG_INDEX       1   // Index to access.

    int         result     = S_OK; // Success or error?
    s_attack    *handle    = NULL; // Property handle.
    //int         index      = 0;    // Property argument.

    // Clear pass by reference argument used to send
    // property data back to calling script.     .
    ScriptVariant_Clear(*pretvar);

    // Verify incoming arguments. There should at least
    // be a pointer for the property handle and an integer
    // to determine which index is accessed.
    if(paramCount < ARG_MINIMUM
       || varlist[ARG_OBJECT]->vt != VT_PTR
       || varlist[ARG_INDEX]->vt != VT_INTEGER)
    {
        *pretvar = NULL;
        goto error_local;
    }
    else
    {
        handle  = (s_attack *)varlist[ARG_OBJECT]->ptrVal;
        //index   = (LONG)varlist[ARG_INDEX]->lVal;
    }

    // If this index has property, send value back to user.
    //if(handle[index])
    //{
        ScriptVariant_ChangeType(*pretvar, VT_PTR);
        //(*pretvar)->ptrVal = handle[index];

        (*pretvar)->ptrVal = handle;
    //}

    return result;

    // Error trapping.
    error_local:

    printf("You must provide a valid handle and index: " SELF_NAME "\n");

    result = E_FAIL;
    return result;

    #undef SELF_NAME
    #undef ARG_MINIMUM
    #undef ARG_OBJECT
    #undef ARG_INDEX
}

// get_attack_property(void handle, int property)
HRESULT openbor_get_attack_property(ScriptVariant **varlist, ScriptVariant **pretvar, int paramCount)
{
    #define SELF_NAME       "get_attack_property(void handle, int property)"
    #define ARG_MINIMUM     2   // Minimum required arguments.
    #define ARG_OBJECT      0   // Handle (pointer to property structure).
    #define ARG_PROPERTY    1   // Property to access.

    int                     result      = S_OK; // Success or error?
    s_attack                *handle     = NULL; // Property handle.
    e_attack_properties     property    = 0;    // Property argument.

    // Clear pass by reference argument used to send
    // property data back to calling script.     .
    ScriptVariant_Clear(*pretvar);

    // Verify incoming arguments. There should at least
    // be a pointer for the property handle and an integer
    // to determine which property is accessed.
    if(paramCount < ARG_MINIMUM
       || varlist[ARG_OBJECT]->vt != VT_PTR
       || varlist[ARG_PROPERTY]->vt != VT_INTEGER)
    {
        *pretvar = NULL;
        goto error_local;
    }
    else
    {
        handle      = (s_attack *)varlist[ARG_OBJECT]->ptrVal;
        property    = (LONG)varlist[ARG_PROPERTY]->lVal;
    }

    // Which property to get?
    switch(property)
    {
        case ATTACK_PROPERTY_BLOCK_COST:

            ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
            (*pretvar)->lVal = (LONG)handle->guardcost;
            break;

        case ATTACK_PROPERTY_BLOCK_PENETRATE:

            ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
            (*pretvar)->lVal = (LONG)handle->no_block;
            break;

        case ATTACK_PROPERTY_COUNTER:

            ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
            (*pretvar)->lVal = (LONG)handle->counterattack;
            break;

        case ATTACK_PROPERTY_DAMAGE_FORCE:

            ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
            (*pretvar)->lVal = (LONG)handle->attack_force;
            break;

        case ATTACK_PROPERTY_DAMAGE_LAND_FORCE:

            ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
            (*pretvar)->lVal = (LONG)handle->damage_on_landing.attack_force;
            break;

        case ATTACK_PROPERTY_DAMAGE_LAND_MODE:

            ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
            (*pretvar)->lVal = (LONG)handle->blast;
            break;

        case ATTACK_PROPERTY_DAMAGE_LETHAL_DISABLE:

            ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
            (*pretvar)->lVal = (LONG)handle->no_kill;
            break;

        case ATTACK_PROPERTY_DAMAGE_STEAL:

            ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
            (*pretvar)->lVal = (LONG)handle->steal;
            break;

        case ATTACK_PROPERTY_DAMAGE_TYPE:

            ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
            (*pretvar)->lVal = (LONG)handle->attack_type;
            break;

//        case ATTACK_PROPERTY_DAMAGE_RECURSIVE_FORCE:
//
//            ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
//            (*pretvar)->lVal = (LONG)handle->dot_force;
//            break;
//
//        case ATTACK_PROPERTY_DAMAGE_RECURSIVE_INDEX:
//
//            ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
//            (*pretvar)->lVal = (LONG)handle->dot_index;
//            break;
//
//        case ATTACK_PROPERTY_DAMAGE_RECURSIVE_MODE:
//
//            ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
//            (*pretvar)->lVal = (LONG)handle->dot;
//            break;
//
//        case ATTACK_PROPERTY_DAMAGE_RECURSIVE_TIME_EXPIRE:
//
//            ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
//            (*pretvar)->lVal = (LONG)handle->dot_time;
//            break;
//
//        case ATTACK_PROPERTY_DAMAGE_RECURSIVE_TIME_RATE:
//
//            ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
//            (*pretvar)->lVal = (LONG)handle->dot_rate;
//            break;

        case ATTACK_PROPERTY_EFFECT_BLOCK_FLASH:

            ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
            (*pretvar)->lVal = (LONG)handle->flash.model_block;
            break;

        case ATTACK_PROPERTY_EFFECT_BLOCK_SOUND:

            ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
            (*pretvar)->lVal = (LONG)handle->blocksound;
            break;

        case ATTACK_PROPERTY_EFFECT_HIT_FLASH:

            ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
            (*pretvar)->lVal = (LONG)handle->flash.model_hit;
            break;

        case ATTACK_PROPERTY_EFFECT_HIT_FLASH_DISABLE:

            ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
            (*pretvar)->lVal = (LONG)handle->no_flash;
            break;

        case ATTACK_PROPERTY_EFFECT_HIT_SOUND:

            ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
            (*pretvar)->lVal = (LONG)handle->hitsound;
            break;

        case ATTACK_PROPERTY_GROUND:

            ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
            (*pretvar)->lVal = (LONG)handle->otg;
            break;

        case ATTACK_PROPERTY_MAP_INDEX:

            ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
            (*pretvar)->lVal = (LONG)handle->forcemap;
            break;

        case ATTACK_PROPERTY_MAP_TIME:

            ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
            (*pretvar)->lVal = (LONG)handle->maptime;
            break;

        case ATTACK_PROPERTY_REACTION_FALL_FORCE:

            ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
            (*pretvar)->lVal = (LONG)handle->attack_drop;
            break;

        case ATTACK_PROPERTY_REACTION_FALL_VELOCITY:

            // Get memory address of sub structure
            // and pass it on as a handle.
            ScriptVariant_ChangeType(*pretvar, VT_PTR);
            (*pretvar)->ptrVal = (VOID *)&handle->dropv;

            break;

        case ATTACK_PROPERTY_REACTION_FREEZE_MODE:

            ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
            (*pretvar)->lVal = (LONG)handle->freeze;
            break;

        case ATTACK_PROPERTY_REACTION_FREEZE_TIME:

            ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
            (*pretvar)->lVal = (LONG)handle->freezetime;
            break;

        case ATTACK_PROPERTY_REACTION_INVINCIBLE_TIME:

            ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
            (*pretvar)->lVal = (LONG)handle->next_hit_time;
            break;

        case ATTACK_PROPERTY_REACTION_PAIN_SKIP:

            ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
            (*pretvar)->lVal = (LONG)handle->no_pain;
            break;

        case ATTACK_PROPERTY_REACTION_PAUSE_TIME:

            ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
            (*pretvar)->lVal = (LONG)handle->pause_add;
            break;

        case ATTACK_PROPERTY_REACTION_REPOSITION_DIRECTION:

            ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
            (*pretvar)->lVal = (LONG)handle->force_direction;
            break;

        case ATTACK_PROPERTY_REACTION_REPOSITION_DISTANCE:

            ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
            (*pretvar)->lVal = (LONG)handle->grab_distance;
            break;

        case ATTACK_PROPERTY_REACTION_REPOSITION_MODE:

            ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
            (*pretvar)->lVal = (LONG)handle->grab;
            break;

        case ATTACK_PROPERTY_SEAL_COST:

            ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
            (*pretvar)->lVal = (LONG)handle->seal;
            break;

        case ATTACK_PROPERTY_SEAL_TIME:

            ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
            (*pretvar)->lVal = (LONG)handle->sealtime;
            break;

        case ATTACK_PROPERTY_STAYDOWN_RISE:

            ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
            (*pretvar)->lVal = (LONG)handle->staydown.rise;
            break;

        case ATTACK_PROPERTY_STAYDOWN_RISEATTACK:

            ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
            (*pretvar)->lVal = (LONG)handle->staydown.riseattack;
            break;

        case ATTACK_PROPERTY_TAG:

            ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
            (*pretvar)->lVal = (LONG)handle->meta_tag;
            break;

        default:

            printf("Unsupported property.\n");
            goto error_local;
            break;
    }

    return result;

    // Error trapping.
    error_local:

    printf("You must provide a valid handle and property: " SELF_NAME "\n");

    result = E_FAIL;
    return result;

    #undef SELF_NAME
    #undef ARG_MINIMUM
    #undef ARG_OBJECT
    #undef ARG_PROPERTY
}

// set_attack_property(void handle, int property, value)
HRESULT openbor_set_attack_property(ScriptVariant **varlist, ScriptVariant **pretvar, int paramCount)
{
    #define SELF_NAME           "set_attack_property(void handle, int property, value)"
    #define ARG_MINIMUM         3   // Minimum required arguments.
    #define ARG_OBJECT          0   // Handle (pointer to property structure).
    #define ARG_PROPERTY        1   // Property to access.
    #define ARG_VALUE           2   // New value to apply.

    int                     result      = S_OK; // Success or error?
    s_attack                *handle     = NULL; // Property handle.
    e_attack_properties     property    = 0;    // Property to access.

    // Value carriers to apply on properties after
    // taken from argument.
    LONG     temp_int;

    // Verify incoming arguments. There must be a
    // pointer for the animation handle, an integer
    // property, and a new value to apply.
    if(paramCount < ARG_MINIMUM
       || varlist[ARG_OBJECT]->vt != VT_PTR
       || varlist[ARG_PROPERTY]->vt != VT_INTEGER)
    {
        *pretvar = NULL;
        goto error_local;
    }
    else
    {
        handle      = (s_attack *)varlist[ARG_OBJECT]->ptrVal;
        property    = (LONG)varlist[ARG_PROPERTY]->lVal;
    }

    // Which property to modify?
    switch(property)
    {
        case ATTACK_PROPERTY_BLOCK_COST:

            if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
            {
                handle->guardcost = temp_int;
            }
            break;

        case ATTACK_PROPERTY_BLOCK_PENETRATE:

            if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
            {
                handle->no_block = temp_int;
            }
            break;

        case ATTACK_PROPERTY_COUNTER:

            if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
            {
                handle->counterattack = temp_int;
            }
            break;

        case ATTACK_PROPERTY_DAMAGE_FORCE:

            if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
            {
                handle->attack_force = temp_int;
            }
            break;

        case ATTACK_PROPERTY_DAMAGE_LAND_FORCE:

            if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
            {
                handle->damage_on_landing.attack_force = temp_int;
            }
            break;

        case ATTACK_PROPERTY_DAMAGE_LAND_MODE:

            if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
            {
                handle->blast = temp_int;
            }
            break;

        case ATTACK_PROPERTY_DAMAGE_LETHAL_DISABLE:

            if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
            {
                handle->no_kill = temp_int;
            }
            break;

        case ATTACK_PROPERTY_DAMAGE_STEAL:

            if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
            {
                handle->steal = temp_int;
            }
            break;

        case ATTACK_PROPERTY_DAMAGE_TYPE:

            if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
            {
                handle->attack_type = temp_int;
            }
            break;

//        case ATTACK_PROPERTY_DAMAGE_RECURSIVE_FORCE:
//
//            if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
//            {
//                handle->dot_force = temp_int;
//            }
//            break;
//
//        case ATTACK_PROPERTY_DAMAGE_RECURSIVE_INDEX:
//
//            if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
//            {
//                handle->dot_index = temp_int;
//            }
//            break;
//
//        case ATTACK_PROPERTY_DAMAGE_RECURSIVE_MODE:
//
//            if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
//            {
//                handle->dot = temp_int;
//            }
//            break;
//
//        case ATTACK_PROPERTY_DAMAGE_RECURSIVE_TIME_EXPIRE:
//
//            if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
//            {
//                handle->dot_time = temp_int;
//            }
//            break;
//
//        case ATTACK_PROPERTY_DAMAGE_RECURSIVE_TIME_RATE:
//
//            if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
//            {
//                handle->dot_rate = temp_int;
//            }
//            break;

        case ATTACK_PROPERTY_EFFECT_BLOCK_FLASH:

            if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
            {
                handle->flash.model_block = temp_int;
            }
            break;

        case ATTACK_PROPERTY_EFFECT_BLOCK_SOUND:

            if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
            {
                handle->blocksound = temp_int;
            }
            break;

        case ATTACK_PROPERTY_EFFECT_HIT_FLASH:

            if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
            {
                handle->flash.model_hit = temp_int;
            }
            break;

        case ATTACK_PROPERTY_EFFECT_HIT_FLASH_DISABLE:

            if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
            {
                handle->no_flash = temp_int;
            }
            break;

        case ATTACK_PROPERTY_EFFECT_HIT_SOUND:

            if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
            {
                handle->hitsound = temp_int;
            }
            break;

        case ATTACK_PROPERTY_GROUND:

            if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
            {
                handle->otg = temp_int;
            }
            break;

        case ATTACK_PROPERTY_MAP_INDEX:

            if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
            {
                handle->forcemap = temp_int;
            }
            break;

        case ATTACK_PROPERTY_MAP_TIME:

            if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
            {
                handle->maptime = temp_int;
            }
            break;

        case ATTACK_PROPERTY_REACTION_FALL_FORCE:

            if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
            {
                handle->attack_drop = temp_int;
            }

            break;

        case ATTACK_PROPERTY_REACTION_FALL_VELOCITY:

            // Reassign sub structure memory address
            // to new handle.
            handle->dropv = *(s_axis_principal_float *)varlist[ARG_VALUE]->ptrVal;

            break;

        case ATTACK_PROPERTY_REACTION_FREEZE_MODE:

            if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
            {
                handle->freeze = temp_int;
            }
            break;

        case ATTACK_PROPERTY_REACTION_FREEZE_TIME:

            if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
            {
                handle->freezetime = temp_int;
            }
            break;

        case ATTACK_PROPERTY_REACTION_INVINCIBLE_TIME:

            if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
            {
                handle->next_hit_time = temp_int;
            }
            break;

        case ATTACK_PROPERTY_REACTION_PAIN_SKIP:

            if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
            {
                handle->no_pain = temp_int;
            }
            break;

        case ATTACK_PROPERTY_REACTION_PAUSE_TIME:

            if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
            {
                handle->pause_add = temp_int;
            }
            break;

        case ATTACK_PROPERTY_REACTION_REPOSITION_DIRECTION:

            if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
            {
                handle->force_direction = temp_int;
            }
            break;

        case ATTACK_PROPERTY_REACTION_REPOSITION_DISTANCE:

            if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
            {
                handle->grab_distance = temp_int;
            }
            break;

        case ATTACK_PROPERTY_REACTION_REPOSITION_MODE:

            if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
            {
                handle->grab = temp_int;
            }
            break;

        case ATTACK_PROPERTY_SEAL_COST:

            if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
            {
                handle->seal = temp_int;
            }
            break;

        case ATTACK_PROPERTY_SEAL_TIME:

            if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
            {
                handle->sealtime = temp_int;
            }
            break;

        case ATTACK_PROPERTY_STAYDOWN_RISE:

            if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
            {
                handle->staydown.rise = temp_int;
            }
            break;

        case ATTACK_PROPERTY_STAYDOWN_RISEATTACK:

            if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
            {
                handle->staydown.riseattack = temp_int;
            }
            break;

        case ATTACK_PROPERTY_TAG:

            if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
            {
                handle->meta_tag = temp_int;
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
    #undef ARG_OBJECT
    #undef ARG_PROPERTY
    #undef ARG_VALUE
}


