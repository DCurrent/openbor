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
		"counter_action",
		"drop",
		"energy_cost",
		"flipframe",
		"follow",
		"hit_count",
		"index",
		"jump_frame",
		"land_frame",
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

        case _ANIMATION_PROP_COUNTER_ACTION:

            // Verify animation has item.
            if(handle->counter_action)
            {
                ScriptVariant_ChangeType(*pretvar, VT_PTR);
                (*pretvar)->ptrVal = (s_counter_action *)handle->counter_action;
            }

            break;

		case _ANIMATION_PROP_DROP:

			// Verify animation has item.
			if (handle->dropframe)
			{
				ScriptVariant_ChangeType(*pretvar, VT_PTR);
				(*pretvar)->ptrVal = (s_onframe_set *)handle->dropframe;
			}

			break;

		case _ANIMATION_PROP_HIT_COUNT:

			ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
			(*pretvar)->lVal = (LONG)handle->hit_count;
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

		case _ANIMATION_PROP_COUNTER_ACTION:

			handle->counter_action = (s_counter_action *)varlist[ARG_VALUE]->ptrVal;

			break;

		case _ANIMATION_PROP_DROP:

			handle->dropframe = (s_onframe_set *)varlist[ARG_VALUE]->ptrVal;

			break;

        case _ANIMATION_PROP_HIT_COUNT:

			if (SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
			{
				handle->hit_count = (int)temp_int;
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

// Frame set

// Use string property argument to find an
// integer property constant and populate
// varlist->lval.
int mapstrings_frame_set_property(ScriptVariant** varlist, int paramCount)
{
#define ARG_MINIMUM     2   // Minimum number of arguments allowed in varlist.
#define ARG_PROPERTY    1   // Varlist element carrying which property is requested.

	char* propname = NULL;  // Placeholder for string property name from varlist.
	int prop;               // Placeholder for integer constant located by string.

	static const char* proplist[] =
	{
		"frame",
		"model_index"
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
	MAPSTRINGS(varlist[ARG_PROPERTY], proplist, _FRAME_SET_PROP_END,
		"\n\n Error: '%s' is not a known frame set property.\n");


	// If we made it this far everything should be OK.
	return 1;

#undef ARG_MINIMUM
#undef ARG_PROPERTY
}

// Get frame set property.
// Caskey, Damon V.
// 2019-11-21
//
// Access property by handle (pointer).
//
// get_frame_set_property(void handle, char property)
HRESULT openbor_get_frame_set_property(ScriptVariant** varlist, ScriptVariant** pretvar, int paramCount)
{
#define SELF_NAME       "get_frame_set_property(void handle, char property)"
#define ARG_MINIMUM     2   // Minimum required arguments.
#define ARG_HANDLE      0   // Handle (pointer to property structure).
#define ARG_PROPERTY    1   // Property to access.

	int                     result = S_OK; // Success or error?
	s_onframe_set*			handle = NULL; // Property handle.
	e_frame_set_properties  property = 0;    // Property argument.

	// Clear pass by reference argument used to send
	// property data back to calling script.
	ScriptVariant_Clear(*pretvar);

	// Map string property name to a
	// matching integer constant.
	mapstrings_frame_set_property(varlist, paramCount);

	// Verify incoming arguments. There should at least
	// be a pointer for the property handle and an integer
	// to determine which property is accessed.
	if (paramCount < ARG_MINIMUM
		|| varlist[ARG_HANDLE]->vt != VT_PTR
		|| varlist[ARG_PROPERTY]->vt != VT_INTEGER)
	{
		*pretvar = NULL;
		goto error_local;
	}
	else
	{
		handle = (s_onframe_set *)varlist[ARG_HANDLE]->ptrVal;
		property = (LONG)varlist[ARG_PROPERTY]->lVal;
	}

	// Which property to get?
	switch (property)
	{
	case _FRAME_SET_PROP_FRAME:

		ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
		(*pretvar)->lVal = (LONG)handle->frame;
		break;

	case _FRAME_SET_PROP_MODEL_INDEX:

		ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
		(*pretvar)->lVal = (LONG)handle->model_index;
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

// Set frame set properties.
// Caskey, Damon V.
// 2016-10-20
//
// Access property by handle (pointer).
//
// set_frame_set_property(void handle, char property, mixed value)
HRESULT openbor_set_frame_set_property(ScriptVariant** varlist, ScriptVariant** pretvar, int paramCount)
{
#define SELF_NAME           "set_frame_set_property(void handle, char property, mixed value)"
#define ARG_MINIMUM         3   // Minimum required arguments.
#define ARG_HANDLE          0   // Handle (pointer to property structure).
#define ARG_PROPERTY        1   // Property to access.
#define ARG_VALUE           2   // New value to apply.

	int                     result = S_OK; // Success or error?
	s_onframe_set*			handle = NULL; // Property handle.
	e_frame_set_properties  property = 0;    // Property to access.

	// Value carriers to apply on properties after
	// taken from argument.
	LONG	temp_int;
	
	// Map string property name to a
	// matching integer constant.
	mapstrings_frame_set_property(varlist, paramCount);

	// Verify incoming arguments. There must be a
	// pointer for the animation handle, an integer
	// property, and a new value to apply.
	if (paramCount < ARG_MINIMUM
		|| varlist[ARG_HANDLE]->vt != VT_PTR
		|| varlist[ARG_PROPERTY]->vt != VT_INTEGER)
	{
		*pretvar = NULL;
		goto error_local;
	}
	else
	{
		handle = (s_onframe_set *)varlist[ARG_HANDLE]->ptrVal;
		property = (LONG)varlist[ARG_PROPERTY]->lVal;
	}

	// Which property to modify?
	switch (property)
	{
	case _FRAME_SET_PROP_FRAME:

		if (SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
		{
			handle->frame = (unsigned int)temp_int;
		}

		break;

	case _FRAME_SET_PROP_MODEL_INDEX:

		if (SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
		{
			handle->model_index = (unsigned int)temp_int;
		}

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

// allocate_frame_set();
HRESULT openbor_allocate_frame_set(ScriptVariant** varlist, ScriptVariant** pretvar, int paramCount)
{

#define SELF_NAME  "openbor_allocate_frame_set()"

	int				result = S_OK; // Success or error?
	s_onframe_set	* frame_set;

	// Run allocation function, and verify we got a pointer.
	frame_set = allocate_frame_set();
		
	if (!frame_set)
	{
		goto error_local;
	}

	// Now place our allocated pointer into the return var, and verify.
	ScriptVariant_ChangeType(*pretvar, VT_PTR);
	(*pretvar)->ptrVal = (s_onframe_set*)frame_set;

	if ((*pretvar)->ptrVal == NULL)
	{
		(*pretvar) = NULL;
		goto error_local;
	}

	// Everything looks good, return OK.
	return result;

// Error trapping.
error_local:

	printf("Failed to allocate a frame set. May be out of memory: " SELF_NAME "\n");

	result = E_FAIL;
	return result;

#undef SELF_NAME
}

// Energy cost

// Use string property argument to find an
// integer property constant and populate
// varlist->lval.
int mapstrings_energy_cost_property(ScriptVariant** varlist, int paramCount)
{
#define ARG_MINIMUM     2   // Minimum number of arguments allowed in varlist.
#define ARG_PROPERTY    1   // Varlist element carrying which property is requested.

	char* propname = NULL;  // Placeholder for string property name from varlist.
	int prop;               // Placeholder for integer constant located by string.

	static const char* proplist[] =
	{
		"frame",
		"model_index"
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
	MAPSTRINGS(varlist[ARG_PROPERTY], proplist, _ENERGY_COST_PROP_END,
		"\n\n Error: '%s' is not a known frame set property.\n");


	// If we made it this far everything should be OK.
	return 1;

#undef ARG_MINIMUM
#undef ARG_PROPERTY
}

// Get frame set property.
// Caskey, Damon V.
// 2019-11-21
//
// Access property by handle (pointer).
//
// get_energy_cost_property(void handle, char property)
HRESULT openbor_get_energy_cost_property(ScriptVariant** varlist, ScriptVariant** pretvar, int paramCount)
{
#define SELF_NAME       "get_energy_cost_property(void handle, char property)"
#define ARG_MINIMUM     2   // Minimum required arguments.
#define ARG_HANDLE      0   // Handle (pointer to property structure).
#define ARG_PROPERTY    1   // Property to access.

	int                     result = S_OK; // Success or error?
	s_energy_cost* handle = NULL; // Property handle.
	e_energy_cost_properties  property = 0;    // Property argument.

	// Clear pass by reference argument used to send
	// property data back to calling script.
	ScriptVariant_Clear(*pretvar);

	// Map string property name to a
	// matching integer constant.
	mapstrings_energy_cost_property(varlist, paramCount);

	// Verify incoming arguments. There should at least
	// be a pointer for the property handle and an integer
	// to determine which property is accessed.
	if (paramCount < ARG_MINIMUM
		|| varlist[ARG_HANDLE]->vt != VT_PTR
		|| varlist[ARG_PROPERTY]->vt != VT_INTEGER)
	{
		*pretvar = NULL;
		goto error_local;
	}
	else
	{
		handle = (s_energy_cost*)varlist[ARG_HANDLE]->ptrVal;
		property = (LONG)varlist[ARG_PROPERTY]->lVal;
	}

	// Which property to get?
	switch (property)
	{
	case _ENERGY_COST_PROP_COST:

		ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
		(*pretvar)->lVal = (LONG)handle->cost;
		break;

	case _ENERGY_COST_PROP_MP_ONLY:

		ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
		(*pretvar)->lVal = (LONG)handle->mponly;
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

// Set frame set properties.
// Caskey, Damon V.
// 2016-10-20
//
// Access property by handle (pointer).
//
// set_energy_cost_property(void handle, char property, mixed value)
HRESULT openbor_set_energy_cost_property(ScriptVariant** varlist, ScriptVariant** pretvar, int paramCount)
{
#define SELF_NAME           "set_energy_cost_property(void handle, char property, mixed value)"
#define ARG_MINIMUM         3   // Minimum required arguments.
#define ARG_HANDLE          0   // Handle (pointer to property structure).
#define ARG_PROPERTY        1   // Property to access.
#define ARG_VALUE           2   // New value to apply.

	int                     result = S_OK; // Success or error?
	s_energy_cost* handle = NULL; // Property handle.
	e_energy_cost_properties  property = 0;    // Property to access.

	// Value carriers to apply on properties after
	// taken from argument.
	LONG	temp_int;

	// Map string property name to a
	// matching integer constant.
	mapstrings_energy_cost_property(varlist, paramCount);

	// Verify incoming arguments. There must be a
	// pointer for the animation handle, an integer
	// property, and a new value to apply.
	if (paramCount < ARG_MINIMUM
		|| varlist[ARG_HANDLE]->vt != VT_PTR
		|| varlist[ARG_PROPERTY]->vt != VT_INTEGER)
	{
		*pretvar = NULL;
		goto error_local;
	}
	else
	{
		handle = (s_energy_cost*)varlist[ARG_HANDLE]->ptrVal;
		property = (LONG)varlist[ARG_PROPERTY]->lVal;
	}

	// Which property to modify?
	switch (property)
	{
	case _ENERGY_COST_PROP_COST:

		if (SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
		{
			handle->cost = (unsigned int)temp_int;
		}

		break;

	case _ENERGY_COST_PROP_MP_ONLY:

		if (SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
		{
			handle->mponly = (unsigned int)temp_int;
		}

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

// allocate_energy_cost();
HRESULT openbor_allocate_energy_cost(ScriptVariant** varlist, ScriptVariant** pretvar, int paramCount)
{

#define SELF_NAME  "openbor_allocate_energy_cost()"

	int				result = S_OK; // Success or error?
	s_energy_cost* energy_cost = NULL;

	// Run allocation function, and verify we got a pointer.
	energy_cost = allocate_energy_cost();

	if (!energy_cost)
	{
		goto error_local;
	}

	// Now place our allocated pointer into the return var, and verify.
	ScriptVariant_ChangeType(*pretvar, VT_PTR);
	(*pretvar)->ptrVal = (s_energy_cost*)energy_cost;

	if ((*pretvar)->ptrVal == NULL)
	{
		(*pretvar) = NULL;
		goto error_local;
	}

	// Everything looks good, return OK.
	return result;

	// Error trapping.
error_local:

	printf("Failed to allocate a frame set. May be out of memory: " SELF_NAME "\n");

	result = E_FAIL;
	return result;

#undef SELF_NAME
}