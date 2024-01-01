/*
 * OpenBOR - http://www.chronocrash.com
 * -----------------------------------------------------------------------
 * All rights reserved. See LICENSE in OpenBOR root for license details.
 *
 * Copyright (c)  OpenBOR Team
 */

// Body Properties
// 2017-04-26
// Caskey, Damon V.
//
// Access to body collision properties.

#include "scriptcommon.h"

// Body collision properties.
// Caskey, Damon V.
// 2016-10-31
//
// get_body_collision_collection(void handle, int frame)
HRESULT openbor_get_body_collision_collection(ScriptVariant **varlist, ScriptVariant **pretvar, int paramCount)
{
    #define SELF_NAME       "get_body_collision_collection(void handle, int frame)"
    #define ARG_MINIMUM     2   // Minimum required arguments.
    #define ARG_OBJECT      0   // Handle (pointer to property structure).
    #define ARG_FRAME       1   // Frame to access.


    int                 result      = S_OK; // Success or error?
    s_collision_body    **handle    = NULL; // Property handle.
    int                 frame       = 0;    // Property argument.

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

    // Populate local handle and frame vars.
    handle  = (s_collision_body**)varlist[ARG_OBJECT]->ptrVal;
    frame   = (LONG)varlist[ARG_FRAME]->lVal;

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

// get_body_collision_instance(void handle, int index)
HRESULT openbor_get_body_collision_instance(ScriptVariant **varlist, ScriptVariant **pretvar, int paramCount)
{
    #define SELF_NAME       "get_body_collision_instance(void handle, int index)"
    #define ARG_MINIMUM     2   // Minimum required arguments.
    #define ARG_OBJECT      0   // Handle (pointer to property structure).
    #define ARG_INDEX       1   // Index to access.

    int                 result      = S_OK; // Success or error?
    s_collision_body**  handle    = NULL; // Property handle.
    int                 index       = 0;    // Property argument.

    // Clear pass by reference argument used to send
    // property data back to calling script.     .
    ScriptVariant_Clear(*pretvar);

    // Verify incoming arguments. There should at least
    // be a pointer for the property handle and an integer
    // to determine which frame is accessed.
    if(paramCount < ARG_MINIMUM
       || varlist[ARG_OBJECT]->vt != VT_PTR
       || varlist[ARG_INDEX]->vt != VT_INTEGER)
    {
        *pretvar = NULL;
        goto error_local;
    }

    // Populate local handle and property vars.
    handle  = (s_collision_body**)varlist[ARG_OBJECT]->ptrVal;
    index   = (LONG)varlist[ARG_INDEX]->lVal;

    // If this index has property, send value back to user.
    if(handle[index])
    {
        ScriptVariant_ChangeType(*pretvar, VT_PTR);
        (*pretvar)->ptrVal = handle[index];
    }

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

// get_body_collision_property(void handle, int property)
HRESULT openbor_get_body_collision_property(ScriptVariant **varlist, ScriptVariant **pretvar, int paramCount)
{
    #define SELF_NAME       "get_body_collision_property(void handle, int property)"
    #define ARG_MINIMUM     2   // Minimum required arguments.
    #define ARG_OBJECT      0   // Handle (pointer to property structure).
    #define ARG_PROPERTY    1   // Property to access.

    int                         result      = S_OK; // Success or error?
    s_collision_body            *handle     = NULL; // Property handle.
    e_body_collision_properties property    = 0;    // Property argument.

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

    // Populate local handle and property vars.
    handle      = (s_collision_body*)varlist[ARG_OBJECT]->ptrVal;
    property    = (LONG)varlist[ARG_PROPERTY]->lVal;

    // Which property to get?
    switch(property)
    {
        case BODY_COLLISION_PROP_COORDINATES:

            // Verify handle and pass it on.
            if(handle->coords)
            {
                ScriptVariant_ChangeType(*pretvar, VT_PTR);
                (*pretvar)->ptrVal = (VOID *)handle->coords;
            }

            break;

        case BODY_COLLISION_PROP_DEFENSE:


            // Verify animation has any defense.
            //if(handle->defense)
            //{
            //    ScriptVariant_ChangeType(*pretvar, VT_PTR);
            //    (*pretvar)->ptrVal = (VOID *)handle->defense;
            //}

            break;

        case BODY_COLLISION_PROP_TAG:

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

// set_body_collision_property(void handle, int property, value)
HRESULT openbor_set_body_collision_property(ScriptVariant **varlist, ScriptVariant **pretvar, int paramCount)
{
    #define SELF_NAME           "set_body_collision_property(void handle, int property, value)"
    #define ARG_MINIMUM         3   // Minimum required arguments.
    #define ARG_OBJECT          0   // Handle (pointer to property structure).
    #define ARG_PROPERTY        1   // Property to access.
    #define ARG_VALUE           2   // New value to apply.

    int                         result      = S_OK; // Success or error?
    s_collision_body            *handle     = NULL; // Property handle.
    e_body_collision_properties property    = 0;    // Property to access.

    // Value carriers to apply on properties after
    // taken from argument.
    LONG         temp_int;

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

    // Populate local handle and property vars.
    handle      = (s_collision_body*)varlist[ARG_OBJECT]->ptrVal;
    property    = (LONG)varlist[ARG_PROPERTY]->lVal;

    // Which property to modify?
    switch(property)
    {
        case BODY_COLLISION_PROP_COORDINATES:

            handle->coords = (s_hitbox *)varlist[ARG_VALUE]->ptrVal;

            break;

        case BODY_COLLISION_PROP_DEFENSE:

            //handle->defense = (s_defense *)varlist[ARG_VALUE]->ptrVal;

            //break;

        case BODY_COLLISION_PROP_TAG:

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


