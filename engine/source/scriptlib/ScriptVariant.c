/*
 * OpenBOR - http://www.chronocrash.com
 * -----------------------------------------------------------------------
 * All rights reserved, see LICENSE in OpenBOR root for details.
 *
 * Copyright (c)  OpenBOR Team
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "globals.h"
#include "ScriptVariant.h"

#define STRCACHE_INC      64

/*
* Caskey, Damon V.
* 2023-04-17
* 
* Populate a list of metadata for
* each variant type.
*/
const s_script_variant_meta script_variant_meta_list[] = {
    [VT_EMPTY] = {        
        .id_string = "VT_EMPTY",
        .print_format = "%s",
    },

    [VT_INTEGER] = {
        .id_string = "VT_INTEGER",
        .print_format = "%d",
    },

    [VT_DECIMAL] = {        
        .id_string = "VT_DECIMAL",
        .print_format = "%f",
    },

    [VT_PTR] = {
        .id_string = "VT_PTR",
        .print_format = "%p",
    },

    [VT_STR] = {
        .id_string = "VT_STR",
        .print_format = "%s",
    },
};

typedef struct
{
    int len;
    int ref;
    CHAR *str;
} Varstr;

// use string cache to cut the memory usage down, because not all variants are string, no need to give each of them an array
Varstr *strcache = NULL;
int   strcache_size = 0;
int   strcache_top = -1;
int  *strcache_index = NULL;

//clear the string cache
void StrCache_Clear()
{
    int i;
    if(strcache)
    {
        for(i = 0; i < strcache_size; i++)
        {
            if(strcache[i].str)
            {
                free(strcache[i].str);
            }
            strcache[i].str = NULL;
        }
        free(strcache);
        strcache = NULL;
    }
    if(strcache_index)
    {
        free(strcache_index);
        strcache_index = NULL;
    }
    strcache_size = 0;
    strcache_top = -1;
}

// init the string cache
void StrCache_Init()
{
    int i;
    StrCache_Clear(); // just in case
    strcache = calloc(STRCACHE_INC, sizeof(*strcache));
    strcache_index = malloc(sizeof(*strcache_index) * STRCACHE_INC);
    for(i = 0; i < STRCACHE_INC; i++)
    {
        strcache[i].str = NULL;
        strcache_index[i] = i;
    }
    strcache_size = STRCACHE_INC;
    strcache_top = strcache_size - 1;
}

// unrefs a string
void StrCache_Collect(int index)
{
    assert(index >= 0);
    assert(strcache[index].ref > 0);

    strcache[index].ref--;
    if(!strcache[index].ref)
    {
        //assert(strcache_top+1<strcache_size);
        free(strcache[index].str);
        strcache[index].str = NULL;
        strcache_index[++strcache_top] = index;
    }
}

int StrCache_Pop(int length)
{
    int i;
    if(strcache_size == 0)
    {
        StrCache_Init();
    }
    if(strcache_top < 0) // realloc
    {
        __reallocto(strcache, strcache_size, strcache_size + STRCACHE_INC);
        __reallocto(strcache_index, strcache_size, strcache_size + STRCACHE_INC);
        for(i = 0; i < STRCACHE_INC; i++)
        {
            strcache_index[i] = strcache_size + i;
        }

        memset(strcache + strcache_size, 0, sizeof(*strcache) * STRCACHE_INC);

        //printf("debug: dumping string cache....\n");
        //for(i=0; i<strcache_size; i++)
        //	printf("\t\"%s\"  %d\n", strcache[i].str, strcache[i].ref);

        strcache_size += STRCACHE_INC;
        strcache_top += STRCACHE_INC;

        //printf("debug: string cache resized to %d \n", strcache_size);
    }
    i = strcache_index[strcache_top--];
    strcache[i].str = malloc(length + 1);
    strcache[i].str[0] = 0;
    strcache[i].len = length;
    strcache[i].ref = 1;
    return i;
}

int StrCache_CreateNewFrom(const CHAR *str)
{
    int len = strlen(str);
    int strVal = StrCache_Pop(len);
    memcpy(StrCache_Get(strVal), str, len + 1);
    return strVal;
}

CHAR *StrCache_Get(int index)
{
    assert(index >= 0);
    //assert(index<strcache_size);
    return strcache[index].str;
}

int StrCache_Len(int index)
{
    assert(index >= 0);
    //assert(index<strcache_size);
    return strcache[index].len;
}

// increments the refcount of a string
void StrCache_Grab(int index)
{
    assert(index >= 0);
    assert(strcache[index].ref > 0);
    ++strcache[index].ref;
}


void ScriptVariant_Clear(ScriptVariant *var)
{
    ScriptVariant_ChangeType(var, VT_EMPTY);
    var->ptrVal = NULL; // not sure, maybe this is the longest member in the union
}

void ScriptVariant_Init(ScriptVariant *var)
{
    //memset(var, 0, 8);
    var->ptrVal = NULL; // not sure, maybe this is the longest member in the union
    var->vt = VT_EMPTY;
}

void ScriptVariant_ChangeType(ScriptVariant *var, VARTYPE cvt)
{
    // Always collect make it safer for string copy
    // since now reference has been added.
    // String variables should never be changed
    // unless the engine is creating a new one
    if(var->vt == VT_STR)
    {
        StrCache_Collect(var->strVal);
    }
    var->vt = cvt;
    if (cvt == VT_STR)
    {
        var->strVal = -1;
    }
}

// find an existing constant before copy
void ScriptVariant_ParseStringConstant(ScriptVariant *var, CHAR *str)
{
    //assert(index<strcache_size);
    //assert(size>0);
    int i;
    for(i = 0; i < strcache_size; i++)
    {
        if (strcache[i].ref && strcmp(str, strcache[i].str) == 0)
        {
            var->strVal = i;
            strcache[i].ref++;
            var->vt = VT_STR;
            return;
        }
    }

    ScriptVariant_ChangeType(var, VT_STR);
    var->strVal = StrCache_CreateNewFrom(str);
}

HRESULT ScriptVariant_IntegerValue(ScriptVariant *var, LONG *pVal)
{
    if(var->vt == VT_INTEGER)
    {
        *pVal = var->lVal;
    }
    else if(var->vt == VT_DECIMAL)
    {
        *pVal = (LONG)var->dblVal;
    }
    else
    {
        return E_FAIL;
    }

    return S_OK;
}

HRESULT ScriptVariant_DecimalValue(ScriptVariant *var, DOUBLE *pVal)
{
    if(var->vt == VT_INTEGER)
    {
        *pVal = (DOUBLE)var->lVal;
    }
    else if(var->vt == VT_DECIMAL)
    {
        *pVal = var->dblVal;
    }
    else
    {
        return E_FAIL;
    }

    return S_OK;
}


BOOL ScriptVariant_IsTrue(ScriptVariant *svar)
{
    switch(svar->vt)
    {
    case VT_STR:
        return StrCache_Get(svar->strVal)[0] != 0;
    case VT_INTEGER:
        return svar->lVal != 0;
    case VT_DECIMAL:
        return svar->dblVal != 0.0;
    case VT_PTR:
        return svar->ptrVal != 0;
    default:
        return 0;
    }
}

void ScriptVariant_ToString(ScriptVariant *svar, LPSTR buffer )
{
    switch( svar->vt )
    {
    case VT_EMPTY:
        sprintf( buffer, "<VT_EMPTY>   Unitialized" );
        break;
    case VT_INTEGER:
        sprintf( buffer, "%ld", (long)svar->lVal);
        break;
    case VT_DECIMAL:
        sprintf( buffer, "%lf", svar->dblVal );
        break;
    case VT_PTR:
        sprintf(buffer, "#%ld", (long)(svar->ptrVal));
        break;
    case VT_STR:
        sprintf(buffer, "%s", StrCache_Get(svar->strVal));
        break;
    default:
        sprintf(buffer, "<Unprintable VARIANT type.>" );
        break;
    }
}

static int ScriptVariant_LengthAsString(ScriptVariant *svar)
{
    switch (svar->vt)
    {
    case VT_EMPTY:
        return snprintf(NULL, 0, "<VT_EMPTY>   Unitialized");
    case VT_INTEGER:
        return snprintf(NULL, 0, "%ld", (long)svar->lVal);
    case VT_DECIMAL:
        return snprintf(NULL, 0, "%lf", svar->dblVal);
    case VT_PTR:
        return snprintf(NULL, 0, "#%ld", (long)(svar->ptrVal));
    case VT_STR:
        // could use StrCache_Len instead to improve speed
        return snprintf(NULL, 0, "%s", StrCache_Get(svar->strVal));
    default:
        return snprintf(NULL, 0, "<Unprintable VARIANT type.>");
    }
}

// faster if it is not VT_STR
void ScriptVariant_Copy(ScriptVariant *svar, ScriptVariant *rightChild )
{
    // collect the str cache index
    if(svar->vt == VT_STR)
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
        svar->strVal = rightChild->strVal;
        StrCache_Grab(svar->strVal);
        break;
    default:
        //should not happen unless the variant is not intialized correctly
        //borShutdown(1, "invalid variant type");
        svar->ptrVal = NULL;
        break;
    }
    svar->vt = rightChild->vt;
}

// light version, for compiled call, faster than above, but not safe in some situations
ScriptVariant *ScriptVariant_Assign(ScriptVariant *svar, ScriptVariant *rightChild )
{
    ScriptVariant_Copy(svar, rightChild);
    return rightChild;
}


ScriptVariant *ScriptVariant_MulAssign(ScriptVariant *svar, ScriptVariant *rightChild )
{
    ScriptVariant_Copy(svar, ScriptVariant_Mul(svar, rightChild));
    return svar;
}


ScriptVariant *ScriptVariant_DivAssign(ScriptVariant *svar, ScriptVariant *rightChild )
{
    ScriptVariant_Copy(svar, ScriptVariant_Div(svar, rightChild));
    return svar;
}


ScriptVariant *ScriptVariant_AddAssign(ScriptVariant *svar, ScriptVariant *rightChild )
{
    ScriptVariant_Copy(svar, ScriptVariant_Add(svar, rightChild));
    return svar;
}


ScriptVariant *ScriptVariant_SubAssign(ScriptVariant *svar, ScriptVariant *rightChild )
{
    ScriptVariant_Copy(svar, ScriptVariant_Sub(svar, rightChild));
    return svar;
}


ScriptVariant *ScriptVariant_ModAssign(ScriptVariant *svar, ScriptVariant *rightChild )
{
    ScriptVariant_Copy(svar, ScriptVariant_Mod(svar, rightChild));
    return svar;
}

//Logical Operations

ScriptVariant *ScriptVariant_Or( ScriptVariant *svar, ScriptVariant *rightChild )
{
    static ScriptVariant retvar = {{.lVal = 0}, VT_INTEGER};
    retvar.lVal = (ScriptVariant_IsTrue(svar) || ScriptVariant_IsTrue(rightChild));
    return &retvar;
}


ScriptVariant *ScriptVariant_And( ScriptVariant *svar, ScriptVariant *rightChild )
{
    static ScriptVariant retvar = {{.lVal = 0}, VT_INTEGER};
    retvar.lVal = (ScriptVariant_IsTrue(svar) && ScriptVariant_IsTrue(rightChild));
    return &retvar;
}

ScriptVariant *ScriptVariant_Bit_Or( ScriptVariant *svar, ScriptVariant *rightChild )
{
    static ScriptVariant retvar = {{.ptrVal = NULL}, VT_EMPTY};
    LONG l1, l2;
    if(ScriptVariant_IntegerValue(svar, &l1) == S_OK &&
            ScriptVariant_IntegerValue(rightChild, &l2) == S_OK)
    {
        retvar.vt = VT_INTEGER;
        retvar.lVal = l1 | l2;
    }
    else
    {
        ScriptVariant_Clear(&retvar);
    }

    return &retvar;
}

ScriptVariant *ScriptVariant_Xor( ScriptVariant *svar, ScriptVariant *rightChild )
{
    static ScriptVariant retvar = {{.ptrVal = NULL}, VT_EMPTY};
    LONG l1, l2;
    if(ScriptVariant_IntegerValue(svar, &l1) == S_OK &&
            ScriptVariant_IntegerValue(rightChild, &l2) == S_OK)
    {
        retvar.vt = VT_INTEGER;
        retvar.lVal = l1 ^ l2;
    }
    else
    {
        ScriptVariant_Clear(&retvar);
    }

    return &retvar;
}

ScriptVariant *ScriptVariant_Bit_And( ScriptVariant *svar, ScriptVariant *rightChild )
{
    static ScriptVariant retvar = {{.ptrVal = NULL}, VT_EMPTY};
    LONG l1, l2;
    if(ScriptVariant_IntegerValue(svar, &l1) == S_OK &&
            ScriptVariant_IntegerValue(rightChild, &l2) == S_OK)
    {
        retvar.vt = VT_INTEGER;
        retvar.lVal = l1 & l2;
    }
    else
    {
        ScriptVariant_Clear(&retvar);
    }

    return &retvar;
}

ScriptVariant *ScriptVariant_Eq( ScriptVariant *svar, ScriptVariant *rightChild )
{
    DOUBLE dbl1, dbl2;
    static ScriptVariant retvar = {{.lVal = 0}, VT_INTEGER};

    if(ScriptVariant_DecimalValue(svar, &dbl1) == S_OK &&
            ScriptVariant_DecimalValue(rightChild, &dbl2) == S_OK)
    {
        retvar.lVal = (dbl1 == dbl2);
    }
    else if(svar->vt == VT_STR && rightChild->vt == VT_STR)
    {
        retvar.lVal = !(strcmp(StrCache_Get(svar->strVal), StrCache_Get(rightChild->strVal)));
    }
    else if(svar->vt == VT_PTR && rightChild->vt == VT_PTR)
    {
        retvar.lVal = (svar->ptrVal == rightChild->ptrVal);
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


ScriptVariant *ScriptVariant_Ne( ScriptVariant *svar, ScriptVariant *rightChild )
{
    DOUBLE dbl1, dbl2;
    static ScriptVariant retvar = {{.lVal = 0}, VT_INTEGER};

    if(ScriptVariant_DecimalValue(svar, &dbl1) == S_OK &&
            ScriptVariant_DecimalValue(rightChild, &dbl2) == S_OK)
    {
        retvar.lVal = (dbl1 != dbl2);
    }
    else if(svar->vt == VT_STR && rightChild->vt == VT_STR)
    {
        retvar.lVal = strcmp(StrCache_Get(svar->strVal), StrCache_Get(rightChild->strVal));
    }
    else if(svar->vt == VT_PTR && rightChild->vt == VT_PTR)
    {
        retvar.lVal = (svar->ptrVal != rightChild->ptrVal);
    }
    else if(svar->vt == VT_EMPTY && rightChild->vt == VT_EMPTY)
    {
        retvar.lVal = 0;
    }
    else
    {
        retvar.lVal = (memcmp(svar, rightChild, sizeof(ScriptVariant)) != 0);
    }

    return &retvar;
}


ScriptVariant *ScriptVariant_Lt( ScriptVariant *svar, ScriptVariant *rightChild )
{
    DOUBLE dbl1, dbl2;
    static ScriptVariant retvar = {{.lVal = 0}, VT_INTEGER};

    if(ScriptVariant_DecimalValue(svar, &dbl1) == S_OK &&
            ScriptVariant_DecimalValue(rightChild, &dbl2) == S_OK)
    {
        retvar.lVal = (dbl1 < dbl2);
    }
    else if(svar->vt == VT_STR && rightChild->vt == VT_STR)
    {
        retvar.lVal = (strcmp(StrCache_Get(svar->strVal), StrCache_Get(rightChild->strVal)) < 0);
    }
    else if(svar->vt == VT_PTR && rightChild->vt == VT_PTR)
    {
        retvar.lVal = (svar->ptrVal < rightChild->ptrVal);
    }
    else if(svar->vt == VT_EMPTY || rightChild->vt == VT_EMPTY)
    {
        retvar.lVal = 0;
    }
    else
    {
        retvar.lVal = (memcmp(svar, rightChild, sizeof(ScriptVariant)) < 0);
    }

    return &retvar;
}



ScriptVariant *ScriptVariant_Gt( ScriptVariant *svar, ScriptVariant *rightChild )
{
    DOUBLE dbl1, dbl2;
    static ScriptVariant retvar = {{.lVal = 0}, VT_INTEGER};

    if(ScriptVariant_DecimalValue(svar, &dbl1) == S_OK &&
            ScriptVariant_DecimalValue(rightChild, &dbl2) == S_OK)
    {
        retvar.lVal = (dbl1 > dbl2);
    }
    else if(svar->vt == VT_STR && rightChild->vt == VT_STR)
    {
        retvar.lVal = (strcmp(StrCache_Get(svar->strVal), StrCache_Get(rightChild->strVal)) > 0);
    }
    else if(svar->vt == VT_PTR && rightChild->vt == VT_PTR)
    {
        retvar.lVal = (svar->ptrVal > rightChild->ptrVal);
    }
    else if(svar->vt == VT_EMPTY || rightChild->vt == VT_EMPTY)
    {
        retvar.lVal = 0;
    }
    else
    {
        retvar.lVal = (memcmp(svar, rightChild, sizeof(ScriptVariant)) > 0);
    }

    return &retvar;
}



ScriptVariant *ScriptVariant_Ge( ScriptVariant *svar, ScriptVariant *rightChild )
{
    DOUBLE dbl1, dbl2;
    static ScriptVariant retvar = {{.lVal = 0}, VT_INTEGER};

    if(ScriptVariant_DecimalValue(svar, &dbl1) == S_OK &&
            ScriptVariant_DecimalValue(rightChild, &dbl2) == S_OK)
    {
        retvar.lVal = (dbl1 >= dbl2);
    }
    else if(svar->vt == VT_STR && rightChild->vt == VT_STR)
    {
        retvar.lVal = (strcmp(StrCache_Get(svar->strVal), StrCache_Get(rightChild->strVal)) >= 0);
    }
    else if(svar->vt == VT_PTR && rightChild->vt == VT_PTR)
    {
        retvar.lVal = (svar->ptrVal >= rightChild->ptrVal);
    }
    else if(svar->vt == VT_EMPTY || rightChild->vt == VT_EMPTY)
    {
        retvar.lVal = 0;
    }
    else
    {
        retvar.lVal = (memcmp(svar, rightChild, sizeof(ScriptVariant)) >= 0);
    }

    return &retvar;
}


ScriptVariant *ScriptVariant_Le( ScriptVariant *svar, ScriptVariant *rightChild )
{
    DOUBLE dbl1, dbl2;
    static ScriptVariant retvar = {{.lVal = 0}, VT_INTEGER};

    if(ScriptVariant_DecimalValue(svar, &dbl1) == S_OK &&
            ScriptVariant_DecimalValue(rightChild, &dbl2) == S_OK)
    {
        retvar.lVal = (dbl1 <= dbl2);
    }
    else if(svar->vt == VT_STR && rightChild->vt == VT_STR)
    {
        retvar.lVal = (strcmp(StrCache_Get(svar->strVal), StrCache_Get(rightChild->strVal)) <= 0);
    }
    else if(svar->vt == VT_PTR && rightChild->vt == VT_PTR)
    {
        retvar.lVal = (svar->ptrVal <= rightChild->ptrVal);
    }
    else if(svar->vt == VT_EMPTY || rightChild->vt == VT_EMPTY)
    {
        retvar.lVal = 0;
    }
    else
    {
        retvar.lVal = (memcmp(svar, rightChild, sizeof(ScriptVariant)) <= 0);
    }

    return &retvar;
}


ScriptVariant *ScriptVariant_Shl( ScriptVariant *svar, ScriptVariant *rightChild )
{
    static ScriptVariant retvar = {{.ptrVal = NULL}, VT_EMPTY};
    LONG l1, l2;
    if(ScriptVariant_IntegerValue(svar, &l1) == S_OK &&
            ScriptVariant_IntegerValue(rightChild, &l2) == S_OK)
    {
        retvar.vt = VT_INTEGER;
        retvar.lVal = ((ULONG)l1) << ((ULONG)l2);
    }
    else
    {
        ScriptVariant_Clear(&retvar);
    }

    return &retvar;
}


ScriptVariant *ScriptVariant_Shr( ScriptVariant *svar, ScriptVariant *rightChild )
{
    static ScriptVariant retvar = {{.ptrVal = NULL}, VT_EMPTY};
    LONG l1, l2;
    if(ScriptVariant_IntegerValue(svar, &l1) == S_OK &&
            ScriptVariant_IntegerValue(rightChild, &l2) == S_OK)
    {
        retvar.vt = VT_INTEGER;
        retvar.lVal = ((ULONG)l1) >> ((ULONG)l2);
    }
    else
    {
        ScriptVariant_Clear(&retvar);
    }

    return &retvar;
}


ScriptVariant *ScriptVariant_Add( ScriptVariant *svar, ScriptVariant *rightChild )
{
    static ScriptVariant retvar = {{.ptrVal = NULL}, VT_EMPTY};
    DOUBLE dbl1, dbl2;
    if(ScriptVariant_DecimalValue(svar, &dbl1) == S_OK &&
            ScriptVariant_DecimalValue(rightChild, &dbl2) == S_OK)
    {
        if(svar->vt == VT_DECIMAL || rightChild->vt == VT_DECIMAL)
        {
            ScriptVariant_ChangeType(&retvar, VT_DECIMAL);
            retvar.dblVal = dbl1 + dbl2;
        }
        else
        {
            ScriptVariant_ChangeType(&retvar, VT_INTEGER);
            retvar.lVal = (LONG)(dbl1 + dbl2);
        }
    }
    else if(svar->vt == VT_STR || rightChild->vt == VT_STR)
    {
        CHAR *dst;
        int len1 = ScriptVariant_LengthAsString(svar),
            len2 = ScriptVariant_LengthAsString(rightChild);

        ScriptVariant_ChangeType(&retvar, VT_STR);
        retvar.strVal = StrCache_Pop(len1 + len2);
        dst = StrCache_Get(retvar.strVal);
        ScriptVariant_ToString(svar, dst);
        ScriptVariant_ToString(rightChild, dst + len1);
        dst[len1 + len2] = '\0'; // shouldn't be needed, but just in case
    }
    else
    {
        ScriptVariant_Clear(&retvar);
    }

    return &retvar;
}


ScriptVariant *ScriptVariant_Sub( ScriptVariant *svar, ScriptVariant *rightChild )
{
    static ScriptVariant retvar = {{.ptrVal = NULL}, VT_EMPTY};
    DOUBLE dbl1, dbl2;
    if(ScriptVariant_DecimalValue(svar, &dbl1) == S_OK &&
            ScriptVariant_DecimalValue(rightChild, &dbl2) == S_OK)
    {
        if(svar->vt == VT_DECIMAL || rightChild->vt == VT_DECIMAL)
        {
            retvar.vt = VT_DECIMAL;
            retvar.dblVal = dbl1 - dbl2;
        }
        else
        {
            retvar.vt = VT_INTEGER;
            retvar.lVal = (LONG)(dbl1 - dbl2);
        }
    }
    else
    {
        ScriptVariant_Clear(&retvar);
    }

    return &retvar;
}


ScriptVariant *ScriptVariant_Mul( ScriptVariant *svar, ScriptVariant *rightChild )
{
    static ScriptVariant retvar = {{.ptrVal = NULL}, VT_EMPTY};
    DOUBLE dbl1, dbl2;
    if(ScriptVariant_DecimalValue(svar, &dbl1) == S_OK &&
            ScriptVariant_DecimalValue(rightChild, &dbl2) == S_OK)
    {
        if(svar->vt == VT_DECIMAL || rightChild->vt == VT_DECIMAL)
        {
            retvar.vt = VT_DECIMAL;
            retvar.dblVal = dbl1 * dbl2;
        }
        else
        {
            retvar.vt = VT_INTEGER;
            retvar.lVal = (LONG)(dbl1 * dbl2);
        }
    }
    else
    {
        ScriptVariant_Clear(&retvar);
    }

    return &retvar;
}


ScriptVariant *ScriptVariant_Div( ScriptVariant *svar, ScriptVariant *rightChild )
{
    static ScriptVariant retvar = {{.ptrVal = NULL}, VT_EMPTY};
    DOUBLE dbl1, dbl2;
    if(ScriptVariant_DecimalValue(svar, &dbl1) == S_OK &&
            ScriptVariant_DecimalValue(rightChild, &dbl2) == S_OK)
    {
        if(dbl2 == 0)
        {
            ScriptVariant_Init(&retvar);
        }
        else if(svar->vt == VT_DECIMAL || rightChild->vt == VT_DECIMAL)
        {
            retvar.vt = VT_DECIMAL;
            retvar.dblVal = dbl1 / dbl2;
        }
        else
        {
            retvar.vt = VT_INTEGER;
            retvar.lVal = (LONG)(dbl1 / dbl2);
        }
    }
    else
    {
        ScriptVariant_Clear(&retvar);
    }

    return &retvar;
}


ScriptVariant *ScriptVariant_Mod( ScriptVariant *svar, ScriptVariant *rightChild )
{
    static ScriptVariant retvar = {{.ptrVal = NULL}, VT_EMPTY};
    LONG l1, l2;
    if(ScriptVariant_IntegerValue(svar, &l1) == S_OK &&
            ScriptVariant_IntegerValue(rightChild, &l2) == S_OK)
    {
        retvar.vt = VT_INTEGER;
        retvar.lVal = l1 % l2;
    }
    else
    {
        ScriptVariant_Clear(&retvar);
    }

    return &retvar;
}

//Unary Operations
//++i

void ScriptVariant_Inc_Op(ScriptVariant *svar )
{
    switch(svar->vt)
    {
    case VT_DECIMAL:
        ++(svar->dblVal);
        break;
    case VT_INTEGER:
        ++(svar->lVal);
        break;
    default:
        break;
    }

    //Send back this ScriptVariant
    //return svar;
}

// i++

ScriptVariant *ScriptVariant_Inc_Op2(ScriptVariant *svar )
{
    static ScriptVariant retvar = {{.ptrVal = NULL}, VT_EMPTY};
    ScriptVariant_Copy(&retvar, svar);

    switch(svar->vt)
    {
    case VT_DECIMAL:
        svar->dblVal++;
        break;
    case VT_INTEGER:
        svar->lVal++;
        break;
    default:
        ScriptVariant_Clear(&retvar);
        break;
    }

    return &retvar;
}

//--i

void ScriptVariant_Dec_Op(ScriptVariant *svar )
{
    switch(svar->vt)
    {
    case VT_DECIMAL:
        --(svar->dblVal);
        break;
    case VT_INTEGER:
        --(svar->lVal);
        break;
    default:
        break;
    }

    //Send back this ScriptVariant
    //return svar;
}

// i--

ScriptVariant *ScriptVariant_Dec_Op2(ScriptVariant *svar )
{
    static ScriptVariant retvar = {{.ptrVal = NULL}, VT_EMPTY};
    ScriptVariant_Copy(&retvar, svar);

    switch(svar->vt)
    {
    case VT_DECIMAL:
        svar->dblVal--;
        break;
    case VT_INTEGER:
        svar->lVal--;
        break;
    default:
        ScriptVariant_Clear(&retvar);
        break;
    }

    return &retvar;
}

//+i

void ScriptVariant_Pos( ScriptVariant *svar)
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

void ScriptVariant_Neg( ScriptVariant *svar)
{
    switch(svar->vt)
    {
    case VT_DECIMAL:
        svar->dblVal = -(svar->dblVal);
		break;
    case VT_INTEGER:
        svar->lVal = -(svar->lVal);
    default:
        break;
    }
    //return svar;
}


void ScriptVariant_Boolean_Not(ScriptVariant *svar )
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

void ScriptVariant_Bitwise_Not(ScriptVariant *svar )
{
    if (svar->vt == VT_INTEGER)
    {
        svar->lVal = ~(svar->lVal);
    }
}



