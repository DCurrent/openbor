/*
 * OpenBOR - http://www.chronocrash.com
 * -----------------------------------------------------------------------
 * All rights reserved, see LICENSE in OpenBOR root for details.
 *
 * Copyright (c) 2004 - 2014 OpenBOR Team
 */

#ifndef STACK_H
#define STACK_H
#include "List.h"

typedef List Stack;

void Stack_Init(Stack *stack);
void Stack_Push( Stack *stack, void *e);
void Stack_Pop(Stack *stack );
void *Stack_Top(const Stack *stack);
int Stack_IsEmpty(const Stack *stack);

#endif

