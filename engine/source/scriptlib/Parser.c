/*
 * OpenBOR - http://www.LavaLit.com
 * -----------------------------------------------------------------------
 * Licensed under the BSD license, see LICENSE in OpenBOR root for details.
 *
 * Copyright (c) 2004 - 2011 OpenBOR Team
 */

#include "Parser.h"
#include "tracemalloc.h"

Parser* pcurParser = NULL;

void Parser_Init(Parser* pparser)
{
    memset(pparser, 0, sizeof(Parser));
    ParserSet_Buildup(&(pparser->theParserSet));
    pparser->LabelCount = 0;
    pparser->theFieldToken.theType = END_OF_TOKENS;
    pparser->theNextToken.theType = END_OF_TOKENS;
}

void Parser_Clear(Parser* pparser){
    Label label;
    Lexer_Clear(&(pparser->theLexer));
    ParserSet_Clear(&(pparser->theParserSet));
    while(!Stack_IsEmpty(&(pparser->LabelStack)))
    {
        label = (Label)Stack_Top(&(pparser->LabelStack));
        tracefree((void*)label);
        Stack_Pop(&(pparser->LabelStack));
    }
    List_Clear(&(pparser->LabelStack));
}

/******************************************************************************
*  ParseText -- This method begins the recursive-descent process for each global
*  variable and function definition.
*  Parameters: pcontext -- Preprocessor context for the script to be parsed.
*              pIList -- pointer to a TList<CInstruction> which takes the
*                        stream of CInstructions representing the parsed text
*              scriptText -- LPCSTR which contains ths script to be parsed.
*              startingLineNumber -- Line on which this script starts.  The
*                        lexer needs this information to maintain accurate
*                        line counts.
*  Returns:
******************************************************************************/
void Parser_ParseText(Parser* pparser, pp_context* pcontext, List* pIList, LPSTR scriptText,
                     ULONG startingLineNumber, LPSTR path )
{
   //Create a new CLexer for this script text.
   TEXTPOS thePosition;
   thePosition.row = startingLineNumber;
   thePosition.col = 1;
   pcurParser = pparser;
   if(path)
   {
       strncpy(pparser->currentPath, path, 255);
   }
   else pparser->currentPath[0] = 0;
   pparser->errorFound = FALSE;
   Lexer_Init(&(pparser->theLexer), pcontext, path, scriptText, thePosition );

   //Get the first token from the CLexer.
   Lexer_GetNextToken(&(pparser->theLexer), &(pparser->theNextToken));

   //Setup the instruction list so we can add instructions.
    pparser->pIList = pIList;

   //Make sure we're pointing at the end of that list
   List_GotoLast(pparser->pIList);

   //Parse the script text until you reach the end of the file, or until
   //an error occurs.
   while( pparser->theNextToken.theType != TOKEN_EOF ){
      Parser_External_decl(pparser );
   }
}



/******************************************************************************
*  ParseExpression -- This method begins the recursive-descent process for each
*  global variable and function definition.
*  Parameters: pIList -- pointer to a TList<CInstruction> which takes the
*                        stream of CInstructions representing the parsed text
*              scriptText -- LPCSTR which contains ths script to be parsed.
*              startingLineNumber -- Line on which this script starts.  The
*                        lexer needs this information to maintain accurate
*                        line counts.
*  Returns:
******************************************************************************/
void Parser_ParseExpression(Parser* pparser, List* pIList, LPSTR scriptText,
                     ULONG startingLineNumber, LPSTR path )
{
   TEXTPOS thePosition;

   //Append a semi-colon to the end of the expression, in order to use the
   //same grammar as regular script text.
   LPSTR expressionText = (CHAR*)tracemalloc("Parser_ParseExpression", sizeof(CHAR) * strlen(scriptText) + 2);
   strcpy( (CHAR*)expressionText, scriptText );
   strcat( (CHAR*)expressionText, ";" );

   //Create a new CLexer for this script text.
   thePosition.row = startingLineNumber;
   thePosition.col = 1;
   if(path)
   {
       strncpy(pparser->currentPath, path, 255);
   }
   else pparser->currentPath[0] = 0;
   pparser->errorFound = FALSE;
   Lexer_Init(&(pparser->theLexer), pparser->theLexer.preprocessor.ctx, path, expressionText, thePosition );

   //Get the first token from the CLexer.
   Lexer_GetNextToken(&(pparser->theLexer), &(pparser->theNextToken));

   //Setup the instruction list so we can add instructions.
   pparser->pIList = pIList;

   //Make sure we're pointing at the end of that list
   List_GotoLast(pparser->pIList);

   //Parse the expression
   Parser_Expr(pparser);

   //release text buffer
   tracefree((void*)expressionText);
}

/******************************************************************************
*  AddInstruction -- This method creates a new CInstruction and adds it to the
*  instruction list.  It serves to simplify the parser code slightly, and to
*  allow for easier extension of the CInstruction class for debugging.
*  Parameters: pCode -- OpCode of the new instruction
*              pToken -- CToken associated with the new instruction
*              label -- entry point label in the instruction list
******************************************************************************/

void Parser_AddInstructionViaToken(Parser* pparser, OpCode pCode, Token* pToken, Label label )
{
    Instruction* pInstruction = NULL;
    pInstruction = (Instruction*)tracemalloc("Parser_AddInstructionViaToken", sizeof(Instruction));
    Instruction_InitViaToken(pInstruction, pCode, pToken);
    List_InsertAfter(pparser->pIList, pInstruction, label );
}

/******************************************************************************
*  AddInstruction -- This method creates a new CInstruction and adds it to the
*  instruction list.  It serves to simplify the parser code slightly, and to
*  allow for easier extension of the CInstruction class for debugging.
*  Parameters: pCode -- OpCode of the new instruction
*              instrLabel -- Label associated with this instruction
*              listLabel -- entry point label in the instruction list
******************************************************************************/
void Parser_AddInstructionViaLabel(Parser* pparser, OpCode pCode, Label instrLabel, Label listLabel )
{
    Instruction* pInstruction = NULL;
    pInstruction = (Instruction*)tracemalloc("Parser_AddInstructionViaLabel", sizeof(Instruction));
    Instruction_InitViaLabel(pInstruction, pCode, instrLabel);
    List_InsertAfter(pparser->pIList, pInstruction, listLabel );
}

/******************************************************************************
*  Check -- This method compares the type of the current token with a specified
*  token type.
*  Parameters: theType -- MY_TOKEN_TYPE specifying the type to compare
*  Returns: true if the current token type matches theType
*           false otherwise.
******************************************************************************/
BOOL Parser_Check(Parser* pparser, MY_TOKEN_TYPE theType )
{
   //compare the token types
   return (pparser->theNextToken.theType == theType);
}

/*****************************************************************************
*  Match -- This method consumes the current token and retrieves the next one.
*  Parameters:
*  Returns:
******************************************************************************/
void Parser_Match( Parser* pparser )
{
   if (FAILED(Lexer_GetNextToken(&(pparser->theLexer), &(pparser->theNextToken))))
      Parser_Error(pparser, error);
}

/******************************************************************************
*   CreateLabel -- This method creates a label, unique within this parser, to
*   Serve as a target for jumps and calls.
*   Parameters: none
*   Returns: A unique Label
******************************************************************************/
Label Parser_CreateLabel( Parser* pparser )
{
    //Allocate a buffer for the new Label.  A long can take 10 characters at
    //most, so allocate that plus two extra for the "" and the null
    //terminator
    Label theLabel = (CHAR*)tracemalloc("Parser_CreateLabel", 12);
    memset((void*)theLabel, 0, 12);

    //Increment the label count.
    pparser->LabelCount++;

    sprintf((CHAR*)theLabel, "L%d", pparser->LabelCount);

    return theLabel;
}

/******************************************************************************
*  Productions -- These methods recursively parse the token stream into an
*  Abstract Syntax Tree.
******************************************************************************/

void Parser_Start(Parser* pparser )
{
   if (ParserSet_First(&(pparser->theParserSet), external_decl, pparser->theNextToken.theType)){
      Parser_External_decl(pparser);
      Parser_Start(pparser );
   }
   else if (ParserSet_Follow(&(pparser->theParserSet), start, pparser->theNextToken.theType )){}
   else Parser_Error(pparser, start );
}

/******************************************************************************
*  Declaration evaluation -- These methods translate declarations into
*  appropriate instructions.
******************************************************************************/
void Parser_External_decl(Parser* pparser )
{
   if (ParserSet_First(&(pparser->theParserSet), decl_spec, pparser->theNextToken.theType )){
      Parser_Decl_spec(pparser );
      Parser_External_decl2(pparser, FALSE); // go for the declaration
   }

   else Parser_Error(pparser, external_decl );
}

// this function is used by Parser_External_decl, because there can be multiple identifiers share only one type token
// variable only means the function only accept variables, not function declaration, e.g.
// int a, b=1, c;
void Parser_External_decl2(Parser* pparser, BOOL variableonly )
{
    Token token = pparser->theNextToken;
    //ignore the type of this declaration
    if(!Parser_Check(pparser, TOKEN_IDENTIFIER ))
    {
        printf("Identifier expected '%s'.\n", token.theSource);
        Parser_Error(pparser, external_decl );
    }
    Parser_Match(pparser);

    //type a =
    if (ParserSet_First(&(pparser->theParserSet), initializer, pparser->theNextToken.theType )){
        //switch to immediate mode and allocate a variable.
        Parser_AddInstructionViaToken(pparser, IMMEDIATE, (Token*)NULL, NULL );
        Parser_AddInstructionViaToken(pparser, DATA, &token, NULL );

        //Get the initializer; type a = expression
        Parser_Initializer(pparser );
        //type a = expresson;
        if(Parser_Check(pparser, TOKEN_SEMICOLON ))
        {
            Parser_Match(pparser);

            //Save the initializer
            Parser_AddInstructionViaToken(pparser, SAVE, &token, NULL );

            //Switch back to deferred mode
            Parser_AddInstructionViaToken(pparser, DEFERRED, (Token*)NULL, NULL );
        }
        //there's a comman instead of semicolon, so there should be another identifier
        else if(Parser_Check(pparser, TOKEN_COMMA ))
        {
            Parser_Match(pparser);
            //Save the initializer
            Parser_AddInstructionViaToken(pparser, SAVE, &token, NULL );
            Parser_External_decl2(pparser, TRUE);
        }
        else
        {
            Parser_Match(pparser);
            printf("Semicolon or comma expected before '%s'\n", pparser->theNextToken.theSource);
            Parser_Error(pparser, external_decl );
        }
    }
    // semmicolon, end expression.
    else if ( Parser_Check(pparser, TOKEN_SEMICOLON )){
       Parser_Match(pparser);
       //switch to immediate mode and allocate a variable.
       Parser_AddInstructionViaToken(pparser, IMMEDIATE, (Token*)NULL, NULL );
       Parser_AddInstructionViaToken(pparser, DATA, &token, NULL );

       //Switch back to deferred mode
       Parser_AddInstructionViaToken(pparser, DEFERRED, (Token*)NULL, NULL );
    }
    // still comma? there should be another identifier so allocate the variable and go for the next
    else if ( Parser_Check(pparser, TOKEN_COMMA )){
       Parser_Match(pparser);
       Parser_AddInstructionViaToken(pparser, DATA, &token, NULL );
       Parser_External_decl2(pparser, TRUE);
    }
    // not a comma, semicolon, or initializer, so must be a function declaration
    else if (variableonly==FALSE && ParserSet_First(&(pparser->theParserSet), funcDecl, pparser->theNextToken.theType )){
       Parser_AddInstructionViaToken(pparser, NOOP, &token, (Label)token.theSource );
       Parser_AddInstructionViaToken(pparser, PUSH, (Token*)NULL, NULL );
       Parser_FuncDecl(pparser );
       Parser_Comp_stmt_Label(pparser, pparser->theRetLabel );
       Parser_AddInstructionViaToken(pparser, POP, (Token*)NULL, NULL );
       Parser_AddInstructionViaToken(pparser, RET, (Token*)NULL, NULL );
    }
    else Parser_Error(pparser, external_decl );
}

void Parser_Decl_spec(Parser* pparser )
{
    if (Parser_Check(pparser, TOKEN_CONST )){
        Parser_Match(pparser);
    }

    if (Parser_Check(pparser, TOKEN_SIGNED )){
        Parser_Match(pparser);
    }
    else if (Parser_Check(pparser, TOKEN_UNSIGNED )){
        Parser_Match(pparser);
    }
// It's OK though not all delow are valid types for C language
    if (Parser_Check(pparser, TOKEN_VOID )){
        Parser_Match(pparser);
    }
    else if (Parser_Check(pparser, TOKEN_CHAR )){
        Parser_Match(pparser);
    }
    else if (Parser_Check(pparser, TOKEN_SHORT )){
        Parser_Match(pparser);
    }
    else if (Parser_Check(pparser, TOKEN_INT )){
        Parser_Match(pparser);
    }
    else if (Parser_Check(pparser, TOKEN_LONG )){
        Parser_Match(pparser);
    }
    else if (Parser_Check(pparser, TOKEN_FLOAT )){
        Parser_Match(pparser);
    }
    else if (Parser_Check(pparser, TOKEN_DOUBLE )){
        Parser_Match(pparser);
    }
    else Parser_Error(pparser, decl_spec );
}

// internal decleraton for variables.
void Parser_Decl(Parser* pparser )
{
    if (ParserSet_First(&(pparser->theParserSet), decl_spec, pparser->theNextToken.theType )){
        //ignore the type of this declaration
        Parser_Decl_spec(pparser );
        Parser_Decl2(pparser);
    }
    else Parser_Error(pparser, decl );
}

// this function is used by Parser_Decl for multiple declaration separated by commas
void Parser_Decl2(Parser* pparser )
{
    Token token = pparser->theNextToken;
    if(Parser_Check(pparser, TOKEN_IDENTIFIER )==FALSE)
    {
        printf("Identifier expected '%s'.\n", token.theSource);
        Parser_Error(pparser, decl );
    }
    Parser_Match(pparser);

    // =
    if (ParserSet_First(&(pparser->theParserSet), initializer, pparser->theNextToken.theType ))
    {
        Parser_AddInstructionViaToken(pparser, DATA, &token, NULL );

        //Get the initializer;
        Parser_Initializer(pparser );
        if(Parser_Check(pparser, TOKEN_SEMICOLON ))
        {
            Parser_Match(pparser);
            //Save the initializer
            Parser_AddInstructionViaToken(pparser, SAVE, &token, NULL );
        }
        else if(Parser_Check(pparser, TOKEN_COMMA ))
        {
            Parser_Match(pparser);
            Parser_AddInstructionViaToken(pparser, SAVE, &token, NULL );
            Parser_Decl2(pparser);
        }
        else
        {
            Parser_Match(pparser);
            printf("Semicolon or comma expected before '%s'\n", pparser->theNextToken.theSource);
            Parser_Error(pparser, decl );
        }
    }
    // ,
    else if ( Parser_Check(pparser, TOKEN_COMMA )){
        Parser_Match(pparser);
        Parser_AddInstructionViaToken(pparser, DATA, &token, NULL );
        Parser_Decl2(pparser);
    }
    // ;
    else if ( Parser_Check(pparser, TOKEN_SEMICOLON )){
        Parser_Match(pparser);
        Parser_AddInstructionViaToken(pparser, DATA, &token, NULL );
    }
    else Parser_Error(pparser, decl );
}

void Parser_FuncDecl(Parser* pparser )
{
    if (Parser_Check(pparser, TOKEN_LPAREN )){
        Parser_Match(pparser);
        Parser_FuncDecl1(pparser);
    }
    else if (ParserSet_Follow(&(pparser->theParserSet), funcDecl, pparser->theNextToken.theType )){}
    else Parser_Error(pparser, funcDecl );
}

void Parser_FuncDecl1(Parser* pparser )
{
    if (ParserSet_First(&(pparser->theParserSet), param_list, pparser->theNextToken.theType )){
        Parser_Param_list(pparser );
        Parser_Check(pparser, TOKEN_RPAREN );
        Parser_Match(pparser);
    }
    else if (Parser_Check(pparser, TOKEN_RPAREN )){
        Parser_AddInstructionViaLabel(pparser, CHECKARG, "0", NULL );
        Parser_Match(pparser);
    }
    else Parser_Error(pparser, funcDecl1 );
}

void Parser_Initializer(Parser* pparser )
{
    if (Parser_Check(pparser, TOKEN_ASSIGN )){
        Parser_Match(pparser);
        Parser_Assignment_expr(pparser );
    }
    else if (ParserSet_Follow(&(pparser->theParserSet), initializer, pparser->theNextToken.theType )){}
    else Parser_Error(pparser, initializer );
}

void Parser_Parm_decl(Parser* pparser )
{
    if (ParserSet_First(&(pparser->theParserSet), decl_spec, pparser->theNextToken.theType )){
        Parser_Decl_spec( pparser);

        if ( Parser_Check(pparser, TOKEN_IDENTIFIER )){
            Parser_AddInstructionViaToken(pparser, PARAM, &(pparser->theNextToken), NULL );
            Parser_Match(pparser);
        }
        else
            Parser_AddInstructionViaToken(pparser, PARAM, (Token*)NULL, NULL );
    }
    else Parser_Error(pparser, parm_decl );
}

void Parser_Param_list(Parser* pparser )
{
    if (ParserSet_First(&(pparser->theParserSet), parm_decl, pparser->theNextToken.theType )){
        Parser_Parm_decl(pparser );
        pparser->paramCount = 1; // start count params
        Parser_Param_list2(pparser );
    }
    else Parser_Error(pparser, param_list );
}

void Parser_Param_list2(Parser* pparser )
{
    int i;
    CHAR buf[4];
    Instruction* pinstruction;
    if (Parser_Check(pparser, TOKEN_COMMA )){
        Parser_Match(pparser);
        Parser_Parm_decl(pparser );
        pparser->paramCount++;
        Parser_Param_list2(pparser );
     }
     else if (ParserSet_Follow(&(pparser->theParserSet), param_list2, pparser->theNextToken.theType ))
     {
         //Walk back up the Instruction list and insert before the first PARAM
         //instruction.
         for(i = 1; i < pparser->paramCount; i++ )
             List_GotoPrevious(pparser->pIList);

         sprintf( buf, "%d", pparser->paramCount );
         pinstruction = (Instruction*)tracemalloc("Parser_Param_list2", sizeof(Instruction));
         Instruction_InitViaLabel(pinstruction, CHECKARG, buf);

         List_InsertBefore(pparser->pIList, (void*)pinstruction, NULL );

         //Walk back down the InstructionList to reset the insertion point
         for (i = 0; i < pparser->paramCount; i++ )
             List_GotoNext(pparser->pIList);

         //pparser->paramCount = 1;
    }

    else
    {
        Parser_Error(pparser, param_list2 );
        //pparser->paramCount =1 ;
    }
}

void Parser_Decl_list(Parser* pparser )
{
    if (ParserSet_First(&(pparser->theParserSet), decl, pparser->theNextToken.theType )){
        Parser_Decl(pparser );
        Parser_Decl_list2(pparser );
    }
    else Parser_Error(pparser, decl_list );
}

void Parser_Decl_list2(Parser* pparser )
{
    if (ParserSet_First(&(pparser->theParserSet), decl, pparser->theNextToken.theType )){
        Parser_Decl(pparser );
        Parser_Decl_list2(pparser );
    }
    if (ParserSet_First(&(pparser->theParserSet), stmt, pparser->theNextToken.theType )){
        Parser_Stmt(pparser );
        Parser_Stmt_list2(pparser );
    }
    else if (ParserSet_Follow(&(pparser->theParserSet), decl_list2, pparser->theNextToken.theType )){}
    else Parser_Error(pparser, decl_list2 );
}

/******************************************************************************
*  Statement evaluation -- These methods translate statements into
*  appropriate instructions.
******************************************************************************/
void Parser_Stmt_list(Parser* pparser )
{
    if (ParserSet_First(&(pparser->theParserSet), stmt, pparser->theNextToken.theType )){
        Parser_Stmt(pparser );
        Parser_Stmt_list2(pparser );
    }
    else Parser_Error(pparser, stmt_list );
}

void Parser_Stmt_list2(Parser* pparser )
{
    if (ParserSet_First(&(pparser->theParserSet), stmt, pparser->theNextToken.theType )){
        Parser_Stmt(pparser );
        Parser_Stmt_list2(pparser );
    }
    else if (ParserSet_First(&(pparser->theParserSet), decl, pparser->theNextToken.theType )){
        Parser_Decl(pparser);
        Parser_Decl_list2(pparser);
    }
    else if (ParserSet_Follow(&(pparser->theParserSet), stmt_list2, pparser->theNextToken.theType )){}
    else Parser_Error(pparser, stmt_list2 );
}

void Parser_Stmt( Parser* pparser)
{
    if (ParserSet_First(&(pparser->theParserSet), expr_stmt, pparser->theNextToken.theType )){
        Parser_Expr_stmt(pparser );
    }
    else if (ParserSet_First(&(pparser->theParserSet), comp_stmt, pparser->theNextToken.theType )){
        Parser_Comp_stmt(pparser );
    }
    else if (ParserSet_First(&(pparser->theParserSet), select_stmt, pparser->theNextToken.theType )){
        Parser_Select_stmt(pparser );
    }
    else if (ParserSet_First(&(pparser->theParserSet), iter_stmt, pparser->theNextToken.theType )){
        Parser_Iter_stmt(pparser );
    }
    else if (ParserSet_First(&(pparser->theParserSet), jump_stmt, pparser->theNextToken.theType )){
        Parser_Jump_stmt(pparser );
    }
    else Parser_Error(pparser, stmt );
}

void Parser_Expr_stmt(Parser* pparser )
{
    if (ParserSet_First(&(pparser->theParserSet), expr, pparser->theNextToken.theType )){
        Parser_Expr(pparser );
        Parser_AddInstructionViaToken(pparser, CLEAN, (Token*)NULL, NULL );
        Parser_Check(pparser, TOKEN_SEMICOLON );
        Parser_Match(pparser);
    }
    else Parser_Error(pparser, expr_stmt );
}

void Parser_Comp_stmt_Label(Parser* pparser, Label theLabel )
{
    Label label;
    if (Parser_Check(pparser, TOKEN_LCURLY )){
       Parser_Match(pparser);
       Parser_AddInstructionViaToken(pparser, PUSH, (Token*)NULL, NULL );
       label = Parser_CreateLabel(pparser);
       strcpy((CHAR*)theLabel, label);
       Parser_Comp_stmt2(pparser );
       Parser_Comp_stmt3(pparser );
       Parser_AddInstructionViaToken(pparser, NOOP, (Token*)NULL, label );
       tracefree((void*)label);//dont forget to free the label
       Parser_AddInstructionViaToken(pparser, POP, (Token*)NULL, NULL );
       Parser_Check(pparser, TOKEN_RCURLY );
       Parser_Match(pparser);
    }
    else Parser_Error(pparser, comp_stmt );
}

void Parser_Comp_stmt(Parser* pparser )
{
    Label jumpLabel;
    if (Parser_Check(pparser, TOKEN_LCURLY )){
        Parser_Match(pparser);
        Parser_AddInstructionViaToken(pparser, PUSH, (Token*)NULL, NULL );
        jumpLabel = Parser_CreateLabel(pparser);
        Parser_Comp_stmt2(pparser );
        Parser_Comp_stmt3(pparser );
        Parser_AddInstructionViaToken(pparser, NOOP, (Token*)NULL, jumpLabel );
        tracefree((void*)jumpLabel);
        Parser_AddInstructionViaToken(pparser, POP, (Token*)NULL, NULL );
        Parser_Check(pparser, TOKEN_RCURLY );
        Parser_Match(pparser);
    }
    else Parser_Error(pparser, comp_stmt );
}

void Parser_Comp_stmt2(Parser* pparser )
{
    if (ParserSet_First(&(pparser->theParserSet), decl_list, pparser->theNextToken.theType )){
        Parser_Decl_list(pparser );
    }
    else if (ParserSet_Follow(&(pparser->theParserSet), comp_stmt2, pparser->theNextToken.theType )){}
    else Parser_Error(pparser, comp_stmt2 );
}

void Parser_Comp_stmt3(Parser* pparser )
{
   if (ParserSet_First(&(pparser->theParserSet), stmt_list, pparser->theNextToken.theType )){
      Parser_Stmt_list(pparser );
   }
   else if (ParserSet_Follow(&(pparser->theParserSet), comp_stmt3, pparser->theNextToken.theType )){}
   else Parser_Error(pparser, comp_stmt3 );
}

void Parser_Select_stmt(Parser* pparser )
{
    Label falseLabel, endLabel;

    if (Parser_Check(pparser, TOKEN_IF )){
        //Create some labels for jump targets
        falseLabel = Parser_CreateLabel(pparser);
        endLabel = Parser_CreateLabel(pparser);
        Parser_Match(pparser);
        Parser_Check(pparser, TOKEN_LPAREN );
        Parser_Match(pparser);
        Parser_Expr(pparser );
        Parser_Check(pparser, TOKEN_RPAREN );
        Parser_AddInstructionViaLabel(pparser, Branch_FALSE, falseLabel, NULL );
        Parser_Match(pparser);
        Parser_Stmt(pparser );
        Parser_AddInstructionViaLabel(pparser, JUMP, endLabel, NULL );
        Parser_AddInstructionViaToken(pparser, NOOP, (Token*)NULL, falseLabel );
        Parser_Opt_else(pparser );
        Parser_AddInstructionViaToken(pparser, NOOP, (Token*)NULL, endLabel );
        //dont forget to free the labels
        tracefree((void*)falseLabel);
        tracefree((void*)endLabel);
    }
    else Parser_Error(pparser, select_stmt );
}

void Parser_Opt_else(Parser* pparser )
{
    if (Parser_Check(pparser, TOKEN_ELSE )){
        Parser_Match(pparser);
        Parser_Stmt(pparser );
    }
    else if (ParserSet_Follow(&(pparser->theParserSet), opt_else, pparser->theNextToken.theType )){}
    else Parser_Error(pparser, opt_else );
}

void Parser_Iter_stmt(Parser* pparser )
{
    Label endLabel, startLabel;
    List* pDefInst = NULL;
    Instruction* pInstruction;
    int i, size;
   //Create some labels for jump targets
    endLabel = Parser_CreateLabel(pparser);
    startLabel = Parser_CreateLabel(pparser);

    if (Parser_Check(pparser, TOKEN_WHILE )){
        Parser_Match(pparser);
        Parser_AddInstructionViaToken(pparser, NOOP, (Token*)NULL, startLabel );
        Parser_Check(pparser, TOKEN_LPAREN );
        Parser_Match(pparser);
        Parser_Expr(pparser );
        Parser_Check(pparser, TOKEN_RPAREN );
        Parser_AddInstructionViaLabel(pparser, Branch_FALSE, endLabel, NULL );
        Stack_Push(&(pparser->LabelStack), (void*)endLabel ); //*****
        Parser_Match(pparser);
        Parser_Stmt(pparser );
        Stack_Pop(&(pparser->LabelStack)); //*****
        Parser_AddInstructionViaLabel(pparser, JUMP, startLabel, NULL );
        Parser_AddInstructionViaToken(pparser, NOOP, (Token*)NULL, endLabel );
    }
    else if (Parser_Check(pparser, TOKEN_DO )){
        Parser_Match(pparser);
        Parser_AddInstructionViaToken(pparser, NOOP, (Token*)NULL, startLabel );
        Stack_Push(&(pparser->LabelStack), (void*)endLabel ); //*****
        Parser_Stmt(pparser );
        Parser_Check(pparser, TOKEN_WHILE );
        Parser_Match(pparser);
        Parser_Check(pparser, TOKEN_LPAREN );
        Parser_Match(pparser);
        Parser_Expr(pparser );
        Parser_Check(pparser, TOKEN_RPAREN );
        Parser_AddInstructionViaLabel(pparser, Branch_TRUE, startLabel, NULL );
        Parser_AddInstructionViaToken(pparser, NOOP, (Token*)NULL, endLabel );
        Stack_Pop(&(pparser->LabelStack)); //*****
        Parser_Match(pparser);
        Parser_Check(pparser, TOKEN_SEMICOLON );
        Parser_Match(pparser);
    }
    else if (Parser_Check(pparser, TOKEN_FOR )){
        Parser_Match(pparser);
        Parser_Check(pparser, TOKEN_LPAREN );
        Parser_Match(pparser);

        //Add any initializer code
        Parser_Opt_expr_stmt(pparser );

        //Set the flag for the conditional to jump back to.
        Parser_AddInstructionViaToken(pparser, NOOP, (Token*)NULL, startLabel );
        Stack_Push(&(pparser->LabelStack), (void*)endLabel ); //*****

        //Build the conditional statement
        if (ParserSet_First(&(pparser->theParserSet), expr, pparser->theNextToken.theType )){
            Parser_Expr(pparser);
            Parser_AddInstructionViaLabel(pparser, Branch_FALSE, endLabel, NULL );
            Parser_Check(pparser, TOKEN_SEMICOLON );
            Parser_Match(pparser);
        }
        else if (ParserSet_Follow(&(pparser->theParserSet), opt_expr_stmt, pparser->theNextToken.theType )){}
        else Parser_Error(pparser, opt_expr_stmt );

        //We need to defer adding the instructions related to the last parameter
        //of the for loop, so catch them.
        pDefInst = Parser_Defer_expr_stmt(pparser );
        Parser_Check(pparser, TOKEN_RPAREN );
        Parser_Match(pparser);
        Parser_Stmt(pparser );

        //Add in the deferred instructions, if there were any.
        if (pDefInst){
            pInstruction = NULL;

            PFOREACH( pDefInst,
                pInstruction = (Instruction*)List_Retrieve(pDefInst);
          /*
            //We have to assume here that the expression didn't need labels.
            AddInstruction( pInstruction->m_OpCode, pInstruction->m_pToken, NULL );
          */
                List_InsertAfter(pparser->pIList, (void*)pInstruction, NULL);
            );

            List_Clear(pDefInst);
            tracefree((void*)pDefInst);
        }

        //Add the jump statement
        Parser_AddInstructionViaLabel(pparser, JUMP, startLabel, NULL );

        //Add the end label.
        Parser_AddInstructionViaToken(pparser, NOOP, (Token*)NULL, endLabel );
        Stack_Pop(&(pparser->LabelStack));//**********
    }
    else Parser_Error(pparser, iter_stmt );
    tracefree((void*)startLabel);
    tracefree((void*)endLabel);
}

void Parser_Opt_expr_stmt(Parser* pparser )
{
    if (ParserSet_First(&(pparser->theParserSet), expr_stmt, pparser->theNextToken.theType )){
        Parser_Expr_stmt(pparser );
    }
    else if (ParserSet_Follow(&(pparser->theParserSet), opt_expr_stmt, pparser->theNextToken.theType )){}
    else Parser_Error(pparser, opt_expr_stmt );
}

List* Parser_Defer_expr_stmt(Parser* pparser )
{
    List* pMainList, *pTemp; //save main list
    //if (ParserSet_First(&(pparser->theParserSet), expr_stmt, pparser->theNextToken.theType )){
    if (ParserSet_First(&(pparser->theParserSet), expr, pparser->theNextToken.theType )){
        //We have to swap out instruction lists
        pMainList = pparser->pIList;
        //create a new Instruction List
        pparser->pIList = (List*)tracemalloc("Parser_Defer_expr_stmt", sizeof(List));
        List_Init(pparser->pIList);

        //Expr_stmt( );
        Parser_Expr(pparser);

        //Swap the instruction lists back and return the deferred list
        pTemp = pparser->pIList;
        pparser->pIList = pMainList;

        return pTemp;
    }
    else if (ParserSet_Follow(&(pparser->theParserSet), defer_expr_stmt, pparser->theNextToken.theType )){}
    else Parser_Error(pparser, defer_expr_stmt );

    return NULL;
}

void Parser_Jump_stmt(Parser* pparser )
{
    Label breakTarget;
    if (Parser_Check(pparser, TOKEN_BREAK )){
        Parser_Match(pparser);
        Parser_Check(pparser, TOKEN_SEMICOLON );
        Parser_Match(pparser);

        breakTarget = Stack_Top(&(pparser->LabelStack));
        Parser_AddInstructionViaLabel(pparser, JUMP, breakTarget, NULL );
    }
    else if (Parser_Check(pparser, TOKEN_RETURN )){
        Parser_Match(pparser);
        Parser_Opt_expr(pparser );
        Parser_Check(pparser, TOKEN_SEMICOLON );
        Parser_Match(pparser);

        Parser_AddInstructionViaLabel(pparser, JUMPR, pparser->theRetLabel, NULL );
    }
    else Parser_Error(pparser, jump_stmt );
}

/******************************************************************************
*  Expression Evaluation -- These methods translate expressions into
*  appropriate instructions.
******************************************************************************/
void Parser_Opt_expr(Parser* pparser )
{
    if (ParserSet_First(&(pparser->theParserSet), expr, pparser->theNextToken.theType )){
        Parser_Expr(pparser );
    }
    else if (ParserSet_Follow(&(pparser->theParserSet), opt_expr, pparser->theNextToken.theType )){}
    else Parser_Error(pparser, opt_expr );
}

void Parser_Expr(Parser* pparser)
{
    if (ParserSet_First(&(pparser->theParserSet), assignment_expr, pparser->theNextToken.theType )){
        Parser_Assignment_expr(pparser );
    }
    else Parser_Error(pparser, expr );
}

OpCode Parser_Assignment_op(Parser* pparser )
{
    if (Parser_Check(pparser, TOKEN_ASSIGN )){
        Parser_Match(pparser);
        return NOOP;
    }
    else if (Parser_Check(pparser, TOKEN_MUL_ASSIGN )){
        Parser_Match(pparser);
        return MUL;
    }
    else if (Parser_Check(pparser, TOKEN_DIV_ASSIGN )){
        Parser_Match(pparser);
        return DIV;
    }
    else if (Parser_Check(pparser, TOKEN_ADD_ASSIGN )){
        Parser_Match(pparser);
        return ADD;
    }
    else if (Parser_Check(pparser, TOKEN_SUB_ASSIGN )){
        Parser_Match(pparser);
        return SUB;
    }
    else if (Parser_Check(pparser, TOKEN_MOD_ASSIGN )){
        Parser_Match(pparser);
        return MOD;
    }
    else Parser_Error(pparser, assignment_op );

    return ERR;
}

void Parser_Assignment_expr(Parser* pparser )
{
    if (ParserSet_First(&(pparser->theParserSet), log_or_expr, pparser->theNextToken.theType )){//= /= += Operator, or a comma and the like (the reputation of a variable)
        Parser_Log_or_expr(pparser );
        Parser_Assignment_expr2(pparser );
    }
    else Parser_Error(pparser, assignment_expr );
}

void Parser_Assignment_expr2(Parser* pparser )
{
    Instruction* pInstruction;
    Token token;
    OpCode code;
    if (ParserSet_First(&(pparser->theParserSet), assignment_op, pparser->theNextToken.theType )){
        pInstruction = (Instruction*)List_Retrieve(pparser->pIList);
        token = pparser->theFieldToken;
        pparser->theFieldToken.theType = END_OF_TOKENS;

        code = Parser_Assignment_op(pparser );
        Parser_Log_or_expr(pparser );
        Parser_AddInstructionViaToken(pparser, code, (Token*)NULL, NULL );

        if (token.theType != END_OF_TOKENS){
            Parser_AddInstructionViaToken(pparser, LOAD, &token, NULL );
            Parser_AddInstructionViaToken(pparser, FIELD, (Token*)NULL, NULL );
        }

        Parser_AddInstructionViaToken(pparser, SAVE, pInstruction->theToken, NULL );
        Parser_Assignment_expr2(pparser );
    }
    else if (ParserSet_Follow(&(pparser->theParserSet), assignment_expr2, pparser->theNextToken.theType ))
    {
        pparser->theFieldToken.theType = END_OF_TOKENS;
    }
    else Parser_Error(pparser, assignment_expr2 );
}

void Parser_Const_expr(Parser* pparser)
{
    if (ParserSet_First(&(pparser->theParserSet), log_or_expr, pparser->theNextToken.theType )){
        Parser_Log_or_expr(pparser );
    }
    else Parser_Error(pparser, const_expr );
}

void Parser_Log_or_expr(Parser* pparser )
{
    if (ParserSet_First(&(pparser->theParserSet), log_and_expr, pparser->theNextToken.theType )){
        Parser_Log_and_expr(pparser );
        Parser_Log_or_expr2(pparser );
    }
    else Parser_Error(pparser, log_or_expr );
}

void Parser_Log_or_expr2(Parser* pparser )
{
    if (Parser_Check(pparser, TOKEN_OR_OP )){
        Parser_Match(pparser);
        Parser_Log_and_expr(pparser );
        Parser_AddInstructionViaToken(pparser, OR, (Token*)NULL, NULL );
        Parser_Log_or_expr2(pparser );
    }
    else if (ParserSet_Follow(&(pparser->theParserSet), log_or_expr2, pparser->theNextToken.theType )){}
    else Parser_Error(pparser, log_or_expr2 );
}

void Parser_Log_and_expr(Parser* pparser )
{
    if (ParserSet_First(&(pparser->theParserSet), equal_expr, pparser->theNextToken.theType )){
        Parser_Equal_expr(pparser );
        Parser_Log_and_expr2(pparser );
    }
    else Parser_Error(pparser, log_and_expr );
}

void Parser_Log_and_expr2(Parser* pparser )
{
    if (Parser_Check(pparser, TOKEN_AND_OP )){
        Parser_Match(pparser);
        Parser_Equal_expr(pparser );
        Parser_AddInstructionViaToken(pparser, AND, (Token*)NULL, NULL );
        Parser_Log_and_expr2( pparser);
    }
    else if (ParserSet_Follow(&(pparser->theParserSet), log_and_expr2, pparser->theNextToken.theType )){}
    else Parser_Error(pparser, log_and_expr2 );
}

OpCode Parser_Eq_op(Parser* pparser )
{
    if (Parser_Check(pparser, TOKEN_EQ_OP )){
        Parser_Match(pparser);
        return EQ;
    }
    else if (Parser_Check(pparser, TOKEN_NE_OP )){
        Parser_Match(pparser);
        return NE;
    }
    else Parser_Error(pparser, eq_op );

    return ERR;
}

void Parser_Equal_expr(Parser* pparser )
{
    if (ParserSet_First(&(pparser->theParserSet), rel_expr, pparser->theNextToken.theType )){
        Parser_Rel_expr(pparser );
        Parser_Equal_expr2(pparser );
    }
    else Parser_Error(pparser, equal_expr );
}

void Parser_Equal_expr2(Parser* pparser )
{
    OpCode code;
    if (ParserSet_First(&(pparser->theParserSet), eq_op, pparser->theNextToken.theType )){
        code = Parser_Eq_op(pparser );
        Parser_Rel_expr(pparser );
        Parser_AddInstructionViaToken(pparser, code, (Token*)NULL, NULL );
        Parser_Equal_expr2(pparser );
    }
    else if (ParserSet_Follow(&(pparser->theParserSet), equal_expr2, pparser->theNextToken.theType )){}
    else Parser_Error(pparser, equal_expr2 );
}

OpCode Parser_Rel_op(Parser* pparser )
{
    if (Parser_Check(pparser, TOKEN_GE_OP )){
        Parser_Match(pparser);
        return GE;
    }
    else if (Parser_Check(pparser, TOKEN_LE_OP )){
        Parser_Match(pparser);
        return LE;
    }
    else if (Parser_Check(pparser, TOKEN_LT )){
        Parser_Match(pparser);
        return LT;
    }
    else if (Parser_Check(pparser, TOKEN_GT )){
        Parser_Match(pparser);
        return GT;
    }
    else Parser_Error(pparser, rel_op );

    return ERR;
}

void Parser_Rel_expr(Parser* pparser )
{
    if (ParserSet_First(&(pparser->theParserSet), add_expr, pparser->theNextToken.theType )){
        Parser_Add_expr(pparser );
        Parser_Rel_expr2(pparser );
    }
    else Parser_Error(pparser, rel_expr );
}

void Parser_Rel_expr2(Parser* pparser )
{
    OpCode code;
    if (ParserSet_First(&(pparser->theParserSet), rel_op, pparser->theNextToken.theType )){
        code = Parser_Rel_op(pparser );
        Parser_Add_expr(pparser );
        Parser_AddInstructionViaToken(pparser, code, (Token*)NULL, NULL );
        Parser_Rel_expr2(pparser );
    }
    else if (ParserSet_Follow(&(pparser->theParserSet), rel_expr2, pparser->theNextToken.theType )){}
    else Parser_Error(pparser, rel_expr2 );
}

OpCode Parser_Add_op(Parser* pparser )
{
    if (Parser_Check(pparser, TOKEN_ADD )){
        Parser_Match(pparser);
        return ADD;
    }
    else if (Parser_Check(pparser, TOKEN_SUB )){
        Parser_Match(pparser);
        return SUB;
    }
    else Parser_Error(pparser, add_op );

    return ERR;
}

void Parser_Add_expr(Parser* pparser )
{
    if (ParserSet_First(&(pparser->theParserSet), mult_expr, pparser->theNextToken.theType )){
        Parser_Mult_expr(pparser );
        Parser_Add_expr2(pparser );
    }
    else Parser_Error(pparser, add_expr );
}

void Parser_Add_expr2(Parser* pparser )
{
    OpCode code;
    if (ParserSet_First(&(pparser->theParserSet), add_op, pparser->theNextToken.theType )){
        code = Parser_Add_op(pparser );
        Parser_Mult_expr(pparser );
        Parser_AddInstructionViaToken(pparser, code, (Token*)NULL, NULL );
        Parser_Add_expr2(pparser );
    }
    else if (ParserSet_Follow(&(pparser->theParserSet), add_expr2, pparser->theNextToken.theType )){}
    else Parser_Error(pparser, add_expr2 );
}

OpCode Parser_Mult_op(Parser* pparser )
{
    if (Parser_Check(pparser, TOKEN_MUL )){
        Parser_Match(pparser);
        return MUL;
    }
    else if (Parser_Check(pparser, TOKEN_DIV )){
        Parser_Match(pparser);
        return DIV;
    }
    else if (Parser_Check(pparser, TOKEN_MOD )){
        Parser_Match(pparser);
        return MOD;
    }



    else Parser_Error(pparser, mult_op );

    return ERR;
}

void Parser_Mult_expr(Parser* pparser )
{
    if (ParserSet_First(&(pparser->theParserSet), unary_expr, pparser->theNextToken.theType )){
        Parser_Unary_expr(pparser);
        Parser_Mult_expr2(pparser );
    }
    else Parser_Error(pparser, mult_expr );
}

void Parser_Mult_expr2(Parser* pparser )
{
    OpCode code;
    if (ParserSet_First(&(pparser->theParserSet), mult_op, pparser->theNextToken.theType )){
        code = Parser_Mult_op(pparser );
        Parser_Unary_expr(pparser );
        Parser_AddInstructionViaToken(pparser, code, (Token*)NULL, NULL );
        Parser_Mult_expr2(pparser );
    }
    else if (ParserSet_Follow(&(pparser->theParserSet), mult_expr2, pparser->theNextToken.theType )){}
    else Parser_Error(pparser, mult_expr2 );
}

void Parser_Unary_expr(Parser* pparser )
{
    Instruction* pInstruction = NULL;
    if (ParserSet_First(&(pparser->theParserSet), postfix_expr, pparser->theNextToken.theType )){
        Parser_Postfix_expr(pparser );
    }
    else if (Parser_Check(pparser, TOKEN_INC_OP )){
        Parser_Match(pparser);
        Parser_Unary_expr(pparser );
        pInstruction = (Instruction*)List_Retrieve(pparser->pIList);
        Parser_AddInstructionViaToken(pparser, INC, (Token*)NULL, NULL );
        Parser_AddInstructionViaToken(pparser, SAVE, pInstruction->theToken, NULL );
        Parser_AddInstructionViaToken(pparser, LOAD, pInstruction->theToken, NULL );
    }
    else if (Parser_Check(pparser, TOKEN_DEC_OP )){
        Parser_Match(pparser);
        Parser_Unary_expr( pparser);
        pInstruction = (Instruction*)List_Retrieve(pparser->pIList);
        Parser_AddInstructionViaToken(pparser, DEC, (Token*)NULL, NULL );
        Parser_AddInstructionViaToken(pparser, SAVE, pInstruction->theToken, NULL );
        Parser_AddInstructionViaToken(pparser, LOAD, pInstruction->theToken, NULL );
    }
    else if (Parser_Check(pparser, TOKEN_ADD )){
        Parser_Match(pparser);
        Parser_Unary_expr( pparser);
    }
    else if (Parser_Check(pparser, TOKEN_SUB )){
        Parser_Match(pparser);
        Parser_Unary_expr(pparser );
        Parser_AddInstructionViaToken(pparser, NEG, (Token*)NULL, NULL );
    }
    else if (Parser_Check(pparser, TOKEN_BOOLEAN_NOT )){
        Parser_Match(pparser);
        Parser_Unary_expr(pparser );
        Parser_AddInstructionViaToken(pparser, NOT, (Token*)NULL, NULL );
    }
    else Parser_Error(pparser, unary_expr );
}

void Parser_Postfix_expr(Parser* pparser )
{
    if (ParserSet_First(&(pparser->theParserSet), primary_expr, pparser->theNextToken.theType )){
        Parser_Primary_expr(pparser );
        Parser_Postfix_expr2(pparser );
    }
    else Parser_Error(pparser, postfix_expr );
}

void Parser_Postfix_expr2(Parser* pparser )
{
    Instruction* pInstruction, *plast;
    Label label;
    if (Parser_Check(pparser, TOKEN_LPAREN )){
        //Remove the top instruction so we can get the arguments in first.
        pInstruction = (Instruction*)List_Retrieve(pparser->pIList);
        plast = (Instruction*)List_GetLast(pparser->pIList);
        List_Remove(pparser->pIList);
        if(plast != pInstruction)
            List_GotoPrevious(pparser->pIList);//walk back, because the list's current node will move if it is the last one

        Parser_Match(pparser);
        Parser_Arg_expr_list( pparser);
        Parser_Check(pparser, TOKEN_RPAREN );
        Parser_Match(pparser);

        //Change the type of the instruction we removed from LOAD to CALL, create
        //a label for it, and add it back to the instruction list.
        label = Parser_CreateLabel(pparser);
        pInstruction->OpCode = CALL;
        List_InsertAfter(pparser->pIList, (void*)pInstruction, label );
        //dont forget to free the label
        tracefree((void*)label);

        Parser_Postfix_expr2(pparser );
    }
    else if (Parser_Check(pparser, TOKEN_FIELD )){
        //cache the token of the field source for assignment expressions.
        pInstruction = (Instruction*)List_Retrieve(pparser->pIList);
        pparser->theFieldToken = *(pInstruction->theToken);

        Parser_AddInstructionViaToken(pparser, FIELD, &(pparser->theNextToken), NULL );
        Parser_Match(pparser);
        Parser_Check(pparser, TOKEN_IDENTIFIER );
        Parser_AddInstructionViaToken(pparser, LOAD, &(pparser->theNextToken), NULL );
        Parser_Match(pparser);
        Parser_Postfix_expr2(pparser );
    }
    else if (Parser_Check(pparser, TOKEN_INC_OP )){
        pInstruction = (Instruction*)List_Retrieve(pparser->pIList);
        Parser_AddInstructionViaToken(pparser, LOAD, pInstruction->theToken, NULL );
        Parser_AddInstructionViaToken(pparser, INC, &(pparser->theNextToken), NULL );
        Parser_Match(pparser);
        Parser_AddInstructionViaToken(pparser, SAVE, pInstruction->theToken, NULL );
        Parser_Postfix_expr2(pparser );
    }
    else if (Parser_Check(pparser, TOKEN_DEC_OP )){
        pInstruction = (Instruction*)List_Retrieve(pparser->pIList);
        Parser_AddInstructionViaToken(pparser, LOAD, pInstruction->theToken, NULL );
        Parser_AddInstructionViaToken(pparser, DEC, &(pparser->theNextToken), NULL );
        Parser_Match(pparser);
        Parser_AddInstructionViaToken(pparser, SAVE, pInstruction->theToken, NULL );
        Parser_Postfix_expr2(pparser );
    }
    else if (ParserSet_Follow(&(pparser->theParserSet), postfix_expr2, pparser->theNextToken.theType )){}
    else Parser_Error(pparser, postfix_expr2 );
}

void Parser_Arg_expr_list(Parser* pparser )
{
    int argRange, i;
    if (ParserSet_First(&(pparser->theParserSet), assignment_expr, pparser->theNextToken.theType )){
        argRange = List_GetSize(pparser->pIList);
        Parser_Assignment_expr(pparser );
        argRange = List_GetSize(pparser->pIList) - argRange;
        for (i = 1; i < argRange; i++)
            List_GotoPrevious(pparser->pIList);
        Parser_Arg_expr_list2(pparser, 1, argRange);
    }
    else if (ParserSet_Follow(&(pparser->theParserSet), arg_expr_list, pparser->theNextToken.theType ))
    {
        //No arguments, so push a zero argument count onto the stack
        Parser_AddInstructionViaLabel(pparser, CONSTINT, "0", NULL );
    }
    else Parser_Error(pparser, arg_expr_list );
}

void Parser_Arg_expr_list2(Parser* pparser, int argCount, int range)
{
   //This is going to get us in trouble if we have function calls as arguments.
   //static int argCount = 1;

   //We push the arguments onto the stack backwards so they come off right,
   //So back up one instruction before inserting.
    int argRange, i;
    CHAR buffer[4];
    List_GotoPrevious(pparser->pIList);

    if (Parser_Check(pparser, TOKEN_COMMA )){
        Parser_Match(pparser);
        argRange = List_GetSize(pparser->pIList);
        Parser_Assignment_expr(pparser );
        argRange = List_GetSize(pparser->pIList) - argRange;
        argCount++;
        for (i = 1; i < argRange; i++)
             List_GotoPrevious(pparser->pIList);
        range += argRange;
        //if( m_pIList->Last()->m_OpCode == CALL) range++;
        Parser_Arg_expr_list2(pparser, argCount, range );
    }
    else if (ParserSet_Follow(&(pparser->theParserSet), arg_expr_list2, pparser->theNextToken.theType ))
    {
        //Run back down the list to insert the argument count
        for (i = 0; i < range; i++)
            List_GotoNext(pparser->pIList);

        sprintf( buffer, "%d", argCount );
        Parser_AddInstructionViaLabel(pparser, CONSTINT, buffer, NULL );
    }
    else
    {
        Parser_Error(pparser, arg_expr_list2 );
    }
}

void Parser_Primary_expr(Parser* pparser )
{
    if (Parser_Check(pparser, TOKEN_IDENTIFIER )){
        Parser_AddInstructionViaToken(pparser, LOAD, &(pparser->theNextToken), NULL );
        Parser_Match(pparser);
    }
    else if (ParserSet_First(&(pparser->theParserSet), constant, pparser->theNextToken.theType )){
        Parser_Constant(pparser);
    }
    else if (Parser_Check(pparser, TOKEN_LPAREN )){
        Parser_Match(pparser);
        Parser_Expr(pparser );
        Parser_Check(pparser, TOKEN_RPAREN );
        Parser_Match(pparser);
    }
    else Parser_Error(pparser, primary_expr );
}

void Parser_Constant(Parser* pparser )
{
    if (Parser_Check(pparser, TOKEN_INTCONSTANT ) || Parser_Check(pparser, TOKEN_HEXCONSTANT )){
        Parser_AddInstructionViaToken(pparser, CONSTINT, &(pparser->theNextToken), NULL );
        Parser_Match(pparser);
    }
    else if (Parser_Check(pparser, TOKEN_FLOATCONSTANT )){
        Parser_AddInstructionViaToken(pparser, CONSTDBL, &(pparser->theNextToken), NULL );
        Parser_Match(pparser);
    }
    else if (Parser_Check(pparser, TOKEN_STRING_LITERAL )){
        Parser_AddInstructionViaToken(pparser, CONSTSTR, &(pparser->theNextToken), NULL );
        Parser_Match(pparser);
    }
    else Parser_Error(pparser, constant );
}

const char*  _production_error_message(Parser* pparser, PRODUCTION offender)
{
    switch(offender)
    {
    case constant:
        return "Invalid constant format";
    case start:
        return "Invalid start of declaration";
    case postfix_expr2:
        return "Invalid function call or expression";
    case funcDecl1:
        return "Parameters or ')' expected after function declaration";
    case external_decl:
        return "Invalid external declaration";
    case decl_spec:
        return "Invalid identifier";
    case decl:
        return "Invalid declaration(expected comma, semicolon or initializer?)";
    default:
        return "Unknown error";
    }
}


void Parser_Error(Parser* pparser, PRODUCTION offender )
{
    //Report the offending token to the error handler, along with the production
    //it offended in.
    if (offender != error)
		pp_error(&(pparser->theLexer.preprocessor), "%s '%s' (in production %u)",
				_production_error_message(pparser, offender), pparser->theNextToken.theSource, offender);
    
    pparser->errorFound = TRUE;

    //The script is obviously not valid, but it's good to try and find all the
    //errors at one time.  Therefore go into Panic Mode error recovery -- keep
    //grabbing tokens until we find one we can use
    do
    {
        while (!SUCCEEDED(Lexer_GetNextToken(&(pparser->theLexer), &(pparser->theNextToken))));
        if (pparser->theNextToken.theType != TOKEN_EOF) break;
    } while (!ParserSet_Follow(&(pparser->theParserSet), offender, pparser->theNextToken.theType));
}
