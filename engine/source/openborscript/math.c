/*
 * OpenBOR - http://www.chronocrash.com
 * -----------------------------------------------------------------------
 * All rights reserved. See LICENSE in OpenBOR root for license details.
 *
 * Copyright (c)  OpenBOR Team
 */

// Math
// 2017-04-26
// Caskey, Damon V.
//
// Mathematical operations. Ordinal functions
// written by White Dragon.

#include "scriptcommon.h"

HRESULT math_sin(ScriptVariant **varlist , ScriptVariant **pretvar, int paramCount)
{
    LONG ltemp;
    if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[0], &ltemp)))
    {
        ScriptVariant_ChangeType(*pretvar, VT_DECIMAL);
        (*pretvar)->dblVal = sin_table[(ltemp % 360 + 360) % 360];
        return S_OK;
    }
    *pretvar = NULL;
    return E_FAIL;
}

HRESULT math_cos(ScriptVariant **varlist , ScriptVariant **pretvar, int paramCount)
{
    LONG ltemp;
    if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[0], &ltemp)))
    {
        ScriptVariant_ChangeType(*pretvar, VT_DECIMAL);
        (*pretvar)->dblVal = cos_table[(ltemp % 360 + 360) % 360];
        return S_OK;
    }
    *pretvar = NULL;
    return E_FAIL;
}

HRESULT math_ssin(ScriptVariant **varlist , ScriptVariant **pretvar, int paramCount)
{
    DOUBLE dbltemp;

    if(SUCCEEDED(ScriptVariant_DecimalValue(varlist[0], &dbltemp)))
    {
        double PI = 3.14159265;

        ScriptVariant_ChangeType(*pretvar, VT_DECIMAL);
        (*pretvar)->dblVal = (DOUBLE)sin(dbltemp*PI/180.0);
        return S_OK;
    }
    *pretvar = NULL;
    return E_FAIL;
}

HRESULT math_scos(ScriptVariant **varlist , ScriptVariant **pretvar, int paramCount)
{
    DOUBLE dbltemp;

    if(SUCCEEDED(ScriptVariant_DecimalValue(varlist[0], &dbltemp)))
    {
        double PI = 3.14159265;

        ScriptVariant_ChangeType(*pretvar, VT_DECIMAL);
        (*pretvar)->dblVal = (DOUBLE)cos(dbltemp*PI/180.0);
        return S_OK;
    }
    *pretvar = NULL;
    return E_FAIL;
}

HRESULT math_sqrt(ScriptVariant **varlist , ScriptVariant **pretvar, int paramCount)
{
    DOUBLE dbltemp;
    float inv;

    if(SUCCEEDED(ScriptVariant_DecimalValue(varlist[0], &dbltemp)))
    {
        ScriptVariant_ChangeType(*pretvar, VT_DECIMAL);
        inv = invsqrt((float)dbltemp);
        assert(inv != 0.0f);
        (*pretvar)->dblVal = (DOUBLE)1.0 / inv;
        return S_OK;
    }
    *pretvar = NULL;

    return E_FAIL;
}

HRESULT math_pow(ScriptVariant **varlist , ScriptVariant **pretvar, int paramCount)
{
    DOUBLE dbltempA, dbltempB;

    if( SUCCEEDED(ScriptVariant_DecimalValue(varlist[0], &dbltempA)) && SUCCEEDED(ScriptVariant_DecimalValue(varlist[1], &dbltempB)) )
    {
        ScriptVariant_ChangeType(*pretvar, VT_DECIMAL);
        (*pretvar)->dblVal = pow((double)dbltempA,(double)dbltempB);
        return S_OK;
    }
    *pretvar = NULL;
    return E_FAIL;
}

HRESULT math_asin(ScriptVariant **varlist , ScriptVariant **pretvar, int paramCount)
{
    DOUBLE dbltemp;

    if(SUCCEEDED(ScriptVariant_DecimalValue(varlist[0], &dbltemp)))
    {
        double PI = 3.14159265;

        ScriptVariant_ChangeType(*pretvar, VT_DECIMAL);
        (*pretvar)->dblVal = (DOUBLE)(asin((double)dbltemp) * 180.0 / PI);
        return S_OK;
    }
    *pretvar = NULL;
    return E_FAIL;
}

HRESULT math_acos(ScriptVariant **varlist , ScriptVariant **pretvar, int paramCount)
{
    DOUBLE dbltemp;

    if(SUCCEEDED(ScriptVariant_DecimalValue(varlist[0], &dbltemp)))
    {
        double PI = 3.14159265;

        ScriptVariant_ChangeType(*pretvar, VT_DECIMAL);
        (*pretvar)->dblVal = (DOUBLE)(aacos((double)dbltemp) * 180.0 / PI);
        return S_OK;
    }
    *pretvar = NULL;
    return E_FAIL;
}

HRESULT math_atan(ScriptVariant **varlist , ScriptVariant **pretvar, int paramCount)
{
    DOUBLE dbltemp;

    if(SUCCEEDED(ScriptVariant_DecimalValue(varlist[0], &dbltemp)))
    {
        double PI = 3.14159265;

        ScriptVariant_ChangeType(*pretvar, VT_DECIMAL);
        (*pretvar)->dblVal = (DOUBLE)(aatan((double)dbltemp) * 180.0 / PI);
        return S_OK;
    }
    *pretvar = NULL;
    return E_FAIL;
}

HRESULT math_trunc(ScriptVariant **varlist , ScriptVariant **pretvar, int paramCount)
{
    DOUBLE dbltemp;

    if(SUCCEEDED(ScriptVariant_DecimalValue(varlist[0], &dbltemp)))
    {
        ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
        (*pretvar)->lVal = (LONG)(trunc(dbltemp));
        return S_OK;
    }
    *pretvar = NULL;
    return E_FAIL;
}

HRESULT math_round(ScriptVariant **varlist , ScriptVariant **pretvar, int paramCount)
{
    DOUBLE dbltemp;

    if(SUCCEEDED(ScriptVariant_DecimalValue(varlist[0], &dbltemp)))
    {
        ScriptVariant_ChangeType(*pretvar, VT_DECIMAL);
        (*pretvar)->dblVal = (DOUBLE)(round(dbltemp));
        return S_OK;
    }
    *pretvar = NULL;
    return E_FAIL;
}

