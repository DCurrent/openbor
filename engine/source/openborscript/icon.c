/*
 * OpenBOR - http://www.chronocrash.com
 * -----------------------------------------------------------------------
 * All rights reserved. See LICENSE in OpenBOR root for license details.
 *
 * Copyright (c) 2004 OpenBOR Team
 */

#include "scriptcommon.h"

/*
* Caskey, Damon  V.
* 2023-03-13
*
* Return a property. Requires
* an object pointer and property 
* constant.
*/ 
HRESULT openbor_get_icon_property(ScriptVariant **varlist , ScriptVariant **pretvar, int paramCount)
{
    #define SELF_NAME       "openbor_get_icon_property(void icon, int property)"
    #define ARG_MINIMUM     2   // Minimum required arguments.
    #define ARG_HANDLE      0   // Handle (pointer to property structure).
    #define ARG_PROPERTY    1   // Property to access.

    s_icon*               handle     = NULL; // Property handle.
    e_icon_properties	   property    = 0;    // Property argument.

    // Clear pass by reference argument used to send
    // property data back to calling script.
    ScriptVariant_Clear(*pretvar);

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
        handle      = (s_icon *)varlist[ARG_HANDLE]->ptrVal;
        property    = (LONG)varlist[ARG_PROPERTY]->lVal;
    }
	
    switch(property)
    {
        case ICON_PROPERTY_DEFAULT:

            ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
            (*pretvar)->lVal = (LONG)handle->def;

            break;

        case ICON_PROPERTY_DIE:

            ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
            (*pretvar)->lVal = (LONG)handle->die;

            break;

        case ICON_PROPERTY_GET:

            ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
            (*pretvar)->lVal = (LONG)handle->get;

            break;

        case ICON_PROPERTY_MP_HIGH:

            ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
            (*pretvar)->lVal = (LONG)handle->mphigh;

            break;

        case ICON_PROPERTY_MP_LOW:

            ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
            (*pretvar)->lVal = (LONG)handle->mplow;

            break;

        case ICON_PROPERTY_MP_MEDIUM:

            ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
            (*pretvar)->lVal = (LONG)handle->mpmed;

            break;

        case ICON_PROPERTY_PAIN:

            ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
            (*pretvar)->lVal = (LONG)handle->pain;

            break;

        case ICON_PROPERTY_POSITION_X:

            ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
            (*pretvar)->lVal = (LONG)handle->position.x;

            break;

        case ICON_PROPERTY_POSITION_Y:

            ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
            (*pretvar)->lVal = (LONG)handle->position.y;

            break;

        case ICON_PROPERTY_USE_MAP:

            ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
            (*pretvar)->lVal = (LONG)handle->usemap;

            break;

        case ICON_PROPERTY_WEAPON:

            ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
            (*pretvar)->lVal = (LONG)handle->weapon;

            break;
        default:

            printf("Unknwon property.\n");
            goto error_local;

            break;
    }

    return S_OK;

    error_local:

    printf("\nYou must provide a valid pointer and property constant: " SELF_NAME "\n");
    *pretvar = NULL;

    return E_FAIL;

    #undef SELF_NAME
    #undef ARG_MINIMUM
    #undef ARG_HANDLE
    #undef ARG_INDEX
}

/*
* Caskey, Damon  V.
* 2023-03-13
*
* Mutate a property. Requires
* the object pointer, property
* constant, and new value.
*/ 
HRESULT openbor_set_icon_property(ScriptVariant **varlist, ScriptVariant **pretvar, int paramCount)
{
    #define SELF_NAME           "openbor_set_icon_property(void icon, int property, value)"
    #define ARG_MINIMUM         3   // Minimum required arguments.
    #define ARG_HANDLE          0   // Handle (pointer to property structure).
    #define ARG_PROPERTY        1   // Property to access.
    #define ARG_VALUE           2   // New value to apply.

    int                 result      = S_OK; // Success or error?
    s_icon*             handle     = NULL; // Property handle.
    e_icon_properties	property    = 0;    // Property to access.

    // Value carriers to apply on properties after
    // taken from argument.
    LONG    temp_int;
		
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
    handle      = (s_icon *)varlist[ARG_HANDLE]->ptrVal;
    property    = (LONG)varlist[ARG_PROPERTY]->lVal;

    // Which property to modify?
    switch(property)
    {
        case ICON_PROPERTY_DEFAULT:

            if (SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
            {
                handle->def = temp_int;
            }

            break;

        case ICON_PROPERTY_DIE:

            if (SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
            {
                handle->die = temp_int;
            }

            break;

        case ICON_PROPERTY_GET:

            if (SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
            {
                handle->get = temp_int;
            }

            break;

        case ICON_PROPERTY_MP_HIGH:

            if (SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
            {
                handle->mphigh = temp_int;
            }

            break;

        case ICON_PROPERTY_MP_LOW:

            if (SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
            {
                handle->mplow = temp_int;
            }

            break;

        case ICON_PROPERTY_MP_MEDIUM:

            if (SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
            {
                handle->mpmed = temp_int;
            }

            break;

        case ICON_PROPERTY_PAIN:

            if (SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
            {
                handle->pain = temp_int;
            }

            break;

        case ICON_PROPERTY_POSITION_X:

            if (SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
            {
                handle->position.x = temp_int;
            }

            break;

        case ICON_PROPERTY_POSITION_Y:

            if (SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
            {
                handle->position.y = temp_int;
            }

            break;

        case ICON_PROPERTY_USE_MAP:

            if (SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
            {
                handle->usemap = temp_int;
            }

            break;

        case ICON_PROPERTY_WEAPON:

            if (SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
            {
                handle->weapon = temp_int;
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

    printf("\nYou must provide a valid pointer, property constant, and new value: " SELF_NAME "\n");

    result = E_FAIL;
    return result;

    #undef SELF_NAME
    #undef ARG_MINIMUM
    #undef ARG_HANDLE
    #undef ARG_PROPERTY
    #undef ARG_VALUE
}
