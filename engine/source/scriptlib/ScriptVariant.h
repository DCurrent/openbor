/*
 * OpenBOR - http://www.chronocrash.com
 * -----------------------------------------------------------------------
 * All rights reserved, see LICENSE in OpenBOR root for details.
 *
 * Copyright (c) 2004 - 2014 OpenBOR Team
 */

#ifndef SCRIPTVARIANT_H
#define SCRIPTVARIANT_H

#include "depends.h"

typedef enum VariantType
{
    VT_EMPTY    = 0,    //not initialized
    VT_INTEGER  = 1,    //int/long
    VT_DECIMAL  = 2,    //double
    VT_PTR      = 5,    //void*
    VT_STR      = 6,    //char*
} VARTYPE;

typedef struct ScriptVariant
{
    union//value
    {
        LONG          lVal;
        VOID         *ptrVal;
        DOUBLE        dblVal;
        int           strVal;
    };
    VARTYPE vt;//variatn type
} ScriptVariant;

//clear the string cache
void StrCache_Clear();
// int the string cache
//void StrCache_Init();
void StrCache_Collect(int index);
int StrCache_Pop();
CHAR *StrCache_Get(int index);
void StrCache_Copy(int index, CHAR *str);
void StrCache_NCopy(int index, CHAR *str, int n);
void ScriptVariant_Clear(ScriptVariant *var);

void ScriptVariant_Init(ScriptVariant *var);
void ScriptVariant_Copy(ScriptVariant *svar, ScriptVariant *rightChild ); // faster in some situations
void ScriptVariant_ChangeType(ScriptVariant *var, VARTYPE cvt);
void ScriptVariant_ParseStringConstant(ScriptVariant *var, CHAR *str);
HRESULT ScriptVariant_IntegerValue(ScriptVariant *var, LONG *pVal);
HRESULT ScriptVariant_DecimalValue(ScriptVariant *var, DOUBLE *pVal);
BOOL ScriptVariant_IsTrue(ScriptVariant *svar);
void ScriptVariant_ToString(ScriptVariant *svar, LPSTR buffer );

// light version, for compiled call, faster than above, but not safe in some situations
// This function are used by compiled scripts
ScriptVariant *ScriptVariant_Assign(ScriptVariant *svar, ScriptVariant *rightChild );
ScriptVariant *ScriptVariant_MulAssign(ScriptVariant *svar, ScriptVariant *rightChild );
ScriptVariant *ScriptVariant_DivAssign(ScriptVariant *svar, ScriptVariant *rightChild );
ScriptVariant *ScriptVariant_AddAssign(ScriptVariant *svar, ScriptVariant *rightChild );
ScriptVariant *ScriptVariant_SubAssign(ScriptVariant *svar, ScriptVariant *rightChild );
ScriptVariant *ScriptVariant_ModAssign(ScriptVariant *svar, ScriptVariant *rightChild );
ScriptVariant *ScriptVariant_Or( ScriptVariant *svar, ScriptVariant *rightChild );
ScriptVariant *ScriptVariant_And( ScriptVariant *svar, ScriptVariant *rightChild );
ScriptVariant *ScriptVariant_Bit_Or( ScriptVariant *svar, ScriptVariant *rightChild );
ScriptVariant *ScriptVariant_Xor( ScriptVariant *svar, ScriptVariant *rightChild );
ScriptVariant *ScriptVariant_Bit_And( ScriptVariant *svar, ScriptVariant *rightChild );
ScriptVariant *ScriptVariant_Eq( ScriptVariant *svar, ScriptVariant *rightChild );
ScriptVariant *ScriptVariant_Ne( ScriptVariant *svar, ScriptVariant *rightChild );
ScriptVariant *ScriptVariant_Lt( ScriptVariant *svar, ScriptVariant *rightChild );
ScriptVariant *ScriptVariant_Gt( ScriptVariant *svar, ScriptVariant *rightChild );
ScriptVariant *ScriptVariant_Ge( ScriptVariant *svar, ScriptVariant *rightChild );
ScriptVariant *ScriptVariant_Le( ScriptVariant *svar, ScriptVariant *rightChild );
ScriptVariant *ScriptVariant_Add( ScriptVariant *svar, ScriptVariant *rightChild );
ScriptVariant *ScriptVariant_Sub( ScriptVariant *svar, ScriptVariant *rightChild );
ScriptVariant *ScriptVariant_Shl( ScriptVariant *svar, ScriptVariant *rightChild );
ScriptVariant *ScriptVariant_Shr( ScriptVariant *svar, ScriptVariant *rightChild );
ScriptVariant *ScriptVariant_Mul( ScriptVariant *svar, ScriptVariant *rightChild );
ScriptVariant *ScriptVariant_Div( ScriptVariant *svar, ScriptVariant *rightChild );
ScriptVariant *ScriptVariant_Mod( ScriptVariant *svar, ScriptVariant *rightChild );
void ScriptVariant_Inc_Op(ScriptVariant *svar );
ScriptVariant *ScriptVariant_Inc_Op2(ScriptVariant *svar );
void ScriptVariant_Dec_Op(ScriptVariant *svar );
ScriptVariant *ScriptVariant_Dec_Op2(ScriptVariant *svar );
void ScriptVariant_Pos( ScriptVariant *svar);
void ScriptVariant_Neg( ScriptVariant *svar);
void ScriptVariant_Boolean_Not(ScriptVariant *svar );

#endif
