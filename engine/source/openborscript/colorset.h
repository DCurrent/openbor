/*
 * OpenBOR - http://www.chronocrash.com
 * -----------------------------------------------------------------------
 * All rights reserved. See LICENSE in OpenBOR root for license details.
 *
 * Copyright (c) 2004 OpenBOR Team
 */

/*
* Colorset Properties
* 2023-03-17
* Caskey, Damon V.
*/

typedef enum e_colorset_properties
{
	COLORSET_PROPERTY_BURN,
	COLORSET_PROPERTY_FROZEN,
	COLORSET_PROPERTY_HIDE_END,
	COLORSET_PROPERTY_HIDE_START,
	COLORSET_PROPERTY_KO,
	COLORSET_PROPERTY_KO_CONFIG,
	COLORSET_PROPERTY_SHOCK,
	COLORSET_PROPERTY_END
} e_colorset_properties;

HRESULT openbor_get_colorset_property(ScriptVariant** varlist, ScriptVariant** pretvar, int paramCount);
HRESULT openbor_set_colorset_property(ScriptVariant** varlist, ScriptVariant** pretvar, int paramCount);