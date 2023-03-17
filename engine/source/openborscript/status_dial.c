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
HRESULT openbor_get_status_dial_property(ScriptVariant **varlist , ScriptVariant **pretvar, int paramCount)
{
    #define SELF_NAME       "openbor_get_status_dial_property(void icon, int property)"
    #define ARG_MINIMUM     2   // Minimum required arguments.
    #define ARG_OBJECT      0   // Handle (pointer to property structure).
    #define ARG_PROPERTY    1   // Property to access.

    s_barstatus*               handle     = NULL; // Property handle.
    e_status_dial_properties  property    = 0;    // Property argument.

    // Clear pass by reference argument used to send
    // property data back to calling script.
    ScriptVariant_Clear(*pretvar);

    // Verify arguments. There should at least
    // be a pointer for the property handle and an integer
    // to determine which property constant is accessed.
    if(paramCount < ARG_MINIMUM
       || varlist[ARG_OBJECT]->vt != VT_PTR
       || varlist[ARG_PROPERTY]->vt != VT_INTEGER)
    {
        *pretvar = NULL;
        goto error_local;
    }
    else
    {
        // Populate local vars for readability.
        handle      = (s_barstatus *)varlist[ARG_OBJECT]->ptrVal;
        property    = (LONG)varlist[ARG_PROPERTY]->lVal;
    }
	
    if (!handle)
    {
        printf("Missing pointer.\n");
        goto error_local;
    }

    switch(property)
    {
        case STATUS_DIAL_PROPERTY_BACK_LAYER:

            ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
            (*pretvar)->lVal = (LONG)handle->backlayer;

            break;

        case STATUS_DIAL_PROPERTY_BORDER_LAYER:

            ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
            (*pretvar)->lVal = (LONG)handle->borderlayer;

            break;

        case STATUS_DIAL_PROPERTY_COLORSET_TABLE:

            ScriptVariant_ChangeType(*pretvar, VT_PTR);
            (*pretvar)->ptrVal = (int*)&handle->colourtable;

            break;

        case STATUS_DIAL_PROPERTY_CONFIG_FLAGS:

            ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
            (*pretvar)->lVal = (e_status_config)handle->config_flags;

            break;

        case STATUS_DIAL_PROPERTY_GRAPH_LAYER:

            ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
            (*pretvar)->lVal = (LONG)handle->barlayer;

            break;

        case STATUS_DIAL_PROPERTY_GRAPH_POSITION_X:

            ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
            (*pretvar)->lVal = (LONG)handle->graph_position.x;

            break;

        case STATUS_DIAL_PROPERTY_GRAPH_POSITION_Y:

            ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
            (*pretvar)->lVal = (LONG)handle->graph_position.y;

            break;

        case STATUS_DIAL_PROPERTY_GRAPH_SIZE_X:

            ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
            (*pretvar)->lVal = (LONG)handle->size.x;

            break;

        case STATUS_DIAL_PROPERTY_GRAPH_SIZE_Y:

            ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
            (*pretvar)->lVal = (LONG)handle->size.y;

            break;

        case STATUS_DIAL_PROPERTY_NAME_POSITION_X:

            ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
            (*pretvar)->lVal = (LONG)handle->name_position.x;

            break;

        case STATUS_DIAL_PROPERTY_NAME_POSITION_Y:

            ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
            (*pretvar)->lVal = (LONG)handle->name_position.y;

            break;

        case STATUS_DIAL_PROPERTY_SHADOW_LAYER:

            ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
            (*pretvar)->lVal = (LONG)handle->shadowlayer;

            break;

        default:

            printf("Unknwon property.\n");
            goto error_local;

            break;
    }

    return S_OK;

    error_local:

    printf("You must provide a valid pointer and property constant: " SELF_NAME "\n");
    *pretvar = NULL;

    return E_FAIL;

    #undef SELF_NAME
    #undef ARG_MINIMUM
    #undef ARG_OBJECT
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
HRESULT openbor_set_status_dial_property(ScriptVariant **varlist, ScriptVariant **pretvar, int paramCount)
{
    #define SELF_NAME           "openbor_set_status_dial_property(void icon, int property, value)"
    #define ARG_MINIMUM         3   // Minimum required arguments.
    #define ARG_OBJECT          0   // Handle (pointer to property structure).
    #define ARG_PROPERTY        1   // Property to access.
    #define ARG_VALUE           2   // New value to apply.

    int                 result      = S_OK; // Success or error?
    s_barstatus*             handle     = NULL; // Property handle.
    e_status_dial_properties	property    = 0;    // Property to access.

    // Value carriers to apply on properties after
    // taken from argument.
    LONG    temp_int;
		
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
    handle      = (s_barstatus*)varlist[ARG_OBJECT]->ptrVal;
    property    = (LONG)varlist[ARG_PROPERTY]->lVal;

    if (!handle)
    {
        printf("Missing pointer.\n");
        goto error_local;
    }

    // Which property to modify?
    switch(property)
    {
        case STATUS_DIAL_PROPERTY_BACK_LAYER:

            if (SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
            {
                handle->backlayer = temp_int;
            }

            break;

        case STATUS_DIAL_PROPERTY_BORDER_LAYER:

            if (SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
            {
                handle->borderlayer = temp_int;
            }

            break;

        case STATUS_DIAL_PROPERTY_COLORSET_TABLE:

            printf("\n\n Warning: Status Popup Colorset Table is a read only pointer. Use the appropriate sub property function to modify values. \n");

            break;

        case STATUS_DIAL_PROPERTY_CONFIG_FLAGS:

            if (SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
            {
                handle->config_flags = temp_int;
            }

            break;

        case STATUS_DIAL_PROPERTY_GRAPH_LAYER:

            if (SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
            {
                handle->barlayer = temp_int;
            }

            break;

        case STATUS_DIAL_PROPERTY_GRAPH_POSITION_X:

            if (SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
            {
                handle->graph_position.x = temp_int;
            }

            break;

        case STATUS_DIAL_PROPERTY_GRAPH_POSITION_Y:

            if (SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
            {
                handle->graph_position.y = temp_int;
            }

            break;

        case STATUS_DIAL_PROPERTY_GRAPH_SIZE_X:

            if (SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
            {
                handle->size.x = temp_int;
            }

            break;

        case STATUS_DIAL_PROPERTY_GRAPH_SIZE_Y:

            if (SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
            {
                handle->size.y = temp_int;
            }

            break;

        case STATUS_DIAL_PROPERTY_NAME_POSITION_X:

            if (SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
            {
                handle->name_position.x = temp_int;
            }

            break;

        case STATUS_DIAL_PROPERTY_NAME_POSITION_Y:

            if (SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
            {
                handle->name_position.y = temp_int;
            }

            break;

        case STATUS_DIAL_PROPERTY_SHADOW_LAYER:

            if (SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
            {
                handle->shadowlayer = temp_int;
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

    printf("You must provide a valid pointer, property, and new value: " SELF_NAME "\n");

    result = E_FAIL;
    return result;

    #undef SELF_NAME
    #undef ARG_MINIMUM
    #undef ARG_OBJECT
    #undef ARG_PROPERTY
    #undef ARG_VALUE
}
