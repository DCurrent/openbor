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

// Use string property argument to find an
// integer property constant and populate
// varlist->lval.
int mapstrings_animation_property(ScriptVariant** varlist, int paramCount)
{
#define ARG_MINIMUM     2   // Minimum number of arguments allowed in varlist.
#define ARG_PROPERTY    1   // Varlist element carrying which property is requested.

	char* propname = NULL;  // Placeholder for string property name from varlist.
	int prop;               // Placeholder for integer constant located by string.

	static const char* proplist[] =
	{
		"attack_one",
		"bounce_factor",
		"cancel",
		"charge_time",
		"counter_action_condition",
		"counter_action_frame_max",
		"counter_action_frame_min",
		"counter_action_take_damage",
		"drop_frame",
		"drop_model_index",
		"energy_cost_amount",
		"energy_cost_disable",
		"energy_cost_type",
		"flip_frame",
		"follow_up_animation_select",
		"follow_up_condition",
		"hit_count",
		"index",
		"jump_frame",
		"land_frame",
		"land_model_index",
		"loop",
		"model_index",
		"numframes",
		"projectile",
		"quake_frame",
		"range",
		"size",
		"spawn_frame",
		"sub_entity_model_index",
		"subject_to_gravity",
		"summon_frame",
		"sync",
		"unsummon_frame",
		"weapon_frame"		
	};

	// If the minimum argument count
	// was not passed, then there is
	// nothing to map. Return true - we'll
	// catch the mistake in property access
	// functions.
	if (paramCount < ARG_MINIMUM)
	{
		return 1;
	}

	// See macro - will return 0 on fail.
	MAPSTRINGS(varlist[ARG_PROPERTY], proplist, _ANIMATION_PROP_END,
		"\n\n Error: '%s' is not a known animation property.\n");


	// If we made it this far everything should be OK.
	return 1;

#undef ARG_MINIMUM
#undef ARG_PROPERTY
}

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
    // property data back to calling script.
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
        case _ANIMATION_PROP_ATTACK_ONE:

            ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
            (*pretvar)->lVal = (LONG)handle->attack_one;
            break;

        case _ANIMATION_PROP_BOUNCE_FACTOR:

            ScriptVariant_ChangeType(*pretvar, VT_DECIMAL);
            (*pretvar)->dblVal = (DOUBLE)handle->bounce_factor;

            break;

        case _ANIMATION_PROP_CANCEL:

            ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
            (*pretvar)->lVal = (LONG)handle->cancel;
            break;

        case _ANIMATION_PROP_CHARGE_TIME:

            ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
            (*pretvar)->lVal = (LONG)handle->charge_time;
            break;

        case _ANIMATION_PROP_COUNTER_ACTION_CONDITION:

			ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
			(*pretvar)->lVal = (LONG)handle->counter_action.condition;
			break;

		case _ANIMATION_PROP_COUNTER_ACTION_FRAME_MAX:

			ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
			(*pretvar)->lVal = (LONG)handle->counter_action.frame.max;
			break;

		case _ANIMATION_PROP_COUNTER_ACTION_FRAME_MIN:

			ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
			(*pretvar)->lVal = (LONG)handle->counter_action.frame.min;
			break;

		case _ANIMATION_PROP_COUNTER_ACTION_TAKE_DAMAGE:

			ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
			(*pretvar)->lVal = (LONG)handle->counter_action.damaged;
			break;

		case _ANIMATION_PROP_DROP_FRAME:

			ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
			(*pretvar)->lVal = (LONG)handle->dropframe.frame;
			break;

		case _ANIMATION_PROP_DROP_MODEL_INDEX:

			ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
			(*pretvar)->lVal = (LONG)handle->dropframe.model_index;
			break;
			
		case _ANIMATION_PROP_ENERGY_COST_AMOUNT:

			ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
			(*pretvar)->lVal = (LONG)handle->energy_cost.cost;
			break;

		case _ANIMATION_PROP_ENERGY_COST_DISABLE:

			ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
			(*pretvar)->lVal = (e_entity_type)handle->energy_cost.disable;
			break;

		case _ANIMATION_PROP_ENERGY_COST_TYPE:

			ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
			(*pretvar)->lVal = (e_cost_type)handle->energy_cost.mponly;
			break;

		case _ANIMATION_PROP_FLIP_FRAME:

			ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
			(*pretvar)->lVal = (LONG)handle->flipframe;
			break;

		case _ANIMATION_PROP_FOLLOW_UP_ANIMATION_SELECT:

			ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
			(*pretvar)->lVal = (LONG)handle->followup.animation;
			break;

		case _ANIMATION_PROP_FOLLOW_UP_CONDITION:

			ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
			(*pretvar)->lVal = (LONG)handle->followup.condition;
			break;

		case _ANIMATION_PROP_HIT_COUNT:

			ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
			(*pretvar)->lVal = (LONG)handle->hit_count;
			break;

		case _ANIMATION_PROP_INDEX:

			ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
			(*pretvar)->lVal = (LONG)handle->index;
			break;

		case _ANIMATION_PROP_LAND_FRAME:

			ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
			(*pretvar)->lVal = (LONG)handle->landframe.frame;
			break;

		case _ANIMATION_PROP_LAND_MODEL_INDEX:

			ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
			(*pretvar)->lVal = (LONG)handle->landframe.model_index;
			break;

        case _ANIMATION_PROP_NUMFRAMES:

            ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
            (*pretvar)->lVal = (LONG)handle->numframes;
            break;
		
		case _ANIMATION_PROP_SUB_ENTITY_MODEL_INDEX:

			ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
			(*pretvar)->lVal = (LONG)handle->sub_entity_model_index;
			break;

		case _ANIMATION_PROP_SUBJECT_TO_GRAVITY:

			ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
			(*pretvar)->lVal = (LONG)handle->subject_to_gravity;
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
    LONG	temp_int;
    DOUBLE	temp_float;

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
		case _ANIMATION_PROP_ATTACK_ONE:

			if (SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
			{
				handle->attack_one = (bool)temp_int;
			}

			break;

		case _ANIMATION_PROP_BOUNCE_FACTOR:

			if (SUCCEEDED(ScriptVariant_DecimalValue(varlist[ARG_VALUE], &temp_float)))
			{
				handle->bounce_factor = (float)temp_float;
			}

			break;

		case _ANIMATION_PROP_CANCEL:

			if (SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
			{
				handle->cancel = (int)temp_int;
			}

			break;

		case _ANIMATION_PROP_CHARGE_TIME:

			if (SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
			{
				handle->charge_time = (unsigned int)temp_int;
			}

			break;

		case _ANIMATION_PROP_COUNTER_ACTION_CONDITION:

			if (SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
			{
				handle->counter_action.condition = (e_counter_action_condition_logic)temp_int;
			}

			break;

		case _ANIMATION_PROP_COUNTER_ACTION_FRAME_MAX:

			if (SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
			{
				handle->counter_action.frame.max = (int)temp_int;
			}

			break;

		case _ANIMATION_PROP_COUNTER_ACTION_FRAME_MIN:

			if (SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
			{
				handle->counter_action.frame.min = (int)temp_int;
			}

			break;

		case _ANIMATION_PROP_COUNTER_ACTION_TAKE_DAMAGE:

			if (SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
			{
				handle->counter_action.damaged = (e_counter_action_take_damage)temp_int;
			}

			break;

		case _ANIMATION_PROP_DROP_FRAME:

			if (SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
			{
				handle->dropframe.frame = (int)temp_int;
			}

			break;

		case _ANIMATION_PROP_DROP_MODEL_INDEX:

			if (SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
			{
				handle->dropframe.model_index = (int)temp_int;
			}

			break;

		case _ANIMATION_PROP_ENERGY_COST_AMOUNT:

			if (SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
			{
				handle->energy_cost.cost = (int)temp_int;
			}

			break;

		case _ANIMATION_PROP_ENERGY_COST_DISABLE:

			if (SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
			{
				handle->energy_cost.disable = (e_entity_type)temp_int;
			}

			break;

		case _ANIMATION_PROP_ENERGY_COST_TYPE:

			if (SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
			{
				handle->energy_cost.mponly = (e_cost_type)temp_int;
			}

			break;
		
		case _ANIMATION_PROP_FLIP_FRAME:

			if (SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
			{
				handle->flipframe = (int)temp_int;
			}

		case _ANIMATION_PROP_FOLLOW_UP_ANIMATION_SELECT:

			if (SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
			{
				handle->followup.animation = (unsigned int)temp_int;
			}

		case _ANIMATION_PROP_FOLLOW_UP_CONDITION:

			if (SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
			{
				handle->followup.condition = (e_follow_condition_logic)temp_int;
			}

        case _ANIMATION_PROP_HIT_COUNT:

			if (SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
			{
				handle->hit_count = (int)temp_int;
			}

			break;

		case _ANIMATION_PROP_INDEX:

			if (SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
			{
				handle->index = (int)temp_int;
			}

			break;

		case _ANIMATION_PROP_LAND_FRAME:

			if (SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
			{
				handle->landframe.frame = (int)temp_int;
			}

			break;

		case _ANIMATION_PROP_LAND_MODEL_INDEX:

			if (SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
			{
				handle->landframe.model_index = (int)temp_int;
			}

			break;

        case _ANIMATION_PROP_NUMFRAMES:

            if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
            {
                handle->numframes = (int)temp_int;
            }

            break;

		case _ANIMATION_PROP_SUB_ENTITY_MODEL_INDEX:

			if (SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
			{
				handle->sub_entity_model_index = (int)temp_int;
			}

			break;

		case _ANIMATION_PROP_SUBJECT_TO_GRAVITY:

			if (SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
			{
				handle->subject_to_gravity = (bool)temp_int;
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

// Use string property argument to find an
// integer property constant and populate
// varlist->lval.
int mapstrings_animation_frame_property(ScriptVariant** varlist, int paramCount)
{
#define ARG_MINIMUM     2   // Minimum number of arguments allowed in varlist.
#define ARG_PROPERTY    1   // Varlist element carrying which property is requested.

	char* propname = NULL;  // Placeholder for string property name from varlist.
	int prop;               // Placeholder for integer constant located by string.

	static const char* proplist[] =
	{
		"attack",
		"body_collision",
		"entity_collision",
		"delay",
		"drawmethods",
		"idle",
		"move",
		"offset",
		"platform",
		"shadow",
		"sound_to_play",
		"sprite",
		"vulnerable"
	};

	// If the minimum argument count
	// was not passed, then there is
	// nothing to map. Return true - we'll
	// catch the mistake in property access
	// functions.
	if (paramCount < ARG_MINIMUM)
	{
		return 1;
	}

	// See macro - will return 0 on fail.
	MAPSTRINGS(varlist[ARG_PROPERTY], proplist, _ANIMATION_FRAME_PROP_END,
		"\n\n Error: '%s' is not a known animation frame property.\n");


	// If we made it this far everything should be OK.
	return 1;

#undef ARG_MINIMUM
#undef ARG_PROPERTY
}