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
* 2023-03-17
*
* Return a property. Requires
* an object pointer and property 
* constant.
*/ 
HRESULT openbor_get_colorset_property(ScriptVariant **varlist , ScriptVariant **pretvar, int paramCount)
{
    #define SELF_NAME       "openbor_get_colorset_property(void colorset, int property)"
    #define ARG_MINIMUM     2   // Minimum required arguments.
    #define ARG_OBJECT      0   // Handle (pointer to property structure).
    #define ARG_PROPERTY    1   // Property to access.

    s_colorset*               object     = NULL; // Property object.
    e_colorset_properties	   property    = 0;    // Property argument.

    /*
    * Clear pass by reference argument used to send
    * property data back to calling script.
    */
    ScriptVariant_Clear(*pretvar);

    /*
    * Verify arguments. There should at least
    * be a pointer for the property object and an integer
    * to determine which property constant is accessed.
    */
    if(paramCount < ARG_MINIMUM
       || varlist[ARG_OBJECT]->vt != VT_PTR
       || varlist[ARG_PROPERTY]->vt != VT_INTEGER)
    {
        *pretvar = NULL;
        goto error_local;
    }
    
    /* 
    * We use local variables for downstream
    * readability. Populate them here.
    */ 
    object      = (s_colorset *)varlist[ARG_OBJECT]->ptrVal;
    property    = (LONG)varlist[ARG_PROPERTY]->lVal;
    
	
    switch(property)
    {
        case COLORSET_PROPERTY_BURN:

            ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
            (*pretvar)->lVal = (LONG)object->burn;

            break;

        case COLORSET_PROPERTY_FROZEN:

            ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
            (*pretvar)->lVal = (LONG)object->frozen;

            break;

        case COLORSET_PROPERTY_HIDE_END:

            ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
            (*pretvar)->lVal = (LONG)object->hide_end;

            break;

        case COLORSET_PROPERTY_HIDE_START:

            ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
            (*pretvar)->lVal = (LONG)object->hide_start;

            break;

        case COLORSET_PROPERTY_KO:

            ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
            (*pretvar)->lVal = (LONG)object->ko;

            break;

        case COLORSET_PROPERTY_KO_CONFIG:

            ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
            (*pretvar)->lVal = (e_ko_colorset_config)object->kotype;

            break;

        case COLORSET_PROPERTY_SHOCK:

            ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
            (*pretvar)->lVal = (LONG)object->shock;

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
    #undef ARG_OBJECT
    #undef ARG_PROPERTY
}

/*
* Caskey, Damon  V.
* 2023-03-17
*
* Mutate a property. Requires
* the object pointer, property
* constant, and new value.
*/ 
HRESULT openbor_set_colorset_property(ScriptVariant **varlist, ScriptVariant **pretvar, int paramCount)
{
    #define SELF_NAME           "openbor_set_colorset_property(void colorset, int property, value)"
    #define ARG_MINIMUM         3   // Minimum required arguments.
    #define ARG_OBJECT          0   // Handle (pointer to property structure).
    #define ARG_PROPERTY        1   // Property to access.
    #define ARG_VALUE           2   // New value to apply.

    int                 result      = S_OK; // Success or error?
    s_colorset*             object     = NULL; // Property object.
    e_colorset_properties	property    = 0;    // Property to access.

    /* 
    * Value carriers to apply on properties after
    * taken from argument.
    */
    LONG    temp_int;
		
    /*
    * Verify incoming arguments.There should at least
    * be a pointer for the property object and an integer
    * to determine which property is accessed.
    */
    if(paramCount < ARG_MINIMUM
       || varlist[ARG_OBJECT]->vt != VT_PTR
       || varlist[ARG_PROPERTY]->vt != VT_INTEGER)
    {
        *pretvar = NULL;
        goto error_local;
    }

    /*
    * We use local variables for downstream
    * readability. Populate them here.
    */

    object      = (s_colorset *)varlist[ARG_OBJECT]->ptrVal;
    property    = (LONG)varlist[ARG_PROPERTY]->lVal;
    
    switch(property)
    {
        case COLORSET_PROPERTY_BURN:

            if (SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
            {
                object->burn = temp_int;
            }

            break;

        case COLORSET_PROPERTY_FROZEN:

            if (SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
            {
                object->frozen = temp_int;
            }

            break;

        case COLORSET_PROPERTY_HIDE_END:

            if (SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
            {
                object->hide_end = temp_int;
            }

            break;

        case COLORSET_PROPERTY_HIDE_START:

            if (SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
            {
                object->hide_start = temp_int;
            }

            break;

        case COLORSET_PROPERTY_KO:

            if (SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
            {
                object->ko = temp_int;
            }

            break;

        case COLORSET_PROPERTY_KO_CONFIG:

            if (SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
            {
                object->kotype = temp_int;
            }

            break;

        case COLORSET_PROPERTY_SHOCK:

            if (SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
            {
                object->shock = temp_int;
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
    #undef ARG_OBJECT
    #undef ARG_PROPERTY
    #undef ARG_VALUE
}
