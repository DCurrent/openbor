/*
 * OpenBOR - http://www.chronocrash.com
 * -----------------------------------------------------------------------
 * All rights reserved. See LICENSE in OpenBOR root for license details.
 *
 * Copyright (c)  OpenBOR Team
 */

 #include "scriptcommon.h"

/*
* Caskey, Damon V.
* 2023-02-23
* 
* Return a faction property.
* Requires a faction pointer 
* and property name.
*/
HRESULT openbor_get_faction_property(ScriptVariant **varlist , ScriptVariant **pretvar, int paramCount)
{
    #define SELF_NAME       "openbor_get_faction_property(void pointer, char property)"
    #define ARG_MINIMUM     2   // Minimum required arguments.
    #define ARG_OBJECT      0   // Handle (pointer to property structure).
    #define ARG_PROPERTY    1   // Property to access.

	s_faction				*handle     = NULL; // Property handle.
	e_faction_properties	property   = 0;    // Property argument.

    // Clear pass by reference argument used to send
    // property data back to calling script.
    ScriptVariant_Clear(*pretvar);

    // Verify arguments. There should at least
    // be a pointer for the property handle and an integer
    // to determine which property constant is accessed.
    if(paramCount < ARG_MINIMUM
       || varlist[ARG_OBJECT]->vt != VT_PTR
       || varlist[ARG_PROPERTY]->vt != VT_INTEGER)
    {
        *pretvar = NULL;
        goto error_local;
    }
    else
    {
        // Populate local vars for readability.
        handle      = (s_faction*)varlist[ARG_OBJECT]->ptrVal;
        property    = (LONG)varlist[ARG_PROPERTY]->lVal;
    }
	
    switch(property)
    {
		case FACTION_PROPERTY_GROUP_DAMAGE_DIRECT:

			ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
			(*pretvar)->lVal = (e_faction_group)handle->damage_direct;

			break;

		case FACTION_PROPERTY_GROUP_DAMAGE_INDIRECT:

			ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
			(*pretvar)->lVal = (e_faction_group)handle->damage_indirect;

			break;

		case FACTION_PROPERTY_GROUP_HOSTILE:

			ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
			(*pretvar)->lVal = (e_faction_group)handle->hostile;

			break;

		case FACTION_PROPERTY_GROUP_MEMBER:

			ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
			(*pretvar)->lVal = (e_faction_group)handle->member;

			break;

		case FACTION_PROPERTY_TYPE_DAMAGE_DIRECT:

			ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
			(*pretvar)->lVal = (e_faction_group)handle->type_damage_direct;

			break;

		case FACTION_PROPERTY_TYPE_DAMAGE_INDIRECT:

			ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
			(*pretvar)->lVal = (e_faction_group)handle->type_damage_indirect;

			break;

		case FACTION_PROPERTY_TYPE_HOSTILE:

			ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
			(*pretvar)->lVal = (e_faction_group)handle->type_hostile;

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

/*
* Caskey, Damon V.
* 2023-02-23
*
* Return a faction property.
* Requires a faction pointer,
* property name, and new value.
*/
HRESULT openbor_set_faction_property(ScriptVariant **varlist, ScriptVariant **pretvar, int paramCount)
{
    #define SELF_NAME           "openbor_set_faction_property(void handle, char property, value)"
    #define ARG_MINIMUM         3   // Minimum required arguments.
    #define ARG_OBJECT          0   // Handle (pointer to property structure).
    #define ARG_PROPERTY        1   // Property to access.
    #define ARG_VALUE           2   // New value to apply.

    int						result     = S_OK; // Success or error?
    s_faction*				handle     = NULL; // Property handle.
    e_faction_properties	property   = 0;    // Property to access.

    // Value carriers to apply on properties after
    // taken from argument.
    LONG    temp_int;

	// Map string property name to a
	// matching integer constant.
	//mapstrings_faction_property(varlist, paramCount);
	
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

    // Populate local handle and property vars.
    handle      = (s_faction *)varlist[ARG_OBJECT]->ptrVal;
    property    = (LONG)varlist[ARG_PROPERTY]->lVal;

    // Which property to modify?
    switch(property)
    {

		case FACTION_PROPERTY_GROUP_DAMAGE_DIRECT:

			if (SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
			{
				handle->damage_direct = temp_int;
			}

			break;

        case FACTION_PROPERTY_GROUP_DAMAGE_INDIRECT:

			if (SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
			{
				handle->damage_indirect = temp_int;
			}

            break;

		case FACTION_PROPERTY_GROUP_HOSTILE:

			if (SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
			{
				handle->hostile = temp_int;
			}

			break;

		case FACTION_PROPERTY_GROUP_MEMBER:

			if (SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
			{
				handle->member = temp_int;
			}

			break;

		case FACTION_PROPERTY_TYPE_DAMAGE_DIRECT:

			if (SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
			{
				handle->type_damage_direct = temp_int;
			}

			break;

		case FACTION_PROPERTY_TYPE_DAMAGE_INDIRECT:

			if (SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
			{
				handle->type_damage_indirect = temp_int;
			}

			break;

		case FACTION_PROPERTY_TYPE_HOSTILE:

			if (SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
			{
				handle->type_hostile = temp_int;
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
