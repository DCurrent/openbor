/*
 * OpenBOR - http://www.chronocrash.com
 * -----------------------------------------------------------------------
 * All rights reserved, see LICENSE in OpenBOR root for details.
 *
 * Copyright (c)  OpenBOR Team
 */

#include <limits.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <inttypes.h>
#include "globals.h"
#include "ScriptVariant.h"
#include "ScriptVariantInteger.h"

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
        .print_format = "%ld",
    },

    [VT_DECIMAL] = {
        .id_string = "VT_DECIMAL",
        .print_format = "%f",
    },

    [VT_INTEGER64] = {
        .id_string = "VT_INTEGER64",
        .print_format = "%" PRId64,
    },

    [VT_UINTEGER64] = {
        .id_string = "VT_UINTEGER64",
        .print_format = "%" PRIu64,
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

    assert(length >= 0);

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
    strcache[i].str[length] = 0;
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

/*
- Caskey, Damon V.
- 2026-08-11
-
- Decode one quoted script string literal. Source length is
  explicit so the lexer may provide a non-owning view instead
  of copying the literal through fixed token storage.
*/
size_t ScriptString_DecodeLiteral(
    CHAR *destination,
    size_t destination_size,
    const CHAR *source,
    size_t source_length
)
{
    const CHAR *cursor;
    const CHAR *end;
    size_t output_length = 0;

    assert(source);

    cursor = source;
    end = source + source_length;

    if(source_length >= 2 && cursor[0] == '"' && end[-1] == '"')
    {
        cursor++;
        end--;
    }

#define APPEND_DECODED_CHARACTER(character) \
    do \
    { \
        if(destination && output_length + 1 < destination_size) \
        { \
            destination[output_length] = (character); \
        } \
        output_length++; \
    } while(0)

    while(cursor < end)
    {
        if(*cursor == '\\' && cursor + 1 < end)
        {
            cursor++;

            switch(*cursor)
            {
            case 's':
                APPEND_DECODED_CHARACTER(' ');
                cursor++;
                break;
            case 'r':
                APPEND_DECODED_CHARACTER('\r');
                cursor++;
                break;
            case 'n':
                APPEND_DECODED_CHARACTER('\n');
                cursor++;
                break;
            case 't':
                APPEND_DECODED_CHARACTER('\t');
                cursor++;
                break;
            case '0':
                APPEND_DECODED_CHARACTER('\0');
                cursor++;
                break;
            case '"':
                APPEND_DECODED_CHARACTER('"');
                cursor++;
                break;
            case '\'':
                APPEND_DECODED_CHARACTER('\'');
                cursor++;
                break;
            case '\\':
                APPEND_DECODED_CHARACTER('\\');
                cursor++;
                break;
            default:
                /* Preserve the legacy invalid-escape result. */
                APPEND_DECODED_CHARACTER('\\');
                APPEND_DECODED_CHARACTER(*cursor);
                break;
            }
        }
        else
        {
            APPEND_DECODED_CHARACTER(*cursor);
            cursor++;
        }
    }

    if(destination && destination_size)
    {
        destination[output_length < destination_size
            ? output_length
            : destination_size - 1] = '\0';
    }

#undef APPEND_DECODED_CHARACTER

    return output_length;
}

// find an existing constant before copy
HRESULT ScriptVariant_ParseStringConstant(ScriptVariant *var, const CHAR *str)
{
    //assert(index<strcache_size);
    //assert(size>0);
    int i;
    size_t length;

    if(!var || !str)
    {
        return E_FAIL;
    }

    for(length = 0;
        length <= MAX_SCRIPT_STRING_LENGTH && str[length];
        length++)
    {
    }

    if(length > MAX_SCRIPT_STRING_LENGTH)
    {
        return E_FAIL;
    }

    for(i = 0; i < strcache_size; i++)
    {
        if (strcache[i].ref && strcmp(str, strcache[i].str) == 0)
        {
            var->strVal = i;
            strcache[i].ref++;
            var->vt = VT_STR;
            return S_OK;
        }
    }

    ScriptVariant_ChangeType(var, VT_STR);
    var->strVal = StrCache_CreateNewFrom(str);

    return S_OK;
}

/*
- Caskey, Damon V.
- 2026-08-11
-
- Convert a length-delimited source literal directly into a
  runtime string constant without fixed intermediate storage.
*/
HRESULT ScriptVariant_ParseStringLiteral(
    ScriptVariant *var,
    const CHAR *source,
    size_t source_length
)
{
    CHAR *decoded;
    size_t decoded_length;

    if(!var || !source)
    {
        return E_FAIL;
    }

    decoded_length = ScriptString_DecodeLiteral(
        NULL,
        0,
        source,
        source_length
    );

    if(decoded_length > MAX_SCRIPT_STRING_LENGTH)
    {
        return E_FAIL;
    }

    decoded = malloc(decoded_length + 1);

    if(!decoded)
    {
        return E_FAIL;
    }

    ScriptString_DecodeLiteral(
        decoded,
        decoded_length + 1,
        source,
        source_length
    );
    if(FAILED(ScriptVariant_ParseStringConstant(var, decoded)))
    {
        free(decoded);
        return E_FAIL;
    }

    free(decoded);

    return S_OK;
}

/*
* Caskey, Damon V.
* Orginal author (Utunels?) and date unknown.
* 
* Reworked 2026-06-02 to safely fail on overflow
* so script can support 64-bit integers.
*
* Get a 32-bit integer value from a variant. 
* Will attempt to convert if possible. 
*/
HRESULT ScriptVariant_IntegerValue(ScriptVariant *var, LONG *pVal) {

    int64_t temp;

    if (!var || !pVal) {
        return E_FAIL;
    }

    switch (var->vt) {

        case VT_INTEGER:
            *pVal = var->lVal;
            return S_OK;

        case VT_DECIMAL:
            *pVal = (LONG)var->dblVal;
            return S_OK;

        case VT_INTEGER64:
            temp = var->llVal;

            if (temp < (int64_t)LONG_MIN || temp > (int64_t)LONG_MAX) {
                return E_FAIL;
            }

            *pVal = (LONG)temp;
            return S_OK;

        case VT_UINTEGER64:
            if (var->ullVal > (uint64_t)LONG_MAX) {
                return E_FAIL;
            }

            *pVal = (LONG)var->ullVal;
            return S_OK;

    default:
        return E_FAIL;
    }
}

/*
* Caskey, Damon V.
* Orginal author (Utunels?) and date unknown.
* 
* Reworked 2026-06-02 to safely handle 64-bit 
* integers.
*
* Get a decimal value from a variant. 
* Will attempt to convert if possible. 
*/
HRESULT ScriptVariant_DecimalValue(ScriptVariant *var, DOUBLE *pVal)
{
    if (!var || !pVal) {
        return E_FAIL;
    }

    switch (var->vt) {
    case VT_INTEGER:
        *pVal = (DOUBLE)var->lVal;
        return S_OK;

    case VT_INTEGER64:
        *pVal = (DOUBLE)var->llVal;
        return S_OK;

    case VT_UINTEGER64:
        *pVal = (DOUBLE)var->ullVal;
        return S_OK;

    case VT_DECIMAL:
        *pVal = var->dblVal;
        return S_OK;

    default:
        return E_FAIL;
    }
}

/*
* Caskey, Damon V.
* 2026-06-02
*
* Get a 64-bit integer value from a variant. 
* Will attempt to convert if possible. 
*/
HRESULT ScriptVariant_Integer64Value(ScriptVariant *var, int64_t *pVal) {
    if (!var || !pVal) {
        return E_FAIL;
    }

    switch (var->vt) {
        case VT_INTEGER:
            *pVal = (int64_t)var->lVal;
            return S_OK;

        case VT_INTEGER64:
            *pVal = var->llVal;
            return S_OK;

        case VT_UINTEGER64:
            if (var->ullVal > (uint64_t)INT64_MAX) {
                return E_FAIL;
            }

            *pVal = (int64_t)var->ullVal;
            return S_OK;

        case VT_DECIMAL:
            *pVal = (int64_t)var->dblVal;
            return S_OK;

        default:
        return E_FAIL;
    }
}

/*
* Caskey, Damon V.
* 2026-06-02
*
* Get an unsigned 64-bit integer value from a variant. 
* Will attempt to convert if possible. 
*/
HRESULT ScriptVariant_Unsigned64Value(ScriptVariant *var, uint64_t *pVal) {
    if (!var || !pVal) {
        return E_FAIL;
    }

    switch (var->vt) {
    case VT_INTEGER:
        if (var->lVal < 0) {
            return E_FAIL;
        }

        *pVal = (uint64_t)var->lVal;
        return S_OK;

    case VT_INTEGER64:
        if (var->llVal < 0) {
            return E_FAIL;
        }

        *pVal = (uint64_t)var->llVal;
        return S_OK;

    case VT_UINTEGER64:
        *pVal = var->ullVal;
        return S_OK;

    case VT_DECIMAL:
        if (var->dblVal < 0.0) {
            return E_FAIL;
        }

        *pVal = (uint64_t)var->dblVal;
        return S_OK;

    default:
        return E_FAIL;
    }
}

/*
* Caskey, Damon V.
* Orginal author (Utunels?) and date unknown.
* 
* Reworked 2026-06-02 to handle 64-bit 
* integers.
*/
BOOL ScriptVariant_IsTrue(ScriptVariant *svar) {
    
    switch(svar->vt) {

        case VT_STR:
            return StrCache_Get(svar->strVal)[0] != 0;

        case VT_INTEGER:
            return svar->lVal != 0;

        case VT_INTEGER64:
            return svar->llVal != 0;

        case VT_UINTEGER64:
            return svar->ullVal != 0;

        case VT_DECIMAL:
            return svar->dblVal != 0.0;

        case VT_PTR:
            return svar->ptrVal != 0;

        default:
            return 0;
    }
}

/*
- Caskey, Damon V.
- 2026-08-11
-
- Return a non-owning view of a script variant's string
  representation. String variants reference their cache-owned
  text directly. Other types use caller-provided conversion
  storage with explicit capacity.
*/
HRESULT ScriptVariant_GetStringView(
    const ScriptVariant *svar,
    CHAR *conversion_buffer,
    size_t conversion_buffer_size,
    ScriptVariantStringView *view
)
{
    const CHAR *terminator;
    int result;

    if(!svar || !view)
    {
        return E_FAIL;
    }

    view->string = NULL;
    view->length = 0;

    if(svar->vt == VT_STR)
    {
        if(svar->strVal < 0 || svar->strVal >= strcache_size ||
           !strcache[svar->strVal].str ||
           strcache[svar->strVal].len < 0 ||
           (size_t)strcache[svar->strVal].len > MAX_SCRIPT_STRING_LENGTH)
        {
            return E_FAIL;
        }

        view->string = strcache[svar->strVal].str;
        terminator = memchr(
            view->string,
            '\0',
            (size_t)strcache[svar->strVal].len + 1
        );

        if(!terminator)
        {
            view->string = NULL;
            view->length = 0;
            return E_FAIL;
        }

        view->length = (size_t)(terminator - view->string);

        return S_OK;
    }

    if(!conversion_buffer || !conversion_buffer_size)
    {
        return E_FAIL;
    }

    conversion_buffer[0] = '\0';

    switch(svar->vt)
    {
    case VT_EMPTY:
        result = snprintf(conversion_buffer, conversion_buffer_size, "<VT_EMPTY> Unitialized");
        break;
    case VT_INTEGER:
        result = snprintf(conversion_buffer, conversion_buffer_size, "%ld", (long)svar->lVal);
        break;
    case VT_INTEGER64:
        result = snprintf(conversion_buffer, conversion_buffer_size, "%" PRId64, (int64_t)svar->llVal);
        break;
    case VT_UINTEGER64:
        result = snprintf(conversion_buffer, conversion_buffer_size, "%" PRIu64, (uint64_t)svar->ullVal);
        break;
    case VT_DECIMAL:
        result = snprintf(conversion_buffer, conversion_buffer_size, "%lf", svar->dblVal);
        break;
    case VT_PTR:
        result = snprintf(conversion_buffer, conversion_buffer_size, "#%" PRIuPTR, (uintptr_t)svar->ptrVal);
        break;
    default:
        result = snprintf(conversion_buffer, conversion_buffer_size, "<Unprintable VARIANT type.>");
        break;
    }

    if(result < 0 || (size_t)result >= conversion_buffer_size ||
       (size_t)result > MAX_SCRIPT_STRING_LENGTH)
    {
        conversion_buffer[0] = '\0';
        return E_FAIL;
    }

    view->string = conversion_buffer;
    view->length = (size_t)result;

    return S_OK;
}

/*
- Caskey, Damon V.
- 2026-08-11
-
- Copy a script variant's string representation into caller
  storage with explicit capacity and optional output length.
*/
HRESULT ScriptVariant_ToString(
    const ScriptVariant *svar,
    LPSTR buffer,
    size_t buffer_size,
    size_t *output_length
)
{
    ScriptVariantStringView view;

    if(output_length)
    {
        *output_length = 0;
    }

    if(!buffer || !buffer_size)
    {
        return E_FAIL;
    }

    buffer[0] = '\0';

    if(FAILED(ScriptVariant_GetStringView(
        svar,
        buffer,
        buffer_size,
        &view
    )))
    {
        return E_FAIL;
    }

    if(view.length >= buffer_size)
    {
        buffer[0] = '\0';
        return E_FAIL;
    }

    if(view.string != buffer)
    {
        memmove(buffer, view.string, view.length + 1);
    }

    if(output_length)
    {
        *output_length = view.length;
    }

    return S_OK;
}

/*
* Caskey, Damon V.
* Orginal author (Utunels?) and date unknown.
* 
* Reworked 2026-06-02 to handle 64-bit integers.
*
* Copy a variant. Handles reference counting for
* string types.
*/
void ScriptVariant_Copy(ScriptVariant *svar, ScriptVariant *rightChild )
{
    if(svar->vt == VT_STR)
    {
        StrCache_Collect(svar->strVal);
    }

    switch( rightChild->vt ) {
        case VT_INTEGER:
            svar->lVal = rightChild->lVal;
            break;

        case VT_INTEGER64:
            svar->llVal = rightChild->llVal;
            break;

        case VT_UINTEGER64:
            svar->ullVal = rightChild->ullVal;
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
            svar->ptrVal = NULL;
            break;
    }

    svar->vt = rightChild->vt;
}

/*
* Caskey, Damon V.
* 2026-06-04
*
* The following functions determine the 
* appropriate type for a math operation
* based on the types of the operands, and
* set the result of a math operation, safely
* handling overflow and type promotion to 
* 64-bit integers when necessary.
*/

static int ScriptVariant_IsDecimalMath(ScriptVariant *left, ScriptVariant *right)
{
    return left->vt == VT_DECIMAL || right->vt == VT_DECIMAL;
}

static int ScriptVariant_IsUnsignedMath(ScriptVariant *left, ScriptVariant *right)
{
    return left->vt == VT_UINTEGER64 || right->vt == VT_UINTEGER64;
}

static int ScriptVariant_Is64BitMath(ScriptVariant *left, ScriptVariant *right)
{
    return left->vt == VT_INTEGER64 ||
           right->vt == VT_INTEGER64 ||
           left->vt == VT_UINTEGER64 ||
           right->vt == VT_UINTEGER64;
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

/*
* Logical OR.
* 
* i || x
*/
ScriptVariant *ScriptVariant_Or( ScriptVariant *svar, ScriptVariant *rightChild )
{
    static ScriptVariant retvar = {{.lVal = 0}, VT_INTEGER};
    retvar.lVal = (ScriptVariant_IsTrue(svar) || ScriptVariant_IsTrue(rightChild));
    return &retvar;
}


/*
* Logical AND.
* 
* i && x
*/
ScriptVariant *ScriptVariant_And( ScriptVariant *svar, ScriptVariant *rightChild )
{
    static ScriptVariant retvar = {{.lVal = 0}, VT_INTEGER};
    retvar.lVal = (ScriptVariant_IsTrue(svar) && ScriptVariant_IsTrue(rightChild));
    return &retvar;
}

/*
* Caskey, Damon V.
* 2026-06-05
*
* Get a 64-bit bit pattern from an integer
* script variant. Decimal operands follow
* legacy integer conversion semantics through
* ScriptVariant_Integer64Value().
*/
static HRESULT ScriptVariant_Bitwise64Value(ScriptVariant *var, uint64_t *pVal) {

    int64_t signed_value;

    if (!var || !pVal) {
        return E_FAIL;
    }

    if (var->vt == VT_UINTEGER64) {
        *pVal = var->ullVal;
        return S_OK;
    }

    if (ScriptVariant_Integer64Value(var, &signed_value) != S_OK) {
        return E_FAIL;
    }

    *pVal = (uint64_t)signed_value;

    return S_OK;
}

/*
* Caskey, Damon V.
* Original author (Utunnels?) and date unknown.
*
* Reworked 2026-06-05 to support 64-bit
* integer carriers without narrowing through
* legacy LONG.
*
* Bitwise OR.
*
* i | x
*/
ScriptVariant *ScriptVariant_Bit_Or(ScriptVariant *svar, ScriptVariant *rightChild) {

    static ScriptVariant retvar = {{.ptrVal = NULL}, VT_EMPTY};

    /*
    * Use 64-bit bitwise behavior when either operand
    * is a 64-bit carrier.
    */
    if (ScriptVariant_Is64BitMath(svar, rightChild)) {

        uint64_t left;
        uint64_t right;
        uint64_t result_bits;

        if (ScriptVariant_Bitwise64Value(svar, &left) == S_OK &&
            ScriptVariant_Bitwise64Value(rightChild, &right) == S_OK) {

            result_bits = left | right;

            if (ScriptVariant_IsUnsignedMath(svar, rightChild)) {
                ScriptVariant_SetUnsignedIntegerResult(&retvar, result_bits, 1);

            } else {
                int64_t signed_result;

                memcpy(&signed_result, &result_bits, sizeof(signed_result));
                ScriptVariant_SetSignedIntegerResult(&retvar, signed_result, 1);
            }

        } else {
            ScriptVariant_Clear(&retvar);
        }

        return &retvar;
    }

    /*
    * Legacy-width bitwise OR. This preserves the old
    * behavior for VT_INTEGER and decimal operands that
    * convert to legacy integer width.
    */
    {
        LONG left;
        LONG right;
        ULONG result_bits;

        if (ScriptVariant_IntegerValue(svar, &left) == S_OK &&
            ScriptVariant_IntegerValue(rightChild, &right) == S_OK) {

            result_bits = ((ULONG)left) | ((ULONG)right);

            ScriptVariant_ChangeType(&retvar, VT_INTEGER);
            retvar.lVal = (LONG)result_bits;

        } else {
            ScriptVariant_Clear(&retvar);
        }
    }

    return &retvar;
}

/*
* Caskey, Damon V.
* Original author (Utunnels?) and date unknown.
*
* Reworked 2026-06-05 to support 64-bit
* integer carriers without narrowing through
* legacy LONG.
*
* Bitwise XOR.
*
* i ^ x
*/
ScriptVariant *ScriptVariant_Xor(ScriptVariant *svar, ScriptVariant *rightChild) {

    static ScriptVariant retvar = {{.ptrVal = NULL}, VT_EMPTY};

    /*
    * Use 64-bit bitwise behavior when either operand
    * is a 64-bit carrier.
    */
    if (ScriptVariant_Is64BitMath(svar, rightChild)) {

        uint64_t left;
        uint64_t right;
        uint64_t result_bits;

        if (ScriptVariant_Bitwise64Value(svar, &left) == S_OK &&
            ScriptVariant_Bitwise64Value(rightChild, &right) == S_OK) {

            result_bits = left ^ right;

            if (ScriptVariant_IsUnsignedMath(svar, rightChild)) {
                ScriptVariant_SetUnsignedIntegerResult(&retvar, result_bits, 1);

            } else {
                int64_t signed_result;

                memcpy(&signed_result, &result_bits, sizeof(signed_result));
                ScriptVariant_SetSignedIntegerResult(&retvar, signed_result, 1);
            }

        } else {
            ScriptVariant_Clear(&retvar);
        }

        return &retvar;
    }

    /*
    * Legacy-width bitwise XOR. This preserves the old
    * behavior for VT_INTEGER and decimal operands that
    * convert to legacy integer width.
    */
    {
        LONG left;
        LONG right;
        ULONG result_bits;

        if (ScriptVariant_IntegerValue(svar, &left) == S_OK &&
            ScriptVariant_IntegerValue(rightChild, &right) == S_OK) {

            result_bits = ((ULONG)left) ^ ((ULONG)right);

            ScriptVariant_ChangeType(&retvar, VT_INTEGER);
            retvar.lVal = (LONG)result_bits;

        } else {
            ScriptVariant_Clear(&retvar);
        }
    }

    return &retvar;
}

/*
* Caskey, Damon V.
* Original author (Utunnels?) and date unknown.
*
* Reworked 2026-06-05 to support 64-bit
* integer carriers without narrowing through
* legacy LONG.
*
* Bitwise AND.
*
* i & x
*/
ScriptVariant *ScriptVariant_Bit_And(ScriptVariant *svar, ScriptVariant *rightChild) {

    static ScriptVariant retvar = {{.ptrVal = NULL}, VT_EMPTY};

    /*
    * Use 64-bit bitwise behavior when either operand
    * is a 64-bit carrier.
    */
    if (ScriptVariant_Is64BitMath(svar, rightChild)) {

        uint64_t left;
        uint64_t right;
        uint64_t result_bits;

        if (ScriptVariant_Bitwise64Value(svar, &left) == S_OK &&
            ScriptVariant_Bitwise64Value(rightChild, &right) == S_OK) {

            result_bits = left & right;

            if (ScriptVariant_IsUnsignedMath(svar, rightChild)) {
                ScriptVariant_SetUnsignedIntegerResult(&retvar, result_bits, 1);

            } else {
                int64_t signed_result;

                memcpy(&signed_result, &result_bits, sizeof(signed_result));
                ScriptVariant_SetSignedIntegerResult(&retvar, signed_result, 1);
            }

        } else {
            ScriptVariant_Clear(&retvar);
        }

        return &retvar;
    }

    /*
    * Legacy-width bitwise AND. This preserves the old
    * behavior for VT_INTEGER and decimal operands that
    * convert to legacy integer width.
    */
    {
        LONG left;
        LONG right;
        ULONG result_bits;

        if (ScriptVariant_IntegerValue(svar, &left) == S_OK &&
            ScriptVariant_IntegerValue(rightChild, &right) == S_OK) {

            result_bits = ((ULONG)left) & ((ULONG)right);

            ScriptVariant_ChangeType(&retvar, VT_INTEGER);
            retvar.lVal = (LONG)result_bits;

        } else {
            ScriptVariant_Clear(&retvar);
        }
    }

    return &retvar;
}

/*
* Caskey, Damon V.
* Original author White Dragon, then fixed by Plombo. Dates unknown.
*
* Reworked 2026-06-05 to apply bitwise NOT
* through unsigned bit patterns for integer
* carriers.
*
* Bitwise NOT.
*
* ~i
*/
void ScriptVariant_Bitwise_Not(ScriptVariant *svar) {

    switch (svar->vt) {
        case VT_INTEGER: {
            ULONG result_bits;

            /*
            * Preserve legacy-width bitwise behavior.
            */
            result_bits = ~((ULONG)svar->lVal);
            svar->lVal = (LONG)result_bits;
            break;
        }

        case VT_INTEGER64: {
            uint64_t result_bits;
            int64_t signed_result;

            result_bits = ~((uint64_t)svar->llVal);
            memcpy(&signed_result, &result_bits, sizeof(signed_result));

            svar->llVal = signed_result;
            break;
        }

        case VT_UINTEGER64:
            svar->ullVal = ~(svar->ullVal);
            break;

        default:
            break;
    }
}

/*
* Caskey, Damon V.
* 2026-06-05
*
* Check if a variant is one of the integer
* carrier types.
*/
static int ScriptVariant_IsIntegerType(ScriptVariant *var) {

    return var->vt == VT_INTEGER ||
           var->vt == VT_INTEGER64 ||
           var->vt == VT_UINTEGER64;
}

/*
* Caskey, Damon V.
* 2026-06-05
*
* Compare two integer script variants without
* converting through DOUBLE.
*
* Sets result to:
*   -1 if left is less than right
*    0 if left equals right
*    1 if left is greater than right
*
* Handles signed and unsigned 64-bit carriers
* without wrapping negative signed values into
* unsigned space.
*/
static HRESULT ScriptVariant_CompareIntegerValues(ScriptVariant *left, ScriptVariant *right, int *result) {

    int64_t signed_left;
    int64_t signed_right;
    uint64_t unsigned_left;
    uint64_t unsigned_right;

    if (!left || !right || !result) {
        return E_FAIL;
    }

    if (!ScriptVariant_IsIntegerType(left) || !ScriptVariant_IsIntegerType(right)) {
        return E_FAIL;
    }

    /*
    * Unsigned against unsigned can compare directly.
    */
    if (left->vt == VT_UINTEGER64 && right->vt == VT_UINTEGER64) {

        if (left->ullVal < right->ullVal) {
            *result = -1;
        
        } else if (left->ullVal > right->ullVal) {
            *result = 1;
        
        } else {
            *result = 0;
        }

        return S_OK;
    }

    /*
    * Unsigned left against signed right.
    */
    if (left->vt == VT_UINTEGER64) {

        if (ScriptVariant_Integer64Value(right, &signed_right) != S_OK) {
            return E_FAIL;
        }

        /*
        * Any unsigned value is greater than a negative
        * signed value.
        */
        if (signed_right < 0) {
            *result = 1;
            return S_OK;
        }

        unsigned_left = left->ullVal;
        unsigned_right = (uint64_t)signed_right;

        if (unsigned_left < unsigned_right) {
            *result = -1;
        
        } else if (unsigned_left > unsigned_right) {
            *result = 1;
        
        } else {
            *result = 0;
        }

        return S_OK;
    }

    /*
    * Signed left against unsigned right.
    */
    if (right->vt == VT_UINTEGER64) {

        if (ScriptVariant_Integer64Value(left, &signed_left) != S_OK) {
            return E_FAIL;
        }

        /*
        * Any negative signed value is less than any
        * unsigned value.
        */
        if (signed_left < 0) {
            *result = -1;
            return S_OK;
        }

        unsigned_left = (uint64_t)signed_left;
        unsigned_right = right->ullVal;

        if (unsigned_left < unsigned_right) {
            *result = -1;
        
        } else if (unsigned_left > unsigned_right) {
            *result = 1;
        
        } else {
            *result = 0;
        }

        return S_OK;
    }

    /*
    * Signed against signed can compare directly.
    */
    if (ScriptVariant_Integer64Value(left, &signed_left) != S_OK ||
        ScriptVariant_Integer64Value(right, &signed_right) != S_OK) {
        return E_FAIL;
    }

    if (signed_left < signed_right) {
        *result = -1;
    
    } else if (signed_left > signed_right) {
        *result = 1;
    
    } else {
        *result = 0;
    }

    return S_OK;
}

/*
* Caskey, Damon V.
* Original author (Utunnels?) and date unknown.
*
* Reworked 2026-06-05 to compare 64-bit
* integer carriers without converting through
* DOUBLE.
*
* Equal.
*
* i == x
*/
ScriptVariant *ScriptVariant_Eq(ScriptVariant *svar, ScriptVariant *rightChild) {

    DOUBLE dbl1;
    DOUBLE dbl2;
    int compare_result;
    static ScriptVariant retvar = {{.lVal = 0}, VT_INTEGER};

    retvar.vt = VT_INTEGER;

    /*
    * Integer equality should remain in integer space
    * so large 64-bit values compare exactly.
    */
    if (ScriptVariant_IsIntegerType(svar) &&
        ScriptVariant_IsIntegerType(rightChild)) {

        if (ScriptVariant_CompareIntegerValues(svar, rightChild, &compare_result) == S_OK) {
            retvar.lVal = (compare_result == 0);
        
        } else {
            retvar.lVal = 0;
        }

        return &retvar;
    }

    /*
    * Decimal comparison keeps legacy behavior when
    * either side is actually decimal.
    */
    if (ScriptVariant_DecimalValue(svar, &dbl1) == S_OK &&
        ScriptVariant_DecimalValue(rightChild, &dbl2) == S_OK) {
        retvar.lVal = (dbl1 == dbl2);
    
    } else if (svar->vt == VT_STR && rightChild->vt == VT_STR) {
        retvar.lVal = !(strcmp(StrCache_Get(svar->strVal), StrCache_Get(rightChild->strVal)));
    
    } else if (svar->vt == VT_PTR && rightChild->vt == VT_PTR) {
        retvar.lVal = (svar->ptrVal == rightChild->ptrVal);
    
    } else if (svar->vt == VT_EMPTY && rightChild->vt == VT_EMPTY) {
        retvar.lVal = 1;
    
    } else {
        retvar.lVal = !(memcmp(svar, rightChild, sizeof(ScriptVariant)));
    }

    return &retvar;
}

/*
* Caskey, Damon V.
* Original author (Utunnels?) and date unknown.
*
* Reworked 2026-06-05 to compare 64-bit
* integer carriers without converting through
* DOUBLE.
*
* Not equal.
*
* i != x
*/
ScriptVariant *ScriptVariant_Ne(ScriptVariant *svar, ScriptVariant *rightChild) {

    DOUBLE dbl1;
    DOUBLE dbl2;
    int compare_result;
    static ScriptVariant retvar = {{.lVal = 0}, VT_INTEGER};

    retvar.vt = VT_INTEGER;

    /*
    * Integer inequality should remain in integer
    * space so large 64-bit values compare exactly.
    */
    if (ScriptVariant_IsIntegerType(svar) &&
        ScriptVariant_IsIntegerType(rightChild)) {

        if (ScriptVariant_CompareIntegerValues(svar, rightChild, &compare_result) == S_OK) {
            retvar.lVal = (compare_result != 0);

        } else {
            retvar.lVal = 1;
        }

        return &retvar;
    }

    /*
    * Decimal comparison keeps legacy behavior when
    * either side is actually decimal.
    */
    if (ScriptVariant_DecimalValue(svar, &dbl1) == S_OK &&
        ScriptVariant_DecimalValue(rightChild, &dbl2) == S_OK) {
        retvar.lVal = (dbl1 != dbl2);

    } else if (svar->vt == VT_STR && rightChild->vt == VT_STR) {
        retvar.lVal = strcmp(StrCache_Get(svar->strVal), StrCache_Get(rightChild->strVal));

    } else if (svar->vt == VT_PTR && rightChild->vt == VT_PTR) {
        retvar.lVal = (svar->ptrVal != rightChild->ptrVal);

    } else if (svar->vt == VT_EMPTY && rightChild->vt == VT_EMPTY) {
        retvar.lVal = 0;

    } else {
        retvar.lVal = (memcmp(svar, rightChild, sizeof(ScriptVariant)) != 0);
    }

    return &retvar;
}

/*
* Caskey, Damon V.
* Original author (Utunnels?) and date unknown.
*
* Reworked 2026-06-05 to compare 64-bit
* integer carriers without converting through
* DOUBLE.
*
* Less than.
*
* i < x
*/
ScriptVariant *ScriptVariant_Lt(ScriptVariant *svar, ScriptVariant *rightChild) {

    DOUBLE dbl1;
    DOUBLE dbl2;
    int compare_result;
    static ScriptVariant retvar = {{.lVal = 0}, VT_INTEGER};

    retvar.vt = VT_INTEGER;

    /*
    * Integer comparison should remain in integer
    * space so large 64-bit values compare exactly.
    */
    if (ScriptVariant_IsIntegerType(svar) &&
        ScriptVariant_IsIntegerType(rightChild)) {

        if (ScriptVariant_CompareIntegerValues(svar, rightChild, &compare_result) == S_OK) {
            retvar.lVal = (compare_result < 0);

        } else {
            retvar.lVal = 0;
        }

        return &retvar;
    }

    /*
    * Decimal comparison keeps legacy behavior when
    * either side is actually decimal.
    */
    if (ScriptVariant_DecimalValue(svar, &dbl1) == S_OK &&
        ScriptVariant_DecimalValue(rightChild, &dbl2) == S_OK) {

        retvar.lVal = (dbl1 < dbl2);

    } else if (svar->vt == VT_STR && rightChild->vt == VT_STR) {
        retvar.lVal = (strcmp(StrCache_Get(svar->strVal), StrCache_Get(rightChild->strVal)) < 0);

    } else if (svar->vt == VT_PTR && rightChild->vt == VT_PTR) {
        retvar.lVal = (svar->ptrVal < rightChild->ptrVal);

    } else if (svar->vt == VT_EMPTY || rightChild->vt == VT_EMPTY) {
        retvar.lVal = 0;

    } else {
        retvar.lVal = (memcmp(svar, rightChild, sizeof(ScriptVariant)) < 0);
    }

    return &retvar;
}


/*
* Caskey, Damon V.
* Original author (Utunnels?) and date unknown.
*
* Reworked 2026-06-05 to compare 64-bit
* integer carriers without converting through
* DOUBLE.
*
* Greater than.
*
* i > x
*/
ScriptVariant *ScriptVariant_Gt(ScriptVariant *svar, ScriptVariant *rightChild) {

    DOUBLE dbl1;
    DOUBLE dbl2;
    int compare_result;
    static ScriptVariant retvar = {{.lVal = 0}, VT_INTEGER};

    retvar.vt = VT_INTEGER;

    /*
    * Integer comparison should remain in integer
    * space so large 64-bit values compare exactly.
    */
    if (ScriptVariant_IsIntegerType(svar) &&
        ScriptVariant_IsIntegerType(rightChild)) {

        if (ScriptVariant_CompareIntegerValues(svar, rightChild, &compare_result) == S_OK) {

            retvar.lVal = (compare_result > 0);

        } else {
            retvar.lVal = 0;
        }

        return &retvar;
    }

    /*
    * Decimal comparison keeps legacy behavior when
    * either side is actually decimal.
    */
    if (ScriptVariant_DecimalValue(svar, &dbl1) == S_OK &&
        ScriptVariant_DecimalValue(rightChild, &dbl2) == S_OK) {
        
        retvar.lVal = (dbl1 > dbl2);

    } else if (svar->vt == VT_STR && rightChild->vt == VT_STR) {
        retvar.lVal = (strcmp(StrCache_Get(svar->strVal), StrCache_Get(rightChild->strVal)) > 0);

    } else if (svar->vt == VT_PTR && rightChild->vt == VT_PTR) {
        retvar.lVal = (svar->ptrVal > rightChild->ptrVal);

    } else if (svar->vt == VT_EMPTY || rightChild->vt == VT_EMPTY) {
        retvar.lVal = 0;

    } else {
        retvar.lVal = (memcmp(svar, rightChild, sizeof(ScriptVariant)) > 0);
    }

    return &retvar;
}


/*
* Caskey, Damon V.
* Original author (Utunnels?) and date unknown.
*
* Reworked 2026-06-05 to compare 64-bit
* integer carriers without converting through
* DOUBLE.
*
* Greater than or equal to.
*
* i >= x
*/
ScriptVariant *ScriptVariant_Ge(ScriptVariant *svar, ScriptVariant *rightChild) {

    DOUBLE dbl1;
    DOUBLE dbl2;
    int compare_result;
    static ScriptVariant retvar = {{.lVal = 0}, VT_INTEGER};

    retvar.vt = VT_INTEGER;

    /*
    * Integer comparison should remain in integer space
    * so large 64-bit values compare exactly.
    */
    if (ScriptVariant_IsIntegerType(svar) &&
        ScriptVariant_IsIntegerType(rightChild)) {

        if (ScriptVariant_CompareIntegerValues(svar, rightChild, &compare_result) == S_OK) {
            retvar.lVal = (compare_result >= 0);

        } else {
            retvar.lVal = 0;
        }

        return &retvar;
    }

    /*
    * Decimal comparison keeps legacy behavior when
    * either side is actually decimal.
    */
    if (ScriptVariant_DecimalValue(svar, &dbl1) == S_OK &&
        ScriptVariant_DecimalValue(rightChild, &dbl2) == S_OK) {
        retvar.lVal = (dbl1 >= dbl2);

    } else if (svar->vt == VT_STR && rightChild->vt == VT_STR) {
        retvar.lVal = (strcmp(StrCache_Get(svar->strVal), StrCache_Get(rightChild->strVal)) >= 0);

    } else if (svar->vt == VT_PTR && rightChild->vt == VT_PTR) {
        retvar.lVal = (svar->ptrVal >= rightChild->ptrVal);

    } else if (svar->vt == VT_EMPTY || rightChild->vt == VT_EMPTY) {
        retvar.lVal = 0;

    } else {
        retvar.lVal = (memcmp(svar, rightChild, sizeof(ScriptVariant)) >= 0);
    }

    return &retvar;
}


/*
* Caskey, Damon V.
* Original author (Utunnels?) and date unknown.
*
* Reworked 2026-06-05 to compare 64-bit
* integer carriers without converting through
* DOUBLE.
*
* Less than or equal to.
*
* i <= x
*/
ScriptVariant *ScriptVariant_Le(ScriptVariant *svar, ScriptVariant *rightChild) {

    DOUBLE dbl1;
    DOUBLE dbl2;
    int compare_result;
    static ScriptVariant retvar = {{.lVal = 0}, VT_INTEGER};

    retvar.vt = VT_INTEGER;

    /*
    * Integer comparison should remain in integer
    * space so large 64-bit values compare exactly.
    */
    if (ScriptVariant_IsIntegerType(svar) &&
        ScriptVariant_IsIntegerType(rightChild)) {

        if (ScriptVariant_CompareIntegerValues(svar, rightChild, &compare_result) == S_OK) {
            retvar.lVal = (compare_result <= 0);

        } else {
            retvar.lVal = 0;
        }

        return &retvar;
    }

    /*
    * Decimal comparison keeps legacy behavior when
    * either side is actually decimal.
    */
    if (ScriptVariant_DecimalValue(svar, &dbl1) == S_OK &&
        ScriptVariant_DecimalValue(rightChild, &dbl2) == S_OK) {
        retvar.lVal = (dbl1 <= dbl2);

    } else if (svar->vt == VT_STR && rightChild->vt == VT_STR) {
        retvar.lVal = (strcmp(StrCache_Get(svar->strVal), StrCache_Get(rightChild->strVal)) <= 0);

    } else if (svar->vt == VT_PTR && rightChild->vt == VT_PTR) {
        retvar.lVal = (svar->ptrVal <= rightChild->ptrVal);

    } else if (svar->vt == VT_EMPTY || rightChild->vt == VT_EMPTY) {
        retvar.lVal = 0;

    } else {
        retvar.lVal = (memcmp(svar, rightChild, sizeof(ScriptVariant)) <= 0);
    }

    return &retvar;
}


/*
* Caskey, Damon V.
* 2026-06-05
*
* Get a non-negative shift count from a variant.
* Decimal operands follow legacy integer conversion
* semantics through ScriptVariant_Integer64Value().
*/
static HRESULT ScriptVariant_ShiftCountValue(ScriptVariant *var, uint64_t *pVal) {

    int64_t signed_value;

    if (!var || !pVal) {
        return E_FAIL;
    }

    if (var->vt == VT_UINTEGER64) {
        *pVal = var->ullVal;
        return S_OK;
    }

    if (ScriptVariant_Integer64Value(var, &signed_value) != S_OK ||
        signed_value < 0) {
        return E_FAIL;
    }

    *pVal = (uint64_t)signed_value;

    return S_OK;
}

/*
* Caskey, Damon V.
* Original author (Utunnels?) and date unknown.
*
* Reworked 2026-06-05 to support 64-bit
* integer carriers and to avoid undefined
* shift counts.
*
* Left bitwise shift.
*
* i << x
*/
ScriptVariant *ScriptVariant_Shl(ScriptVariant *svar, ScriptVariant *rightChild) {

    static ScriptVariant retvar = {{.ptrVal = NULL}, VT_EMPTY};
    uint64_t shift_count;

    if (ScriptVariant_ShiftCountValue(rightChild, &shift_count) != S_OK) {
        ScriptVariant_Clear(&retvar);
        return &retvar;
    }

    /*
    * Unsigned 64-bit left shift.
    */
    if (svar->vt == VT_UINTEGER64) {

        uint64_t result;

        if (shift_count >= 64) {
            ScriptVariant_Clear(&retvar);
            return &retvar;
        }

        result = svar->ullVal << shift_count;

        ScriptVariant_SetUnsignedIntegerResult(&retvar, result, 1);

        return &retvar;
    }

    /*
    * Signed 64-bit left shift.
    *
    * Shift as bits to avoid signed left-shift
    * overflow, then preserve the resulting bit
    * pattern in a signed 64-bit carrier.
    */
    if (svar->vt == VT_INTEGER64) {

        int64_t left;
        int64_t signed_result;
        uint64_t result_bits;

        if (shift_count >= 64 ||
            ScriptVariant_Integer64Value(svar, &left) != S_OK) {
            ScriptVariant_Clear(&retvar);
            return &retvar;
        }

        result_bits = ((uint64_t)left) << shift_count;
        memcpy(&signed_result, &result_bits, sizeof(signed_result));

        ScriptVariant_SetSignedIntegerResult(&retvar, signed_result, 1);

        return &retvar;
    }

    /*
    * Legacy-width left shift. This preserves the old
    * ULONG shift behavior for VT_INTEGER and decimal
    * operands that can convert to legacy integer width.
    */
    {
        LONG left;
        ULONG result;

        if (shift_count >= (uint64_t)(sizeof(ULONG) * CHAR_BIT) ||
            ScriptVariant_IntegerValue(svar, &left) != S_OK) {
            ScriptVariant_Clear(&retvar);
            return &retvar;
        }

        result = ((ULONG)left) << shift_count;

        ScriptVariant_ChangeType(&retvar, VT_INTEGER);
        retvar.lVal = (LONG)result;
    }

    return &retvar;
}


/*
* Caskey, Damon V.
* Original author (Utunnels?) and date unknown.
*
* Reworked 2026-06-05 to support 64-bit
* integer carriers and to avoid undefined
* shift counts.
*
* Right bitwise shift.
*
* i >> x
*/
ScriptVariant *ScriptVariant_Shr(ScriptVariant *svar, ScriptVariant *rightChild) {

    static ScriptVariant retvar = {{.ptrVal = NULL}, VT_EMPTY};
    uint64_t shift_count;

    if (ScriptVariant_ShiftCountValue(rightChild, &shift_count) != S_OK) {
        ScriptVariant_Clear(&retvar);
        return &retvar;
    }

    /*
    * Unsigned 64-bit right shift.
    */
    if (svar->vt == VT_UINTEGER64) {

        uint64_t result;

        if (shift_count >= 64) {
            ScriptVariant_Clear(&retvar);
            return &retvar;
        }

        result = svar->ullVal >> (unsigned int)shift_count;

        ScriptVariant_SetUnsignedIntegerResult(&retvar, result, 1);

        return &retvar;
    }

    /*
    * Signed 64-bit right shift.
    *
    * Shift as bits to preserve the legacy logical
    * right-shift behavior from the old ULONG cast.
    */
    if (svar->vt == VT_INTEGER64) {

        int64_t left;
        int64_t signed_result;
        uint64_t result_bits;

        if (shift_count >= 64 ||
            ScriptVariant_Integer64Value(svar, &left) != S_OK) {
            ScriptVariant_Clear(&retvar);
            return &retvar;
        }

        result_bits = ((uint64_t)left) >> (unsigned int)shift_count;
        memcpy(&signed_result, &result_bits, sizeof(signed_result));

        ScriptVariant_SetSignedIntegerResult(&retvar, signed_result, 1);

        return &retvar;
    }

    /*
    * Legacy-width right shift. This preserves the old
    * ULONG shift behavior for VT_INTEGER and decimal
    * operands that can convert to legacy integer width.
    */
    {
        LONG left;
        ULONG result;

        if (shift_count >= (uint64_t)(sizeof(ULONG) * CHAR_BIT) ||
            ScriptVariant_IntegerValue(svar, &left) != S_OK) {
            ScriptVariant_Clear(&retvar);
            return &retvar;
        }

        result = ((ULONG)left) >> (unsigned int)shift_count;

        ScriptVariant_ChangeType(&retvar, VT_INTEGER);
        retvar.lVal = (LONG)result;
    }

    return &retvar;
}

/*
* Caskey, Damon V.
* 2026-06-04
*
* Adds two script variants.
*
* i + x
*
* String operands concatenate as before. Decimal operands
* use floating-point arithmetic. Integer operands stay in
* integer space so 64-bit carriers do not lose precision
* through DOUBLE conversion.
*/

ScriptVariant *ScriptVariant_Add(ScriptVariant *svar, ScriptVariant *rightChild) {
    
    static ScriptVariant retvar = {{.ptrVal = NULL}, VT_EMPTY};

    /*
    * String concatenation behavior.
    *
    * If either operand is a string, convert both operands
    * to strings and concatenate them into a new cache entry.
    */
    if (svar->vt == VT_STR || rightChild->vt == VT_STR) {

        CHAR *destination_string;
        CHAR conversion_a[SCRIPT_VARIANT_CONVERSION_BUFFER_LENGTH];
        CHAR conversion_b[SCRIPT_VARIANT_CONVERSION_BUFFER_LENGTH];
        ScriptVariantStringView view_a;
        ScriptVariantStringView view_b;

        if(FAILED(ScriptVariant_GetStringView(
                svar,
                conversion_a,
                sizeof(conversion_a),
                &view_a)) ||
           FAILED(ScriptVariant_GetStringView(
                rightChild,
                conversion_b,
                sizeof(conversion_b),
                &view_b)) ||
           view_a.length > MAX_SCRIPT_STRING_LENGTH - view_b.length)
        {
            ScriptVariant_Clear(&retvar);
            return &retvar;
        }

        ScriptVariant_ChangeType(&retvar, VT_STR);

        /*
        * String variants store a string-cache index, not
        * inline text. Reserve a cache entry large enough for
        * both operands and get its writable character buffer.
        */
        retvar.strVal = StrCache_Pop((int)(view_a.length + view_b.length));
        destination_string = StrCache_Get(retvar.strVal);

        /*
        * Fill the cache-owned buffer: left operand first,
        * then right operand at the end of the left text.
        */
        memcpy(destination_string, view_a.string, view_a.length);
        memcpy(destination_string + view_a.length, view_b.string, view_b.length);

        /*
        * Finalize the cache-owned buffer as a C string.
        * The cache releases it later through reference 
        * counting.
        */
        destination_string[view_a.length + view_b.length] = '\0';

        return &retvar;
    }

    /*
    * If either operand is a decimal, use decimal arithmetic.
    */
    if (ScriptVariant_IsDecimalMath(svar, rightChild)) {
        DOUBLE left;
        DOUBLE right;

        if (ScriptVariant_DecimalValue(svar, &left) == S_OK &&
            ScriptVariant_DecimalValue(rightChild, &right) == S_OK) {

            ScriptVariant_ChangeType(&retvar, VT_DECIMAL);
            retvar.dblVal = left + right;
        
        } else {
            ScriptVariant_Clear(&retvar);
        }

        return &retvar;
    }

    /*
    * Unsigned 64-bit arithmetic is used when either
    * operand is an unsigned 64-bit carrier. Legacy
    * integer operands remain in the signed path unless
    * a true unsigned 64-bit value participates.
    */
    if (ScriptVariant_IsUnsignedMath(svar, rightChild)) {

        uint64_t left;
        uint64_t right;
        uint64_t result;

        if (ScriptVariant_Unsigned64Value(svar, &left) == S_OK &&
            ScriptVariant_Unsigned64Value(rightChild, &right) == S_OK &&
            ScriptVariant_AddUnsigned64(left, right, &result)) {

            ScriptVariant_SetUnsignedIntegerResult(&retvar, result, 1);
        
        } else {
            ScriptVariant_Clear(&retvar);
        }

        return &retvar;
    }

    /*
    * Signed integer arithmetic covers legacy integers
    * and signed 64-bit carriers.
    */
    {
        int64_t left;
        int64_t right;
        int64_t result;
        int force64 = ScriptVariant_Is64BitMath(svar, rightChild);

        if (ScriptVariant_Integer64Value(svar, &left) == S_OK &&
            ScriptVariant_Integer64Value(rightChild, &right) == S_OK &&
            ScriptVariant_AddSigned64(left, right, &result)) {

            ScriptVariant_SetSignedIntegerResult(&retvar, result, force64);
        
        } else {
            ScriptVariant_Clear(&retvar);
        }
    }    

    return &retvar;
}

/*
* Caskey, Damon V.
* 2026-06-05
*
* Subtracts two script variants.
*
* i - x
*
* Decimal operands use floating-point arithmetic.
* Integer operands stay in integer space so 64-bit
* carriers do not lose precision through DOUBLE
* conversion.
*/
ScriptVariant *ScriptVariant_Sub(ScriptVariant *svar, ScriptVariant *rightChild) {

    static ScriptVariant retvar = {{.ptrVal = NULL}, VT_EMPTY};

    /*
    * If either operand is a decimal, use decimal arithmetic.
    */
    if (ScriptVariant_IsDecimalMath(svar, rightChild)) {
        DOUBLE left;
        DOUBLE right;

        if (ScriptVariant_DecimalValue(svar, &left) == S_OK &&
            ScriptVariant_DecimalValue(rightChild, &right) == S_OK) {

            ScriptVariant_ChangeType(&retvar, VT_DECIMAL);
            retvar.dblVal = left - right;

        } else {
            ScriptVariant_Clear(&retvar);
        }

        return &retvar;
    }

    /*
    * Unsigned 64-bit arithmetic is used when either
    * operand is an unsigned 64-bit carrier. Subtraction
    * fails if the result would underflow below zero.
    */
    if (ScriptVariant_IsUnsignedMath(svar, rightChild)) {

        uint64_t left;
        uint64_t right;
        uint64_t result;

        if (ScriptVariant_Unsigned64Value(svar, &left) == S_OK &&
            ScriptVariant_Unsigned64Value(rightChild, &right) == S_OK &&
            ScriptVariant_SubUnsigned64(left, right, &result)) {

            ScriptVariant_SetUnsignedIntegerResult(&retvar, result, 1);

        } else {
            ScriptVariant_Clear(&retvar);
        }

        return &retvar;
    }

    /*
    * Signed integer arithmetic covers legacy integers
    * and signed 64-bit carriers. 
    */
    {
        int64_t left;
        int64_t right;
        int64_t result;
        int force64 = ScriptVariant_Is64BitMath(svar, rightChild);

        if (ScriptVariant_Integer64Value(svar, &left) == S_OK &&
            ScriptVariant_Integer64Value(rightChild, &right) == S_OK &&
            ScriptVariant_SubSigned64(left, right, &result)) {

            ScriptVariant_SetSignedIntegerResult(&retvar, result, force64);

        } else {
            ScriptVariant_Clear(&retvar);
        }
    }

    return &retvar;
}

/*
* Caskey, Damon V.
* 2026-06-05
*
* Multiplies two script variants.
*
* Decimal operands use floating-point arithmetic.
* Integer operands stay in integer space so 64-bit
* carriers do not lose precision through DOUBLE
* conversion.
*/
ScriptVariant *ScriptVariant_Mul(ScriptVariant *svar, ScriptVariant *rightChild) {

    static ScriptVariant retvar = {{.ptrVal = NULL}, VT_EMPTY};

    /*
    * If either operand is a decimal, use decimal arithmetic.
    */
    if (ScriptVariant_IsDecimalMath(svar, rightChild)) {
        DOUBLE left;
        DOUBLE right;

        if (ScriptVariant_DecimalValue(svar, &left) == S_OK &&
            ScriptVariant_DecimalValue(rightChild, &right) == S_OK) {

            ScriptVariant_ChangeType(&retvar, VT_DECIMAL);
            retvar.dblVal = left * right;

        } else {
            ScriptVariant_Clear(&retvar);
        }

        return &retvar;
    }

    /*
    * Unsigned 64-bit arithmetic is used when either
    * operand is an unsigned 64-bit carrier. Multiplication
    * fails if the result would overflow uint64_t.
    */
    if (ScriptVariant_IsUnsignedMath(svar, rightChild)) {

        uint64_t left;
        uint64_t right;
        uint64_t result;

        if (ScriptVariant_Unsigned64Value(svar, &left) == S_OK &&
            ScriptVariant_Unsigned64Value(rightChild, &right) == S_OK &&
            ScriptVariant_MulUnsigned64(left, right, &result)) {

            ScriptVariant_SetUnsignedIntegerResult(&retvar, result, 1);

        } else {
            ScriptVariant_Clear(&retvar);
        }

        return &retvar;
    }

    /*
    * Signed integer arithmetic covers legacy integers
    * and signed 64-bit carriers.
    */
    {
        int64_t left;
        int64_t right;
        int64_t result;
        int force64 = ScriptVariant_Is64BitMath(svar, rightChild);

        if (ScriptVariant_Integer64Value(svar, &left) == S_OK &&
            ScriptVariant_Integer64Value(rightChild, &right) == S_OK &&
            ScriptVariant_MulSigned64(left, right, &result)) {

            ScriptVariant_SetSignedIntegerResult(&retvar, result, force64);

        } else {
            ScriptVariant_Clear(&retvar);
        }
    }

    return &retvar;
}

/*
* Caskey, Damon V.
* 2026-06-05
*
* Divides two script variants.
*
* i / x
*
* Decimal operands use floating-point arithmetic.
* Integer operands stay in integer space so 64-bit
* carriers do not lose precision through DOUBLE
* conversion.
*/
ScriptVariant *ScriptVariant_Div(ScriptVariant *svar, ScriptVariant *rightChild) {

    static ScriptVariant retvar = {{.ptrVal = NULL}, VT_EMPTY};

    /*
    * If either operand is a decimal, use decimal arithmetic.
    */
    if (ScriptVariant_IsDecimalMath(svar, rightChild)) {
        DOUBLE left;
        DOUBLE right;

        if (ScriptVariant_DecimalValue(svar, &left) == S_OK &&
            ScriptVariant_DecimalValue(rightChild, &right) == S_OK &&
            right != 0.0) {

            ScriptVariant_ChangeType(&retvar, VT_DECIMAL);
            retvar.dblVal = left / right;

        } else {
            ScriptVariant_Clear(&retvar);
        }

        return &retvar;
    }

    /*
    * Unsigned 64-bit arithmetic is used when either
    * operand is an unsigned 64-bit carrier. Division
    * fails if the divisor is zero.
    */
    if (ScriptVariant_IsUnsignedMath(svar, rightChild)) {

        uint64_t left;
        uint64_t right;
        uint64_t result;

        if (ScriptVariant_Unsigned64Value(svar, &left) == S_OK &&
            ScriptVariant_Unsigned64Value(rightChild, &right) == S_OK &&
            ScriptVariant_DivUnsigned64(left, right, &result)) {

            ScriptVariant_SetUnsignedIntegerResult(&retvar, result, 1);

        } else {
            ScriptVariant_Clear(&retvar);
        }

        return &retvar;
    }

    /*
    * Signed integer arithmetic covers legacy integers
    * and signed 64-bit carriers.
    */
    {
        int64_t left;
        int64_t right;
        int64_t result;
        int force64 = ScriptVariant_Is64BitMath(svar, rightChild);

        if (ScriptVariant_Integer64Value(svar, &left) == S_OK &&
            ScriptVariant_Integer64Value(rightChild, &right) == S_OK &&
            ScriptVariant_DivSigned64(left, right, &result)) {

            ScriptVariant_SetSignedIntegerResult(&retvar, result, force64);

        } else {
            ScriptVariant_Clear(&retvar);
        }
    }

    return &retvar;
}

/*
* Caskey, Damon V.
* 2026-06-05
*
* Performs modulo on two script variants.
*
* i % x
*
* Integer operands stay in integer space so 64-bit
* carriers do not lose precision through DOUBLE
* conversion. Decimal operands follow legacy integer
* conversion semantics.
*/
ScriptVariant *ScriptVariant_Mod(ScriptVariant *svar, ScriptVariant *rightChild) {

    static ScriptVariant retvar = {{.ptrVal = NULL}, VT_EMPTY};

    /*
    * Unsigned 64-bit modulo is used when either
    * operand is an unsigned 64-bit carrier. Modulo
    * fails if the divisor is zero.
    */
    if (ScriptVariant_IsUnsignedMath(svar, rightChild)) {

        uint64_t left;
        uint64_t right;
        uint64_t result;

        if (ScriptVariant_Unsigned64Value(svar, &left) == S_OK &&
            ScriptVariant_Unsigned64Value(rightChild, &right) == S_OK &&
            ScriptVariant_ModUnsigned64(left, right, &result)) {

            ScriptVariant_SetUnsignedIntegerResult(&retvar, result, 1);

        } else {
            ScriptVariant_Clear(&retvar);
        }

        return &retvar;
    }

    /*
    * Signed integer modulo covers legacy integers
    * and signed 64-bit carriers.
    */
    {
        int64_t left;
        int64_t right;
        int64_t result;
        int force64 = ScriptVariant_Is64BitMath(svar, rightChild);

        if (ScriptVariant_Integer64Value(svar, &left) == S_OK &&
            ScriptVariant_Integer64Value(rightChild, &right) == S_OK &&
            ScriptVariant_ModSigned64(left, right, &result)) {

            ScriptVariant_SetSignedIntegerResult(&retvar, result, force64);

        } else {
            ScriptVariant_Clear(&retvar);
        }
    }

    return &retvar;
}

// Unary Operations

/*
* Caskey, Damon V.
* Original author (Utunnels?) and date unknown.
*
* Reworked 2026-06-05 to avoid signed
* overflow when incrementing integer carriers.
*
* Pre-increments a script variant in place.
*
* ++i
*/
void ScriptVariant_Inc_Op(ScriptVariant *svar) {

    switch(svar->vt) {
        case VT_DECIMAL:
            ++(svar->dblVal);
            break;

        case VT_INTEGER: {
            int64_t value;

            value = (int64_t)svar->lVal;

            /*
            * Cannot represent INT64_MAX + 1.
            */
            if (value == INT64_MAX) {
                break;
            }

            value++;

            /*
            * This may promote LONG_MAX to VT_INTEGER64
            * on platforms where LONG is narrower than
            * int64_t.
            */
            ScriptVariant_SetSignedIntegerResult(svar, value, 0);
            break;
        }

        case VT_INTEGER64:
            /*
            * Cannot represent INT64_MAX + 1.
            */
            if (svar->llVal == INT64_MAX) {
                break;
            }

            ScriptVariant_SetSignedIntegerResult(svar, svar->llVal + 1, 1);
            break;

        case VT_UINTEGER64:
            /*
            * Do not wrap UINT64_MAX back to zero.
            */
            if (svar->ullVal == UINT64_MAX) {
                break;
            }

            ScriptVariant_SetUnsignedIntegerResult(svar, svar->ullVal + 1, 1);
            break;

        default:
            break;
    }
}

/*
* Caskey, Damon V.
* Original author (Utunnels?) and date unknown.
*
* Reworked 2026-06-05 to avoid signed
* overflow when post-incrementing integer
* carriers.
*
* Post-increments a script variant in place,
* returning the previous value.
*
* i++
*/
ScriptVariant *ScriptVariant_Inc_Op2(ScriptVariant *svar) {

    static ScriptVariant retvar = {{.ptrVal = NULL}, VT_EMPTY};

    ScriptVariant_Copy(&retvar, svar);

    switch(svar->vt) {
        case VT_DECIMAL:
            svar->dblVal++;
            break;

        case VT_INTEGER: {
            int64_t value;

            value = (int64_t)svar->lVal;

            /*
            * Cannot represent INT64_MAX + 1.
            */
            if (value == INT64_MAX) {
                break;
            }

            value++;

            /*
            * This may promote LONG_MAX to VT_INTEGER64
            * on platforms where LONG is narrower than
            * int64_t.
            */
            ScriptVariant_SetSignedIntegerResult(svar, value, 0);
            break;
        }

        case VT_INTEGER64:
            /*
            * Cannot represent INT64_MAX + 1.
            */
            if (svar->llVal == INT64_MAX) {
                break;
            }

            ScriptVariant_SetSignedIntegerResult(svar, svar->llVal + 1, 1);
            break;

        case VT_UINTEGER64:
            /*
            * Do not wrap UINT64_MAX back to zero.
            */
            if (svar->ullVal == UINT64_MAX) {
                break;
            }

            ScriptVariant_SetUnsignedIntegerResult(svar, svar->ullVal + 1, 1);
            break;

        default:
            ScriptVariant_Clear(&retvar);
            break;
    }

    return &retvar;
}

/*
* Caskey, Damon V.
* Original author (Utunnels?) and date unknown.
*
* Reworked 2026-06-05 to avoid signed
* underflow when decrementing integer carriers.
*
* Pre-decrements a script variant in place.
*
* --i
*/
void ScriptVariant_Dec_Op(ScriptVariant *svar) {

    switch(svar->vt) {
        case VT_DECIMAL:
            --(svar->dblVal);
            break;

        case VT_INTEGER: {
            int64_t value;

            value = (int64_t)svar->lVal;

            /*
            * Cannot represent INT64_MIN - 1.
            */
            if (value == INT64_MIN) {
                break;
            }

            value--;

            /*
            * This keeps the result as VT_INTEGER when
            * it fits legacy width, otherwise promotes.
            */
            ScriptVariant_SetSignedIntegerResult(svar, value, 0);
            break;
        }

        case VT_INTEGER64:
            /*
            * Cannot represent INT64_MIN - 1.
            */
            if (svar->llVal == INT64_MIN) {
                break;
            }

            ScriptVariant_SetSignedIntegerResult(svar, svar->llVal - 1, 1);
            break;

        case VT_UINTEGER64:
            /*
            * Do not wrap 0 down to UINT64_MAX.
            */
            if (svar->ullVal == 0) {
                break;
            }

            ScriptVariant_SetUnsignedIntegerResult(svar, svar->ullVal - 1, 1);
            break;

        default:
            break;
    }
}

/*
* Caskey, Damon V.
* Original author (Utunnels?) and date unknown.
*
* Reworked 2026-06-05 to avoid signed
* underflow when post-decrementing integer
* carriers.
*
* Post-decrements a script variant in place,
* returning the previous value.
*
* i--
*/
ScriptVariant *ScriptVariant_Dec_Op2(ScriptVariant *svar) {

    static ScriptVariant retvar = {{.ptrVal = NULL}, VT_EMPTY};

    ScriptVariant_Copy(&retvar, svar);

    switch(svar->vt) {
        case VT_DECIMAL:
            svar->dblVal--;
            break;

        case VT_INTEGER: {
            int64_t value;

            value = (int64_t)svar->lVal;

            /*
            * Cannot represent INT64_MIN - 1.
            */
            if (value == INT64_MIN) {
                break;
            }

            value--;

            /*
            * This keeps the result as VT_INTEGER when
            * it fits legacy width, otherwise promotes.
            */
            ScriptVariant_SetSignedIntegerResult(svar, value, 0);
            break;
        }

        case VT_INTEGER64:
            /*
            * Cannot represent INT64_MIN - 1.
            */
            if (svar->llVal == INT64_MIN) {
                break;
            }

            ScriptVariant_SetSignedIntegerResult(svar, svar->llVal - 1, 1);
            break;

        case VT_UINTEGER64:
            /*
            * Do not wrap 0 down to UINT64_MAX.
            */
            if (svar->ullVal == 0) {
                break;
            }

            ScriptVariant_SetUnsignedIntegerResult(svar, svar->ullVal - 1, 1);
            break;

        default:
            ScriptVariant_Clear(&retvar);
            break;
    }

    return &retvar;
}

//+i
void ScriptVariant_Pos( ScriptVariant *svar) {
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

/*
* Caskey, Damon V.
* Original author (Utunnels?) and date unknown.
*
* Reworked 2026-06-05 to avoid signed
* overflow when negating integer carriers.
*
* Negates a script variant in place.
*
* -i
*/
void ScriptVariant_Neg(ScriptVariant *svar) {

    switch(svar->vt) {
        case VT_DECIMAL:
            svar->dblVal = -(svar->dblVal);
            break;

        case VT_INTEGER: {
            int64_t value;
            int64_t result;

            value = (int64_t)svar->lVal;

            /*
            * Cannot represent -INT64_MIN.
            */
            if (value == INT64_MIN) {
                break;
            }

            result = -value;

            /*
            * This may promote LONG_MIN to VT_INTEGER64
            * on platforms where LONG is narrower than
            * int64_t.
            */
            ScriptVariant_SetSignedIntegerResult(svar, result, 0);
            break;
        }

        case VT_INTEGER64:
            /*
            * Cannot represent -INT64_MIN.
            */
            if (svar->llVal == INT64_MIN) {
                break;
            }

            ScriptVariant_SetSignedIntegerResult(svar, -(svar->llVal), 1);
            break;

        case VT_UINTEGER64:

            /*
            * Unsigned values cannot be meaningfully negated.
            * Convert to signed only if the positive magnitude
            * fits in int64_t.
            */
            if (svar->ullVal <= (uint64_t)INT64_MAX) {
                uint64_t temp = svar->ullVal;

                ScriptVariant_ChangeType(svar, VT_INTEGER64);
                svar->llVal = -(int64_t)temp;
            }
            break;

        default:
            break;
    }
}

// !i
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
