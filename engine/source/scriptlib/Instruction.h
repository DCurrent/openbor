/*
 * OpenBOR - http://www.LavaLit.com
 * -----------------------------------------------------------------------
 * All rights reserved, see LICENSE in OpenBOR root for details.
 *
 * Copyright (c) 2004 - 2011 OpenBOR Team
 */

#ifndef INSTRUCTION_H
#define INSTRUCTION_H
#include "depends.h"
#include "Lexer.h"
#include "List.h"
#include "ScriptVariant.h"

typedef LPCSTR Label;

typedef enum OpCode{ CONSTSTR, CONSTDBL, CONSTINT, LOAD, SAVE, INC, DEC, FIELD, CALL, POS, NEG,
			 NOT, MUL, DIV,MOD, ERR, ADD, SUB, JUMP, GE, LE, LT, GT, EQ, NE, OR,
			 AND, NOOP, PUSH, POP, Branch_FALSE, Branch_TRUE, DATA, PARAM,
			 IMMEDIATE, DEFERRED, RET, CHECKARG, CLEAN, JUMPR, FUNCDECL, OPCODE_END
}OpCode;

typedef struct Instruction{
   // put these first two into a bitfield to save memory
   unsigned OpCode:31;
   unsigned jumpTargetType:1;
   Token* theToken;
   CHAR* Label;//[MAX_STR_LEN+1];
   ScriptVariant* theVal;
   ScriptVariant* theVal2;
   ScriptVariant* theRef;
   ScriptVariant* theRef2;
   List* theRefList;
   HRESULT (*functionRef)(ScriptVariant**, ScriptVariant**, int);
   union{
	  int theJumpTargetIndex;
	  struct Instruction** ptheJumpTarget;
	  //struct Instruction* theJumpTarget;
   };
}Instruction;


void Instruction_InitViaToken(Instruction* pins, OpCode code, Token* pToken );
void Instruction_InitViaLabel(Instruction* pins, OpCode code, LPCSTR label );
void Instruction_Init(Instruction* pins);
void Instruction_Clear(Instruction* pins);

void Instruction_NewData(Instruction* pins);
void Instruction_NewData2(Instruction* pins);
void Instruction_ConvertConstant(Instruction* pins);

void Instruction_ToString(Instruction* pins, LPSTR strRep);
#endif
