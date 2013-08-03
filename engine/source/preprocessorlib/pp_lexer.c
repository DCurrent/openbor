/*
 * OpenBOR - http://www.LavaLit.com
 * -----------------------------------------------------------------------
 * All rights reserved, see LICENSE in OpenBOR root for details.
 *
 * Copyright (c) 2004 - 2011 OpenBOR Team
 */

/**
 * This is derived from Lexer.c in scriptlib, but is modified to be used by the
 * script preprocessor.  Although the two script lexers share much of their codebase,
 * there are some significant differences - for example, the preprocessor lexer
 * can detect preprocessor tokens (#include, #define) and doesn't eat whitespace.
 *
 * @author Plombo (original Lexer.c by utunnels)
 * @date 15 October 2010
 */

#include "pp_lexer.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef PP_TEST
#undef printf
#endif

//MACROS
/******************************************************************************
*  CONSUMECHARACTER -- This macro inserts code to remove a character from the
*  input stream and add it to the current token buffer.
******************************************************************************/
#define CONSUMECHARACTER \
   plexer->theTokenSource[plexer->theTokenLen++] = *(plexer->pcurChar);\
   plexer->theTokenSource[plexer->theTokenLen] = '\0';\
   plexer->pcurChar++; \
   plexer->theTextPosition.col++; \
   plexer->offset++;

/******************************************************************************
*  MAKETOKEN(x) -- This macro inserts code to create a new CToken object of
*  type x, using the current token position, and source.
******************************************************************************/
#define MAKETOKEN(x) \
   pp_token_Init(theNextToken, x, plexer->theTokenSource, plexer->theTokenPosition, \
   plexer->tokOffset);

/******************************************************************************
*  SKIPCHARACTER -- Skip a char without adding it in plexer->theTokenSource
******************************************************************************/
#define SKIPCHARACTER \
   plexer->pcurChar++; \
   plexer->theTextPosition.col++; \
   plexer->offset++;


//Constructor
void pp_token_Init(pp_token *ptoken, PP_TOKEN_TYPE theType, LPCSTR theSource, TEXTPOS theTextPosition, ULONG charOffset)
{
    ptoken->theType = theType;
    ptoken->theTextPosition = theTextPosition;
    ptoken->charOffset = charOffset;
    strcpy(ptoken->theSource, theSource );
}


void pp_lexer_Init(pp_lexer *plexer, LPCSTR theSource, TEXTPOS theStartingPosition)
{
    plexer->ptheSource = theSource;
    plexer->theTextPosition = theStartingPosition;
    plexer->pcurChar = (CHAR *)plexer->ptheSource;
    plexer->offset = 0;
    plexer->tokOffset = 0;
}

void pp_lexer_Clear(pp_lexer *plexer)
{
    memset(plexer, 0, sizeof(pp_lexer));
}

/******************************************************************************
*  getNextToken -- Thie method searches the input stream and returns the next
*  token found within that stream, using the principle of maximal munch.  It
*  embodies the start state of the FSA.
*
*  Parameters: theNextToken -- address of the next CToken found in the stream
*  Returns: S_OK
*           E_FAIL
******************************************************************************/
HRESULT pp_lexer_GetNextToken (pp_lexer *plexer, pp_token *theNextToken)
{
    for(;;)
    {
        plexer->theTokenSource[0] = plexer->theTokenLen = 0;
        plexer->theTokenPosition = plexer->theTextPosition;
        plexer->tokOffset = plexer->offset;

        //Whenever we get a new token, we need to watch out for the end-of-input.
        //Otherwise, we could walk right off the end of the stream.
        if ( !strncmp( plexer->pcurChar, "\0", 1))    //A null character marks the end of the stream
        {
            MAKETOKEN( PP_TOKEN_EOF );
            return S_OK;
        }

        //Windows line break (\r\n)
        else if ( !strncmp( plexer->pcurChar, "\r\n", 2))
        {
            //interpret as a newline
            plexer->theTokenSource[plexer->theTokenLen++] = '\n';
            plexer->theTokenSource[plexer->theTokenLen] = '\0';
            plexer->theTextPosition.col = 0;
            plexer->theTextPosition.row++;
            plexer->pcurChar += 2;
            plexer->offset += 2;
            MAKETOKEN( PP_TOKEN_NEWLINE );
            return S_OK;
        }

        //newline (\n), carriage return (\r), or form feed (\f)
        else if ( !strncmp( plexer->pcurChar, "\n", 1) || !strncmp( plexer->pcurChar, "\r", 1) ||
                  !strncmp( plexer->pcurChar, "\f", 1))
        {
            //interpret as a newline
            plexer->theTokenSource[plexer->theTokenLen++] = '\n';
            plexer->theTokenSource[plexer->theTokenLen] = '\0';
            plexer->theTextPosition.col = 0;
            plexer->theTextPosition.row++;
            plexer->pcurChar++;
            plexer->offset++;
            MAKETOKEN( PP_TOKEN_NEWLINE );
            return S_OK;
        }

        //Backslash-escaped Windows line break (\r\n)
        else if ( !strncmp( plexer->pcurChar, "\\\r\n", 3))
        {
            //interpret as a newline, but not as a PP_TOKEN_NEWLINE
            plexer->theTokenSource[plexer->theTokenLen++] = '\n';
            plexer->theTokenSource[plexer->theTokenLen] = '\0';
            plexer->theTextPosition.col = 0;
            plexer->theTextPosition.row++;
            plexer->pcurChar += 3;
            plexer->offset += 3;
            MAKETOKEN( PP_TOKEN_WHITESPACE );
            return S_OK;
        }

        //Backslash-escaped newline (\n), carriage return (\r), or form feed (\f)
        else if ( !strncmp( plexer->pcurChar, "\\\n", 2) || !strncmp( plexer->pcurChar, "\\\r", 2) ||
                  !strncmp( plexer->pcurChar, "\\\f", 2))
        {
            //interpret as a newline, but not as a PP_TOKEN_NEWLINE
            plexer->theTokenSource[plexer->theTokenLen++] = '\n';
            plexer->theTokenSource[plexer->theTokenLen] = '\0';
            plexer->theTextPosition.col = 0;
            plexer->theTextPosition.row++;
            plexer->pcurChar += 2;
            plexer->offset += 2;
            MAKETOKEN( PP_TOKEN_WHITESPACE );
            return S_OK;
        }

        //tab
        else if ( !strncmp( plexer->pcurChar, "\t", 1))
        {
            //increment the offset counter by TABSIZE
            int numSpaces = TABSIZE - (plexer->theTextPosition.col % TABSIZE);
            strcpy(plexer->theTokenSource, "    ");
            plexer->theTokenSource[numSpaces] = '\0';
            plexer->theTokenLen = numSpaces;
            plexer->theTextPosition.col += numSpaces;
            plexer->pcurChar++;
            plexer->offset++;
            MAKETOKEN( PP_TOKEN_WHITESPACE );
            return S_OK;
        }

        //space
        else if ( !strncmp(plexer->pcurChar, " ", 1))
        {
            //increment the offset counter
            CONSUMECHARACTER;
            MAKETOKEN( PP_TOKEN_WHITESPACE );
            return S_OK;
        }

        //non-breaking space (A0 in Windows-1252 and ISO-8859-* encodings)
        else if ( !strncmp(plexer->pcurChar, "\xa0", 1))
        {
            //increment the offset counter and replace with a normal space
            plexer->theTokenSource[plexer->theTokenLen++] = ' ';
            plexer->theTokenSource[plexer->theTokenLen] = '\0';
            plexer->pcurChar++;
            plexer->offset++;
            plexer->theTextPosition.col++;
            MAKETOKEN( PP_TOKEN_WHITESPACE );
            return S_OK;
        }

        //an Identifier starts with an alphabetical character or underscore
        else if ( *plexer->pcurChar == '_' || (*plexer->pcurChar >= 'a' && *plexer->pcurChar <= 'z') ||
                  (*plexer->pcurChar >= 'A' && *plexer->pcurChar <= 'Z'))
        {
            return pp_lexer_GetTokenIdentifier(plexer, theNextToken );
        }

        //a Number starts with a numerical character
        else if ((*plexer->pcurChar >= '0' && *plexer->pcurChar <= '9') )
        {
            return pp_lexer_GetTokenNumber(plexer, theNextToken );
        }
        //string
        else if (!strncmp( plexer->pcurChar, "\"", 1))
        {
            return pp_lexer_GetTokenStringLiteral(plexer, theNextToken );
        }
        //character
        else if (!strncmp( plexer->pcurChar, "'", 1))
        {
            CONSUMECHARACTER;
            //escape characters
            if (!strncmp( plexer->pcurChar, "\\", 1))
            {
                CONSUMECHARACTER;
                CONSUMECHARACTER;
            }
            //must not be an empty character
            else if(strncmp( plexer->pcurChar, "'", 1))
            {
                CONSUMECHARACTER;
            }
            else
            {
                CONSUMECHARACTER;
                CONSUMECHARACTER;
                MAKETOKEN( PP_TOKEN_ERROR );
                return S_OK;
            }
            if (!strncmp( plexer->pcurChar, "'", 1))
            {
                CONSUMECHARACTER;

                MAKETOKEN( PP_TOKEN_STRING_LITERAL );
                return S_OK;
            }
            else
            {
                CONSUMECHARACTER;
                CONSUMECHARACTER;
                MAKETOKEN( PP_TOKEN_ERROR );
                return S_OK;
            }
        }

        //Before checking for comments
        else if ( !strncmp( plexer->pcurChar, "/", 1))
        {
            CONSUMECHARACTER;
            if ( !strncmp( plexer->pcurChar, "/", 1))
            {
                pp_lexer_SkipComment(plexer, COMMENT_SLASH);
                //CONSUMECHARACTER;
                //MAKETOKEN( PP_TOKEN_COMMENT_SLASH );
                //return S_OK;
            }
            else if ( !strncmp( plexer->pcurChar, "*", 1))
            {
                pp_lexer_SkipComment(plexer, COMMENT_STAR);
                //CONSUMECHARACTER;
                //MAKETOKEN( PP_TOKEN_COMMENT_STAR_BEGIN );
                //return S_OK;
            }

            //Now complete the symbol scan for regular symbols.
            else if ( !strncmp( plexer->pcurChar, "=", 1))
            {
                CONSUMECHARACTER;
                MAKETOKEN( PP_TOKEN_DIV_ASSIGN );
                return S_OK;
            }
            else
            {
                MAKETOKEN( PP_TOKEN_DIV );
                return S_OK;
            }
        }

        //Concatenation operator (inside of #defines)
        else if ( !strncmp( plexer->pcurChar, "##", 2))
        {
            CONSUMECHARACTER;
            CONSUMECHARACTER;
            MAKETOKEN( PP_TOKEN_CONCATENATE );
            return S_OK;
        }

        //Preprocessor directive
        else if ( !strncmp( plexer->pcurChar, "#", 1))
        {
            CONSUMECHARACTER;
            MAKETOKEN( PP_TOKEN_DIRECTIVE );
            return S_OK;
        }

        //a Symbol starts with one of these characters
        else if (( !strncmp( plexer->pcurChar, ">", 1)) || ( !strncmp( plexer->pcurChar, "<", 1))
                 || ( !strncmp( plexer->pcurChar, "+", 1)) || ( !strncmp( plexer->pcurChar, "-", 1))
                 || ( !strncmp( plexer->pcurChar, "*", 1)) || ( !strncmp( plexer->pcurChar, "/", 1))
                 || ( !strncmp( plexer->pcurChar, "%", 1)) || ( !strncmp( plexer->pcurChar, "&", 1))
                 || ( !strncmp( plexer->pcurChar, "^", 1)) || ( !strncmp( plexer->pcurChar, "|", 1))
                 || ( !strncmp( plexer->pcurChar, "=", 1)) || ( !strncmp( plexer->pcurChar, "!", 1))
                 || ( !strncmp( plexer->pcurChar, ";", 1)) || ( !strncmp( plexer->pcurChar, "{", 1))
                 || ( !strncmp( plexer->pcurChar, "}", 1)) || ( !strncmp( plexer->pcurChar, ",", 1))
                 || ( !strncmp( plexer->pcurChar, ":", 1)) || ( !strncmp( plexer->pcurChar, "(", 1))
                 || ( !strncmp( plexer->pcurChar, ")", 1)) || ( !strncmp( plexer->pcurChar, "[", 1))
                 || ( !strncmp( plexer->pcurChar, "]", 1)) || ( !strncmp( plexer->pcurChar, ".", 1))
                 || ( !strncmp( plexer->pcurChar, "~", 1)) || ( !strncmp( plexer->pcurChar, "?", 1)))
        {
            return pp_lexer_GetTokenSymbol(plexer, theNextToken );
        }

        //If we get here, we've hit a character we don't recognize
        else
        {
            //Consume the character
            CONSUMECHARACTER;

            /* Create an "error" token, but continue normally since unrecognized
             * characters are none of the preprocessor's business.  Scriptlib can
             * deal with them if necessary. */
            MAKETOKEN( PP_TOKEN_ERROR );
            //HandleCompileError( *theNextToken, UNRECOGNIZED_CHARACTER );
            return S_OK;
        }
    }
}

/******************************************************************************
*  Identifier -- This method extracts an identifier from the stream, once it's
*  recognized as an identifier.  After it is extracted, this method determines
*  if the identifier is a keyword.
*  Parameters: theNextToken -- address of the next CToken found in the stream
*  Returns: S_OK
*           E_FAIL
******************************************************************************/
HRESULT pp_lexer_GetTokenIdentifier(pp_lexer *plexer, pp_token *theNextToken)
{
    int len = 0;

    //copy the source that makes up this token
    //an identifier is a string of letters, digits and/or underscores
    do
    {
        CONSUMECHARACTER;
        if(++len > MAX_TOKEN_LENGTH)
        {
            printf("Warning: unable to lex token longer than %d characters\n", MAX_TOKEN_LENGTH);
            MAKETOKEN( PP_TOKEN_IDENTIFIER );
            return E_FAIL;
        }
    }
    while ((*plexer->pcurChar >= '0' && *plexer->pcurChar <= '9')  ||
            (*plexer->pcurChar >= 'a' && *plexer->pcurChar <= 'z')  ||
            (*plexer->pcurChar >= 'A' && *plexer->pcurChar <= 'Z') ||
            ( !strncmp( plexer->pcurChar, "_", 1)));

    //Check the Identifier against current keywords
    if (!strcmp( plexer->theTokenSource, "auto"))
    {
        MAKETOKEN( PP_TOKEN_AUTO );
    }
    else if (!strcmp( plexer->theTokenSource, "break"))
    {
        MAKETOKEN( PP_TOKEN_BREAK );
    }
    else if (!strcmp( plexer->theTokenSource, "case"))
    {
        MAKETOKEN( PP_TOKEN_CASE );
    }
    else if (!strcmp( plexer->theTokenSource, "char"))
    {
        MAKETOKEN( PP_TOKEN_CHAR );
    }
    else if (!strcmp( plexer->theTokenSource, "const"))
    {
        MAKETOKEN( PP_TOKEN_CONST );
    }
    else if (!strcmp( plexer->theTokenSource, "continue"))
    {
        MAKETOKEN( PP_TOKEN_CONTINUE );
    }
    else if (!strcmp( plexer->theTokenSource, "default"))
    {
        MAKETOKEN( PP_TOKEN_DEFAULT );
    }
    else if (!strcmp( plexer->theTokenSource, "do"))
    {
        MAKETOKEN( PP_TOKEN_DO );
    }
    else if (!strcmp( plexer->theTokenSource, "double"))
    {
        MAKETOKEN( PP_TOKEN_DOUBLE );
    }
    else if (!strcmp( plexer->theTokenSource, "else"))
    {
        MAKETOKEN( PP_TOKEN_ELSE );
    }
    else if (!strcmp( plexer->theTokenSource, "enum"))
    {
        MAKETOKEN( PP_TOKEN_ENUM );
    }
    else if (!strcmp( plexer->theTokenSource, "extern"))
    {
        MAKETOKEN( PP_TOKEN_EXTERN );
    }
    else if (!strcmp( plexer->theTokenSource, "float"))
    {
        MAKETOKEN( PP_TOKEN_FLOAT );
    }
    else if (!strcmp( plexer->theTokenSource, "for"))
    {
        MAKETOKEN( PP_TOKEN_FOR );
    }
    else if (!strcmp( plexer->theTokenSource, "goto"))
    {
        MAKETOKEN( PP_TOKEN_GOTO );
    }
    else if (!strcmp( plexer->theTokenSource, "if"))
    {
        MAKETOKEN( PP_TOKEN_IF );
    }
    else if (!strcmp( plexer->theTokenSource, "int"))
    {
        MAKETOKEN( PP_TOKEN_INT );
    }
    else if (!strcmp( plexer->theTokenSource, "long"))
    {
        MAKETOKEN( PP_TOKEN_LONG );
    }
    else if (!strcmp( plexer->theTokenSource, "register"))
    {
        MAKETOKEN( PP_TOKEN_REGISTER );
    }
    else if (!strcmp( plexer->theTokenSource, "return"))
    {
        MAKETOKEN( PP_TOKEN_RETURN );
    }
    else if (!strcmp( plexer->theTokenSource, "short"))
    {
        MAKETOKEN( PP_TOKEN_SHORT );
    }
    else if (!strcmp( plexer->theTokenSource, "signed"))
    {
        MAKETOKEN( PP_TOKEN_SIGNED );
    }
    else if (!strcmp( plexer->theTokenSource, "sizeof"))
    {
        MAKETOKEN( PP_TOKEN_SIZEOF );
    }
    else if (!strcmp( plexer->theTokenSource, "static"))
    {
        MAKETOKEN( PP_TOKEN_STATIC );
    }
    else if (!strcmp( plexer->theTokenSource, "struct"))
    {
        MAKETOKEN( PP_TOKEN_STRUCT );
    }
    else if (!strcmp( plexer->theTokenSource, "switch"))
    {
        MAKETOKEN( PP_TOKEN_SWITCH );
    }
    else if (!strcmp( plexer->theTokenSource, "typedef"))
    {
        MAKETOKEN( PP_TOKEN_TYPEDEF );
    }
    else if (!strcmp( plexer->theTokenSource, "union"))
    {
        MAKETOKEN( PP_TOKEN_UNION );
    }
    else if (!strcmp( plexer->theTokenSource, "unsigned"))
    {
        MAKETOKEN( PP_TOKEN_UNSIGNED );
    }
    else if (!strcmp( plexer->theTokenSource, "void"))
    {
        MAKETOKEN( PP_TOKEN_VOID );
    }
    else if (!strcmp( plexer->theTokenSource, "volatile"))
    {
        MAKETOKEN( PP_TOKEN_VOLATILE );
    }
    else if (!strcmp( plexer->theTokenSource, "while"))
    {
        MAKETOKEN( PP_TOKEN_WHILE );
    }
    else if (!strcmp( plexer->theTokenSource, "include"))
    {
        MAKETOKEN( PP_TOKEN_INCLUDE );
    }
    else if (!strcmp( plexer->theTokenSource, "import"))
    {
        MAKETOKEN( PP_TOKEN_IMPORT );
    }
    else if (!strcmp( plexer->theTokenSource, "define"))
    {
        MAKETOKEN( PP_TOKEN_DEFINE );
    }
    else if (!strcmp( plexer->theTokenSource, "undef"))
    {
        MAKETOKEN( PP_TOKEN_UNDEF );
    }
    else if (!strcmp( plexer->theTokenSource, "pragma"))
    {
        MAKETOKEN( PP_TOKEN_PRAGMA );
    }
    else if (!strcmp( plexer->theTokenSource, "ifdef"))
    {
        MAKETOKEN( PP_TOKEN_IFDEF );
    }
    else if (!strcmp( plexer->theTokenSource, "ifndef"))
    {
        MAKETOKEN( PP_TOKEN_IFNDEF );
    }
    else if (!strcmp( plexer->theTokenSource, "elif"))
    {
        MAKETOKEN( PP_TOKEN_ELIF );
    }
    else if (!strcmp( plexer->theTokenSource, "endif"))
    {
        MAKETOKEN( PP_TOKEN_ENDIF );
    }
    else if (!strcmp( plexer->theTokenSource, "defined"))
    {
        MAKETOKEN( PP_TOKEN_DEFINED );
    }
    else if (!strcmp( plexer->theTokenSource, "warning"))
    {
        MAKETOKEN( PP_TOKEN_WARNING );
    }
    else if (!strcmp( plexer->theTokenSource, "error"))
    {
        MAKETOKEN( PP_TOKEN_ERROR_TEXT );
    } // this is completely different from PP_TOKEN_ERROR!
    else
    {
        MAKETOKEN( PP_TOKEN_IDENTIFIER );
    }

    return S_OK;
}

/******************************************************************************
*  Number -- This method extracts a numerical constant from the stream.  It
*  only extracts the digits that make up the number.  No conversion from string
*  to numeral is performed here.
*  Parameters: theNextToken -- address of the next CToken found in the stream
*  Returns: S_OK
*           E_FAIL
******************************************************************************/
HRESULT pp_lexer_GetTokenNumber(pp_lexer *plexer, pp_token *theNextToken)
{
    //copy the source that makes up this token
    //a constant is one of these:

    //0[xX][a-fA-F0-9]+{u|U|l|L}
    //0{D}+{u|U|l|L}
    if (( !strncmp( plexer->pcurChar, "0X", 2)) || ( !strncmp( plexer->pcurChar, "0x", 2)))
    {
        CONSUMECHARACTER;
        CONSUMECHARACTER;
        while ((*plexer->pcurChar >= '0' && *plexer->pcurChar <= '9') ||
                (*plexer->pcurChar >= 'a' && *plexer->pcurChar <= 'f') ||
                (*plexer->pcurChar >= 'A' && *plexer->pcurChar <= 'F'))
        {
            CONSUMECHARACTER;
        }

        if (( !strncmp( plexer->pcurChar, "u", 1)) || ( !strncmp( plexer->pcurChar, "U", 1)) ||
                ( !strncmp( plexer->pcurChar, "l", 1)) || ( !strncmp( plexer->pcurChar, "L", 1)))
        {
            CONSUMECHARACTER;
        }

        MAKETOKEN( PP_TOKEN_HEXCONSTANT );
    }
    else
    {
        while (*plexer->pcurChar >= '0' && *plexer->pcurChar <= '9')
        {
            CONSUMECHARACTER;
        }

        if (( !strncmp( plexer->pcurChar, "E", 1)) || ( !strncmp( plexer->pcurChar, "e", 1)))
        {
            CONSUMECHARACTER;
            while (*plexer->pcurChar >= '0' && *plexer->pcurChar <= '9')
            {
                CONSUMECHARACTER;
            }

            if (( !strncmp( plexer->pcurChar, "f", 1)) || ( !strncmp( plexer->pcurChar, "F", 1)) ||
                    ( !strncmp( plexer->pcurChar, "l", 1)) || ( !strncmp( plexer->pcurChar, "L", 1)))
            {
                CONSUMECHARACTER;
            }

            MAKETOKEN( PP_TOKEN_FLOATCONSTANT );
        }
        else if ( !strncmp( plexer->pcurChar, ".", 1))
        {
            CONSUMECHARACTER;
            while (*plexer->pcurChar >= '0' && *plexer->pcurChar <= '9')
            {
                CONSUMECHARACTER;
            }

            if (( !strncmp( plexer->pcurChar, "E", 1)) || ( !strncmp( plexer->pcurChar, "e", 1)))
            {
                CONSUMECHARACTER;

                while (*plexer->pcurChar >= '0' && *plexer->pcurChar <= '9')
                {
                    CONSUMECHARACTER;
                }

                if (( !strncmp( plexer->pcurChar, "f", 1)) ||
                        ( !strncmp( plexer->pcurChar, "F", 1)) ||
                        ( !strncmp( plexer->pcurChar, "l", 1)) ||
                        ( !strncmp( plexer->pcurChar, "L", 1)))
                {
                    CONSUMECHARACTER;
                }
            }
            MAKETOKEN( PP_TOKEN_FLOATCONSTANT );

        }
        else
        {
            if (( !strncmp( plexer->pcurChar, "u", 1)) || ( !strncmp( plexer->pcurChar, "U", 1)) ||
                    ( !strncmp( plexer->pcurChar, "l", 1)) || ( !strncmp( plexer->pcurChar, "L", 1)))
            {
                CONSUMECHARACTER;
            }
            MAKETOKEN( PP_TOKEN_INTCONSTANT );
        }
    }
    return S_OK;
}

/******************************************************************************
*  StringLiteral -- This method extracts a string literal from the character
*  stream.
*  Parameters: theNextToken -- address of the next CToken found in the stream
*  Returns: S_OK
*           E_FAIL
******************************************************************************/
HRESULT pp_lexer_GetTokenStringLiteral(pp_lexer *plexer, pp_token *theNextToken)
{
    //copy the source that makes up this token
    //an identifier is a string of letters, digits and/or underscores
    //consume that first quote mark
    int esc = 0;
    CONSUMECHARACTER;
    while ( strncmp( plexer->pcurChar, "\"", 1))
    {
        if(!strncmp( plexer->pcurChar, "\\", 1))
        {
            esc = 1;
        }
        CONSUMECHARACTER;
        if(esc)
        {
            CONSUMECHARACTER;
            esc = 0;
        }
    }

    //consume that last quote mark
    CONSUMECHARACTER;

    MAKETOKEN( PP_TOKEN_STRING_LITERAL );
    return S_OK;
}
/******************************************************************************
*  Symbol -- This method extracts a symbol from the character stream.  For the
*  purposes of lexing, comments are considered symbols.
*  Parameters: theNextToken -- address of the next CToken found in the stream
*  Returns: S_OK
*           E_FAIL
******************************************************************************/
HRESULT pp_lexer_GetTokenSymbol(pp_lexer *plexer, pp_token *theNextToken)
{
    //">>="
    if ( !strncmp( plexer->pcurChar, ">>=", 3))
    {
        CONSUMECHARACTER;
        CONSUMECHARACTER;
        CONSUMECHARACTER;
        MAKETOKEN( PP_TOKEN_RIGHT_ASSIGN );
    }

    //">>"
    else if ( !strncmp( plexer->pcurChar, ">>", 2))
    {
        CONSUMECHARACTER;
        CONSUMECHARACTER;
        MAKETOKEN( PP_TOKEN_RIGHT_OP );
    }

    //">="
    else if ( !strncmp( plexer->pcurChar, ">=", 2))
    {
        CONSUMECHARACTER;
        CONSUMECHARACTER;
        MAKETOKEN( PP_TOKEN_GE_OP );
    }

    //">"
    else if ( !strncmp( plexer->pcurChar, ">", 1))
    {
        CONSUMECHARACTER;
        MAKETOKEN( PP_TOKEN_GT );
    }

    //"<<="
    else if ( !strncmp( plexer->pcurChar, "<<=", 3))
    {
        CONSUMECHARACTER;
        CONSUMECHARACTER;
        CONSUMECHARACTER;
        MAKETOKEN( PP_TOKEN_LEFT_ASSIGN );
    }

    //"<<"
    else if ( !strncmp( plexer->pcurChar, "<<", 2))
    {
        CONSUMECHARACTER;
        CONSUMECHARACTER;
        MAKETOKEN( PP_TOKEN_LEFT_OP );
    }

    //"<="
    else if ( !strncmp( plexer->pcurChar, "<=", 2))
    {
        CONSUMECHARACTER;
        CONSUMECHARACTER;
        MAKETOKEN( PP_TOKEN_LE_OP );
    }

    //"<"
    else if ( !strncmp( plexer->pcurChar, "<", 1))
    {
        CONSUMECHARACTER;
        MAKETOKEN( PP_TOKEN_LT );
    }

    //"++"
    else if ( !strncmp( plexer->pcurChar, "++", 2))
    {
        CONSUMECHARACTER;
        CONSUMECHARACTER;
        MAKETOKEN( PP_TOKEN_INC_OP );
    }

    //"+="
    else if ( !strncmp( plexer->pcurChar, "+=", 2))
    {
        CONSUMECHARACTER;
        CONSUMECHARACTER;
        MAKETOKEN( PP_TOKEN_ADD_ASSIGN );
    }

    //"+"
    else if ( !strncmp( plexer->pcurChar, "+", 1))
    {
        CONSUMECHARACTER;
        MAKETOKEN( PP_TOKEN_ADD );
    }

    //"--"
    else if ( !strncmp( plexer->pcurChar, "--", 2))
    {
        CONSUMECHARACTER;
        CONSUMECHARACTER;
        MAKETOKEN( PP_TOKEN_DEC_OP );
    }

    //"-="
    else if ( !strncmp( plexer->pcurChar, "-=", 2))
    {
        CONSUMECHARACTER;
        CONSUMECHARACTER;
        MAKETOKEN( PP_TOKEN_SUB_ASSIGN );
    }

    //"-"
    else if ( !strncmp( plexer->pcurChar, "-", 1))
    {
        CONSUMECHARACTER;
        MAKETOKEN( PP_TOKEN_SUB );
    }

    //"*="
    else if ( !strncmp( plexer->pcurChar, "*=", 2))
    {
        CONSUMECHARACTER;
        CONSUMECHARACTER;
        MAKETOKEN( PP_TOKEN_MUL_ASSIGN );
    }

    //"*"
    else if ( !strncmp( plexer->pcurChar, "*", 1))
    {
        CONSUMECHARACTER;
        MAKETOKEN( PP_TOKEN_MUL );
    }

    //"%="
    else if ( !strncmp( plexer->pcurChar, "%=", 2))
    {
        CONSUMECHARACTER;
        CONSUMECHARACTER;
        MAKETOKEN( PP_TOKEN_MOD_ASSIGN );
    }

    //"%"
    else if ( !strncmp( plexer->pcurChar, "%", 1))
    {
        CONSUMECHARACTER;
        MAKETOKEN( PP_TOKEN_MOD );
    }

    //"&&"
    else if ( !strncmp( plexer->pcurChar, "&&", 2))
    {
        CONSUMECHARACTER;
        CONSUMECHARACTER;
        MAKETOKEN( PP_TOKEN_AND_OP );
    }

    //"&="
    else if ( !strncmp( plexer->pcurChar, "&=", 2))
    {
        CONSUMECHARACTER;
        CONSUMECHARACTER;
        MAKETOKEN( PP_TOKEN_AND_ASSIGN );
    }

    //"&"
    else if ( !strncmp( plexer->pcurChar, "&", 1))
    {
        CONSUMECHARACTER;
        MAKETOKEN( PP_TOKEN_BITWISE_AND );
    }

    //"^="
    else if ( !strncmp( plexer->pcurChar, "^=", 2))
    {
        CONSUMECHARACTER;
        CONSUMECHARACTER;
        MAKETOKEN( PP_TOKEN_XOR_ASSIGN );
    }

    //"^"
    else if ( !strncmp( plexer->pcurChar, "^", 1))
    {
        CONSUMECHARACTER;
        MAKETOKEN( PP_TOKEN_XOR );
    }

    //"||"
    else if ( !strncmp( plexer->pcurChar, "||", 2))
    {
        CONSUMECHARACTER;
        CONSUMECHARACTER;
        MAKETOKEN( PP_TOKEN_OR_OP );
    }

    //"|="
    else if ( !strncmp( plexer->pcurChar, "|=", 2))
    {
        CONSUMECHARACTER;
        CONSUMECHARACTER;
        MAKETOKEN( PP_TOKEN_OR_ASSIGN );
    }

    //"|"
    else if ( !strncmp( plexer->pcurChar, "|", 1))
    {
        CONSUMECHARACTER;
        MAKETOKEN( PP_TOKEN_BITWISE_OR );
    }

    //"=="
    else if ( !strncmp( plexer->pcurChar, "==", 2))

    {
        CONSUMECHARACTER;
        CONSUMECHARACTER;
        MAKETOKEN( PP_TOKEN_EQ_OP );
    }

    //"="
    else if ( !strncmp( plexer->pcurChar, "=", 1))
    {
        CONSUMECHARACTER;
        MAKETOKEN( PP_TOKEN_ASSIGN );
    }

    //"!="
    else if ( !strncmp( plexer->pcurChar, "!=", 2))
    {
        CONSUMECHARACTER;
        CONSUMECHARACTER;
        MAKETOKEN( PP_TOKEN_NE_OP );
    }

    //"!"
    else if ( !strncmp( plexer->pcurChar, "!", 1))
    {
        CONSUMECHARACTER;
        MAKETOKEN( PP_TOKEN_BOOLEAN_NOT );
    }

    //";"
    else if ( !strncmp( plexer->pcurChar, ";", 1))
    {
        CONSUMECHARACTER;
        MAKETOKEN( PP_TOKEN_SEMICOLON);
    }

    //"{"
    else if ( !strncmp( plexer->pcurChar, "{", 1))
    {
        CONSUMECHARACTER;
        MAKETOKEN( PP_TOKEN_LCURLY);
    }

    //"}"
    else if ( !strncmp( plexer->pcurChar, "}", 1))
    {
        CONSUMECHARACTER;
        MAKETOKEN( PP_TOKEN_RCURLY);
    }

    //","
    else if ( !strncmp( plexer->pcurChar, ",", 1))
    {
        CONSUMECHARACTER;
        MAKETOKEN( PP_TOKEN_COMMA);
    }

    //":"
    else if ( !strncmp( plexer->pcurChar, ":", 1))
    {
        CONSUMECHARACTER;
        MAKETOKEN( PP_TOKEN_COLON);
    }

    //"("
    else if ( !strncmp( plexer->pcurChar, "(", 1))
    {
        CONSUMECHARACTER;
        MAKETOKEN( PP_TOKEN_LPAREN);
    }

    //")"
    else if ( !strncmp( plexer->pcurChar, ")", 1))
    {
        CONSUMECHARACTER;
        MAKETOKEN( PP_TOKEN_RPAREN);
    }

    //"["
    else if ( !strncmp( plexer->pcurChar, "[", 1))
    {
        CONSUMECHARACTER;
        MAKETOKEN( PP_TOKEN_LBRACKET);
    }

    //"]"
    else if ( !strncmp( plexer->pcurChar, "]", 1))
    {
        CONSUMECHARACTER;
        MAKETOKEN( PP_TOKEN_RBRACKET);
    }

    //"."
    else if ( !strncmp( plexer->pcurChar, ".", 1))
    {
        CONSUMECHARACTER;
        MAKETOKEN( PP_TOKEN_FIELD);
    }

    //"~"
    else if ( !strncmp( plexer->pcurChar, "~", 1))
    {
        CONSUMECHARACTER;
        MAKETOKEN( PP_TOKEN_BITWISE_NOT);
    }

    //"?"
    else if ( !strncmp( plexer->pcurChar, "?", 1))
    {
        CONSUMECHARACTER;
        MAKETOKEN( PP_TOKEN_CONDITIONAL);
    }

    return S_OK;
}

/******************************************************************************
*  Comment -- This method extracts a symbol from the character stream.
*  Parameters: theNextToken -- address of the next CToken found in the stream
*  Returns: S_OK
*           E_FAIL
******************************************************************************/
HRESULT pp_lexer_SkipComment(pp_lexer *plexer, COMMENT_TYPE theType)
{

    if (theType == COMMENT_SLASH)
    {
        do
        {
            SKIPCHARACTER;
            //keep going if we hit a backslash-escaped line break
            if (!strncmp( plexer->pcurChar, "\\\r\n", 3))
            {
                SKIPCHARACTER;
                SKIPCHARACTER;
                plexer->theTextPosition.col = 0;
                plexer->theTextPosition.row++;
                continue;
            }
            if (!strncmp( plexer->pcurChar, "\\\n", 2) ||
                    !strncmp( plexer->pcurChar, "\\\r", 2) ||
                    !strncmp( plexer->pcurChar, "\\\f", 2))
            {
                SKIPCHARACTER;
                plexer->theTextPosition.col = 0;
                plexer->theTextPosition.row++;
                continue;
            }
            //break out if we hit a new line
            if (!strncmp( plexer->pcurChar, "\r\n", 2) ||
                    !strncmp( plexer->pcurChar, "\n", 1) ||
                    !strncmp( plexer->pcurChar, "\r", 1) ||
                    !strncmp( plexer->pcurChar, "\f", 1))
            {
                break;
            }
        }
        while (strncmp( plexer->pcurChar, "\0", 1));
    }
    else if (theType == COMMENT_STAR)
    {
        //consume the '*' that gets this comment started
        SKIPCHARACTER;

        //loop through the characters till we hit '*/'
        while (strncmp( plexer->pcurChar, "\0", 1))
        {
            if (0 == strncmp( plexer->pcurChar, "*/", 2))
            {
                SKIPCHARACTER;
                SKIPCHARACTER;
                break;
            }
            else if (!strncmp( plexer->pcurChar, "\r\n", 2))
            {
                SKIPCHARACTER;
                plexer->theTextPosition.col = 0;
                plexer->theTextPosition.row++;
            }
            else if (!strncmp( plexer->pcurChar, "\n", 1))
            {
                plexer->theTextPosition.col = 0;
                plexer->theTextPosition.row++;
            }
            else if (!strncmp( plexer->pcurChar, "\r", 1))
            {
                plexer->theTextPosition.col = 0;
                plexer->theTextPosition.row++;
            }
            else if (!strncmp( plexer->pcurChar, "\f", 1))
            {
                plexer->theTextPosition.col = 0;
                plexer->theTextPosition.row++;
            }
            SKIPCHARACTER;
        };
    }

    return S_OK;
}

