/*
 * OpenBOR - http://www.chronocrash.com
 * -----------------------------------------------------------------------
 * All rights reserved. See LICENSE in OpenBOR root for license details.
 *
 * Copyright (c)  OpenBOR Team
 */

// Level Properties
// 2017-04-27
// Caskey, Damon V.
//
// Access to layer (panel, bglayer, fglayer, etc) properties.

#include "scriptcommon.h"

// Layer handle.
// Caskey, Damon V.
//
// Get layer handle by index.
//
// get_layer_handle(void level_handle, int index)
HRESULT openbor_get_layer_handle(ScriptVariant **varlist, ScriptVariant **pretvar, int paramCount)
{
    #define SELF_NAME       "openbor_get_layer_handle(void level_handle, int index)"
    #define ARG_MINIMUM     2   // Minimum required arguments.
    #define ARG_LEVEL       0   // Level handle.
    #define ARG_INDEX       1   // Array index.

    int     result;
    LONG    index;
    s_level *level;

    // Verify arguments and populate local
    // variables as needed.
    if(paramCount != ARG_MINIMUM)
    {
        goto error_local;
    }

    if(varlist[ARG_LEVEL]->vt != VT_PTR && varlist[ARG_LEVEL]->vt != VT_EMPTY)
    {
        goto error_local;
    }

    if(FAILED(ScriptVariant_IntegerValue(varlist[ARG_INDEX], &index)))
    {
        goto error_local;
    }

    // Clear pass by var value.
    ScriptVariant_Clear(*pretvar);

    // Get level pointer.
    level = varlist[ARG_LEVEL]->ptrVal;

    // Verify the index is within bounds, and
    // if so, dereference the array element pointer.
    if(index < level->numlayers && index >= 0)
    {
        ScriptVariant_ChangeType(*pretvar, VT_PTR);
        (*pretvar)->ptrVal = &level->layersref[index];
    }

    result = S_OK;

    return result;

    // Error trapping.
    error_local:

    printf("You must provide a valid handle and index: " SELF_NAME "\n");

    *pretvar    = NULL;
    result      = E_FAIL;
    return result;

    #undef SELF_NAME
    #undef ARG_MINIMUM
    #undef ARG_LEVEL
    #undef ARG_INDEX
}

