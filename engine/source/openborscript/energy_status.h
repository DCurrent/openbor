/*
 * OpenBOR - http://www.chronocrash.com
 * -----------------------------------------------------------------------
 * All rights reserved. See LICENSE in OpenBOR root for license details.
 *
 * Copyright (c) 2004 - 2018 OpenBOR Team
 */

// Energy status Properties
// 2018-04-25
// Caskey, Damon V.

typedef enum
{
    _ENERGY_HEALTH_CURRENT,
    _ENERGY_HEALTH_OLD,
    _ENERGY_MP_CURRENT,
    _ENERGY_MP_OLD,
    _ENERGY_END,
} e_energy_properties;

HRESULT openbor_get_energy_property(ScriptVariant **varlist , ScriptVariant **pretvar, int paramCount);
HRESULT openbor_set_energy_property(ScriptVariant **varlist , ScriptVariant **pretvar, int paramCount);

int mapstrings_energy_property(ScriptVariant **varlist, int paramCount);
