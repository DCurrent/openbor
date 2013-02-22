/*
 * OpenBOR - http://www.LavaLit.com
 * -----------------------------------------------------------------------
 * All rights reserved, see LICENSE in OpenBOR root for details.
 *
 * Copyright (c) 2004 - 2011 OpenBOR Team
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
}VARTYPE;

typedef struct ScriptVariant
{
	union//value
	{
		LONG          lVal;
		VOID *        ptrVal;
		DOUBLE        dblVal;
		int           strVal;
	};
	VARTYPE vt;//variatn type
}ScriptVariant;


extern CHAR** strcache;
extern int   strcache_size;
extern int*  strcache_index;

//clear the string cache
inline void StrCache_Clear();
// int the string cache
//void StrCache_Init();
inline void StrCache_Collect(int index);
inline int StrCache_Pop();
inline CHAR* StrCache_Get(int index);
inline void ScriptVariant_Clear(ScriptVariant* var);

inline void ScriptVariant_Init(ScriptVariant* var);
inline void ScriptVariant_Copy(ScriptVariant* svar, ScriptVariant* rightChild ); // faster in some situations
inline void ScriptVariant_ChangeType(ScriptVariant* var, VARTYPE cvt);
inline HRESULT ScriptVariant_IntegerValue(ScriptVariant* var, LONG* pVal);
inline HRESULT ScriptVariant_DecimalValue(ScriptVariant* var, DOUBLE* pVal);
inline BOOL ScriptVariant_IsTrue(ScriptVariant* svar);
inline void ScriptVariant_ToString(ScriptVariant* svar, LPSTR buffer );

// light version, for compiled call, faster than above, but not safe in some situations
// This function are used by compiled scripts
inline ScriptVariant* ScriptVariant_Assign(ScriptVariant* svar, ScriptVariant* rightChild );
inline ScriptVariant* ScriptVariant_MulAssign(ScriptVariant* svar, ScriptVariant* rightChild );
inline ScriptVariant* ScriptVariant_DivAssign(ScriptVariant* svar, ScriptVariant* rightChild );
inline ScriptVariant* ScriptVariant_AddAssign(ScriptVariant* svar, ScriptVariant* rightChild );
inline ScriptVariant* ScriptVariant_SubAssign(ScriptVariant* svar, ScriptVariant* rightChild );
inline ScriptVariant* ScriptVariant_ModAssign(ScriptVariant* svar, ScriptVariant* rightChild );
inline ScriptVariant* ScriptVariant_Or( ScriptVariant* svar, ScriptVariant* rightChild );
inline ScriptVariant* ScriptVariant_And( ScriptVariant* svar, ScriptVariant* rightChild );
inline ScriptVariant* ScriptVariant_Bit_Or( ScriptVariant* svar, ScriptVariant* rightChild );
inline ScriptVariant* ScriptVariant_Xor( ScriptVariant* svar, ScriptVariant* rightChild );
inline ScriptVariant* ScriptVariant_Bit_And( ScriptVariant* svar, ScriptVariant* rightChild );
inline ScriptVariant* ScriptVariant_Eq( ScriptVariant* svar, ScriptVariant* rightChild );
inline ScriptVariant* ScriptVariant_Ne( ScriptVariant* svar, ScriptVariant* rightChild );
inline ScriptVariant* ScriptVariant_Lt( ScriptVariant* svar, ScriptVariant* rightChild );
inline ScriptVariant* ScriptVariant_Gt( ScriptVariant* svar, ScriptVariant* rightChild );
inline ScriptVariant* ScriptVariant_Ge( ScriptVariant* svar, ScriptVariant* rightChild );
inline ScriptVariant* ScriptVariant_Le( ScriptVariant* svar, ScriptVariant* rightChild );
inline ScriptVariant* ScriptVariant_Add( ScriptVariant* svar, ScriptVariant* rightChild );
inline ScriptVariant* ScriptVariant_Sub( ScriptVariant* svar, ScriptVariant* rightChild );
inline ScriptVariant* ScriptVariant_Shl( ScriptVariant* svar, ScriptVariant* rightChild );
inline ScriptVariant* ScriptVariant_Shr( ScriptVariant* svar, ScriptVariant* rightChild );
inline ScriptVariant* ScriptVariant_Mul( ScriptVariant* svar, ScriptVariant* rightChild );
inline ScriptVariant* ScriptVariant_Div( ScriptVariant* svar, ScriptVariant* rightChild );
inline ScriptVariant* ScriptVariant_Mod( ScriptVariant* svar, ScriptVariant* rightChild );
inline void ScriptVariant_Inc_Op(ScriptVariant* svar );
inline ScriptVariant* ScriptVariant_Inc_Op2(ScriptVariant* svar );
inline void ScriptVariant_Dec_Op(ScriptVariant* svar );
inline ScriptVariant* ScriptVariant_Dec_Op2(ScriptVariant* svar );
inline void ScriptVariant_Pos( ScriptVariant* svar);
inline void ScriptVariant_Neg( ScriptVariant* svar);
inline void ScriptVariant_Boolean_Not(ScriptVariant* svar );

#endif
