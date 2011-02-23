/*
 * OpenBOR - http://www.LavaLit.com
 * -----------------------------------------------------------------------
 * Licensed under the BSD license, see LICENSE in OpenBOR root for details.
 *
 * Copyright (c) 2004 - 2011 OpenBOR Team
 */

#ifndef PARSER_H
#define PARSER_H
#include "ParserSet.h"
#include "Instruction.h"
#include "Stack.h"
typedef struct Parser{
   //Private data members
    Lexer theLexer;                    //A pointer to this parser's lexer
    ParserSet theParserSet;            //A pointer to this parsers' parserSet
    Token theNextToken;                //A pointer to the next token
    List*  pIList;                      //A pointer to the instruction list
    LONG LabelCount;                   //A counter to track the number of labels
    Stack LabelStack;                  //A stack of labels for use in jumps
    CHAR theRetLabel[MAX_STR_LEN+1];    //A label which holds the target of returns
    Token theFieldToken;               //A pointer to the field source token
    int paramCount;
    char currentPath[256];                 // current path info of the text
    BOOL errorFound;
} Parser;

void Parser_Init(Parser* pparser);
void Parser_Clear(Parser* pparser);
void Parser_ParseText(Parser* pparser, pp_context* pcontext, List* pIList, LPSTR scriptText,
                     ULONG startingLineNumber, LPSTR path );
void Parser_ParseExpression(Parser* pparser, List* pIList, LPSTR scriptText,
                     ULONG startingLineNumber, LPSTR path );
void Parser_AddInstructionViaToken(Parser* pparser, OpCode pCode, Token* pToken, Label label );
void Parser_AddInstructionViaLabel(Parser* pparser, OpCode pCode, Label instrLabel, Label listLabel );
BOOL Parser_Check(Parser* pparser, MY_TOKEN_TYPE theType );
void Parser_Match( Parser* pparser);
Label Parser_CreateLabel( Parser* pparser );
void Parser_Start(Parser* pparser );
void Parser_External_decl(Parser* pparser );
void Parser_External_decl2(Parser* pparser, BOOL variableonly);
void Parser_Decl_spec(Parser* pparser );
void Parser_Decl(Parser* pparser );
void Parser_Decl2(Parser* pparser );
void Parser_FuncDecl(Parser* pparser );
void Parser_FuncDecl1(Parser* pparser );
void Parser_Initializer(Parser* pparser );
void Parser_Parm_decl(Parser* pparser );
void Parser_Param_list(Parser* pparser );
void Parser_Param_list2(Parser* pparser );
void Parser_Decl_list(Parser* pparser );
void Parser_Decl_list2(Parser* pparser );
void Parser_Stmt_list(Parser* pparser );
void Parser_Stmt_list2(Parser* pparser );
void Parser_Stmt( Parser* pparser);
void Parser_Expr_stmt(Parser* pparser );
void Parser_Comp_stmt_Label(Parser* pparser, Label theLabel );
void Parser_Comp_stmt(Parser* pparser );
void Parser_Comp_stmt2(Parser* pparser );
void Parser_Comp_stmt3(Parser* pparser );
void Parser_Select_stmt(Parser* pparser );
void Parser_Opt_else(Parser* pparser );
void Parser_Iter_stmt(Parser* pparser );
void Parser_Opt_expr_stmt(Parser* pparser );
List* Parser_Defer_expr_stmt(Parser* pparser );
void Parser_Jump_stmt(Parser* pparser );
void Parser_Opt_expr(Parser* pparser );
void Parser_Expr(Parser* pparser);
OpCode Parser_Assignment_op(Parser* pparser );
void Parser_Assignment_expr(Parser* pparser );
void Parser_Assignment_expr2(Parser* pparser );
void Parser_Const_expr(Parser* pparser);
void Parser_Log_or_expr(Parser* pparser );
void Parser_Log_or_expr2(Parser* pparser );
void Parser_Log_and_expr(Parser* pparser );
void Parser_Log_and_expr2(Parser* pparser );
OpCode Parser_Eq_op(Parser* pparser );
void Parser_Equal_expr(Parser* pparser );
void Parser_Equal_expr2(Parser* pparser );
OpCode Parser_Rel_op(Parser* pparser );
void Parser_Rel_expr(Parser* pparser );
void Parser_Rel_expr2(Parser* pparser );
OpCode Parser_Add_op(Parser* pparser );
void Parser_Add_expr(Parser* pparser );
void Parser_Add_expr2(Parser* pparser );
OpCode Parser_Mult_op(Parser* pparser );
void Parser_Mult_expr(Parser* pparser );
void Parser_Mult_expr2(Parser* pparser );
void Parser_Unary_expr(Parser* pparser );
void Parser_Postfix_expr(Parser* pparser );
void Parser_Postfix_expr2(Parser* pparser );
void Parser_Arg_expr_list(Parser* pparser );
void Parser_Arg_expr_list2(Parser* pparser, int argCount, int range);
void Parser_Primary_expr(Parser* pparser );
void Parser_Constant(Parser* pparser );
void Parser_Error(Parser* pparser, PRODUCTION offender );

#endif
