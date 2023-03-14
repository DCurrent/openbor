/*
 * OpenBOR - http://www.chronocrash.com
 * -----------------------------------------------------------------------
 * All rights reserved. See LICENSE in OpenBOR root for license details.
 *
 * Copyright (c) 2004 OpenBOR Team
 */

/*
* Icon Properties
* 2023-03-13
* Caskey, Damon V.
*/

typedef enum e_icon_properties
{
	ICON_PROPERTY_DEFAULT,
	ICON_PROPERTY_DIE,
	ICON_PROPERTY_GET,
	ICON_PROPERTY_MP_HIGH,
	ICON_PROPERTY_MP_LOW,
	ICON_PROPERTY_MP_MEDIUM,
	ICON_PROPERTY_PAIN,
	ICON_PROPERTY_POSITION_X,
	ICON_PROPERTY_POSITION_Y,
	ICON_PROPERTY_USE_MAP,
	ICON_PROPERTY_WEAPON,
	ICON_PROPERTY_END
} e_icon_properties;

HRESULT openbor_get_icon_property(ScriptVariant** varlist, ScriptVariant** pretvar, int paramCount);
HRESULT openbor_set_icon_property(ScriptVariant** varlist, ScriptVariant** pretvar, int paramCount);