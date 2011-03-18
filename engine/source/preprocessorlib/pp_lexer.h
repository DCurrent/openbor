/*
 * OpenBOR - http://www.LavaLit.com
 * -----------------------------------------------------------------------
 * Licensed under the BSD license, see LICENSE in OpenBOR root for details.
 *
 * Copyright (c) 2004 - 2011 OpenBOR Team
 */

/**
 * This is derived from Lexer.h in scriptlib, but is modified to be used by the 
 * script preprocessor.  Although the two script lexers share much of their codebase, 
 * there are some significant differences - for example, the preprocessor lexer 
 * can detect preprocessor tokens (#include, #define) and doesn't eat whitespace.
 * 
 * @author Plombo
 * @date 15 October 2010
 */

#ifndef PP_LEXER_H
#define PP_LEXER_H

#include "depends.h"

// define some values for use in CLexer
#define MAX_TOKEN_LENGTH MAX_STR_LEN
#define MAX_LEXER_LENGTH 1024
#define TABSIZE 4

//enumerate the possible token types.  Some tokens here, such as LBRACKET, are
//never used.
typedef enum PP_TOKEN_TYPE {
   PP_TOKEN_IDENTIFIER, PP_TOKEN_HEXCONSTANT, PP_TOKEN_INTCONSTANT, PP_TOKEN_FLOATCONSTANT,
	  PP_TOKEN_STRING_LITERAL, PP_TOKEN_SIZEOF, PP_TOKEN_PTR_OP, PP_TOKEN_INC_OP,
	  PP_TOKEN_DEC_OP, PP_TOKEN_LEFT_OP, PP_TOKEN_RIGHT_OP, PP_TOKEN_CONDITIONAL,
	  PP_TOKEN_LE_OP, PP_TOKEN_GE_OP, PP_TOKEN_EQ_OP, PP_TOKEN_NE_OP, PP_TOKEN_AND_OP,
	  PP_TOKEN_OR_OP, PP_TOKEN_MUL_ASSIGN, PP_TOKEN_DIV_ASSIGN, PP_TOKEN_MOD_ASSIGN,
	  PP_TOKEN_ADD_ASSIGN, PP_TOKEN_SUB_ASSIGN, PP_TOKEN_LEFT_ASSIGN, PP_TOKEN_RIGHT_ASSIGN,
	  PP_TOKEN_AND_ASSIGN, PP_TOKEN_XOR_ASSIGN, PP_TOKEN_OR_ASSIGN, PP_TOKEN_TYPE_NAME,
	  PP_TOKEN_TYPEDEF, PP_TOKEN_EXTERN, PP_TOKEN_STATIC, PP_TOKEN_AUTO, PP_TOKEN_REGISTER,
	  PP_TOKEN_CHAR, PP_TOKEN_SHORT, PP_TOKEN_INT, PP_TOKEN_LONG, PP_TOKEN_SIGNED,
	  PP_TOKEN_UNSIGNED, PP_TOKEN_FLOAT, PP_TOKEN_DOUBLE, PP_TOKEN_CONST, PP_TOKEN_VOLATILE,
	  PP_TOKEN_VOID, PP_TOKEN_STRUCT, PP_TOKEN_UNION, PP_TOKEN_ENUM, PP_TOKEN_ELLIPSIS,
	  PP_TOKEN_CASE, PP_TOKEN_DEFAULT, PP_TOKEN_IF, PP_TOKEN_ELSE, PP_TOKEN_SWITCH,
	  PP_TOKEN_WHILE, PP_TOKEN_DO, PP_TOKEN_FOR, PP_TOKEN_GOTO, PP_TOKEN_CONTINUE,
	  PP_TOKEN_BREAK, PP_TOKEN_RETURN, PP_TOKEN_SEMICOLON, PP_TOKEN_LCURLY, PP_TOKEN_RCURLY,
	  PP_TOKEN_COMMA, PP_TOKEN_COLON, PP_TOKEN_ASSIGN, PP_TOKEN_LPAREN, PP_TOKEN_RPAREN,
	  PP_TOKEN_LBRACKET, PP_TOKEN_RBRACKET, PP_TOKEN_FIELD, PP_TOKEN_BITWISE_AND,
	  PP_TOKEN_BOOLEAN_NOT, PP_TOKEN_BITWISE_NOT, PP_TOKEN_ADD, PP_TOKEN_SUB, PP_TOKEN_MUL,
	  PP_TOKEN_DIV, PP_TOKEN_MOD, PP_TOKEN_LT, PP_TOKEN_GT, PP_TOKEN_XOR, PP_TOKEN_BITWISE_OR,
	  PP_TOKEN_ERROR, PP_TOKEN_COMMENT_SLASH, PP_TOKEN_COMMENT_STAR_BEGIN, 
	  PP_TOKEN_COMMENT_STAR_END, PP_TOKEN_NEWLINE, PP_TOKEN_WHITESPACE, PP_TOKEN_DIRECTIVE,
	  PP_TOKEN_CONCATENATE, PP_TOKEN_INCLUDE, PP_TOKEN_DEFINE, PP_TOKEN_UNDEF, PP_TOKEN_PRAGMA, 
	  PP_TOKEN_ELIF, PP_TOKEN_IFDEF, PP_TOKEN_IFNDEF, PP_TOKEN_ENDIF, PP_TOKEN_IMPORT, 
	  PP_TOKEN_WARNING, PP_TOKEN_ERROR_TEXT, PP_TOKEN_EOF, PP_EPSILON, PP_END_OF_TOKENS
}PP_TOKEN_TYPE;

//define a structure to contain the position of each token
typedef struct TEXTPOS{
   int row;
   int col;
}TEXTPOS;

/******************************************************************************
*  CToken -- This class encapsulates the tokens that CLexer creates.  It serves
*  to encapsulate the information for OOD purposes.
******************************************************************************/
typedef struct pp_token {
   PP_TOKEN_TYPE theType;
   CHAR theSource[MAX_TOKEN_LENGTH+1];
   TEXTPOS theTextPosition;
   ULONG charOffset;
}pp_token;

//Enumerate the comment types for use in CLexer
typedef enum COMMENT_TYPE {
   COMMENT_STAR,
   COMMENT_SLASH
}COMMENT_TYPE;

/******************************************************************************
*  CLexer -- This class is created with a string of unicode characters and a
*  starting position, which it uses to create a series of CTokens based on the
*  script.  Its purpose is to break down the characters into "words" for the
*  parser.
******************************************************************************/
typedef struct pp_lexer {
	LPCSTR ptheSource;
	TEXTPOS theTextPosition;
	ULONG offset;
	ULONG tokOffset;
	CHAR* pcurChar;
	//Character buffer for the tokens
	CHAR theTokenSource [MAX_TOKEN_LENGTH];
	TEXTPOS theTokenPosition;
} pp_lexer;


//Constructor
void pp_token_Init(pp_token* ptoken, PP_TOKEN_TYPE theType, LPCSTR theSource, TEXTPOS theTextPosition, ULONG charOffset);
void pp_lexer_Init(pp_lexer* plexer, LPCSTR theSource, TEXTPOS theStartingPosition);
void pp_lexer_Clear(pp_lexer* plexer);
HRESULT pp_lexer_GetNextToken(pp_lexer* plexer, pp_token* theNextToken);
HRESULT pp_lexer_GetTokenIdentifier(pp_lexer* plexer, pp_token* theNextToken);
HRESULT pp_lexer_GetTokenNumber(pp_lexer* plexer, pp_token* theNextToken);
HRESULT pp_lexer_GetTokenStringLiteral(pp_lexer* plexer, pp_token* theNextToken);
HRESULT pp_lexer_GetTokenSymbol(pp_lexer* plexer, pp_token* theNextToken);
HRESULT pp_lexer_SkipComment(pp_lexer* lexer, COMMENT_TYPE theType);

#endif



