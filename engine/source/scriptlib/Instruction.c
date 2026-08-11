/*
 * OpenBOR - http://www.chronocrash.com
 * -----------------------------------------------------------------------
 * All rights reserved, see LICENSE in OpenBOR root for details.
 *
 * Copyright (c)  OpenBOR Team
 */

#include "Instruction.h"
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <stdint.h>

void Instruction_InitViaToken(Instruction *pins, OpCode code, Token *pToken )
{
    memset(pins, 0, sizeof(Instruction));
    pins->OpCode = code;
    pins->theToken = malloc(sizeof(Token));
    memset(pins->theToken, 0, sizeof(Token));
    if(pToken)
    {
        *(pins->theToken) = *pToken;

        /*
        * Materialize string constants while their non-owning
        * source view is guaranteed to remain valid.
        */
        if(code == CONSTSTR)
        {
            Instruction_ConvertConstant(pins);
        }
    }
    else
    {
        pins->theToken->theType = END_OF_TOKENS;
    }
}

void Instruction_InitViaLabel(Instruction *pins, OpCode code, LPCSTR label )
{
    memset(pins, 0, sizeof(Instruction));
    pins->OpCode = code;
    pins->theToken = malloc(sizeof(Token));
    memset(pins->theToken, 0, sizeof(Token));
    pins->theToken->theType = END_OF_TOKENS;
    pins->Label = malloc(sizeof(CHAR) * (MAX_STR_LEN + 1));
    strcpy(pins->Label, label);
}

void Instruction_Init(Instruction *pins)
{
    memset(pins, 0, sizeof(Instruction));
    pins->theToken = malloc(sizeof(Token));
    memset(pins->theToken, 0, sizeof(Token));
    pins->theToken->theType = END_OF_TOKENS;
}

void Instruction_Clear(Instruction *pins)
{
    if(pins->theVal)
    {
        ScriptVariant_Clear(pins->theVal);
        free((void *)pins->theVal);
    }
    if(pins->theRefList)
    {
        List_Clear(pins->theRefList);
        free(pins->theRefList);
    }
    if(pins->Label)
    {
        free(pins->Label);
    }
    if(pins->theToken)
    {
        free(pins->theToken);
    }
    memset(pins, 0, sizeof(Instruction));
}

void Instruction_NewData(Instruction *pins)
{
    if(pins->theVal)
    {
        return;
    }
    pins->theVal = (ScriptVariant *)malloc(sizeof(ScriptVariant));
    ScriptVariant_Init( pins->theVal);
}

static int Instruction_IsIntegerSuffixChar(char c)
{
    return c == 'u' || c == 'U' ||
           c == 'l' || c == 'L';
}

static int Instruction_IsUSuffix(char c)
{
    return c == 'u' || c == 'U';
}

static int Instruction_IsLSuffix(char c)
{
    return c == 'l' || c == 'L';
}

/*
* Validate suffixes already captured by the lexer.
*
* Accepted:
*   u, l, ul, lu, ll, ull, llu
*
* Case-insensitive.
*/
static int Instruction_ParseIntegerSuffix(const char *suffix, int *unsigned_suffix, int *long_count) {
    const char *cursor = suffix;

    *unsigned_suffix = 0;
    *long_count = 0;

    if (Instruction_IsUSuffix(*cursor)) {
        *unsigned_suffix = 1;
        cursor++;
    }

    while (*long_count < 2 && Instruction_IsLSuffix(*cursor)) {
        (*long_count)++;
        cursor++;
    }

    if (!*unsigned_suffix && Instruction_IsUSuffix(*cursor)) {
        *unsigned_suffix = 1;
        cursor++;
    }

    return *cursor == '\0';
}

/*
* Caskey, Damon V.
* 2026-06-04 - Original author and date unknown (Plombo?).
*
* Reworked 2026-06-04 to safely handle 
* 64-bit integers and to validate suffixes.
*/
static HRESULT Instruction_ParseIntegerConstant(ScriptVariant *pvar, const char *source, int hex_constant) {
    
    char *work = NULL;
    char *suffix = NULL;
    char *endptr = NULL;
    const char *cursor = source;
    size_t len;
    size_t suffix_index;
    uint64_t magnitude;
    int unsigned_suffix = 0;
    int long_count = 0;
    int negate = 0;
    int logical_not = 0;
    int base = hex_constant ? 16 : 10;

    if (!pvar || !source) {
        return E_FAIL;
    }

    if (*cursor == '!' || *cursor == '-') {
        logical_not = (*cursor == '!');
        negate = (*cursor == '-');
        cursor++;
    }

    len = strlen(cursor);
    if (!len) {
        return E_FAIL;
    }

    work = (char *)malloc(len + 1);
    if (!work) {
        return E_FAIL;
    }

    memcpy(work, cursor, len + 1);

    /*
    * Split numeric body from suffix. The lexer 
    * should only give us a valid prefix, but 
    * this still lets conversion validate what 
    * it received.
    */
    suffix_index = len;

    while (suffix_index > 0 && Instruction_IsIntegerSuffixChar(work[suffix_index - 1])) {
        suffix_index--;
    }

    suffix = work + suffix_index;

    if (!Instruction_ParseIntegerSuffix(suffix, &unsigned_suffix, &long_count)) {
        free(work);
        return E_FAIL;
    }

    work[suffix_index] = '\0';

    errno = 0;
    magnitude = strtoull(work, &endptr, base);

    if (errno == ERANGE || endptr == work || *endptr != '\0') {
        free(work);
        return E_FAIL;
    }

    free(work);

    /*
    * Logical not always produces legacy integer 0/1.
    */
    if (logical_not) {
        ScriptVariant_ChangeType(pvar, VT_INTEGER);
        pvar->lVal = (LONG)(!magnitude);
        return S_OK;
    }

    /*
    * Negative constants are signed. Allow -2147483648 and
    * -9223372036854775808 by permitting one extra magnitude value.
    */
    if (negate) {
        if (magnitude <= ((uint64_t)MAX_INT + 1ULL)) {

            ScriptVariant_ChangeType(pvar, VT_INTEGER);

            if (magnitude == ((uint64_t)MAX_INT + 1ULL)){
                pvar->lVal = (LONG)MIN_INT;
            
            } else {
                pvar->lVal = -(LONG)magnitude;
            }

            return S_OK;
        }

        if (magnitude <= ((uint64_t)INT64_MAX + 1ULL)) {

            ScriptVariant_ChangeType(pvar, VT_INTEGER64);

            if (magnitude == ((uint64_t)INT64_MAX + 1ULL)) {
                pvar->llVal = INT64_MIN;
            
            } else {           
                pvar->llVal = -(int64_t)magnitude;
            }

            return S_OK;
        }

        return E_FAIL;
    }

    /*
    * Positive unsigned path.
    *
    * Keep small U-suffixed values as 
    * VT_INTEGER for legacy friendliness.
    * Promote if the suffix asks for long 
    * long or if the value needs it.
    */

    if (unsigned_suffix) {
        
        if (long_count < 2 && magnitude <= (uint64_t)MAX_INT) {

            ScriptVariant_ChangeType(pvar, VT_INTEGER);
            pvar->lVal = (LONG)magnitude;
            return S_OK;
        }

        ScriptVariant_ChangeType(pvar, VT_UINTEGER64);
        pvar->ullVal = magnitude;
        return S_OK;
    }

    /*
    * Positive signed path.
    */

    if (long_count < 2 && magnitude <= (uint64_t)MAX_INT) {
        ScriptVariant_ChangeType(pvar, VT_INTEGER);
        pvar->lVal = (LONG)magnitude;
        return S_OK;
    }

    if (magnitude <= (uint64_t)INT64_MAX) {
        ScriptVariant_ChangeType(pvar, VT_INTEGER64);
        pvar->llVal = (int64_t)magnitude;
        return S_OK;
    }

    /*
    * Positive literals above INT64_MAX still fit
    * the unsigned 64-bit carrier. Let them land
    * in VT_UINTEGER64 so script literals behave
    * like values produced by integer math, such
    * as pow(2, 63).
    */

    ScriptVariant_ChangeType(pvar, VT_UINTEGER64);
    pvar->ullVal = magnitude;
    return S_OK;
}

/*
* Caskey, Damon V.
* 2026-06-04 - Original author and date unknown (Plombo?).
*
* Convert a hexadecimal integer constant 
* to legacy int width. Accepts optional 
* integer suffixes and tolerates leading 
* '-' or '!', but does not apply unary 
* semantics. Values outside 32 bits return 
* 0.
*/
int htoi(const char *src) {

    const char *cursor;
    char *work = NULL;
    char *suffix = NULL;
    char *endptr = NULL;
    size_t len;
    size_t suffix_index;
    uint64_t value;
    int unsigned_suffix = 0;
    int long_count = 0;

    if (!src) {
        return 0;
    }

    cursor = src;

    /*
    * Legacy htoi() expects a hex source. Let it tolerate the same leading
    * unary characters handled by Instruction_ParseIntegerConstant().
    */
    if (*cursor == '-' || *cursor == '!') {
        cursor++;
    }

    if (cursor[0] != '0' || (cursor[1] != 'x' && cursor[1] != 'X')) {
        return 0;
    }

    cursor += 2;

    len = strlen(cursor);

    if (!len) {
        return 0;
    }

    work = (char *)malloc(len + 1);

    if (!work) {
        return 0;
    }

    memcpy(work, cursor, len + 1);

    suffix_index = len;

    while (suffix_index > 0 && Instruction_IsIntegerSuffixChar(work[suffix_index - 1])) {
        suffix_index--;
    }

    suffix = work + suffix_index;

    if (!Instruction_ParseIntegerSuffix(suffix, &unsigned_suffix, &long_count)) {
        free(work);
        return 0;
    }

    work[suffix_index] = '\0';

    if (!work[0]) {
        free(work);
        return 0;
    }

    errno = 0;
    value = strtoull(work, &endptr, 16);

    if (errno == ERANGE || endptr == work || *endptr != '\0') {
        free(work);
        return 0;
    }

    free(work);

    /*
    * htoi() is legacy int-width. Reject values that 
    * do not fit 32 bits. Values from 0x80000000 through 
    * 0xFFFFFFFF preserve the old bitmask-style
    * behavior on normal two's-complement targets.
    */

    if (value > UINT32_MAX) {
        return 0;
    }

    return (int)(uint32_t)value;
}

/*
* Caskey, Damon V.
* 2026-06-04
*
* Check if the source string for an integer 
* constant looks like hex.
*/
static int Instruction_IsHexIntegerSource(const char *source) {
    
    if (!source) {
        return 0;
    }

    if (*source == '-' || *source == '!') {
        source++;
    }

    return source[0] == '0' && (source[1] == 'x' || source[1] == 'X');
}

//'compile' constant to improve speed
void Instruction_ConvertConstant(Instruction *pins) {
    
    ScriptVariant *pvar;
    CHAR *sc;
    if(pins->theVal)
    {
        return;    //already have the constant as a variant
    }

    if( pins->OpCode == CONSTDBL)
    {
        pvar = (ScriptVariant *)malloc(sizeof(ScriptVariant));
        ScriptVariant_Init(pvar);
        ScriptVariant_ChangeType(pvar, VT_DECIMAL);
        //Note: There shouldn't be any double constants added via a label,
        //      unless you added something I don't know...
        sc = pins->theToken->theSource;
        if(sc[0] == '!' || sc[0] == '-')
        {
            sc++;
        }
        pvar->dblVal = (DOUBLE)atof(sc);
        if(pins->theToken->theSource[0] == '!')
        {
            pvar->dblVal = (DOUBLE)(!pvar->dblVal);
        }
        else if(pins->theToken->theSource[0] == '-')
        {
            pvar->dblVal = -pvar->dblVal;
        }

    } else if(pins->OpCode == CONSTINT || pins->OpCode == CHECKARG) {

        pvar = (ScriptVariant *)malloc(sizeof(ScriptVariant));

        if (!pvar) {
            return;
        }

        ScriptVariant_Init(pvar);

        if (pins->theToken->theType != END_OF_TOKENS)  {
            sc = pins->theToken->theSource;

            if (Instruction_ParseIntegerConstant(
                    pvar,
                    sc,
                    pins->theToken->theType == TOKEN_HEXCONSTANT) != S_OK) {

                printf("Invalid integer constant '%s'.\n", sc);

                ScriptVariant_ChangeType(pvar, VT_INTEGER);
                pvar->lVal = 0;
            }
        
        } else {

            /*
            * This now handles decimal, hex, and optional 
            * leading '-' or '!' uniformly.
            */

            if (Instruction_ParseIntegerConstant(
                    pvar,
                    pins->Label,
                    Instruction_IsHexIntegerSource(pins->Label)) != S_OK) {

                ScriptVariant_ChangeType(pvar, VT_INTEGER);
                pvar->lVal = 0;
            }
        }
    
    } else if(pins->OpCode == CONSTSTR) {
        pvar = (ScriptVariant *)malloc(sizeof(ScriptVariant));
        ScriptVariant_Init(pvar);

        if(pins->theToken->theStringLiteralSource)
        {
            ScriptVariant_ParseStringLiteral(
                pvar,
                pins->theToken->theStringLiteralSource,
                pins->theToken->theStringLiteralLength
            );
        }
        else
        {
            ScriptVariant_ParseStringConstant(pvar, pins->theToken->theSource);
        }
    
    } else {
        return;
    }

    pins->theVal = pvar;
}


void Instruction_ToString(Instruction *pins, LPSTR strRep)
{
    strRep[0] = 0;

    switch( pins->OpCode )
    {
    case CONSTSTR:
        strcpy( strRep, "CONSTSTR " );
        break;
    case CONSTDBL:
        strcpy( strRep, "CONSTDBL " );
        break;
    case CONSTINT:
        strcpy( strRep, "CONSTINT " );
        break;
    case LOAD:
        strcpy( strRep, "LOAD " );
        break;
    case SAVE:
        strcpy( strRep, "SAVE " );
        break;
    case INC:
        strcpy( strRep, "INC " );
        break;
    case DEC:
        strcpy( strRep, "DEC " );
        break;
    case FIELD:
        strcpy( strRep, "FIELD " );
        break;
    case CALL:
        strcpy( strRep, "CALL " );
        break;
    case POS:
        strcpy( strRep, "POS " );
        break;
    case NEG:
        strcpy( strRep, "NEG " );
        break;
    case NOT:
        strcpy( strRep, "NOT " );
        break;
    case MUL:
        strcpy( strRep, "MUL " );
        break;
    case MOD:
        strcpy( strRep, "MOD " );
        break;
    case DIV:
        strcpy( strRep, "DIV " );
        break;
    case ERR:
        strcpy( strRep, "ERR " );
        break;
    case ADD:
        strcpy( strRep, "ADD " );
        break;
    case SUB:
        strcpy( strRep, "SUB " );
        break;
    case SHL:
        strcpy( strRep, "SHL " );
        break;
    case SHR:
        strcpy( strRep, "SHR " );
        break;
    case JUMP:
        strcpy( strRep, "JUMP " );
        break;
    case PJUMP:
        strcpy( strRep, "PJUMP " );
        break;
    case GE:
        strcpy( strRep, "GE " );
        break;
    case LE:
        strcpy( strRep, "LE " );
        break;
    case LT:
        strcpy( strRep, "LT " );
        break;
    case GT:
        strcpy( strRep, "GT " );
        break;
    case EQ:
        strcpy( strRep, "EQ " );
        break;
    case NE:
        strcpy( strRep, "NE " );
        break;
    case OR:
        strcpy( strRep, "OR " );
        break;
    case AND:
        strcpy( strRep, "AND " );
        break;
    case BIT_OR:
        strcpy( strRep, "BIT_OR " );
        break;
    case XOR:
        strcpy( strRep, "XOR " );
        break;
    case BIT_AND:
        strcpy( strRep, "BIT_AND " );
        break;
    case NOOP:
        strcpy( strRep, "NOOP " );
        break;
    case PUSH:
        strcpy( strRep, "PUSH " );
        break;
    case POP:
        strcpy( strRep, "POP " );
        break;
    case Branch_FALSE:
        strcpy( strRep, "Branch_FALSE " );
        break;
    case Branch_TRUE:
        strcpy( strRep, "Branch_TRUE " );
        break;
    case Branch_EQUAL:
        strcpy( strRep, "Branch_EQUAL " );
        break;
    case DATA:
        strcpy( strRep, "DATA " );
        break;
    case PARAM:
        strcpy( strRep, "PARAM " );
        break;
    case IMMEDIATE:
        strcpy( strRep, "IMMEDIATE " );
        break;
    case DEFERRED:
        strcpy( strRep, "DEFERRED " );
        break;
    case RET:
        strcpy( strRep, "RET " );
        break;
    case CHECKARG:
        strcpy( strRep, "CHECKARG " );
        break;
    case CLEAN:
        strcpy( strRep, "CLEAN " );
        break;
    case JUMPR:
        strcpy( strRep, "JUMPR " );
        break;
    case FUNCDECL:
        strcpy( strRep, "FUNCDECL " );
        break;
    default:
        strcpy( strRep, "[unknown] " );
        break;
    }

    //If the label isn't NULL, then copy that into the buffer as well
    if (pins->Label && pins->Label[0])
    {
        strcat( strRep, pins->Label);
    }
    if (pins->theToken && pins->theToken->theType != END_OF_TOKENS)
    {
        if(pins->OpCode == CONSTSTR)
        {
            strcat( strRep, "\"");
        }
        strcat( strRep, pins->theToken->theSource );
        if(pins->OpCode == CONSTSTR)
        {
            strcat( strRep, "\"");
        }
    }

}
