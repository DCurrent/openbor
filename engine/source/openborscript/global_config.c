/*
 * OpenBOR - http://www.chronocrash.com
 * -----------------------------------------------------------------------
 * All rights reserved. See LICENSE in OpenBOR root for license details.
 *
 * Copyright (c) 2004 - 2017 OpenBOR Team
 */

 /* 
 * Global Config Properties
 * 2022-05-10
 * Caskey, Damon V.
 */

#include "scriptcommon.h"

/* 
* Use string property argument to find an
* integer property constant and populate
* varlist->lval.
*/ 
int mapstrings_global_config_property(ScriptVariant** varlist, int paramCount)
{
#define ARG_MINIMUM     2   // Minimum number of arguments allowed in varlist.
#define ARG_PROPERTY    1   // Varlist element carrying which property is requested.

	char* propname = NULL;  // Placeholder for string property name from varlist.
	int prop = 0;           // Placeholder for integer constant located by string.

	static const char* proplist[] =
	{
		"ajspecial",
		"block_ratio",
		"block_type",
		"cheats",
		"flash_layer_adjust",
		"flash_layer_source",
		"flash_z_source",
		"show_go"
	};

	//printf("\n\n mapstrings_global_config_property(%s)", varlist[ARG_PROPERTY]);

	/*
	* If the minimum argument count
	* was not passed, then there is
	* nothing to map. Return true - we'll
	* catch the mistake in property access
	* functions.
	*/
	if (paramCount < ARG_MINIMUM)
	{
		return 1;
	}

	/* See macro - will return 0 on fail. */
	MAPSTRINGS(varlist[ARG_PROPERTY], proplist, _GLOBAL_CONFIG_END,
		"Property name '%s' is not supported by global config.\n");

	//const char *eps = (varlist[ARG_PROPERTY]->lVal < _GLOBAL_CONFIG_END && varlist[ARG_PROPERTY]->lVal >= 0) ? proplist[varlist[ARG_PROPERTY]->lVal] : "";
	//printf("\Global Config arg: %s\n", eps);

	/* If we made it this far everything should be OK. */
	return 1;

#undef ARG_MINIMUM
#undef ARG_PROPERTY
}

/*
* Caskey, Damon  V.
* 2022-05-10
*
* Return a global config property. 
* Requires the handle from global 
* config variant and property name 
* to access.
*/
HRESULT openbor_get_global_config_property(ScriptVariant** varlist, ScriptVariant** pretvar, int paramCount)
{
#define SELF_NAME       "get_global_config_property(void global_config, char property)"
#define ARG_MINIMUM     2   // Minimum required arguments.
#define ARG_OBJECT      0   // Handle (pointer to property structure).
#define ARG_PROPERTY    1   // Property to access.

	s_global_config* handle = NULL; // Property handle.
	e_global_config_properties	property = 0;    // Property argument.

	/*
	* Clear pass by reference argument used to send
	* property data back to calling script.     .
	*/
	ScriptVariant_Clear(*pretvar);

	/*
	* Map string property name to a
	* matching integer constant.
	*/
	mapstrings_global_config_property(varlist, paramCount);

	/*
	* Verify arguments. There should at least
	* be a pointer for the property handle and an integer
	* to determine which property constant is accessed.
	*/
	if (paramCount < ARG_MINIMUM
		|| varlist[ARG_OBJECT]->vt != VT_PTR
		|| varlist[ARG_PROPERTY]->vt != VT_INTEGER)
	{
		*pretvar = NULL;
		goto error_local;
	}
	else
	{
		/* Populate local vars for readability. */
		handle = (s_global_config*)varlist[ARG_OBJECT]->ptrVal;
		property = (e_global_config_properties)varlist[ARG_PROPERTY]->lVal;
	}

	switch (property)
	{
	case _GLOBAL_CONFIG_AJSPECIAL:

		ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
		(*pretvar)->lVal = (e_ajspecial_config)handle->ajspecial;

		break;

	case _GLOBAL_CONFIG_BLOCK_RATIO:

		ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
		(*pretvar)->lVal = handle->block_ratio;

		break;

	case _GLOBAL_CONFIG_BLOCK_TYPE:

		ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
		(*pretvar)->lVal = handle->block_type;

		break;

	case _GLOBAL_CONFIG_CHEATS:

		ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
		(*pretvar)->lVal = (e_cheat_options)handle->cheats;

		break;

	case _GLOBAL_CONFIG_FLASH_LAYER_ADJUST:

		ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
		(*pretvar)->lVal = (int)handle->flash_layer_adjust;

		break;

	case _GLOBAL_CONFIG_FLASH_LAYER_SOURCE:

		ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
		(*pretvar)->lVal = (int)handle->flash_layer_source;

		break;	

	case _GLOBAL_CONFIG_FLASH_Z_SOURCE:

		ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
		(*pretvar)->lVal = (int)handle->flash_z_source;

		break;

	case _GLOBAL_CONFIG_SHOW_GO:

		ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
		(*pretvar)->lVal = (int)handle->showgo;

		break;

	default:

		printf("Unsupported property.\n");
		goto error_local;

		break;
	}

	return S_OK;

error_local:

	printf("\nYou must provide a valid pointer and property name: " SELF_NAME "\n");
	*pretvar = NULL;

	return E_FAIL;

#undef SELF_NAME
#undef ARG_MINIMUM
#undef ARG_OBJECT
#undef ARG_INDEX
}

/*
* Caskey, Damon  V.
* 2022-05-10
*
* Mutate a global config property. 
* Requires the handle from global 
* config, property name to modify,
* and the new value.
*/
HRESULT openbor_set_global_config_property(ScriptVariant** varlist, ScriptVariant** pretvar, int paramCount)
{
#define SELF_NAME           "set_global_config_property(void global_config, char property, mixed value)"
#define ARG_MINIMUM         3   // Minimum required arguments.
#define ARG_OBJECT          0   // Handle (pointer to property structure).
#define ARG_PROPERTY        1   // Property to access.
#define ARG_VALUE           2   // New value to apply.

	int                     result = S_OK; // Success or error?
	s_global_config* handle = NULL; // Property handle.
	e_global_config_properties    property = 0;    // Property to access.

	/*
	* Value carriers to apply on properties after
	* taken from argument.
	*/
	LONG         temp_int;

	/*
	* Map string property name to a
	* matching integer constant.
	*/
	mapstrings_global_config_property(varlist, paramCount);

	/*
	* Verify incoming arguments. There should at least
	* be a pointer for the property handle and an integer
	* to determine which property is accessed.
	*/
	if (paramCount < ARG_MINIMUM
		|| varlist[ARG_OBJECT]->vt != VT_PTR
		|| varlist[ARG_PROPERTY]->vt != VT_INTEGER)
	{
		*pretvar = NULL;
		goto error_local;
	}

	/* Populate local handleand property vars. */
	handle = (s_global_config*)varlist[ARG_OBJECT]->ptrVal;
	property = (e_global_config_properties)varlist[ARG_PROPERTY]->lVal;

	/* Which property to modify ? */
	switch (property)
	{

	case _GLOBAL_CONFIG_AJSPECIAL:

		if (SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
		{
			handle->ajspecial = temp_int;
		}

		break;

	case _GLOBAL_CONFIG_BLOCK_RATIO:

		if (SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
		{
			handle->block_ratio = temp_int;
		}

		break;

	case _GLOBAL_CONFIG_BLOCK_TYPE:

		if (SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
		{
			handle->block_type = temp_int;
		}

		break;

	case _GLOBAL_CONFIG_CHEATS:

		if (SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
		{
			handle->cheats = temp_int;
		}

		break;

	case _GLOBAL_CONFIG_FLASH_LAYER_ADJUST:

		if (SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
		{
			handle->flash_layer_adjust = temp_int;
		}

		break;

	case _GLOBAL_CONFIG_FLASH_LAYER_SOURCE:

		if (SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
		{
			handle->flash_layer_source = temp_int;
		}

		break;

	case _GLOBAL_CONFIG_FLASH_Z_SOURCE:

		if (SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
		{
			handle->flash_z_source = temp_int;
		}

		break;

	case _GLOBAL_CONFIG_SHOW_GO:

		if (SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
		{
			handle->showgo = temp_int;
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

	printf("\nYou must provide a valid pointer, property name, and new value: " SELF_NAME "\n");

	result = E_FAIL;
	return result;

#undef SELF_NAME
#undef ARG_MINIMUM
#undef ARG_OBJECT
#undef ARG_PROPERTY
#undef ARG_VALUE
}
