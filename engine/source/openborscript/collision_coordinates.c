/*
 * OpenBOR - http://www.chronocrash.com
 * -----------------------------------------------------------------------
 * All rights reserved. See LICENSE in OpenBOR root for license details.
 *
 * Copyright (c) 2004 - 2017 OpenBOR Team
 */

// Coordinate Properties
// 2018-01-03
// Caskey, Damon V.
//
// Access to body coordinate properties by handle.

#include "scriptcommon.h"

// get_collision_coordinates_property(void handle, int property)
HRESULT openbor_get_collision_coordinates_property(ScriptVariant **varlist, ScriptVariant **pretvar, int paramCount)
{
    #define SELF_NAME       "get_collision_coordinates_property(void handle, int property)"
    #define ARG_MINIMUM     2   // Minimum required arguments.
    #define ARG_HANDLE      0   // Handle (pointer to property structure).
    #define ARG_PROPERTY    1   // Property to access.

    int                     result      = S_OK; // Success or error?
    s_hitbox                *handle     = NULL; // Property handle.
    e_collision_coordinates property    = 0;    // Property argument.

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

    handle      = (s_hitbox *)varlist[ARG_HANDLE]->ptrVal;
    property    = (LONG)varlist[ARG_PROPERTY]->lVal;

    // Which property to access?
    switch(property)
    {
        case COLLISION_COORDINATES_PROP_DEPTH_BACKGROUND:

            ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
            (*pretvar)->lVal = (LONG)handle->z1;

            break;

        case COLLISION_COORDINATES_PROP_DEPTH_FOREGROUND:

            ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
            (*pretvar)->lVal = (LONG)handle->z2;
            break;

        case COLLISION_COORDINATES_PROP_HEIGHT:

            ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
            (*pretvar)->lVal = (LONG)handle->height;
            break;

        case COLLISION_COORDINATES_PROP_WIDTH:

            ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
            (*pretvar)->lVal = (LONG)handle->width;
            break;

        case COLLISION_COORDINATES_PROP_X:

            ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
            (*pretvar)->lVal = (LONG)handle->x;
            break;

        case COLLISION_COORDINATES_PROP_Y:

            ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
            (*pretvar)->lVal = (LONG)handle->y;
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

// set_collision_coordinates_property(void handle, int property, value)
HRESULT openbor_set_collision_coordinates_property(ScriptVariant **varlist, ScriptVariant **pretvar, int paramCount)
{
    #define SELF_NAME           "set_collision_coordinates_property(void handle, int property, value)"
    #define ARG_MINIMUM         3   // Minimum required arguments.
    #define ARG_HANDLE          0   // Handle (pointer to property structure).
    #define ARG_PROPERTY        1   // Property to access.
    #define ARG_VALUE           2   // New value to apply.

    int                     result      = S_OK; // Success or error?
    s_hitbox                *handle     = NULL; // Property handle.
    e_collision_coordinates property    = 0;    // Property to access.


    // Value carriers to apply on properties after
    // taken from argument.
    LONG         temp_int;

    // Verify incoming arguments. There must be a
    // pointer for the property handle, an integer
    // property, and a new value to apply.
    if(paramCount < ARG_MINIMUM
       || varlist[ARG_HANDLE]->vt != VT_PTR
       || varlist[ARG_PROPERTY]->vt != VT_INTEGER)
    {
        *pretvar = NULL;
        goto error_local;
    }

    handle      = (s_hitbox *)varlist[ARG_HANDLE]->ptrVal;
    property    = (LONG)varlist[ARG_PROPERTY]->lVal;

    // Which property to modify?
    switch(property)
    {
        case COLLISION_COORDINATES_PROP_DEPTH_BACKGROUND:

            if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
            {
                handle->z1 = temp_int;
            }

            break;

        case COLLISION_COORDINATES_PROP_DEPTH_FOREGROUND:

            if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
            {
                handle->z2 = temp_int;
            }
            break;

        case COLLISION_COORDINATES_PROP_HEIGHT:

            if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
            {
                handle->height = temp_int;
            }
            break;

        case COLLISION_COORDINATES_PROP_WIDTH:

            if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
            {
                handle->width = temp_int;
            }
            break;

        case COLLISION_COORDINATES_PROP_X:

            if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
            {
                handle->x = temp_int;
            }
            break;

        case COLLISION_COORDINATES_PROP_Y:

            if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
            {
                handle->y = temp_int;
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


