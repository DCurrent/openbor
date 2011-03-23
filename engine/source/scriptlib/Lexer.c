/*
 * OpenBOR - http://www.LavaLit.com
 * -----------------------------------------------------------------------
 * Licensed under the BSD license, see LICENSE in OpenBOR root for details.
 *
 * Copyright (c) 2004 - 2011 OpenBOR Team
 */

#include "Lexer.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

//Constructor
void Token_Init(Token* ptoken, MY_TOKEN_TYPE theType, LPCSTR theSource, TEXTPOS theTextPosition, ULONG charOffset)
{
	ptoken->theType = theType;
	ptoken->theTextPosition = theTextPosition;
	ptoken->charOffset = charOffset;
	strcpy(ptoken->theSource, theSource );
}

//Construct from a pp_token
HRESULT Token_InitFromPreprocessor(Token* ptoken, pp_token* ppToken)
{
	ptoken->theTextPosition = ppToken->theTextPosition;
	ptoken->charOffset = ppToken->charOffset;
	strncpy(ptoken->theSource, ppToken->theSource, MAX_TOKEN_LENGTH);

	switch (ppToken->theType)
	{
		case PP_TOKEN_IDENTIFIER:
		case PP_TOKEN_INCLUDE:
		case PP_TOKEN_IMPORT:
		case PP_TOKEN_DEFINE:
		case PP_TOKEN_UNDEF:
		case PP_TOKEN_PRAGMA:
		case PP_TOKEN_ELIF:
		case PP_TOKEN_IFDEF:
		case PP_TOKEN_IFNDEF:
		case PP_TOKEN_ENDIF:
		case PP_TOKEN_WARNING:
		case PP_TOKEN_ERROR_TEXT:
			ptoken->theType = TOKEN_IDENTIFIER;
			break;
		case PP_TOKEN_HEXCONSTANT: ptoken->theType = TOKEN_HEXCONSTANT; break;
		case PP_TOKEN_INTCONSTANT: ptoken->theType = TOKEN_INTCONSTANT; break;
		case PP_TOKEN_FLOATCONSTANT: ptoken->theType = TOKEN_FLOATCONSTANT; break;
		case PP_TOKEN_STRING_LITERAL:
		{
			char *src, *dest;

			// handle escape sequences and convert to correct format
			ptoken->theType = TOKEN_STRING_LITERAL;
			src = ppToken->theSource + 1; // skip first quote mark
			dest = ptoken->theSource;

			while (*src && *src != '"')
			{
				if (*src == '\\')
				{
					switch (*(++src))
					{
						case 's':
							*dest++ = ' ';
							break;
						case 'r':
							*dest++ = '\r';
							break;
						case 'n':
							*dest++ = '\n';
							break;
						case 't':
							*dest++ = '\t';
							break;
						case '0':
							*dest++ = '\0';
							break;
						case '\"':
							*dest++ = '\"';
							break;
						case '\'':
							*dest++ = '\'';
							break;
						case '\\':
							*dest++ = '\\';
							break;
						default: // invalid escape sequence
							// TODO: emit a warning here
							*dest++ = '\\';
							*dest++ = *src;
					}
				}
				else *dest++ = *src++;
			}
			*dest = '\0';
			break;
		}
		case PP_TOKEN_SIZEOF: ptoken->theType = TOKEN_SIZEOF; break;
		case PP_TOKEN_PTR_OP: ptoken->theType = TOKEN_PTR_OP; break;
		case PP_TOKEN_INC_OP: ptoken->theType = TOKEN_INC_OP; break;
		case PP_TOKEN_DEC_OP: ptoken->theType = TOKEN_DEC_OP; break;
		case PP_TOKEN_LEFT_OP: ptoken->theType = TOKEN_LEFT_OP; break;
		case PP_TOKEN_RIGHT_OP: ptoken->theType = TOKEN_RIGHT_OP; break;
		case PP_TOKEN_CONDITIONAL: ptoken->theType = TOKEN_CONDITIONAL; break;
		case PP_TOKEN_LE_OP: ptoken->theType = TOKEN_LE_OP; break;
		case PP_TOKEN_GE_OP: ptoken->theType = TOKEN_GE_OP; break;
		case PP_TOKEN_EQ_OP: ptoken->theType = TOKEN_EQ_OP; break;
		case PP_TOKEN_NE_OP: ptoken->theType = TOKEN_NE_OP; break;
		case PP_TOKEN_AND_OP: ptoken->theType = TOKEN_AND_OP; break;
		case PP_TOKEN_OR_OP: ptoken->theType = TOKEN_OR_OP; break;
		case PP_TOKEN_MUL_ASSIGN: ptoken->theType = TOKEN_MUL_ASSIGN; break;
		case PP_TOKEN_DIV_ASSIGN: ptoken->theType = TOKEN_DIV_ASSIGN; break;
		case PP_TOKEN_MOD_ASSIGN: ptoken->theType = TOKEN_MOD_ASSIGN; break;
		case PP_TOKEN_ADD_ASSIGN: ptoken->theType = TOKEN_ADD_ASSIGN; break;
		case PP_TOKEN_SUB_ASSIGN: ptoken->theType = TOKEN_SUB_ASSIGN; break;
		case PP_TOKEN_LEFT_ASSIGN: ptoken->theType = TOKEN_LEFT_ASSIGN; break;
		case PP_TOKEN_RIGHT_ASSIGN: ptoken->theType = TOKEN_RIGHT_ASSIGN; break;
		case PP_TOKEN_AND_ASSIGN: ptoken->theType = TOKEN_AND_ASSIGN; break;
		case PP_TOKEN_XOR_ASSIGN: ptoken->theType = TOKEN_XOR_ASSIGN; break;
		case PP_TOKEN_OR_ASSIGN: ptoken->theType = TOKEN_OR_ASSIGN; break;
		case PP_TOKEN_TYPE_NAME: ptoken->theType = TOKEN_TYPE_NAME; break;
		case PP_TOKEN_TYPEDEF: ptoken->theType = TOKEN_TYPEDEF; break;
		case PP_TOKEN_EXTERN: ptoken->theType = TOKEN_EXTERN; break;
		case PP_TOKEN_STATIC: ptoken->theType = TOKEN_STATIC; break;
		case PP_TOKEN_AUTO: ptoken->theType = TOKEN_AUTO; break;
		case PP_TOKEN_REGISTER: ptoken->theType = TOKEN_REGISTER; break;
		case PP_TOKEN_CHAR: ptoken->theType = TOKEN_CHAR; break;
		case PP_TOKEN_SHORT: ptoken->theType = TOKEN_SHORT; break;
		case PP_TOKEN_INT: ptoken->theType = TOKEN_INT; break;
		case PP_TOKEN_LONG: ptoken->theType = TOKEN_LONG; break;
		case PP_TOKEN_SIGNED: ptoken->theType = TOKEN_SIGNED; break;
		case PP_TOKEN_UNSIGNED: ptoken->theType = TOKEN_UNSIGNED; break;
		case PP_TOKEN_FLOAT: ptoken->theType = TOKEN_FLOAT; break;
		case PP_TOKEN_DOUBLE: ptoken->theType = TOKEN_DOUBLE; break;
		case PP_TOKEN_CONST: ptoken->theType = TOKEN_CONST; break;
		case PP_TOKEN_VOLATILE: ptoken->theType = TOKEN_VOLATILE; break;
		case PP_TOKEN_VOID: ptoken->theType = TOKEN_VOID; break;
		case PP_TOKEN_STRUCT: ptoken->theType = TOKEN_STRUCT; break;
		case PP_TOKEN_UNION: ptoken->theType = TOKEN_UNION; break;
		case PP_TOKEN_ENUM: ptoken->theType = TOKEN_ENUM; break;
		case PP_TOKEN_ELLIPSIS: ptoken->theType = TOKEN_ELLIPSIS; break;
		case PP_TOKEN_CASE: ptoken->theType = TOKEN_CASE; break;
		case PP_TOKEN_DEFAULT: ptoken->theType = TOKEN_DEFAULT; break;
		case PP_TOKEN_IF: ptoken->theType = TOKEN_IF; break;
		case PP_TOKEN_ELSE: ptoken->theType = TOKEN_ELSE; break;
		case PP_TOKEN_SWITCH: ptoken->theType = TOKEN_SWITCH; break;
		case PP_TOKEN_WHILE: ptoken->theType = TOKEN_WHILE; break;
		case PP_TOKEN_DO: ptoken->theType = TOKEN_DO; break;
		case PP_TOKEN_FOR: ptoken->theType = TOKEN_FOR; break;
		case PP_TOKEN_GOTO: ptoken->theType = TOKEN_GOTO; break;
		case PP_TOKEN_CONTINUE: ptoken->theType = TOKEN_CONTINUE; break;
		case PP_TOKEN_BREAK: ptoken->theType = TOKEN_BREAK; break;
		case PP_TOKEN_RETURN: ptoken->theType = TOKEN_RETURN; break;
		case PP_TOKEN_SEMICOLON: ptoken->theType = TOKEN_SEMICOLON; break;
		case PP_TOKEN_LCURLY: ptoken->theType = TOKEN_LCURLY; break;
		case PP_TOKEN_RCURLY: ptoken->theType = TOKEN_RCURLY; break;
		case PP_TOKEN_COMMA: ptoken->theType = TOKEN_COMMA; break;
		case PP_TOKEN_COLON: ptoken->theType = TOKEN_COLON; break;
		case PP_TOKEN_ASSIGN: ptoken->theType = TOKEN_ASSIGN; break;
		case PP_TOKEN_LPAREN: ptoken->theType = TOKEN_LPAREN; break;
		case PP_TOKEN_RPAREN: ptoken->theType = TOKEN_RPAREN; break;
		case PP_TOKEN_LBRACKET: ptoken->theType = TOKEN_LBRACKET; break;
		case PP_TOKEN_RBRACKET: ptoken->theType = TOKEN_RBRACKET; break;
		case PP_TOKEN_FIELD: ptoken->theType = TOKEN_FIELD; break;
		case PP_TOKEN_BITWISE_AND: ptoken->theType = TOKEN_BITWISE_AND; break;
		case PP_TOKEN_BOOLEAN_NOT: ptoken->theType = TOKEN_BOOLEAN_NOT; break;
		case PP_TOKEN_BITWISE_NOT: ptoken->theType = TOKEN_BITWISE_NOT; break;
		case PP_TOKEN_ADD: ptoken->theType = TOKEN_ADD; break;
		case PP_TOKEN_SUB: ptoken->theType = TOKEN_SUB; break;
		case PP_TOKEN_MUL: ptoken->theType = TOKEN_MUL; break;
		case PP_TOKEN_DIV: ptoken->theType = TOKEN_DIV; break;
		case PP_TOKEN_MOD: ptoken->theType = TOKEN_MOD; break;
		case PP_TOKEN_LT: ptoken->theType = TOKEN_LT; break;
		case PP_TOKEN_GT: ptoken->theType = TOKEN_GT; break;
		case PP_TOKEN_XOR: ptoken->theType = TOKEN_XOR; break;
		case PP_TOKEN_BITWISE_OR: ptoken->theType = TOKEN_BITWISE_OR; break;
		case PP_TOKEN_COMMENT_SLASH:
		case PP_TOKEN_COMMENT_STAR_BEGIN:
		case PP_TOKEN_COMMENT_STAR_END:
			ptoken->theType = TOKEN_COMMENT; break;
		case PP_TOKEN_EOF: ptoken->theType = TOKEN_EOF; break;
		case PP_TOKEN_ERROR:
		case PP_TOKEN_DIRECTIVE:
		case PP_TOKEN_CONCATENATE:
		case PP_TOKEN_WHITESPACE:
		case PP_TOKEN_NEWLINE:
		case PP_EPSILON:
		case PP_END_OF_TOKENS:
			ptoken->theType = TOKEN_ERROR;
			return E_FAIL;
		default:
			printf("Script error: unknown token type %d\n", ppToken->theType);
			ptoken->theType = TOKEN_ERROR;
			return E_FAIL;
	}

	return S_OK;
}

void Lexer_Init(Lexer* plexer, pp_context* pcontext, LPCSTR thePath, LPSTR theSource, TEXTPOS theStartingPosition)
{
	 plexer->thePath = thePath;
	 plexer->ptheSource = theSource;
	 plexer->theTokenPosition = theStartingPosition;
	 pp_parser_init(&plexer->preprocessor, pcontext, thePath, theSource, theStartingPosition);
}

void Lexer_Clear(Lexer* plexer)
{
	memset(plexer, 0, sizeof(Lexer));
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
HRESULT Lexer_GetNextToken(Lexer* plexer, Token* theNextToken)
{
	pp_token* ppToken;

	// get the next non-whitespace, non-newline token from the preprocessor
	do
	{
		ppToken = pp_parser_emit_token(&plexer->preprocessor);
		if (ppToken == NULL) return E_FAIL;
	} while (ppToken->theType == PP_TOKEN_WHITESPACE || ppToken->theType == PP_TOKEN_NEWLINE);

	plexer->theTokenPosition = plexer->preprocessor.lexer.theTokenPosition;
	plexer->pcurChar = plexer->preprocessor.lexer.pcurChar;

	return Token_InitFromPreprocessor(theNextToken, ppToken);
}

