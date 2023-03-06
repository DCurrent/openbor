/*
 * OpenBOR - http://www.chronocrash.com
 * -----------------------------------------------------------------------
 * All rights reserved. See LICENSE in OpenBOR root for license details.
 *
 * Copyright (c) 2004 OpenBOR Team
 */

// Model Properties
// 2023-03-03
// Caskey, Damon V.

typedef enum
{
	MODEL_PROPERTY_AIR_CONTROL,
	MODEL_PROPERTY_ANTI_GRAVITY,
	MODEL_PROPERTY_FACTION,
	MODEL_PROPERTY_INDEX,
	MODEL_PROPERTY_MOVE_CONSTRAINT,
	MODEL_PROPERTY_WEAPON,
	MODEL_PROPERTY_END,
} e_model_properties;

HRESULT openbor_get_model_property(ScriptVariant **varlist, ScriptVariant **pretvar, int paramCount);
HRESULT openbor_set_model_property(ScriptVariant **varlist, ScriptVariant **pretvar, int paramCount);
