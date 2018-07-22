/*
 * OpenBOR - http://www.chronocrash.com
 * -----------------------------------------------------------------------
 * All rights reserved. See LICENSE in OpenBOR root for license details.
 *
 * Copyright (c) 2004 - 2017 OpenBOR Team
 */

// Animation
// 2017-04-26
// Caskey, Damon V.
//
// Access animation properties.

#include "scriptcommon.h"

// Get animation property.
// Caskey, Damon V.
// 2016-10-20
//
// Access animation property by handle (pointer).
//
// get_animation_property(void handle, int frame, int property)
HRESULT openbor_get_animation_property(ScriptVariant **varlist, ScriptVariant **pretvar, int paramCount)
{
    #define SELF_NAME       "get_animation_property(void handle, int property)"
    #define ARG_MINIMUM     2   // Minimum required arguments.
    #define ARG_HANDLE      0   // Handle (pointer to property structure).
    #define ARG_PROPERTY    1   // Property to access.

    int                     result      = S_OK; // Success or error?
    s_anim                  *handle     = NULL; // Property handle.
    e_animation_properties  property    = 0;    // Property argument.

    // Clear pass by reference argument used to send
    // property data back to calling script.     .
    ScriptVariant_Clear(*pretvar);

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
    else
    {
        handle      = (s_anim *)varlist[ARG_HANDLE]->ptrVal;
        property    = (LONG)varlist[ARG_PROPERTY]->lVal;
    }

    // Which property to get?
    switch(property)
    {
        case ANI_PROP_ANIMHITS:

            ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
            (*pretvar)->lVal = (LONG)handle->animhits;
            break;

        case ANI_PROP_ANTIGRAV:

            ScriptVariant_ChangeType(*pretvar, VT_DECIMAL);
            (*pretvar)->dblVal = (DOUBLE)handle->antigrav;
            break;

        case ANI_PROP_ATTACK:

            // Verify animation has attacks.
            if(handle->collision_attack)
            {
                ScriptVariant_ChangeType(*pretvar, VT_PTR);
                (*pretvar)->ptrVal = (VOID *)handle->collision_attack;
            }

            break;

        case ANI_PROP_COLLISIONONE:

            ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
            (*pretvar)->lVal = (LONG)handle->attackone;
            break;

        case ANI_PROP_BODY_COLLISION:

            // Verify animation has any bbox.
            if(handle->collision_body)
            {
                ScriptVariant_ChangeType(*pretvar, VT_PTR);
                (*pretvar)->ptrVal = (VOID *)handle->collision_body;
            }

            break;

        case ANI_PROP_ENTITY_COLLISION:

            // Verify animation has any bbox.
            if(handle->collision_entity)
            {
                ScriptVariant_ChangeType(*pretvar, VT_PTR);
                (*pretvar)->ptrVal = (VOID *)handle->collision_entity;
            }

            break;

        case ANI_PROP_BOUNCE:

            ScriptVariant_ChangeType(*pretvar, VT_DECIMAL);
            (*pretvar)->dblVal = (DOUBLE)handle->bounce;

            break;

        case ANI_PROP_CANCEL:

            ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
            (*pretvar)->lVal = (LONG)handle->cancel;
            break;

        case ANI_PROP_CHARGETIME:

            ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
            (*pretvar)->lVal = (LONG)handle->chargetime;
            break;

        case ANI_PROP_COUNTERRANGE:

            // Verify animation has item.
            if(handle->counterrange)
            {
                ScriptVariant_ChangeType(*pretvar, VT_PTR);
                (*pretvar)->ptrVal = (VOID *)handle->counterrange;
            }

            break;

        case ANI_PROP_NUMFRAMES:

            ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
            (*pretvar)->lVal = (LONG)handle->numframes;
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
    #undef ARG_HANDLE
    #undef ARG_PROPERTY
}

// Set animation properties.
// Caskey, Damon V.
// 2016-10-20
//
// Access animation property by handle (pointer).
//
// set_animation_property(void handle, int property, value)
HRESULT openbor_set_animation_property(ScriptVariant **varlist, ScriptVariant **pretvar, int paramCount)
{
    #define SELF_NAME           "set_animation_property(void handle, int property, value)"
    #define ARG_MINIMUM         3   // Minimum required arguments.
    #define ARG_HANDLE          0   // Handle (pointer to property structure).
    #define ARG_PROPERTY        1   // Property to access.
    #define ARG_VALUE           2   // New value to apply.

    int                     result      = S_OK; // Success or error?
    s_anim                  *handle     = NULL; // Property handle.
    e_animation_properties  property    = 0;    // Property to access.

    // Value carriers to apply on properties after
    // taken from argument.
    LONG     temp_int;
    //DOUBLE  temp_float;

    // Verify incoming arguments. There must be a
    // pointer for the animation handle, an integer
    // property, and a new value to apply.
    if(paramCount < ARG_MINIMUM
       || varlist[ARG_HANDLE]->vt != VT_PTR
       || varlist[ARG_PROPERTY]->vt != VT_INTEGER)
    {
        *pretvar = NULL;
        goto error_local;
    }
    else
    {
        handle      = (s_anim *)varlist[ARG_HANDLE]->ptrVal;
        property    = (LONG)varlist[ARG_PROPERTY]->lVal;
    }

    // Which property to modify?
    switch(property)
    {
        case ANI_PROP_ANIMHITS:

            if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
            {
                handle->animhits = (int)temp_int;
            }

            break;

        case ANI_PROP_ANTIGRAV:

            if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
            {
                handle->antigrav = (int)temp_int;
            }

            break;

        case ANI_PROP_NUMFRAMES:

            if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
            {
                handle->numframes = (int)temp_int;
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

    printf("You must provide a valid handle and property: " SELF_NAME "\n");

    result = E_FAIL;
    return result;

    #undef SELF_NAME
    #undef ARG_MINIMUM
    #undef ARG_HANDLE
    #undef ARG_PROPERTY
    #undef ARG_VALUE
}

