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

typedef enum e_spawn_hud_properties
{
	SPAWN_HUD_PROPERTY_POSITION_X,
	SPAWN_HUD_PROPERTY_POSITION_Y,
	SPAWN_HUD_PROPERTY_SPRITE,
	SPAWN_HUD_PROPERTY_END
} e_spawn_hud_properties;

HRESULT openbor_get_spawn_hud_property(ScriptVariant** varlist, ScriptVariant** pretvar, int paramCount);
HRESULT openbor_set_spawn_hud_property(ScriptVariant** varlist, ScriptVariant** pretvar, int paramCount);