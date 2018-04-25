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
    _ENERGY_STATUS_HEALTH_CURRENT,
    _ENERGY_STATUS_HEALTH_OLD,
    _ENERGY_STATUS_MP_CURRENT,
    _ENERGY_STATUS_MP_OLD,
    _ENERGY_STATUS_END,
} e_energy_status_properties;

HRESULT openbor_get_energy_status_property(ScriptVariant **varlist , ScriptVariant **pretvar, int paramCount);
HRESULT openbor_set_energy_status_property(ScriptVariant **varlist , ScriptVariant **pretvar, int paramCount);

int mapstrings_energy_status_property(ScriptVariant **varlist, int paramCount);
