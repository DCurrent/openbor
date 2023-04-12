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
HRESULT openbor_get_spawn_hud_property(ScriptVariant **varlist , ScriptVariant **pretvar, int paramCount)
{
    #define SELF_NAME       "openbor_get_spawn_hud_property(void spawn_hud, int player_index, int property)"
    #define ARG_MINIMUM     3   // Minimum required arguments.
    #define ARG_OBJECT      0   // Handle (pointer to property structure).
    #define ARG_INDEX       1   // Element to access.
    #define ARG_PROPERTY    2   // Property to access.

    s_spawn_hud**              handle;      
    e_spawn_hud_properties	   property;
    int                        index;       

    /*
    * Clear pass by reference argument used to send
    * property data back to calling script.
    */ 
    ScriptVariant_Clear(*pretvar);

    /*
    * Verify arguments. There should at least
    * be a pointer for the property handle, an integer
    * to determine which handle element to access, and
    * a property constant.
    */
    if(paramCount < ARG_MINIMUM
       || varlist[ARG_OBJECT]->vt != VT_PTR
       || varlist[ARG_INDEX]->vt != VT_INTEGER
       || varlist[ARG_PROPERTY]->vt != VT_INTEGER)
    {
        *pretvar = NULL;
        goto error_local;
    }
    
    /*
    * Populate local vars for readability.
    */

    handle      = (s_spawn_hud **)varlist[ARG_OBJECT]->ptrVal;
    index       = (LONG)varlist[ARG_INDEX]->lVal;
    property    = (LONG)varlist[ARG_PROPERTY]->lVal;
    
    /*
    * Catch out of bounds indexes.
    */

    if (index < 0 || index > (MAX_PLAYERS - 1))
    {
        printf("\n Index (%d) out of bounds (min 0, max %d).\n", index, MAX_PLAYERS - 1);
        goto error_local;
    }
	
    switch(property)
    {
        case SPAWN_HUD_PROPERTY_POSITION_X:

            ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
            (*pretvar)->lVal = (LONG)handle[index]->position.x;

            break;

        case SPAWN_HUD_PROPERTY_POSITION_Y:

            ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
            (*pretvar)->lVal = (LONG)handle[index]->position.y;

            break;

        case SPAWN_HUD_PROPERTY_SPRITE:

            ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
            (*pretvar)->lVal = (LONG)handle[index]->sprite;

            break;
        default:

            printf("\nUnknwon property.\n");
            goto error_local;

            break;
    }

    return S_OK;

    error_local:

    printf("\nYou must provide a valid pointer, index, and property constant: " SELF_NAME "\n");
    *pretvar = NULL;

    return E_FAIL;

    #undef SELF_NAME
    #undef ARG_MINIMUM
    #undef ARG_PROPERTY
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
HRESULT openbor_set_spawn_hud_property(ScriptVariant **varlist, ScriptVariant **pretvar, int paramCount)
{
    #define SELF_NAME           "openbor_set_spawn_hud_property(void spawn_hud, int player_index, int property, value)"
    #define ARG_MINIMUM     4   // Minimum required arguments.
    #define ARG_OBJECT      0   // Handle (pointer to property structure).
    #define ARG_INDEX       1   // Element to access.
    #define ARG_PROPERTY    2   // Property to access.
    #define ARG_VALUE       3   // New value to apply.

    int                     result = S_OK; // Success or error?
    s_spawn_hud**           handle;
    e_spawn_hud_properties  property;
    int                     index;

    /* 
    * Value carriers to apply on properties after
    * taken from argument.
    */
    LONG    temp_int;

    /*
    * Clear pass by reference argument used to send
    * property data back to calling script.
    */
    ScriptVariant_Clear(*pretvar);

    /*
    * Verify arguments. There should at least
    * be a pointer for the property handle, an integer
    * to determine which handle element to access, and
    * a property constant.
    */
    if (paramCount < ARG_MINIMUM
        || varlist[ARG_OBJECT]->vt != VT_PTR
        || varlist[ARG_INDEX]->vt != VT_INTEGER
        || varlist[ARG_PROPERTY]->vt != VT_INTEGER)
    {
        *pretvar = NULL;
        goto error_local;
    }

    /*
    * Populate local vars for readability.
    */

    handle = (s_spawn_hud**)varlist[ARG_OBJECT]->ptrVal;
    index = (LONG)varlist[ARG_INDEX]->lVal;
    property = (LONG)varlist[ARG_PROPERTY]->lVal;

    /*
    * Catch out of bounds indexes.
    */

    if (index < 0 || index > (MAX_PLAYERS - 1))
    {
        printf("\n Index (%d) out of bounds (min 0, max %d).\n", index, MAX_PLAYERS - 1);
        goto error_local;
    }

    // Which property to modify?
    switch(property)
    {
        case SPAWN_HUD_PROPERTY_POSITION_X:

            if (SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
            {
                handle[index]->position.x = temp_int;
            }

            break;

        case SPAWN_HUD_PROPERTY_POSITION_Y:

            if (SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
            {
                handle[index]->position.y = temp_int;
            }

            break;

        case SPAWN_HUD_PROPERTY_SPRITE:

            if (SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
            {
                handle[index]->sprite = temp_int;
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

    printf("\nYou must provide a valid pointer, index, property constant, and new value: " SELF_NAME "\n");

    result = E_FAIL;
    return result;

    #undef SELF_NAME
    #undef ARG_MINIMUM
    #undef ARG_OBJECT
    #undef ARG_INDEX
    #undef ARG_PROPERTY
    #undef ARG_VALUE
}
