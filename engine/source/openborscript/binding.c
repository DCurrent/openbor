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
int mapstrings_bind_property(ScriptVariant **varlist, int paramCount)
{
    #define ARG_MINIMUM     2   // Minimum number of arguments allowed in varlist.
    #define ARG_PROPERTY    1   // Varlist element carrying which property is requested.

    char *propname = NULL;  // Placeholder for string property name from varlist.
    int prop;               // Placeholder for integer constant located by string.

    static const char *proplist[] =
    {
		"animation_frame",
		"animation_id",
		"animation_match",
		"direction",
		"mode_x",
		"mode_y",
		"mode_z",
		"offset_x",
		"offset_y",
		"offset_z",
        "override",
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
    MAPSTRINGS(varlist[ARG_PROPERTY], proplist, _BIND_END,
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
HRESULT openbor_get_bind_property(ScriptVariant **varlist , ScriptVariant **pretvar, int paramCount)
{
    #define SELF_NAME       "openbor_get_bind_property(void bind, char property)"
    #define ARG_MINIMUM     2   // Minimum required arguments.
    #define ARG_HANDLE      0   // Handle (pointer to property structure).
    #define ARG_PROPERTY    1   // Property to access.

    s_bind              *handle     = NULL; // Property handle.
    e_bind_properties	property    = 0;    // Property argument.

    // Clear pass by reference argument used to send
    // property data back to calling script.     .
    ScriptVariant_Clear(*pretvar);

    // Map string property name to a
    // matching integer constant.
    mapstrings_bind_property(varlist, paramCount);

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
		case _BIND_ANIMATION_FRAME:

			ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
			(*pretvar)->lVal = (LONG)handle->frame;

			break;

		case _BIND_ANIMATION_ID:

			ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
			(*pretvar)->lVal = (LONG)handle->animation;

			break;

		case _BIND_ANIMATION_MATCH:

			ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
			(*pretvar)->lVal = (LONG)handle->match;

			break;

        case _BIND_DIRECTION:

            ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
            (*pretvar)->lVal = (LONG)handle->direction;

            break;

		case _BIND_MODE_X:

			ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
			(*pretvar)->lVal = (LONG)handle->positioning.x;

			break;

		case _BIND_MODE_Y:

			ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
			(*pretvar)->lVal = (LONG)handle->positioning.y;

			break;

		case _BIND_MODE_Z:

			ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
			(*pretvar)->lVal = (LONG)handle->positioning.z;

			break;

        case _BIND_OFFSET_X:

			ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
			(*pretvar)->lVal = (LONG)handle->offset.x;

			break;

		case _BIND_OFFSET_Y:

			ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
			(*pretvar)->lVal = (LONG)handle->offset.y;

			break;

		case _BIND_OFFSET_Z:

			ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
			(*pretvar)->lVal = (LONG)handle->offset.z;

			break;

        case _BIND_OVERRIDE:

            ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
            (*pretvar)->lVal = (LONG)handle->overriding;

            break;

        case _BIND_SORT_ID:

            ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
            (*pretvar)->lVal = (LONG)handle->sortid;

            break;

        case _BIND_TAG:

            ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
            (*pretvar)->lVal = (LONG)handle->tag;

            break;

        case _BIND_TARGET:

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

    printf("\nYou must provide a valid pointer and property name: " SELF_NAME "\n");
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
HRESULT openbor_set_bind_property(ScriptVariant **varlist, ScriptVariant **pretvar, int paramCount)
{
    #define SELF_NAME           "set_bind_property(void bind, char property, mixed value)"
    #define ARG_MINIMUM         3   // Minimum required arguments.
    #define ARG_HANDLE          0   // Handle (pointer to property structure).
    #define ARG_PROPERTY        1   // Property to access.
    #define ARG_VALUE           2   // New value to apply.

	int                     result = S_OK; // Success or error?
	s_bind                  *handle = NULL; // Property handle.
	e_bind_properties    property = 0;    // Property to access.

	// Value carriers to apply on properties after
	// taken from argument.
	LONG         temp_int;

	// Map string property name to a
	// matching integer constant.
	mapstrings_bind_property(varlist, paramCount);

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

		case _BIND_ANIMATION_FRAME:

			if (SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
			{
				handle->frame = temp_int;
			}

			break;

		case _BIND_ANIMATION_ID:

			if (SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
			{
				handle->animation = temp_int;
			}

			break;

		case _BIND_ANIMATION_MATCH:

			if (SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
			{
				handle->match = temp_int;
			}

			break;

		case _BIND_DIRECTION:

			if (SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
			{
				handle->direction = temp_int;
			}

			break;

		case _BIND_MODE_X:

			if (SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
			{
				handle->positioning.x = temp_int;
			}

			break;

		case _BIND_MODE_Y:

			if (SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
			{
				handle->positioning.y = temp_int;
			}

			break;

		case _BIND_MODE_Z:

			if (SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
			{
				handle->positioning.z = temp_int;
			}

			break;

		case _BIND_OFFSET_X:

			if (SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
			{
				handle->offset.x = temp_int;
			}

			break;

		case _BIND_OFFSET_Y:

			if (SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
			{
				handle->offset.y = temp_int;
			}

			break;

		case _BIND_OFFSET_Z:

			if (SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
			{
				handle->offset.z = temp_int;
			}

			break;

		case _BIND_OVERRIDE:

			if (SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
			{
				handle->overriding = temp_int;
			}

			break;

		case _BIND_SORT_ID:

			if (SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
			{
				handle->sortid = temp_int;
			}

			break;

		case _BIND_TAG:

			if (SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
			{
				handle->tag = temp_int;
			}

			break;

		case _BIND_TARGET:

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

	printf("\nYou must provide a valid pointer, property name, and new value: " SELF_NAME "\n");

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
HRESULT openbor_update_bind(ScriptVariant **varlist, ScriptVariant **pretvar, int paramCount)
{
#define SELF_NAME           "update_bind(void entity)"
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