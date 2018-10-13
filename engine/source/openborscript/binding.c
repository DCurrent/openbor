/*
 * OpenBOR - http://www.chronocrash.com
 * -----------------------------------------------------------------------
 * All rights reserved. See LICENSE in OpenBOR root for license details.
 *
 * Copyright (c) 2004 - 2017 OpenBOR Team
 */

// Binding Properties
// 2018-03-31
// Caskey, Damon V.

#include "scriptcommon.h"

// Use string property argument to find an
// integer property constant and populate
// varlist->lval.
int mapstrings_binding(ScriptVariant **varlist, int paramCount)
{
    #define ARG_MINIMUM     2   // Minimum number of arguments allowed in varlist.
    #define ARG_PROPERTY    1   // Varlist element carrying which property is requested.

    char *propname = NULL;  // Placeholder for string property name from varlist.
    int prop;               // Placeholder for integer constant located by string.

    static const char *proplist[] =
    {
        "direction",
        "matching",
        "offset",
        "overriding",
        "positioning",
        "sort_id",
        "tag",
        "target"
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
    MAPSTRINGS(varlist[ARG_PROPERTY], proplist, _BINDING_END,
               "Property name '%s' is not supported by binding.\n");


    // If we made it this far everything should be OK.
    return 1;

    #undef ARG_MINIMUM
    #undef ARG_PROPERTY
}

// Caskey, Damon  V.
// 2018-03-31
//
// Return a binding property. Requires
// the handle from binding entity
// property and property name to
// access.
HRESULT openbor_get_binding_property(ScriptVariant **varlist , ScriptVariant **pretvar, int paramCount)
{
    #define SELF_NAME       "openbor_get_binding_property(void handle, char property)"
    #define ARG_MINIMUM     2   // Minimum required arguments.
    #define ARG_HANDLE      0   // Handle (pointer to property structure).
    #define ARG_PROPERTY    1   // Property to access.

    s_bind                  *handle     = NULL; // Property handle.
    e_binding_properties    property    = 0;    // Property argument.

    // Clear pass by reference argument used to send
    // property data back to calling script.     .
    ScriptVariant_Clear(*pretvar);

    // Map string property name to a
    // matching integer constant.
    mapstrings_binding(varlist, paramCount);

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
        handle      = (s_bind *)varlist[ARG_HANDLE]->ptrVal;
        property    = (LONG)varlist[ARG_PROPERTY]->lVal;
    }

    switch(property)
    {
        case _BINDING_MATCHING:

            ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
            (*pretvar)->lVal = (LONG)handle->matching;

            break;

        case _BINDING_DIRECTION:

            ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
            (*pretvar)->lVal = (LONG)handle->direction;

            break;

        case _BINDING_OFFSET:

            ScriptVariant_ChangeType(*pretvar, VT_PTR);
            (*pretvar)->ptrVal = (VOID *)&handle->offset;

            break;

        case _BINDING_OVERRIDING:

            ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
            (*pretvar)->lVal = (LONG)handle->overriding;

            break;

        case _BINDING_POSITIONING:

            ScriptVariant_ChangeType(*pretvar, VT_PTR);
            (*pretvar)->ptrVal = (VOID *)&handle->positioning;

            break;

        case _BINDING_SORT_ID:

            ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
            (*pretvar)->lVal = (LONG)handle->sortid;

            break;

        case _BINDING_TAG:

            ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
            (*pretvar)->lVal = (LONG)handle->tag;

            break;

        case _BINDING_TARGET:

            // If there is no entity bound, we just
            // leave the return var empty.
            if(handle->ent)
            {
                ScriptVariant_ChangeType(*pretvar, VT_PTR);
                (*pretvar)->ptrVal = (entity *)handle->ent;
            }

            break;

        default:

            printf("Unsupported property.\n");
            goto error_local;

            break;
    }

    return S_OK;

    error_local:

    printf("You must provide a valid binding handle and property name: " SELF_NAME "\n");
    *pretvar = NULL;

    return E_FAIL;

    #undef SELF_NAME
    #undef ARG_MINIMUM
    #undef ARG_HANDLE
    #undef ARG_INDEX
}

// Caskey, Damon  V.
// 2018-04-01
//
// Mutate a binding property. Requires
// the handle from binding entity
// property, property name to modify,
// and the new value.
HRESULT openbor_set_binding_property(ScriptVariant **varlist, ScriptVariant **pretvar, int paramCount)
{
    #define SELF_NAME           "set_binding_property(void handle, char property, value)"
    #define ARG_MINIMUM         3   // Minimum required arguments.
    #define ARG_HANDLE          0   // Handle (pointer to property structure).
    #define ARG_PROPERTY        1   // Property to access.
    #define ARG_VALUE           2   // New value to apply.

	int                     result = S_OK; // Success or error?
	s_bind                  *handle = NULL; // Property handle.
	e_binding_properties    property = 0;    // Property to access.

	// Value carriers to apply on properties after
	// taken from argument.
	LONG         temp_int;

	// Map string property name to a
	// matching integer constant.
	mapstrings_binding(varlist, paramCount);

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

	// Populate local handle and property vars.
	handle = (s_bind *)varlist[ARG_HANDLE]->ptrVal;
	property = (LONG)varlist[ARG_PROPERTY]->lVal;

	// Which property to modify?
	switch (property)
	{

	case _BINDING_MATCHING:

		if (SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
		{
			handle->matching = temp_int;
		}

		break;

	case _BINDING_DIRECTION:

		if (SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
		{
			handle->direction = temp_int;
		}

		break;

	case _BINDING_OFFSET:

		// Read only.

		break;

    case _BINDING_OVERRIDING:

		if (SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
		{
			handle->overriding = temp_int;
		}

		break;

    case _BINDING_POSITIONING:

		// Read only.

		break;

	case _BINDING_SORT_ID:

		if (SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
		{
			handle->sortid = temp_int;
		}

		break;

    case _BINDING_TAG:

		if (SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
		{
			handle->tag = temp_int;
		}

		break;

	case _BINDING_TARGET:

		handle->ent = (entity *)varlist[ARG_VALUE]->ptrVal;

		break;

	default:

		printf("Unsupported property.\n");
		goto error_local;

		break;
	}

	return result;

	// Error trapping.
    error_local:

	printf("You must provide a valid binding handle, property, and new value: " SELF_NAME "\n");

	result = E_FAIL;
	return result;

    #undef SELF_NAME
    #undef ARG_MINIMUM
    #undef ARG_HANDLE
    #undef ARG_PROPERTY
    #undef ARG_VALUE
}

// Caskey, Damon  V.
// 2018-10-11
//
// Run engine's internal bind update function.
HRESULT openbor_update_binding(ScriptVariant **varlist, ScriptVariant **pretvar, int paramCount)
{
#define SELF_NAME           "update_binding(void entity)"
#define ARG_MINIMUM         1   // Minimum required arguments.
#define ARG_ENTITY          0   // Target entity.

	int		result	= S_OK;	// Success or error?
	entity	*ent	= NULL; // Target entity.

	// Verify incoming arguments.
	if (paramCount < ARG_MINIMUM
		|| varlist[ARG_ENTITY]->vt != VT_PTR)
	{
		*pretvar = NULL;
		goto error_local;
	}

	// Populate local handle and property vars.
	ent = (entity *)varlist[ARG_ENTITY]->ptrVal;

	adjust_bind(ent);

	return result;

	// Error trapping.
error_local:

	printf("You must provide a valid entity: " SELF_NAME "\n");

	result = E_FAIL;
	return result;

#undef SELF_NAME
#undef ARG_MINIMUM
#undef ARG_ENTITY
}