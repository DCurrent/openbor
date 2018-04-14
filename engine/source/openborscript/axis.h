/*
 * OpenBOR - http://www.chronocrash.com
 * -----------------------------------------------------------------------
 * All rights reserved. See LICENSE in OpenBOR root for license details.
 *
 * Copyright (c) 2004 - 2018 OpenBOR Team
 */

// Axis Properties
// 2018-04-14
// Caskey, Damon V.

typedef enum
{
    _axis_bi_x,
    _axis_bi_y,
    _axis_bi_the_end,
} e_axis_bi_properties;

typedef enum
{
    _axis_tri_x,
    _axis_tri_y,
    _axis_tri_z,
    _axis_tri_the_end,
} e_axis_tri_properties;

HRESULT openbor_get_axis_tri_property(ScriptVariant **varlist, ScriptVariant **pretvar, int paramCount);
HRESULT openbor_set_axis_tri_property(ScriptVariant **varlist, ScriptVariant **pretvar, int paramCount);

HRESULT openbor_get_axis_tri_property(ScriptVariant **varlist, ScriptVariant **pretvar, int paramCount);
HRESULT openbor_set_axis_tri_property(ScriptVariant **varlist, ScriptVariant **pretvar, int paramCount);

int mapstrings_axis_bi_property(ScriptVariant **varlist, int paramCount);
int mapstrings_axis_tri_property(ScriptVariant **varlist, int paramCount);
