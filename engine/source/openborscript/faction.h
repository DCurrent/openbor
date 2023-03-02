/*
 * OpenBOR - http://www.chronocrash.com
 * -----------------------------------------------------------------------
 * All rights reserved. See LICENSE in OpenBOR root for license details.
 *
 * Copyright (c) 2004 - 2019 OpenBOR Team
 */


/* 
* Faction Properties
* Caskey, Damon V.
* 2023-02-28
*/

typedef enum e_faction_properties
{
	FACTION_PROPERTY_GROUP_DAMAGE_DIRECT,
	FACTION_PROPERTY_GROUP_DAMAGE_INDIRECT,
	FACTION_PROPERTY_GROUP_HOSTILE,
	FACTION_PROPERTY_GROUP_MEMBER,
	FACTION_PROPERTY_TYPE_DAMAGE_DIRECT,
	FACTION_PROPERTY_TYPE_DAMAGE_INDIRECT,
	FACTION_PROPERTY_TYPE_HOSTILE,
	FACTION_PROPERTY_END,
} e_faction_properties;

HRESULT openbor_get_faction_property(ScriptVariant **varlist, ScriptVariant **pretvar, int paramCount);
HRESULT openbor_set_faction_property(ScriptVariant **varlist, ScriptVariant **pretvar, int paramCount);