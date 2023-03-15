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
* 2023-03-03
*
* Return a property. Requires
* a object pointer and property 
* constant to access.
*/ 
HRESULT openbor_get_model_property(ScriptVariant **varlist , ScriptVariant **pretvar, int paramCount)
{
    #define SELF_NAME       "openbor_get_model_property(void model, int property)"
    #define ARG_MINIMUM     2   // Minimum required arguments.
    #define ARG_HANDLE      0   // Handle (pointer to property structure).
    #define ARG_PROPERTY    1   // Property to access.

    s_model*               handle     = NULL; // Property handle.
    e_model_properties	   property    = 0;    // Property argument.

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
        handle      = (s_model *)varlist[ARG_HANDLE]->ptrVal;
        property    = (LONG)varlist[ARG_PROPERTY]->lVal;
    }
	
    switch(property)
    {
        case MODEL_PROPERTY_ACTION_FREEZE:

            ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
            (*pretvar)->lVal = (LONG)handle->dofreeze;

            break;

        case MODEL_PROPERTY_AIR_CONTROL:

            ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
            (*pretvar)->lVal = (LONG)handle->air_control;

            break;

        case MODEL_PROPERTY_ANTI_GRAVITY:

            ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
            (*pretvar)->lVal = (LONG)handle->antigravity;

            break;

        case MODEL_PROPERTY_BOUNCE:

            ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
            (*pretvar)->lVal = (LONG)handle->bounce;

            break;

        case MODEL_PROPERTY_FACTION:
            
            ScriptVariant_ChangeType(*pretvar, VT_PTR);
            (*pretvar)->ptrVal = (s_faction*)&handle->faction;
		
            break;

        case MODEL_PROPERTY_GROUND:

            ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
            (*pretvar)->lVal = (LONG)handle->ground;

            break;

        case MODEL_PROPERTY_HP:

            ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
            (*pretvar)->lVal = (LONG)handle->health;

            break;

        case MODEL_PROPERTY_HUD_DISABLE:

            ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
            (*pretvar)->lVal = (LONG)handle->nolife;

            break;

        case MODEL_PROPERTY_HUD_POPUP:

            ScriptVariant_ChangeType(*pretvar, VT_PTR);
            (*pretvar)->ptrVal = (s_barstatus*)&handle->hud_popup;

            break;

        case MODEL_PROPERTY_ICON:

            ScriptVariant_ChangeType(*pretvar, VT_PTR);
            (*pretvar)->ptrVal = (s_icon*)&handle->icon;

            break;

		case MODEL_PROPERTY_INDEX:

			ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
			(*pretvar)->lVal = (LONG)handle->index;

			break;

        case MODEL_PROPERTY_MAKE_INVINCIBLE:

            ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
            (*pretvar)->lVal = (LONG)handle->makeinv;

            break;

        case MODEL_PROPERTY_MOVE_CONSTRAINT:

            ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
            (*pretvar)->lVal = (LONG)handle->move_constraint;

            break;

        case MODEL_PROPERTY_MP:

            ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
            (*pretvar)->lVal = (LONG)handle->mp;

            break;

        case MODEL_PROPERTY_MULTIPLE:

            ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
            (*pretvar)->lVal = (LONG)handle->multiple;

            break;

        case MODEL_PROPERTY_NAME:

            ScriptVariant_ChangeType(*pretvar, VT_STR);
            (*pretvar)->strVal = StrCache_CreateNewFrom(handle->name);
            
            break;     

        case MODEL_PROPERTY_OFF_SCREEN_KILL:

            ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
            (*pretvar)->lVal = (LONG)handle->offscreenkill;

            break;

        case MODEL_PROPERTY_OFF_SCREEN_NO_ATTACK:

            ScriptVariant_ChangeType(*pretvar, VT_DECIMAL);
            (*pretvar)->dblVal = (DOUBLE)handle->offscreen_noatk_factor;

            break;

        case MODEL_PROPERTY_PATH:

            ScriptVariant_ChangeType(*pretvar, VT_STR);
            (*pretvar)->strVal = StrCache_CreateNewFrom(handle->path);

            break;

        case MODEL_PROPERTY_PRIORITY:

            ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
            (*pretvar)->lVal = (LONG)handle->priority;

            break;

        case MODEL_PROPERTY_QUAKE_CONFIG:

            ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
            (*pretvar)->lVal = (e_quake_config)handle->quake_config;

            break;

        case MODEL_PROPERTY_RISE_INVINCIBLE:

            ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
            (*pretvar)->lVal = (LONG)handle->riseinv;

            break;

        case MODEL_PROPERTY_SCORE:

            ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
            (*pretvar)->lVal = (LONG)handle->score;

            break;

        case MODEL_PROPERTY_SCROLL:

            ScriptVariant_ChangeType(*pretvar, VT_DECIMAL);
            (*pretvar)->dblVal = (DOUBLE)handle->scroll;

            break;

        case MODEL_PROPERTY_SPAWN_HUD:

            ScriptVariant_ChangeType(*pretvar, VT_PTR);
            (*pretvar)->ptrVal = (s_spawn_hud*)&handle->player_arrow;

            break;

        case MODEL_PROPERTY_SUBTYPE:

            ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
            (*pretvar)->lVal = (e_entity_type_sub)handle->subtype;

            break;

        case MODEL_PROPERTY_TYPE:

            ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
            (*pretvar)->lVal = (e_entity_type)handle->type;

            break;

        case MODEL_PROPERTY_WEAPON:

            ScriptVariant_ChangeType(*pretvar, VT_PTR);
            (*pretvar)->ptrVal = (s_weapon*)&handle->weapon_properties;

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
    #undef ARG_HANDLE
    #undef ARG_PROPERTY
}

/*
* Caskey, Damon  V.
* 2018-04-03
*
* Mutate a model property. Requires
* the model pointer, a string property
* name, and new value.
*/ 
HRESULT openbor_set_model_property(ScriptVariant **varlist, ScriptVariant **pretvar, int paramCount)
{
    #define SELF_NAME           "openbor_set_model_property(void model, int property, value)"
    #define ARG_MINIMUM         3   // Minimum required arguments.
    #define ARG_HANDLE          0   // Handle (pointer to property structure).
    #define ARG_PROPERTY        1   // Property to access.
    #define ARG_VALUE           2   // New value to apply.

    int                 result      = S_OK; // Success or error?
    s_model*            handle     = NULL; // Property handle.
    e_model_properties	property    = 0;    // Property to access.

    // Value carriers to apply on properties after
    // taken from argument.
    LONG    temp_int;
    DOUBLE  temp_float;
		
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
    handle      = (s_model *)varlist[ARG_HANDLE]->ptrVal;
    property    = (LONG)varlist[ARG_PROPERTY]->lVal;

    // Which property to modify?
    switch(property)
    {
        case MODEL_PROPERTY_ACTION_FREEZE:

            if (SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
            {
                handle->dofreeze = temp_int;
            }

            break;

        case MODEL_PROPERTY_AIR_CONTROL:

            if (SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
            {
                handle->air_control = temp_int;
            }

            break;

        case MODEL_PROPERTY_ANTI_GRAVITY:

            if (SUCCEEDED(ScriptVariant_DecimalValue(varlist[ARG_VALUE], &temp_float)))
            {
                handle->antigravity = temp_float;
            }

            break;

        case MODEL_PROPERTY_BOUNCE:

            if (SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
            {
                handle->bounce = temp_int;
            }

            break;

        case MODEL_PROPERTY_FACTION:

            printf("\n\n Warning: Model Faction is a read only pointer. Use the appropriate sub property function to modify values. \n");

            break;

        case MODEL_PROPERTY_GROUND:

            if (SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
            {
                handle->ground = temp_int;
            }

            break;

        case MODEL_PROPERTY_HP:

            if (SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
            {
                handle->health = temp_int;
            }

            break;

        case MODEL_PROPERTY_HUD_DISABLE:

            if (SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
            {
                handle->nolife = temp_int;
            }

            break;

        case MODEL_PROPERTY_HUD_POPUP:

            printf("\n\n Warning: Model HUD Popup is a read only pointer. Use the appropriate sub property function to modify values. \n");

            break;

        case MODEL_PROPERTY_ICON:

            printf("\n\n Warning: Model Icon is a read only pointer. Use the appropriate sub property function to modify values. \n");

            break;

		case MODEL_PROPERTY_INDEX:

			if (SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
			{
				handle->index = temp_int;
			}

            break;

        case MODEL_PROPERTY_MAKE_INVINCIBLE:

            if (SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
            {
                handle->makeinv = temp_int;
            }

            break;

        case MODEL_PROPERTY_MOVE_CONSTRAINT:

            if (SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
            {
                handle->move_constraint = temp_int;
            }

            break;

        case MODEL_PROPERTY_MP:

            if (SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
            {
                handle->mp = temp_int;
            }

            break;

        case MODEL_PROPERTY_MULTIPLE:

            if (SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
            {
                handle->multiple = temp_int;
            }

            break;

        case MODEL_PROPERTY_NAME:

            if (varlist[ARG_VALUE]->vt == VT_STR)
            {
                strcpy(handle->name, (char*)StrCache_Get(varlist[ARG_VALUE]->strVal));
            }

            break;

        case MODEL_PROPERTY_OFF_SCREEN_KILL:

            if (SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
            {
                handle->offscreenkill = temp_int;
            }

            break;

        case MODEL_PROPERTY_OFF_SCREEN_NO_ATTACK:

            if (SUCCEEDED(ScriptVariant_DecimalValue(varlist[ARG_VALUE], &temp_float)))
            {
                handle->offscreen_noatk_factor = temp_float;
            }

            break;        

        case MODEL_PROPERTY_PATH:

            if (varlist[ARG_VALUE]->vt == VT_STR)
            {
                strcpy(handle->path, (char*)StrCache_Get(varlist[ARG_VALUE]->strVal));
            }

            break;

        case MODEL_PROPERTY_PRIORITY:

            if (SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
            {
                handle->priority = temp_int;
            }

            break;

        case MODEL_PROPERTY_QUAKE_CONFIG:

            if (SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
            {
                handle->quake_config = temp_int;
            }

            break;

        case MODEL_PROPERTY_RISE_INVINCIBLE:

            if (SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
            {
                handle->riseinv = temp_int;
            }

            break;

        case MODEL_PROPERTY_SCORE:

            if (SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
            {
                handle->score = temp_int;
            }

            break;

        case MODEL_PROPERTY_SCROLL:

            if (SUCCEEDED(ScriptVariant_DecimalValue(varlist[ARG_VALUE], &temp_float)))
            {
                handle->scroll = temp_float;
            }

            break;

        case MODEL_PROPERTY_SPAWN_HUD:

            printf("\n\n Warning: Model Spawn HUD is a read only pointer. Use the appropriate sub property function to modify values. \n");

            break;

        case MODEL_PROPERTY_SUBTYPE:

            if (SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
            {
                handle->subtype = temp_int;
            }

            break;

        case MODEL_PROPERTY_TYPE:

            if (SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
            {
                handle->type = temp_int;
            }

            break;

        case MODEL_PROPERTY_WEAPON:

            printf("\n\n Warning: Model Weapon is a read only pointer. Use the appropriate sub property function to modify values.\n");

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
