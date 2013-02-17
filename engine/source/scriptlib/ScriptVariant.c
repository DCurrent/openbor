/*
 * OpenBOR - http://www.LavaLit.com
 * -----------------------------------------------------------------------
 * All rights reserved, see LICENSE in OpenBOR root for details.
 *
 * Copyright (c) 2004 - 2011 OpenBOR Team
 */

#include "ScriptVariant.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

// use string cache to cut the memory usage down, because not all variants are string, no need to give each of them an array
#define STRCACHE_INC      64
CHAR** strcache = NULL;
int   strcache_size = 0;
int   strcache_top = -1;
int*  strcache_index = NULL;

//clear the string cache
void StrCache_Clear()
{
	int i;
	if(strcache)
	{
		for(i=0; i<strcache_size; i++)
		{
			free(strcache[i]);
			strcache[i] = NULL;
		}
		free(strcache);
		strcache = NULL;
	}
	if(strcache_index) {free(strcache_index); strcache_index = NULL;}
	strcache_size = 0;
	strcache_top = -1;
}

// int the string cache
void StrCache_Init()
{
	int i;
	StrCache_Clear(); // just in case
	strcache = malloc(sizeof(CHAR*)*STRCACHE_INC);
	//if(!strcache) shutdown(1, "out of memory");
	strcache_index = malloc(sizeof(int)*STRCACHE_INC);
	//if(!strcache_index) shutdown(1, "out of memory");
	for(i=0; i<STRCACHE_INC; i++)
	{
		strcache[i] = malloc((MAX_STR_VAR_LEN+1)*sizeof(CHAR));
		//if(!strcache[i]) shutdown(1, "out of memory");
		strcache[i][0] = 0;
		strcache_index[i] = i;
	}
	strcache_size = STRCACHE_INC;
	strcache_top = strcache_size-1;
}

void StrCache_Collect(int index)
{
	if(strcache_size)
	{
		strcache_top++;
		if(strcache_top<strcache_size)
		{
			strcache_index[strcache_top] = index;
		}
	}
}

int StrCache_Pop()
{
	CHAR** temp;
	int*   tempi;
	int i;
	if(strcache_size==0)
	{
		StrCache_Init();
	}
	if(strcache_top<0) // realloc
	{
		temp = malloc(sizeof(CHAR*)*(strcache_size+STRCACHE_INC));
		//if(!temp) shutdown(1, "out of memory");
		for(i=strcache_size; i<strcache_size+STRCACHE_INC; i++)
		{
			temp[i] = malloc((MAX_STR_VAR_LEN+1)*sizeof(CHAR));
			//if(!strcache[i]) shutdown(1, "out of memory");
			temp[i][0] = 0;
		}
		for(i=0; i<strcache_size; i++)
		{
			temp[i] = strcache[i];
		}
		free(strcache);
		strcache = temp;
		tempi = malloc(sizeof(int)*(strcache_size+STRCACHE_INC));
		//if(!tempi) shutdown(1, "out of memory");
		for(i=0; i<STRCACHE_INC; i++)
		{
			tempi[i] = strcache_size+i;
		}
		free(strcache_index);
		strcache_index = tempi;
		strcache_size += STRCACHE_INC;
		strcache_top += STRCACHE_INC;
	}
	return strcache_index[strcache_top--];
}

CHAR* StrCache_Get(int index)
{
	if(index<strcache_size)
	{
		return strcache[index];
	}
	return NULL;
}


void ScriptVariant_Clear(ScriptVariant* var)
{
	ScriptVariant_ChangeType(var, VT_EMPTY);
	var->ptrVal = NULL; // not sure, maybe this is the longest member in the union
}

void ScriptVariant_Init(ScriptVariant* var)
{
	//memset(var, 0, 8);
	var->ptrVal = NULL; // not sure, maybe this is the longest member in the union
	var->vt = VT_EMPTY;
}

void ScriptVariant_ChangeType(ScriptVariant* var, VARTYPE cvt)
{
	if(var->vt == cvt) return;
/*
	if(var->vt == VT_INTEGER)
	{
		switch(cvt)
		{
		case VT_DECIMAL:
			var->dblVal = (DOUBLE)var->lVal;
			break;
		case VT_STR:
			var->strVal = StrCache_Pop();
			sprintf(StrCache_Get(var->strVal), "%ld", var->lVal);
			break;
		}
	}
	else if(var->vt == VT_DECIMAL)
	{
		switch(cvt)
		{
		case VT_INTEGER:
			var->dblVal = (LONG)var->lVal;
			break;
		case VT_STR:
			var->strVal = StrCache_Pop();
			sprintf(StrCache_Get(var->strVal), "%lf", var->dblVal);
			break;
		}
	}
	else */if(var->vt == VT_STR)
	{
		if(cvt != VT_STR)
		{
			StrCache_Collect(var->strVal);
		}
	}
	else if(cvt == VT_STR)
	{
	   var->strVal = StrCache_Pop();
	}
	var->vt = cvt;

}

HRESULT ScriptVariant_IntegerValue(ScriptVariant* var, LONG* pVal)
{
	if(var->vt == VT_INTEGER)
	{
		*pVal = var->lVal;
	}
	else if(var->vt == VT_DECIMAL)
	{
		*pVal = (LONG)var->dblVal;
	}
	else return E_FAIL;

	return S_OK;
}

HRESULT ScriptVariant_DecimalValue(ScriptVariant* var, DOUBLE* pVal)
{
	if(var->vt == VT_INTEGER)
	{
		*pVal = (DOUBLE)var->lVal;
	}
	else if(var->vt == VT_DECIMAL)
	{
		*pVal = var->dblVal;
	}
	else return E_FAIL;

	return S_OK;
}


BOOL ScriptVariant_IsTrue(ScriptVariant* svar)
{
	switch(svar->vt)
	{
	case VT_STR:
		return StrCache_Get(svar->strVal)[0]!=0;
	case VT_INTEGER:
		return svar->lVal !=0;
	case VT_DECIMAL:
		return svar->dblVal != 0.0;
	case VT_PTR:
		return svar->ptrVal != 0;
	default:
		return 0;
	}
}

void ScriptVariant_ToString(ScriptVariant* svar, LPSTR buffer )
{
   switch( svar->vt ){
   case VT_EMPTY:
	  sprintf( buffer, "<VT_EMPTY>   Unitialized" );
	  break;
   case VT_INTEGER:
	  sprintf( buffer, "%d", svar->lVal);
	  break;
   case VT_DECIMAL:
	  sprintf( buffer, "%lf", svar->dblVal );
	  break;
   case VT_PTR:
	   sprintf(buffer, "#%ld",(long)(svar->ptrVal));
	   break;
   case VT_STR:
	   sprintf(buffer, "%s", StrCache_Get(svar->strVal));
	   break;
   default:
	  sprintf(buffer, "<Unprintable VARIANT type.>" );
	  break;
   }
}

// faster if it is not VT_STR
void ScriptVariant_Copy(ScriptVariant* svar, ScriptVariant* rightChild )
{
   // collect the str cache index
   if(svar->vt==VT_STR && rightChild->vt!=VT_STR)
   {
	   StrCache_Collect(svar->strVal);
   }
   switch( rightChild->vt )
   {
   case VT_INTEGER:
	  svar->lVal = rightChild->lVal;
	  break;
   case VT_DECIMAL:
	  svar->dblVal = rightChild->dblVal;
	  break;
   case VT_PTR:
	   svar->ptrVal = rightChild->ptrVal;
	   break;
   case VT_STR:
	   // if it is not string, give it a string cache index
	   if(svar->vt!=VT_STR) svar->strVal = StrCache_Pop();
	   StrCache_Get(rightChild->strVal)[MAX_STR_VAR_LEN] = 0;
	   strcpy(StrCache_Get(svar->strVal), StrCache_Get(rightChild->strVal));
	   break;
   default:
	  //should not happen unless the variant is not intialized correctly
	  //shutdown(1, "invalid variant type");
	  svar->ptrVal = NULL;
	  break;
   }
   svar->vt = rightChild->vt;
}

// light version, for compiled call, faster than above, but not safe in some situations
ScriptVariant* ScriptVariant_Assign(ScriptVariant* svar, ScriptVariant* rightChild )
{
	ScriptVariant_Copy(svar, rightChild);
	return rightChild;
}


ScriptVariant* ScriptVariant_MulAssign(ScriptVariant* svar, ScriptVariant* rightChild )
{
   ScriptVariant_Copy(svar, ScriptVariant_Mul(svar, rightChild));
   return svar;
}


ScriptVariant* ScriptVariant_DivAssign(ScriptVariant* svar, ScriptVariant* rightChild )
{
   ScriptVariant_Copy(svar, ScriptVariant_Div(svar, rightChild));
   return svar;
}


ScriptVariant* ScriptVariant_AddAssign(ScriptVariant* svar, ScriptVariant* rightChild )
{
   ScriptVariant_Copy(svar, ScriptVariant_Add(svar, rightChild));
   return svar;
}


ScriptVariant* ScriptVariant_SubAssign(ScriptVariant* svar, ScriptVariant* rightChild )
{
   ScriptVariant_Copy(svar, ScriptVariant_Sub(svar, rightChild));
   return svar;
}


ScriptVariant* ScriptVariant_ModAssign(ScriptVariant* svar, ScriptVariant* rightChild )
{
   ScriptVariant_Copy(svar, ScriptVariant_Mod(svar, rightChild));
   return svar;
}

//Logical Operations

ScriptVariant* ScriptVariant_Or( ScriptVariant* svar, ScriptVariant* rightChild )
{
	static ScriptVariant retvar = {{.lVal=0},VT_INTEGER};
	retvar.lVal = (ScriptVariant_IsTrue(svar) || ScriptVariant_IsTrue(rightChild));
	return &retvar;
}


ScriptVariant* ScriptVariant_And( ScriptVariant* svar, ScriptVariant* rightChild )
{
	static ScriptVariant retvar = {{.lVal=0},VT_INTEGER};
	retvar.lVal = (ScriptVariant_IsTrue(svar) && ScriptVariant_IsTrue(rightChild));
	return &retvar;
}

ScriptVariant* ScriptVariant_Eq( ScriptVariant* svar, ScriptVariant* rightChild )
{
	DOUBLE dbl1,dbl2;
	static ScriptVariant retvar = {{.lVal=0},VT_INTEGER};

	if(ScriptVariant_DecimalValue(svar, &dbl1)==S_OK &&
	   ScriptVariant_DecimalValue(rightChild, &dbl2)==S_OK)
	{
		retvar.lVal = (dbl1==dbl2);
	}
	else if(svar->vt == VT_STR && rightChild->vt == VT_STR)
	{
		retvar.lVal = !(strcmp(StrCache_Get(svar->strVal), StrCache_Get(rightChild->strVal)));
	}
	else if(svar->vt == VT_PTR && rightChild->vt == VT_PTR)
	{
		retvar.lVal = (svar->ptrVal==rightChild->ptrVal);
	}
	else if(svar->vt == VT_EMPTY && rightChild->vt == VT_EMPTY)
	{
		retvar.lVal = 1;
	}
	else
	{
		retvar.lVal = !(memcmp(svar, rightChild, sizeof(ScriptVariant)));
	}

	return &retvar;
}


ScriptVariant* ScriptVariant_Ne( ScriptVariant* svar, ScriptVariant* rightChild )
{
	DOUBLE dbl1,dbl2;
	static ScriptVariant retvar = {{.lVal=0},VT_INTEGER};

	if(ScriptVariant_DecimalValue(svar, &dbl1)==S_OK &&
	   ScriptVariant_DecimalValue(rightChild, &dbl2)==S_OK)
	{
		retvar.lVal = (dbl1!=dbl2);
	}
	else if(svar->vt == VT_STR && rightChild->vt == VT_STR)
	{
		retvar.lVal = strcmp(StrCache_Get(svar->strVal), StrCache_Get(rightChild->strVal));
	}
	else if(svar->vt == VT_PTR && rightChild->vt == VT_PTR)
	{
		retvar.lVal = (svar->ptrVal!=rightChild->ptrVal);
	}
	else if(svar->vt == VT_EMPTY && rightChild->vt == VT_EMPTY)
	{
		retvar.lVal = 0;
	}
	else
	{
		retvar.lVal = (memcmp(svar, rightChild, sizeof(ScriptVariant))!=0);
	}

	return &retvar;
}


ScriptVariant* ScriptVariant_Lt( ScriptVariant* svar, ScriptVariant* rightChild )
{
	DOUBLE dbl1,dbl2;
	static ScriptVariant retvar = {{.lVal=0},VT_INTEGER};

	if(ScriptVariant_DecimalValue(svar, &dbl1)==S_OK &&
	   ScriptVariant_DecimalValue(rightChild, &dbl2)==S_OK)
	{
		retvar.lVal = (dbl1 < dbl2);
	}
	else if(svar->vt == VT_STR && rightChild->vt == VT_STR)
	{
		retvar.lVal = (strcmp(StrCache_Get(svar->strVal), StrCache_Get(rightChild->strVal))<0);
	}
	else if(svar->vt == VT_PTR && rightChild->vt == VT_PTR)
	{
		retvar.lVal = (svar->ptrVal<rightChild->ptrVal);
	}
	else if(svar->vt == VT_EMPTY || rightChild->vt == VT_EMPTY)
	{
		retvar.lVal = 0;
	}
	else
	{
		retvar.lVal = (memcmp(svar, rightChild, sizeof(ScriptVariant))<0);
	}

	return &retvar;
}



ScriptVariant* ScriptVariant_Gt( ScriptVariant* svar, ScriptVariant* rightChild )
{
	DOUBLE dbl1,dbl2;
	static ScriptVariant retvar = {{.lVal=0},VT_INTEGER};

	if(ScriptVariant_DecimalValue(svar, &dbl1)==S_OK &&
	   ScriptVariant_DecimalValue(rightChild, &dbl2)==S_OK)
	{
		retvar.lVal = (dbl1 > dbl2);
	}
	else if(svar->vt == VT_STR && rightChild->vt == VT_STR)
	{
		retvar.lVal = (strcmp(StrCache_Get(svar->strVal), StrCache_Get(rightChild->strVal))>0);
	}
	else if(svar->vt == VT_PTR && rightChild->vt == VT_PTR)
	{
		retvar.lVal = (svar->ptrVal>rightChild->ptrVal);
	}
	else if(svar->vt == VT_EMPTY || rightChild->vt == VT_EMPTY)
	{
		retvar.lVal = 0;
	}
	else
	{
		retvar.lVal = (memcmp(svar, rightChild, sizeof(ScriptVariant))>0);
	}

	return &retvar;
}



ScriptVariant* ScriptVariant_Ge( ScriptVariant* svar, ScriptVariant* rightChild )
{
	DOUBLE dbl1,dbl2;
	static ScriptVariant retvar = {{.lVal=0}, VT_INTEGER};

	if(ScriptVariant_DecimalValue(svar, &dbl1)==S_OK &&
	   ScriptVariant_DecimalValue(rightChild, &dbl2)==S_OK)
	{
		retvar.lVal = (dbl1 >= dbl2);
	}
	else if(svar->vt == VT_STR && rightChild->vt == VT_STR)
	{
		retvar.lVal = (strcmp(StrCache_Get(svar->strVal), StrCache_Get(rightChild->strVal))>=0);
	}
	else if(svar->vt == VT_PTR && rightChild->vt == VT_PTR)
	{
		retvar.lVal = (svar->ptrVal>=rightChild->ptrVal);
	}
	else if(svar->vt == VT_EMPTY || rightChild->vt == VT_EMPTY)
	{
		retvar.lVal = 0;
	}
	else
	{
		retvar.lVal = (memcmp(svar, rightChild, sizeof(ScriptVariant))>=0);
	}

	return &retvar;
}


ScriptVariant* ScriptVariant_Le( ScriptVariant* svar, ScriptVariant* rightChild )
{
	DOUBLE dbl1,dbl2;
	static ScriptVariant retvar = {{.lVal=0},VT_INTEGER};

	if(ScriptVariant_DecimalValue(svar, &dbl1)==S_OK &&
	   ScriptVariant_DecimalValue(rightChild, &dbl2)==S_OK)
	{
		retvar.lVal = (dbl1 <= dbl2);
	}
	else if(svar->vt == VT_STR && rightChild->vt == VT_STR)
	{
		retvar.lVal = (strcmp(StrCache_Get(svar->strVal), StrCache_Get(rightChild->strVal))<=0);
	}
	else if(svar->vt == VT_PTR && rightChild->vt == VT_PTR)
	{
		retvar.lVal = (svar->ptrVal<=rightChild->ptrVal);
	}
	else if(svar->vt == VT_EMPTY || rightChild->vt == VT_EMPTY)
	{
		retvar.lVal = 0;
	}
	else
	{
		retvar.lVal = (memcmp(svar, rightChild, sizeof(ScriptVariant))<=0);
	}

	return &retvar;
}


ScriptVariant* ScriptVariant_Add( ScriptVariant* svar, ScriptVariant* rightChild )
{
	static ScriptVariant retvar = {{.ptrVal=NULL}, VT_EMPTY};
	DOUBLE dbl1,dbl2;
	CHAR buf[MAX_STR_VAR_LEN+1];
	if(ScriptVariant_DecimalValue(svar, &dbl1)==S_OK &&
	   ScriptVariant_DecimalValue(rightChild, &dbl2)==S_OK)
	{
		if(svar->vt==VT_DECIMAL || rightChild->vt==VT_DECIMAL)
		{
			ScriptVariant_ChangeType(&retvar, VT_DECIMAL);
			retvar.dblVal = dbl1+dbl2;
		}
		else
		{
			ScriptVariant_ChangeType(&retvar, VT_INTEGER);
			retvar.lVal = (LONG)(dbl1+dbl2);
		}
	}
	else if(svar->vt == VT_STR || rightChild->vt == VT_STR)
	{
		ScriptVariant_ChangeType(&retvar, VT_STR);
		StrCache_Get(retvar.strVal)[0] = 0;
		ScriptVariant_ToString(svar, StrCache_Get(retvar.strVal));
		ScriptVariant_ToString(rightChild, buf);
		strcat(StrCache_Get(retvar.strVal), buf);
	}
	else ScriptVariant_Clear(&retvar);

	return &retvar;
}


ScriptVariant* ScriptVariant_Sub( ScriptVariant* svar, ScriptVariant* rightChild )
{
	static ScriptVariant retvar = {{.ptrVal=NULL}, VT_EMPTY};
	DOUBLE dbl1,dbl2;
	if(ScriptVariant_DecimalValue(svar, &dbl1)==S_OK &&
	   ScriptVariant_DecimalValue(rightChild, &dbl2)==S_OK)
	{
		if(svar->vt==VT_DECIMAL || rightChild->vt==VT_DECIMAL)
		{
			retvar.vt=VT_DECIMAL;
			retvar.dblVal = dbl1-dbl2;
		}
		else
		{
			retvar.vt=VT_INTEGER;
			retvar.lVal = (LONG)(dbl1-dbl2);
		}
	}
	else ScriptVariant_Clear(&retvar);

	return &retvar;
}


ScriptVariant* ScriptVariant_Mul( ScriptVariant* svar, ScriptVariant* rightChild )
{
	static ScriptVariant retvar = {{.ptrVal=NULL}, VT_EMPTY};
	DOUBLE dbl1,dbl2;
	if(ScriptVariant_DecimalValue(svar, &dbl1)==S_OK &&
	   ScriptVariant_DecimalValue(rightChild, &dbl2)==S_OK)
	{
		if(svar->vt==VT_DECIMAL || rightChild->vt==VT_DECIMAL)
		{
			retvar.vt=VT_DECIMAL;
			retvar.dblVal = dbl1*dbl2;
		}
		else
		{
			retvar.vt=VT_INTEGER;
			retvar.lVal = (LONG)(dbl1*dbl2);
		}
	}
	else ScriptVariant_Clear(&retvar);

	return &retvar;
}


ScriptVariant* ScriptVariant_Div( ScriptVariant* svar, ScriptVariant* rightChild )
{
	static ScriptVariant retvar = {{.ptrVal=NULL}, VT_EMPTY};
	DOUBLE dbl1,dbl2;
	if(ScriptVariant_DecimalValue(svar, &dbl1)==S_OK &&
	   ScriptVariant_DecimalValue(rightChild, &dbl2)==S_OK)
	{
		if(dbl2 == 0)
		{
			ScriptVariant_Init(&retvar);
		}
		else if(svar->vt==VT_DECIMAL || rightChild->vt==VT_DECIMAL)
		{
			retvar.vt=VT_DECIMAL;
			retvar.dblVal = dbl1/dbl2;
		}
		else
		{
			retvar.vt=VT_INTEGER;
			retvar.lVal = (LONG)(dbl1/dbl2);
		}
	}
	else ScriptVariant_Clear(&retvar);

	return &retvar;
}


ScriptVariant* ScriptVariant_Mod( ScriptVariant* svar, ScriptVariant* rightChild )
{
	static ScriptVariant retvar = {{.ptrVal=NULL}, VT_EMPTY};
	LONG l1, l2;
	if(ScriptVariant_IntegerValue(svar, &l1)==S_OK &&
	   ScriptVariant_IntegerValue(rightChild, &l2)==S_OK)
	{
		retvar.vt=VT_INTEGER;
		retvar.lVal = l1 % l2;
	}
	else ScriptVariant_Clear(&retvar);

	return &retvar;
}

//Unary Operations
//++i

void ScriptVariant_Inc_Op(ScriptVariant* svar )
{
	switch(svar->vt)
	{
	case VT_DECIMAL:++(svar->dblVal);break;
	case VT_INTEGER:++(svar->lVal);break;
	default:break;
	}

   //Send back this ScriptVariant
   //return svar;
}

// i++

ScriptVariant* ScriptVariant_Inc_Op2(ScriptVariant* svar )
{
	static ScriptVariant retvar = {{.ptrVal=NULL}, VT_EMPTY};
	ScriptVariant_Copy(&retvar, svar);

	switch(svar->vt)
	{
	case VT_DECIMAL:svar->dblVal++;break;
	case VT_INTEGER:svar->lVal++;break;
	default:ScriptVariant_Clear(&retvar);break;
	}

	return &retvar;
}

//--i

void ScriptVariant_Dec_Op(ScriptVariant* svar )
{
	switch(svar->vt)
	{
	case VT_DECIMAL:--(svar->dblVal);break;
	case VT_INTEGER:--(svar->lVal);break;
	default:break;
	}

   //Send back this ScriptVariant
   //return svar;
}

// i--

ScriptVariant* ScriptVariant_Dec_Op2(ScriptVariant* svar )
{
	static ScriptVariant retvar = {{.ptrVal=NULL}, VT_EMPTY};
	ScriptVariant_Copy(&retvar, svar);

	switch(svar->vt)
	{
	case VT_DECIMAL:svar->dblVal--;break;
	case VT_INTEGER:svar->lVal--;break;
	default:ScriptVariant_Clear(&retvar);break;
	}

	return &retvar;
}

//+i

void ScriptVariant_Pos( ScriptVariant* svar)
{
	/*
	static ScriptVariant retvar = {{.ptrVal=NULL}, VT_EMPTY};
	switch(svar->vt) 
	{
	case VT_DECIMAL:retvar.vt=VT_DECIMAL;retvar.dblVal = +(svar->dblVal);
	case VT_INTEGER:retvar.vt=VT_INTEGER;retvar.lVal = +(svar->lVal);
	default:break;
	}
   ScriptVariant_Copy(svar, &retvar);
	return svar;*/
}

//-i

void ScriptVariant_Neg( ScriptVariant* svar)
{
	switch(svar->vt) 
	{
	case VT_DECIMAL:svar->dblVal = -(svar->dblVal);
	case VT_INTEGER:svar->lVal = -(svar->lVal);
	default:break;
	}
	//return svar;
}


void ScriptVariant_Boolean_Not(ScriptVariant* svar )
{
	/*
   static ScriptVariant retvar = {{.lVal=0}, VT_INTEGER};
   retvar.lVal = !ScriptVariant_IsTrue(svar);
   ScriptVariant_Copy(svar, &retvar);
   return svar;*/
   BOOL b = !ScriptVariant_IsTrue(svar);
   ScriptVariant_ChangeType(svar, VT_INTEGER);
   svar->lVal = b;

}



