/*
 * OpenBOR - http://www.chronocrash.com
 * -----------------------------------------------------------------------
 * All rights reserved. See LICENSE in OpenBOR root for license details.
 *
 * Copyright (c)  OpenBOR Team
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
		"frame_count",
		"hit_count",
		"index",
		"jump_frame",
		"jump_model_index",
		"jump_velocity_x",
		"jump_velocity_y",
		"jump_velocity_z",
		"land_frame",
		"land_model_index",
		"loop_frame_end",
		"loop_frame_start",
		"loop_state",
		"model_index",
		"projectile",
		"quake_frame_start",
		"quake_move_y",
		"quake_repeat_count",
		"quake_repeat_max",
		"range_base_max",
		"range_base_min",
		"range_x_max",
		"range_x_min",
		"range_y_max",
		"range_y_min",
		"range_z_max",
		"range_z_min",
		"size_x",
		"size_y",
		"sub_entity_model_index",
		"sub_entity_spawn",
		"sub_entity_summon",
		"sub_entity_unsummon"
		"sync",
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
    #define ARG_OBJECT      0   // Handle (pointer to property structure).
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
       || varlist[ARG_OBJECT]->vt != VT_PTR
       || varlist[ARG_PROPERTY]->vt != VT_INTEGER)
    {
        *pretvar = NULL;
        goto error_local;
    }
    else
    {
        handle      = (s_anim *)varlist[ARG_OBJECT]->ptrVal;
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

		case _ANIMATION_PROP_FRAME_COUNT:

			ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
			(*pretvar)->lVal = (LONG)handle->numframes;
			break;

		case _ANIMATION_PROP_HIT_COUNT:

			ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
			(*pretvar)->lVal = (LONG)handle->hit_count;
			break;

		case _ANIMATION_PROP_INDEX:

			ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
			(*pretvar)->lVal = (LONG)handle->index;
			break;

		case _ANIMATION_PROP_JUMP_FRAME:

			ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
			(*pretvar)->lVal = (LONG)handle->jumpframe.frame;
			break;

		case _ANIMATION_PROP_JUMP_MODEL_INDEX:

			ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
			(*pretvar)->lVal = (LONG)handle->jumpframe.model_index;
			break;

		case _ANIMATION_PROP_JUMP_VELOCITY_X:

			ScriptVariant_ChangeType(*pretvar, VT_DECIMAL);
			(*pretvar)->dblVal = (DOUBLE)handle->jumpframe.velocity.x;
			break;

		case _ANIMATION_PROP_JUMP_VELOCITY_Y:

			ScriptVariant_ChangeType(*pretvar, VT_DECIMAL);
			(*pretvar)->dblVal = (DOUBLE)handle->jumpframe.velocity.y;
			break;

		case _ANIMATION_PROP_JUMP_VELOCITY_Z:

			ScriptVariant_ChangeType(*pretvar, VT_DECIMAL);
			(*pretvar)->dblVal = (DOUBLE)handle->jumpframe.velocity.z;
			break;

		case _ANIMATION_PROP_LAND_FRAME:

			ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
			(*pretvar)->lVal = (LONG)handle->landframe.frame;
			break;

		case _ANIMATION_PROP_LAND_MODEL_INDEX:

			ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
			(*pretvar)->lVal = (LONG)handle->landframe.model_index;
			break;

		case _ANIMATION_PROP_LOOP_FRAME_END:

			ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
			(*pretvar)->lVal = (LONG)handle->loop.frame.max;
			break;

		case _ANIMATION_PROP_LOOP_FRAME_START:

			ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
			(*pretvar)->lVal = (LONG)handle->loop.frame.min;
			break;

		case _ANIMATION_PROP_LOOP_STATE:

			ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
			(*pretvar)->lVal = (LONG)handle->loop.mode;
			break;

		case _ANIMATION_PROP_PROJECTILE:

			ScriptVariant_ChangeType(*pretvar, VT_PTR);
			(*pretvar)->ptrVal = (s_projectile*)handle->projectile;

			break;
		
		case _ANIMATION_PROP_QUAKE_FRAME_START:

			ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
			(*pretvar)->lVal = (LONG)handle->quakeframe.framestart;
			break;

		case _ANIMATION_PROP_QUAKE_MOVE_Y:

			ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
			(*pretvar)->lVal = (LONG)handle->quakeframe.v;
			break;

		case _ANIMATION_PROP_QUAKE_REPEAT_COUNT:

			ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
			(*pretvar)->lVal = (LONG)handle->quakeframe.cnt;
			break;

		case _ANIMATION_PROP_QUAKE_REPEAT_MAX:

			ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
			(*pretvar)->lVal = (LONG)handle->quakeframe.repeat;
			break;

		case _ANIMATION_PROP_RANGE_BASE_MAX:

			ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
			(*pretvar)->lVal = (LONG)handle->range.base.max;
			break;

		case _ANIMATION_PROP_RANGE_BASE_MIN:

			ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
			(*pretvar)->lVal = (LONG)handle->range.base.min;
			break;

		case _ANIMATION_PROP_RANGE_X_MAX:

			ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
			(*pretvar)->lVal = (LONG)handle->range.x.max;
			break;

		case _ANIMATION_PROP_RANGE_X_MIN:

			ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
			(*pretvar)->lVal = (LONG)handle->range.x.min;
			break;

		case _ANIMATION_PROP_RANGE_Y_MAX:

			ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
			(*pretvar)->lVal = (LONG)handle->range.y.max;
			break;

		case _ANIMATION_PROP_RANGE_Y_MIN:

			ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
			(*pretvar)->lVal = (LONG)handle->range.y.min;
			break;

		case _ANIMATION_PROP_RANGE_Z_MAX:

			ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
			(*pretvar)->lVal = (LONG)handle->range.z.max;
			break;

		case _ANIMATION_PROP_RANGE_Z_MIN:

			ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
			(*pretvar)->lVal = (LONG)handle->range.z.min;
			break;

		case _ANIMATION_PROP_SIZE_X:

			ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
			(*pretvar)->lVal = (LONG)handle->size.x;
			break;

		case _ANIMATION_PROP_SIZE_Y:

			ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
			(*pretvar)->lVal = (LONG)handle->size.y;
			break;
		
		case _ANIMATION_PROP_SUB_ENTITY_MODEL_INDEX:

			ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
			(*pretvar)->lVal = (LONG)handle->sub_entity_model_index;
			break;

		case _ANIMATION_PROP_SUB_ENTITY_SPAWN:
			
			ScriptVariant_ChangeType(*pretvar, VT_PTR);
			(*pretvar)->ptrVal = (s_sub_entity*)handle->sub_entity_spawn;

			break;

		case _ANIMATION_PROP_SUB_ENTITY_SUMMON:

			ScriptVariant_ChangeType(*pretvar, VT_PTR);
			(*pretvar)->ptrVal = (s_sub_entity*)handle->sub_entity_summon;

			break;

		case _ANIMATION_PROP_SUB_ENTITY_UNSUMMON:

			ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
			(*pretvar)->lVal = (LONG)handle->sub_entity_unsummon;
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
    #define ARG_OBJECT          0   // Handle (pointer to property structure).
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
       || varlist[ARG_OBJECT]->vt != VT_PTR
       || varlist[ARG_PROPERTY]->vt != VT_INTEGER)
    {
        *pretvar = NULL;
        goto error_local;
    }
    else
    {
        handle      = (s_anim *)varlist[ARG_OBJECT]->ptrVal;
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

		case _ANIMATION_PROP_FRAME_COUNT:

			if (SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
			{
				handle->numframes = (int)temp_int;
			}

			break;

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

		case _ANIMATION_PROP_JUMP_FRAME:

			if (SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
			{
				handle->jumpframe.frame = (int)temp_int;
			}

			break;

		case _ANIMATION_PROP_JUMP_MODEL_INDEX:

			if (SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
			{
				handle->jumpframe.model_index = (int)temp_int;
			}

			break;

		case _ANIMATION_PROP_JUMP_VELOCITY_X:

			if (SUCCEEDED(ScriptVariant_DecimalValue(varlist[ARG_VALUE], &temp_float)))
			{
				handle->jumpframe.velocity.x = (float)temp_float;
			}

			break;

		case _ANIMATION_PROP_JUMP_VELOCITY_Y:

			if (SUCCEEDED(ScriptVariant_DecimalValue(varlist[ARG_VALUE], &temp_float)))
			{
				handle->jumpframe.velocity.y = (float)temp_float;
			}

			break;

		case _ANIMATION_PROP_JUMP_VELOCITY_Z:

			if (SUCCEEDED(ScriptVariant_DecimalValue(varlist[ARG_VALUE], &temp_float)))
			{
				handle->jumpframe.velocity.z = (float)temp_float;
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

		case _ANIMATION_PROP_LOOP_FRAME_END:

			if (SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
			{
				handle->loop.frame.max = (int)temp_int;
			}

			break;

		case _ANIMATION_PROP_LOOP_FRAME_START:

			if (SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
			{
				handle->loop.frame.min = (int)temp_int;
			}

			break;

		case _ANIMATION_PROP_LOOP_STATE:

			if (SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
			{
				handle->loop.mode = (int)temp_int;
			}

			break;
					
		case _ANIMATION_PROP_PROJECTILE:

			
			handle->projectile = (s_projectile*)varlist[ARG_VALUE]->ptrVal;
			
			break;

		
		case _ANIMATION_PROP_QUAKE_FRAME_START:

			if (SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
			{
				handle->quakeframe.framestart = (int)temp_int;
			}

			break;

		case _ANIMATION_PROP_QUAKE_MOVE_Y:

			if (SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
			{
				handle->quakeframe.v = (int)temp_int;
			}

			break;

		case _ANIMATION_PROP_QUAKE_REPEAT_COUNT:

			if (SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
			{
				handle->quakeframe.cnt = (int)temp_int;
			}

			break;

		case _ANIMATION_PROP_QUAKE_REPEAT_MAX:

			if (SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
			{
				handle->quakeframe.repeat = (int)temp_int;
			}

			break;

		case _ANIMATION_PROP_RANGE_BASE_MAX:

			if (SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
			{
				handle->range.base.max = (int)temp_int;
			}

			break;

		case _ANIMATION_PROP_RANGE_BASE_MIN:

			if (SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
			{
				handle->range.base.min = (int)temp_int;
			}

			break;

		case _ANIMATION_PROP_RANGE_X_MAX:

			if (SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
			{
				handle->range.x.max = (int)temp_int;
			}

			break;

		case _ANIMATION_PROP_RANGE_X_MIN:

			if (SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
			{
				handle->range.x.min = (int)temp_int;
			}

			break;

		case _ANIMATION_PROP_RANGE_Y_MAX:

			if (SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
			{
				handle->range.y.max = (int)temp_int;
			}

			break;

		case _ANIMATION_PROP_RANGE_Y_MIN:

			if (SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
			{
				handle->range.y.min = (int)temp_int;
			}

			break;

		case _ANIMATION_PROP_RANGE_Z_MAX:

			if (SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
			{
				handle->range.z.max = (int)temp_int;
			}

			break;

		case _ANIMATION_PROP_RANGE_Z_MIN:

			if (SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
			{
				handle->range.z.min = (int)temp_int;
			}

			break;

		case _ANIMATION_PROP_SIZE_X:

			if (SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
			{
				handle->size.x = (int)temp_int;
			}

			break;

		case _ANIMATION_PROP_SIZE_Y:

			if (SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
			{
				handle->size.y = (int)temp_int;
			}

			break;

		case _ANIMATION_PROP_SUB_ENTITY_MODEL_INDEX:

			if (SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
			{
				handle->sub_entity_model_index = (int)temp_int;
			}

			break;

		case _ANIMATION_PROP_SUB_ENTITY_SPAWN:

			handle->sub_entity_spawn = (s_sub_entity*)varlist[ARG_VALUE]->ptrVal;

			break;

		case _ANIMATION_PROP_SUB_ENTITY_SUMMON:

			handle->sub_entity_summon = (s_sub_entity*)varlist[ARG_VALUE]->ptrVal;

			break;

		case _ANIMATION_PROP_SUB_ENTITY_UNSUMMON:

			if (SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
			{
				handle->sub_entity_unsummon = (int)temp_int;
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
    #undef ARG_OBJECT
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

// Sub entity (spawn/summon)

// Caskey, Damon  V.
// 2019-12-12
//
// Allocate a new sub entity and return the pointer.
HRESULT openbor_allocate_sub_entity(ScriptVariant** varlist, ScriptVariant** pretvar, int paramCount)
{
	extern s_sub_entity* allocate_sub_entity();
	s_sub_entity* result;

	ScriptVariant_ChangeType(*pretvar, VT_PTR);

	if ((result = allocate_sub_entity()))
	{
		(*pretvar)->ptrVal = (s_sub_entity*)result;
	}

	return S_OK;
}

// Use string property argument to find an
// integer property constant and populate
// varlist->lval.
int mapstrings_sub_entity_property(ScriptVariant** varlist, int paramCount)
{
#define ARG_MINIMUM     2   // Minimum number of arguments allowed in varlist.
#define ARG_PROPERTY    1   // Varlist element carrying which property is requested.

	char* propname = NULL;  // Placeholder for string property name from varlist.
	int prop;               // Placeholder for integer constant located by string.

	static const char* proplist[] =
	{
		"frame",
		"placement",
		"position_x",
		"position_y",
		"position_z",
		"spawn_type"
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
	MAPSTRINGS(varlist[ARG_PROPERTY], proplist, _SUB_ENTITY_PROP_END,
		"\n\n Error: '%s' is not a known sub entity property.\n");

	// If we made it this far everything should be OK.
	return 1;

#undef ARG_MINIMUM
#undef ARG_PROPERTY
}

// Caskey, Damon V.
// 2019-12-11
//
// Access sub_entity property by handle (pointer).
//
// get_sub_entity_property(void handle, int property)
HRESULT openbor_get_sub_entity_property(ScriptVariant** varlist, ScriptVariant** pretvar, int paramCount)
{
#define SELF_NAME       "get_sub_entity_property(void handle, int property)"
#define ARG_MINIMUM     2   // Minimum required arguments.
#define ARG_OBJECT      0   // Handle (pointer to property structure).
#define ARG_PROPERTY    1   // Property to access.

	int                     result = S_OK; // Success or error?
	s_sub_entity			*handle = NULL; // Property handle.
	e_sub_entity_properties  property = 0;    // Property argument.

	// Clear pass by reference argument used to send
	// property data back to calling script.
	ScriptVariant_Clear(*pretvar);

	// Verify incoming arguments. There should at least
	// be a pointer for the property handle and an integer
	// to determine which property is accessed.
	if (paramCount < ARG_MINIMUM
		|| varlist[ARG_OBJECT]->vt != VT_PTR
		|| varlist[ARG_PROPERTY]->vt != VT_INTEGER)
	{
		*pretvar = NULL;
		goto error_local;
	}
	else
	{
		handle = (s_sub_entity*)varlist[ARG_OBJECT]->ptrVal;
		property = (e_sub_entity_properties)varlist[ARG_PROPERTY]->lVal;
	}

	// Which property to get?
	switch (property)
	{
	case _SUB_ENTITY_PROP_FRAME:

		ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
		(*pretvar)->lVal = (LONG)handle->frame;
		break;

	case _SUB_ENTITY_PROP_PLACEMENT:

		ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
		(*pretvar)->lVal = (e_sub_entity_placement)handle->placement;
		break;

	case _SUB_ENTITY_PROP_POSITION_X:

		ScriptVariant_ChangeType(*pretvar, VT_DECIMAL);
		(*pretvar)->dblVal = (DOUBLE)handle->position.x;
		break;

	case _SUB_ENTITY_PROP_POSITION_Y:

		ScriptVariant_ChangeType(*pretvar, VT_DECIMAL);
		(*pretvar)->dblVal = (DOUBLE)handle->position.y;
		break;

	case _SUB_ENTITY_PROP_POSITION_Z:

		ScriptVariant_ChangeType(*pretvar, VT_DECIMAL);
		(*pretvar)->dblVal = (DOUBLE)handle->position.z;
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

// Caskey, Damon V.
// 2019-12-11
//
// Access sub entity property by handle (pointer).
//
// set_sub_entity_property(void handle, int property, value)
HRESULT openbor_set_sub_entity_property(ScriptVariant** varlist, ScriptVariant** pretvar, int paramCount)
{
#define SELF_NAME           "set_sub_entity_property(void handle, int property, value)"
#define ARG_MINIMUM         3   // Minimum required arguments.
#define ARG_OBJECT          0   // Handle (pointer to property structure).
#define ARG_PROPERTY        1   // Property to access.
#define ARG_VALUE           2   // New value to apply.

	int                     result = S_OK; // Success or error?
	s_sub_entity			*handle = NULL; // Property handle.
	e_sub_entity_properties  property = 0;    // Property to access.

	// Value carriers to apply on properties after
	// taken from argument.
	LONG	temp_int;
	DOUBLE	temp_float;

	// Verify incoming arguments. There must be a
	// pointer for the animation handle, an integer
	// property, and a new value to apply.
	if (paramCount < ARG_MINIMUM
		|| varlist[ARG_OBJECT]->vt != VT_PTR
		|| varlist[ARG_PROPERTY]->vt != VT_INTEGER)
	{
		*pretvar = NULL;
		goto error_local;
	}
	else
	{
		handle = (s_sub_entity*)varlist[ARG_OBJECT]->ptrVal;
		property = (e_sub_entity_properties)varlist[ARG_PROPERTY]->lVal;
	}

	// Which property to modify?
	switch (property)
	{
	case _SUB_ENTITY_PROP_FRAME:

		if (SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
		{
			handle->frame = (int)temp_int;
		}

		break;	

	case _SUB_ENTITY_PROP_PLACEMENT:

		if (SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
		{
			handle->placement = (e_sub_entity_placement)temp_int;
		}

		break;

	case _SUB_ENTITY_PROP_POSITION_X:

		if (SUCCEEDED(ScriptVariant_DecimalValue(varlist[ARG_VALUE], &temp_float)))
		{
			handle->position.x = temp_float;
		}

		break;

	case _SUB_ENTITY_PROP_POSITION_Y:

		if (SUCCEEDED(ScriptVariant_DecimalValue(varlist[ARG_VALUE], &temp_float)))
		{
			handle->position.y = temp_float;
		}

		break;

	case _SUB_ENTITY_PROP_POSITION_Z:

		if (SUCCEEDED(ScriptVariant_DecimalValue(varlist[ARG_VALUE], &temp_float)))
		{
			handle->position.z = temp_float;
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
#undef ARG_OBJECT
#undef ARG_PROPERTY
#undef ARG_VALUE
}