/*
 * OpenBOR - http://www.chronocrash.com
 * -----------------------------------------------------------------------
 * All rights reserved. See LICENSE in OpenBOR root for license details.
 *
 * Copyright (c) 2004 - 2017 OpenBOR Team
 */

// Coordinate Properties
// 2019-01-03
// Caskey, Damon V.
//
// Access to body coordinate properties by handle.

#include "scriptcommon.h"



// get_coordinate_property(void handle, int property)
HRESULT openbor_get_coordinate_property(ScriptVariant **varlist, ScriptVariant **pretvar, int paramCount)
{
    #define SELF_NAME       "get_body_collision_property(void handle, int property)"
    #define ARG_MINIMUM     2   // Minimum required arguments.
    #define ARG_HANDLE      0   // Handle (pointer to property structure).
    #define ARG_PROPERTY    1   // Property to access.

    int                         result      = S_OK; // Success or error?
    s_collision_body                      *handle     = NULL; // Property handle.
    e_body_collision_properties property    = 0;    // Property argument.

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
        handle      = (s_collision_body *)varlist[ARG_HANDLE]->ptrVal;
        property    = (LONG)varlist[ARG_PROPERTY]->lVal;
    }

    // Which property to get?
    switch(property)
    {
        case BODY_COLLISION_PROP_DEFENSE:

            /*
            // Verify animation has any defense.
            if(handle->defense)
            {
                ScriptVariant_ChangeType(*pretvar, VT_PTR);
                (*pretvar)->ptrVal = (VOID *)handle->defense;
            }
            */

            break;

        case BODY_COLLISION_PROP_POSITION_X:

            ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
            //(*pretvar)->lVal = (LONG)handle->coords.x;
            break;

        case BODY_COLLISION_PROP_POSISTION_Y:

            ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
            //(*pretvar)->lVal = (LONG)handle->coords.y;
            break;

        case BODY_COLLISION_PROP_SIZE_X:

            ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
            //(*pretvar)->lVal = (LONG)handle->coords.width;
            break;

        case BODY_COLLISION_PROP_SIZE_Y:

            ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
            //(*pretvar)->lVal = (LONG)handle->coords.height;
            break;

        case BODY_COLLISION_PROP_SIZE_Z_1:

            ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
            //(*pretvar)->lVal = (LONG)handle->coords.z1;
            break;

        case BODY_COLLISION_PROP_SIZE_Z_2:

            ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
            //(*pretvar)->lVal = (LONG)handle->coords.z2;
            break;

        case BODY_COLLISION_PROP_TAG:

            ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
            (*pretvar)->lVal = (LONG)handle->tag;
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

// set_coordinate_property(void handle, int property, value)
HRESULT openbor_set_coordinate_property(ScriptVariant **varlist, ScriptVariant **pretvar, int paramCount)
{
    #define SELF_NAME           "set_coordinate_property(void handle, int property, value)"
    #define ARG_MINIMUM         3   // Minimum required arguments.
    #define ARG_HANDLE          0   // Handle (pointer to property structure).
    #define ARG_PROPERTY        1   // Property to access.
    #define ARG_VALUE           2   // New value to apply.

    int                         result      = S_OK; // Success or error?
    s_collision_body                      *handle     = NULL; // Property handle.
    e_body_collision_properties property    = 0;    // Property to access.


    // Value carriers to apply on properties after
    // taken from argument.
    int         temp_int;
    //s_defense   *temp_defense;

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
        handle      = (s_collision_body *)varlist[ARG_HANDLE]->ptrVal;
        property    = (LONG)varlist[ARG_PROPERTY]->lVal;
    }

    // Which property to modify?
    switch(property)
    {
        case BODY_COLLISION_PROP_DEFENSE:

                //handle->defense = (temp_defense *)varlist[ARG_VALUE]->ptrVal;

            break;

        case BODY_COLLISION_PROP_POSITION_X:

            if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
            {
                //handle->coords.x = temp_int;
            }
            break;

        case BODY_COLLISION_PROP_POSISTION_Y:

            if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
            {
                //handle->coords.y = temp_int;
            }
            break;

        case BODY_COLLISION_PROP_SIZE_X:

            if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
            {
                //handle->coords.width = temp_int;
            }
            break;

        case BODY_COLLISION_PROP_SIZE_Y:

            if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
            {
                //handle->coords.height = temp_int;
            }
            break;

        case BODY_COLLISION_PROP_SIZE_Z_1:

            if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
            {
                //handle->coords.z1 = temp_int;
            }
            break;

        case BODY_COLLISION_PROP_SIZE_Z_2:

            if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
            {
                //handle->coords.z2 = temp_int;
            }
            break;

        case BODY_COLLISION_PROP_TAG:

            if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
            {
                handle->tag = temp_int;
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


