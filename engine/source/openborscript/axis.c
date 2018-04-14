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
int mapstrings_axis_bi_property(ScriptVariant **varlist, int paramCount)
{
    #define ARG_MINIMUM     2   // Minimum number of arguments allowed in varlist.
    #define ARG_PROPERTY    1   // Varlist element carrying which property is requested.

    char *propname = NULL;  // Placeholder for string property name from varlist.
    int prop;               // Placeholder for integer constant located by string.

    static const char *proplist[] =
    {
        "x",
        "y",
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
    MAPSTRINGS(varlist[ARG_PROPERTY], proplist, _axis_the_end,
               "\n\n Error: '%s' is not a known bi-dimensional axis property.\n");


    // If we made it this far everything should be OK.
    return 1;

    #undef ARG_MINIMUM
    #undef ARG_PROPERTY
}

// Use string property argument to find an
// integer property constant and populate
// varlist->lval.
int mapstrings_axis_tri_property(ScriptVariant **varlist, int paramCount)
{
    #define ARG_MINIMUM     2   // Minimum number of arguments allowed in varlist.
    #define ARG_PROPERTY    1   // Varlist element carrying which property is requested.

    char *propname = NULL;  // Placeholder for string property name from varlist.
    int prop;               // Placeholder for integer constant located by string.

    static const char *proplist[] =
    {
        "x",
        "y",
        "z",
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
    MAPSTRINGS(varlist[ARG_PROPERTY], proplist, _axis_the_end,
               "\n\n Error: '%s' is not a known tri-dimensional axis property.\n");


    // If we made it this far everything should be OK.
    return 1;

    #undef ARG_MINIMUM
    #undef ARG_PROPERTY
}


// Caskey, Damon  V.
// 2018-04-14
//
// Return an axis property on bi-dimensional plane. Requires
// an axis handle and property name to access.
HRESULT openbor_get_axis_bi_int_property(ScriptVariant **varlist , ScriptVariant **pretvar, int paramCount)
{
    #define SELF_NAME       "openbor_get_axis_bi_int_property(void handle, char property)"
    #define ARG_MINIMUM     2   // Minimum required arguments.
    #define ARG_HANDLE      0   // Handle (pointer to property structure).
    #define ARG_PROPERTY    1   // Property to access.

    s_axis_f_2d            *handle     = NULL; // Property handle.
    e_axis_properties       property    = 0;    // Property argument.

    // Clear pass by reference argument used to send
    // property data back to calling script.     .
    ScriptVariant_Clear(*pretvar);

    // Map string property name to a
    // matching integer constant.
    mapstrings_axis_bi_property(varlist, paramCount);

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
        handle      = (s_axis_f_2d *)varlist[ARG_HANDLE]->ptrVal;
        property    = (LONG)varlist[ARG_PROPERTY]->lVal;
    }

    switch(property)
    {
        case _axis_x:

            ScriptVariant_ChangeType(*pretvar, VT_DECIMAL);
            (*pretvar)->dblVal = (DOUBLE)handle->x;

            break;

        case _axis_y:

            ScriptVariant_ChangeType(*pretvar, VT_DECIMAL);
            (*pretvar)->dblVal = (DOUBLE)handle->y;

            break;

        default:

            printf("%s is an unsupported property for %s.\n", varlist[ARG_HANDLE]->strVal, SELF_NAME);
            goto error_local;

            break;
    }

    return S_OK;

    error_local:

    printf("You must provide a valid handle and property name: %s \n", SELF_NAME);
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
// Mutate an axis bi-dimensional property. Requires
// the axis handle, a string property
// name, and new value.
HRESULT openbor_set_axis_bi_int_property(ScriptVariant **varlist, ScriptVariant **pretvar, int paramCount)
{
    #define SELF_NAME           "set_axis_bi_int_property(void handle, char property, value)"
    #define ARG_MINIMUM         3   // Minimum required arguments.
    #define ARG_HANDLE          0   // Handle (pointer to property structure).
    #define ARG_PROPERTY        1   // Property to access.
    #define ARG_VALUE           2   // New value to apply.

    int                 result      = S_OK; // Success or error?
    s_axis_i_2d         *handle     = NULL; // Property handle.
    e_axis_properties   property    = 0;    // Property to access.

    // Value carriers to apply on properties after
    // taken from argument.
    DOUBLE  temp_float;

    // Map string property name to a
    // matching integer constant.
    mapstrings_axis_bi_property(varlist, paramCount);

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
    handle      = (s_axis_i_2d *)varlist[ARG_HANDLE]->ptrVal;
    property    = (LONG)varlist[ARG_PROPERTY]->lVal;

    // Which property to modify?
    switch(property)
    {

        case _axis_x:

            if(SUCCEEDED(ScriptVariant_DecimalValue(varlist[ARG_VALUE], &temp_float)))
            {
                handle->x = temp_float;
            }

            break;

        case _axis_y:

            if(SUCCEEDED(ScriptVariant_DecimalValue(varlist[ARG_VALUE], &temp_float)))
            {
                handle->y = temp_float;
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

    printf("%s is an unsupported property for %s.\n", varlist[ARG_HANDLE]->strVal, SELF_NAME);

    result = E_FAIL;
    return result;

    #undef SELF_NAME
    #undef ARG_MINIMUM
    #undef ARG_HANDLE
    #undef ARG_PROPERTY
    #undef ARG_VALUE
}

// Caskey, Damon  V.
// 2018-04-14
//
// Return an axis property on tri-dimensional plane. Requires
// an axis handle and property name to access.
HRESULT openbor_get_axis_tri_int_property(ScriptVariant **varlist , ScriptVariant **pretvar, int paramCount)
{
    #define SELF_NAME       "openbor_get_axis_tri_int_property(void handle, char property)"
    #define ARG_MINIMUM     2   // Minimum required arguments.
    #define ARG_HANDLE      0   // Handle (pointer to property structure).
    #define ARG_PROPERTY    1   // Property to access.

    s_axis_i            *handle     = NULL; // Property handle.
    e_axis_properties   property    = 0;    // Property argument.

    // Clear pass by reference argument used to send
    // property data back to calling script.     .
    ScriptVariant_Clear(*pretvar);

    // Map string property name to a
    // matching integer constant.
    mapstrings_axis_tri_property(varlist, paramCount);

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
        handle      = (s_axis_i *)varlist[ARG_HANDLE]->ptrVal;
        property    = (LONG)varlist[ARG_PROPERTY]->lVal;
    }

    switch(property)
    {
        case _axis_x:

            ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
            (*pretvar)->lVal = (LONG)handle->x;

            break;

        case _axis_y:

            ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
            (*pretvar)->lVal = (LONG)handle->y;

            break;

        case _axis_z:

            ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
            (*pretvar)->lVal = (LONG)handle->z;

            break;

        default:

            printf("%s is an unsupported property for %s.\n", varlist[ARG_HANDLE]->strVal, SELF_NAME);
            goto error_local;

            break;
    }

    return S_OK;

    error_local:

    printf("You must provide a valid handle and property name: %s \n", SELF_NAME);
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
// Mutate an axis tri-dimensional property. Requires
// the axis handle, a string property
// name, and new value.
HRESULT openbor_set_axis_tri_int_property(ScriptVariant **varlist, ScriptVariant **pretvar, int paramCount)
{
    #define SELF_NAME           "set_axis_tri_int_property(void handle, char property, value)"
    #define ARG_MINIMUM         3   // Minimum required arguments.
    #define ARG_HANDLE          0   // Handle (pointer to property structure).
    #define ARG_PROPERTY        1   // Property to access.
    #define ARG_VALUE           2   // New value to apply.

    int                 result      = S_OK; // Success or error?
    s_axis_i            *handle     = NULL; // Property handle.
    e_axis_properties   property    = 0;    // Property to access.

    // Value carriers to apply on properties after
    // taken from argument.
    LONG  temp_int;

    // Map string property name to a
    // matching integer constant.
    mapstrings_axis_tri_property(varlist, paramCount);

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
    handle      = (s_axis_i *)varlist[ARG_HANDLE]->ptrVal;
    property    = (LONG)varlist[ARG_PROPERTY]->lVal;

    // Which property to modify?
    switch(property)
    {

        case _axis_x:

            if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
            {
                handle->x = temp_int;
            }

            break;

        case _axis_y:

            if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
            {
                handle->y = temp_int;
            }

            break;

        case _axis_z:

            if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
            {
                handle->z = temp_int;
            }

            break;

        default:

            printf("%s is an unsupported property for %s.\n", varlist[ARG_HANDLE]->strVal, SELF_NAME);
            goto error_local;

            break;
    }

    return result;

    // Error trapping.
    error_local:

    printf("%s is an unsupported property for %s.\n", varlist[ARG_HANDLE]->strVal, SELF_NAME);

    result = E_FAIL;
    return result;

    #undef SELF_NAME
    #undef ARG_MINIMUM
    #undef ARG_HANDLE
    #undef ARG_PROPERTY
    #undef ARG_VALUE
}
