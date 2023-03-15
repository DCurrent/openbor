/*
 * OpenBOR - http://www.chronocrash.com
 * -----------------------------------------------------------------------
 * All rights reserved. See LICENSE in OpenBOR root for license details.
 *
 * Copyright (c) 2004 OpenBOR Team
 */

/*
* Properties
* 2023-03-13
* Caskey, Damon V.
*/

typedef enum e_status_dial_properties
{
	STATUS_DIAL_PROPERTY_BACK_LAYER,
	STATUS_DIAL_PROPERTY_BORDER_LAYER,
	STATUS_DIAL_PROPERTY_COLORSET_TABLE,
	STATUS_DIAL_PROPERTY_CONFIG_FLAGS,
	STATUS_DIAL_PROPERTY_GRAPH_LAYER,
	STATUS_DIAL_PROPERTY_GRAPH_POSITION_X,
	STATUS_DIAL_PROPERTY_GRAPH_POSITION_Y,
	STATUS_DIAL_PROPERTY_GRAPH_SIZE_X,
	STATUS_DIAL_PROPERTY_GRAPH_SIZE_Y,
	STATUS_DIAL_PROPERTY_NAME_POSITION_X,
	STATUS_DIAL_PROPERTY_NAME_POSITION_Y,
	STATUS_DIAL_PROPERTY_SHADOW_LAYER,
	STATUS_DIAL_PROPERTY_END
} e_status_dial_properties;

HRESULT openbor_get_status_dial_property(ScriptVariant** varlist, ScriptVariant** pretvar, int paramCount);
HRESULT openbor_set_status_dial_property(ScriptVariant** varlist, ScriptVariant** pretvar, int paramCount);