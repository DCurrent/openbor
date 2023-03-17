/*
 * OpenBOR - http://www.chronocrash.com
 * -----------------------------------------------------------------------
 * All rights reserved. See LICENSE in OpenBOR root for license details.
 *
 * Copyright (c) 2004 - 2019 OpenBOR Team
 */

#include "scriptcommon.h"

 // Use string property argument to find an
 // integer property constant and populate
 // varlist->lval.
int mapstrings_recursive_damage_property(ScriptVariant **varlist, int paramCount)
{
#define ARG_MINIMUM     2   // Minimum number of arguments allowed in varlist.
#define ARG_PROPERTY    1   // Varlist element carrying which property is requested.

	char *propname = NULL;  // Placeholder for string property name from varlist.
	int prop;               // Placeholder for integer constant located by string.

	static const char *proplist[] =
	{
		"force",
		"index",
		"mode",
		"next",
		"owner",
		"rate",
		"tag",
		"tick",
		"time",
		"type",
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
	MAPSTRINGS(varlist[ARG_PROPERTY], proplist, _RECURSIVE_DAMAGE_END,
		"\n\n Error: '%s' is not a known recursive damage property.\n");


	// If we made it this far everything should be OK.
	return 1;

#undef ARG_MINIMUM
#undef ARG_PROPERTY
}


// Caskey, Damon  V.
// 2018-04-02
//
// Return a property. Requires
// a pointer and property name to
// access.
HRESULT openbor_get_recursive_damage_property(ScriptVariant **varlist, ScriptVariant **pretvar, int paramCount)
{
#define SELF_NAME       "openbor_get_recursive_damage_property(void handle, char property)"
#define ARG_MINIMUM     2   // Minimum required arguments.
#define ARG_OBJECT      0   // Handle (pointer to property structure).
#define ARG_PROPERTY    1   // Property to access.

	s_damage_recursive		*handle = NULL; // Property handle.
	e_recursive_damage_properties    property = 0;    // Property argument.

	// Clear pass by reference argument used to send
	// property data back to calling script.     .
	ScriptVariant_Clear(*pretvar);

	// Verify arguments. There should at least
	// be a pointer for the property handle and an integer
	// to determine which property constant is accessed.
	if (paramCount < ARG_MINIMUM
		|| varlist[ARG_OBJECT]->vt != VT_PTR
		|| varlist[ARG_PROPERTY]->vt != VT_INTEGER)
	{
		*pretvar = NULL;
		goto error_local;
	}
	else
	{
		// Populate local vars for readability.
		handle = (s_damage_recursive *)varlist[ARG_OBJECT]->ptrVal;
		property = (LONG)varlist[ARG_PROPERTY]->lVal;
	}

	switch (property)
	{
	case _RECURSIVE_DAMAGE_FORCE:

		ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
		(*pretvar)->lVal = (LONG)handle->force;

		break;

	case _RECURSIVE_DAMAGE_INDEX:

		ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
		(*pretvar)->lVal = (LONG)handle->force;

		break;

	case _RECURSIVE_DAMAGE_MODE:

		ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
		(*pretvar)->lVal = (LONG)handle->index;

		break;

	case _RECURSIVE_DAMAGE_NEXT:

		if (handle->next)
		{
			ScriptVariant_ChangeType(*pretvar, VT_PTR);
			(*pretvar)->ptrVal = (VOID *)handle->next;
		}

		break;

	case _RECURSIVE_DAMAGE_OWNER:

		if (handle->owner)
		{
			ScriptVariant_ChangeType(*pretvar, VT_PTR);
			(*pretvar)->ptrVal = (VOID *)handle->owner;
		}

		break;

	case _RECURSIVE_DAMAGE_RATE:

		ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
		(*pretvar)->lVal = (LONG)handle->rate;

		break;

	case _RECURSIVE_DAMAGE_TAG:

		ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
		(*pretvar)->lVal = (LONG)handle->meta_tag;

		break;

	case _RECURSIVE_DAMAGE_TICK:

		ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
		(*pretvar)->lVal = (LONG)handle->tick;

		break;

	case _RECURSIVE_DAMAGE_TIME:

		ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
		(*pretvar)->lVal = (LONG)handle->time;

		break;

	case _RECURSIVE_DAMAGE_TYPE:

		ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
		(*pretvar)->lVal = (LONG)handle->type;

		break;
		
	default:

		printf("Unsupported property.\n");
		goto error_local;

		break;
	}

	return S_OK;

error_local:

	printf("You must provide a valid handle and property name: " SELF_NAME "\n");
	*pretvar = NULL;

	return E_FAIL;

#undef SELF_NAME
#undef ARG_MINIMUM
#undef ARG_OBJECT
#undef ARG_INDEX
}

// Caskey, Damon  V.
// 2018-04-03
//
// Mutate a entity property. Requires
// the pointer, a string property
// name, and new value.
HRESULT openbor_set_recursive_damage_property(ScriptVariant **varlist, ScriptVariant **pretvar, int paramCount)
{
#define SELF_NAME           "openbor_set_recursive_damage_property(void handle, char property, value)"
#define ARG_MINIMUM         3   // Minimum required arguments.
#define ARG_OBJECT          0   // Handle (pointer to property structure).
#define ARG_PROPERTY        1   // Property to access.
#define ARG_VALUE           2   // New value to apply.

	int								result	= S_OK;	// Success or error?
	s_damage_recursive				*handle		= NULL;	// Property handle.
	e_recursive_damage_properties	property	= 0;	// Property to access.

	// Value carriers to apply on properties after
	// taken from argument.
	LONG    temp_int;

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

	// Populate local handle and property vars.
	handle = (s_damage_recursive *)varlist[ARG_OBJECT]->ptrVal;
	property = (LONG)varlist[ARG_PROPERTY]->lVal;

	// Which property to modify?
	switch (property)
	{

	case _RECURSIVE_DAMAGE_FORCE:

		if (SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
		{
			handle->force = temp_int;
		}

		break;

	case _RECURSIVE_DAMAGE_INDEX:

		if (SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
		{
			handle->index = temp_int;
		}

		break;

	case _RECURSIVE_DAMAGE_MODE:

		if (SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
		{
			handle->mode = temp_int;
		}

		break;

	case _RECURSIVE_DAMAGE_NEXT:

		handle->next = (s_damage_recursive *)varlist[ARG_VALUE]->ptrVal;

		break;

	case _RECURSIVE_DAMAGE_OWNER:

		handle->owner = (entity *)varlist[ARG_VALUE]->ptrVal;

		break;

	case _RECURSIVE_DAMAGE_RATE:

		if (SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
		{
			handle->rate = temp_int;
		}

		break;

	case _RECURSIVE_DAMAGE_TAG:

		if (SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
		{
			handle->meta_tag = temp_int;
		}

		break;

	case _RECURSIVE_DAMAGE_TICK:

		if (SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
		{
			handle->tick = temp_int;
		}

		break;

	case _RECURSIVE_DAMAGE_TIME:

		if (SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
		{
			handle->time = temp_int;
		}

		break;

	case _RECURSIVE_DAMAGE_TYPE:

		if (SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
		{
			handle->type = temp_int;
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
