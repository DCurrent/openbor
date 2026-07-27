/*
 * OpenBOR - http://www.chronocrash.com
 * -----------------------------------------------------------------------
 * All rights reserved, see LICENSE in OpenBOR root for details.
 *
 * Copyright (c)  OpenBOR Team
 */

#ifndef SCRIPTVARIANT_H
#define SCRIPTVARIANT_H

#include "depends.h"
#include <stdint.h>

typedef enum VariantType {

    VT_EMPTY       = 0,           // Not initialized.
    VT_INTEGER     = (1U << 0),   // LONG / legacy script integer.
    VT_DECIMAL     = (1U << 1),   // double.
    VT_INTEGER64   = (1U << 2),   // Signed 64-bit integer.
    VT_UINTEGER64  = (1U << 3),   // Unsigned 64-bit integer.
    VT_PTR         = (1U << 4),   // void*.
    VT_STR         = (1U << 5)    // char*.
} VARTYPE;

/*
* Query masks only. These are not concrete
* types and must never be stored in
* ScriptVariant.vt or property type maps.
*/
#define VT_INTANY  (VT_INTEGER | VT_INTEGER64 | VT_UINTEGER64)
#define VT_NUMERIC (VT_DECIMAL | VT_INTANY)

#pragma pack(4)

typedef struct ScriptVariant {
    
    union{
        LONG                lVal;
        int64_t             llVal;
        uint64_t            ullVal;
        VOID               *ptrVal;
        DOUBLE              dblVal;
        int                 strVal;
    };

    VARTYPE vt;
} ScriptVariant;

/*
* Caskey, Damon V.
* 2023-04-17
* 
* Keep meta data about a variant type
* for debugging output.
*/
typedef struct s_script_variant_meta
{
    const char* id_string;
    const char* print_format;
} s_script_variant_meta;

extern const s_script_variant_meta script_variant_meta_list[];

#pragma pack()

//clear the string cache
void StrCache_Clear();
// int the string cache
//void StrCache_Init();
void StrCache_Collect(int index);
int StrCache_Pop(int length);
int StrCache_CreateNewFrom(const CHAR *str);
CHAR *StrCache_Get(int index);
void ScriptVariant_Clear(ScriptVariant *var);

void ScriptVariant_Init(ScriptVariant *var);
void ScriptVariant_Copy(ScriptVariant *svar, ScriptVariant *rightChild ); // faster in some situations
void ScriptVariant_ChangeType(ScriptVariant *var, VARTYPE cvt);
void ScriptVariant_ParseStringConstant(ScriptVariant *var, CHAR *str);
HRESULT ScriptVariant_IntegerValue(ScriptVariant *var, LONG *pVal);
HRESULT ScriptVariant_DecimalValue(ScriptVariant *var, DOUBLE *pVal);
HRESULT ScriptVariant_Integer64Value(ScriptVariant *var, int64_t *pVal);
HRESULT ScriptVariant_Unsigned64Value(ScriptVariant *var, uint64_t *pVal);
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
void ScriptVariant_Bitwise_Not(ScriptVariant *svar );

#endif
