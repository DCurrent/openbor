/*
 * OpenBOR - http://www.chronocrash.com
 * -----------------------------------------------------------------------
 * All rights reserved. See LICENSE in OpenBOR root for license details.
 *
 * Copyright (c) 2004 OpenBOR Team
 */

// Binding Properties
// 2018-03-31
// Caskey, Damon V.

#include "scriptcommon.h"

/*
* Caskey, Damon  V.
* 2018-03-31
*
* Return a binding property. Requires
* the object from binding entity
* property and property name to
* access.
*/
HRESULT openbor_get_bind_property(ScriptVariant **varlist , ScriptVariant **pretvar, int paramCount)
{
    #define SELF_NAME       "openbor_get_bind_property(void bind, int property)"
    #define ARG_MINIMUM     2   // Minimum required arguments.
    #define ARG_HANDLE      0   // Handle (pointer to property structure).
    #define ARG_PROPERTY    1   // Property to access.

    s_bind              *object     = NULL; // Property object.
    e_bind_properties	property    = 0;    // Property argument.

    /* 
	* Clear pass by reference argument used to send
    * property data back to calling script.     .
    */
	ScriptVariant_Clear(*pretvar);

	/*
    * Verify arguments. There should at least
    * be a pointer for the property object and an integer
    * to determine which property constant is accessed.
    */
	if(paramCount < ARG_MINIMUM
       || varlist[ARG_HANDLE]->vt != VT_PTR
       || varlist[ARG_PROPERTY]->vt != VT_INTEGER)
    {
        *pretvar = NULL;
        goto error_local;
    }
    else
    {
        /* Populate local vars for readability. */
        object      = (s_bind *)varlist[ARG_HANDLE]->ptrVal;
        property    = (LONG)varlist[ARG_PROPERTY]->lVal;
    }

    switch(property)
    {
		case BIND_PROPERTY_ANIMATION_FRAME:

			ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
			(*pretvar)->lVal = (LONG)object->frame;

			break;

		case BIND_PROPERTY_ANIMATION_ID:

			ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
			(*pretvar)->lVal = (LONG)object->animation;

			break;

		case BIND_PROPERTY_CONFIG:

			ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
			(*pretvar)->lVal = (e_bind_config)object->config;

			break;

        case BIND_PROPERTY_DIRECTION_ADJUST:

            ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
            (*pretvar)->lVal = (e_direction_adjust)object->direction_adjust;

            break;

		case BIND_PROPERTY_META_DATA:

			ScriptVariant_ChangeType(*pretvar, VT_PTR);
			(*pretvar)->ptrVal = (s_meta_data*)object->meta_data;

			break;

		case BIND_PROPERTY_META_TAG:

			ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
			(*pretvar)->lVal = (LONG)object->meta_tag;

			break;		

        case BIND_PROPERTY_OFFSET_X:

			ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
			(*pretvar)->lVal = (LONG)object->offset.x;

			break;

		case BIND_PROPERTY_OFFSET_Y:

			ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
			(*pretvar)->lVal = (LONG)object->offset.y;

			break;

		case BIND_PROPERTY_OFFSET_Z:

			ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
			(*pretvar)->lVal = (LONG)object->offset.z;

			break;			       

        case BIND_PROPERTY_SORT_ID:

            ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
            (*pretvar)->lVal = (LONG)object->sortid;

            break;

        case BIND_PROPERTY_TARGET:

            // If there is no entity bound, we just
            // leave the return var empty.
            if(object->target)
            {
                ScriptVariant_ChangeType(*pretvar, VT_PTR);
                (*pretvar)->ptrVal = (entity *)object->target;
            }

            break;

        default:

            printf("Unsupported property.\n");
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
    #undef ARG_HANDLE
    #undef ARG_INDEX
}

/*
* Caskey, Damon  V.
* 2018-04-01
*
* Mutate a binding property. Requires
* the object from binding entity
* property, property name to modify,
* and the new value.
*/
HRESULT openbor_set_bind_property(ScriptVariant **varlist, ScriptVariant **pretvar, int paramCount)
{
    #define SELF_NAME           "set_bind_property(void bind, int property, mixed value)"
    #define ARG_MINIMUM         3   // Minimum required arguments.
    #define ARG_HANDLE          0   // Handle (pointer to property structure).
    #define ARG_PROPERTY        1   // Property to access.
    #define ARG_VALUE           2   // New value to apply.

	int                     result = S_OK; // Success or error?
	s_bind                  *object = NULL; // Property object.
	e_bind_properties    property = 0;    // Property to access.

	/*
	* Value carriers to apply on properties after
	* taken from argument.
	*/
	LONG         temp_int;
	
	/*
	* Verify incoming arguments. There should at least
	* be a pointer for the property object and an integer
	* to determine which property is accessed.
	*/
	if (paramCount < ARG_MINIMUM
		|| varlist[ARG_HANDLE]->vt != VT_PTR
		|| varlist[ARG_PROPERTY]->vt != VT_INTEGER)
	{
		*pretvar = NULL;
		goto error_local;
	}

	/* Populate local objectand property vars. */
	object = (s_bind *)varlist[ARG_HANDLE]->ptrVal;
	property = (LONG)varlist[ARG_PROPERTY]->lVal;

	/* Which property to modify ? */
	switch (property)
	{

		case BIND_PROPERTY_ANIMATION_FRAME:

			if (SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
			{
				object->frame = temp_int;
			}

			break;

		case BIND_PROPERTY_ANIMATION_ID:

			if (SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
			{
				object->animation = temp_int;
			}

			break;

		case BIND_PROPERTY_CONFIG:

			if (SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
			{
				object->config = temp_int;
			}

			break;

		case BIND_PROPERTY_DIRECTION_ADJUST:

			if (SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
			{
				object->direction_adjust = temp_int;
			}

			break;

		case BIND_PROPERTY_META_DATA:

			object->meta_data = (s_meta_data*)varlist[ARG_VALUE]->ptrVal;

			break;

		case BIND_PROPERTY_META_TAG:

			if (SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
			{
				object->meta_tag = temp_int;
			}

			break;

		case BIND_PROPERTY_OFFSET_X:

			if (SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
			{
				object->offset.x = temp_int;
			}

			break;

		case BIND_PROPERTY_OFFSET_Y:

			if (SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
			{
				object->offset.y = temp_int;
			}

			break;

		case BIND_PROPERTY_OFFSET_Z:

			if (SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
			{
				object->offset.z = temp_int;
			}

			break;

		case BIND_PROPERTY_SORT_ID:

			if (SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
			{
				object->sortid = temp_int;
			}

			break;

		case BIND_PROPERTY_TARGET:

			object->target = (entity *)varlist[ARG_VALUE]->ptrVal;

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

/*
* Caskey, Damon  V.
* 2018-10-11
*
* Run engine's native bind update function.
*/
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

	// Populate local object and property vars.
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