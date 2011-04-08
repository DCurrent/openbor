/*
 * OpenBOR - http://www.LavaLit.com
 * -----------------------------------------------------------------------
 * All rights reserved, see LICENSE in OpenBOR root for details.
 *
 * Copyright (c) 2004 - 2011 OpenBOR Team
 */

#ifndef LEXER_H
#define LEXER_H

#include "depends.h"
#include "pp_parser.h"

//enumerate the possible token types.  Some tokens here, such as LBRACKET, are
//never used.
typedef enum MY_TOKEN_TYPE {
   TOKEN_IDENTIFIER, TOKEN_HEXCONSTANT, TOKEN_INTCONSTANT, TOKEN_FLOATCONSTANT,
	  TOKEN_STRING_LITERAL, TOKEN_SIZEOF, TOKEN_PTR_OP, TOKEN_INC_OP,
	  TOKEN_DEC_OP, TOKEN_LEFT_OP, TOKEN_RIGHT_OP, TOKEN_CONDITIONAL,
	  TOKEN_LE_OP, TOKEN_GE_OP, TOKEN_EQ_OP, TOKEN_NE_OP, TOKEN_AND_OP,
	  TOKEN_OR_OP, TOKEN_MUL_ASSIGN, TOKEN_DIV_ASSIGN, TOKEN_MOD_ASSIGN,
	  TOKEN_ADD_ASSIGN, TOKEN_SUB_ASSIGN, TOKEN_LEFT_ASSIGN, TOKEN_RIGHT_ASSIGN,
	  TOKEN_AND_ASSIGN, TOKEN_XOR_ASSIGN, TOKEN_OR_ASSIGN, TOKEN_TYPE_NAME,
	  TOKEN_TYPEDEF, TOKEN_EXTERN, TOKEN_STATIC, TOKEN_AUTO, TOKEN_REGISTER,
	  TOKEN_CHAR, TOKEN_SHORT, TOKEN_INT, TOKEN_LONG, TOKEN_SIGNED,
	  TOKEN_UNSIGNED, TOKEN_FLOAT, TOKEN_DOUBLE, TOKEN_CONST, TOKEN_VOLATILE,
	  TOKEN_VOID, TOKEN_STRUCT, TOKEN_UNION, TOKEN_ENUM, TOKEN_ELLIPSIS,
	  TOKEN_CASE, TOKEN_DEFAULT, TOKEN_IF, TOKEN_ELSE, TOKEN_SWITCH,
	  TOKEN_WHILE, TOKEN_DO, TOKEN_FOR, TOKEN_GOTO, TOKEN_CONTINUE,
	  TOKEN_BREAK, TOKEN_RETURN, TOKEN_SEMICOLON, TOKEN_LCURLY, TOKEN_RCURLY,
	  TOKEN_COMMA, TOKEN_COLON, TOKEN_ASSIGN, TOKEN_LPAREN, TOKEN_RPAREN,
	  TOKEN_LBRACKET, TOKEN_RBRACKET, TOKEN_FIELD, TOKEN_BITWISE_AND,
	  TOKEN_BOOLEAN_NOT, TOKEN_BITWISE_NOT, TOKEN_ADD, TOKEN_SUB, TOKEN_MUL,
	  TOKEN_DIV, TOKEN_MOD, TOKEN_LT, TOKEN_GT, TOKEN_XOR, TOKEN_BITWISE_OR,
	  TOKEN_ERROR, TOKEN_COMMENT, TOKEN_EOF, EPSILON, END_OF_TOKENS
}MY_TOKEN_TYPE;

/******************************************************************************
*  CToken -- This class encapsulates the tokens that CLexer creates.  It serves
*  to encapsulate the information for OOD purposes.
******************************************************************************/
typedef struct Token {
   MY_TOKEN_TYPE theType;
   CHAR theSource[MAX_TOKEN_LENGTH+1];
   TEXTPOS theTextPosition;
   ULONG charOffset;
}Token;

/******************************************************************************
*  CLexer -- This class is created with a string of unicode characters and a
*  starting position, which it uses to create a series of CTokens based on the
*  script.  Its purpose is to break down the characters into "words" for the
*  parser.
******************************************************************************/
typedef struct Lexer {
	LPCSTR thePath;
	LPCSTR ptheSource;
	pp_parser preprocessor;
	CHAR* pcurChar;
	TEXTPOS theTokenPosition;
} Lexer;


//Constructor
void Token_Init(Token* ptoken, MY_TOKEN_TYPE theType, LPCSTR theSource, TEXTPOS theTextPosition, ULONG charOffset);
void Lexer_Init(Lexer* plexer, pp_context* pcontext, LPCSTR thePath, LPSTR theSource, TEXTPOS theStartingPosition);
void Lexer_Clear(Lexer* plexer);
HRESULT Lexer_GetNextToken(Lexer* plexer, Token* theNextToken);
HRESULT Lexer_GetTokenIdentifier(Lexer* plexer, Token* theNextToken);
HRESULT Lexer_GetTokenNumber(Lexer* plexer, Token* theNextToken);
HRESULT Lexer_GetTokenStringLiteral(Lexer* plexer, Token* theNextToken);
HRESULT Lexer_GetTokenSymbol(Lexer* plexer, Token* theNextToken);
HRESULT Lexer_SkipComment(Lexer* lexer, COMMENT_TYPE theType);

#endif



