/*
 * OpenBOR - http://www.LavaLit.com
 * -----------------------------------------------------------------------
 * Licensed under the BSD license, see LICENSE in OpenBOR root for details.
 *
 * Copyright (c) 2004 - 2011 OpenBOR Team
 */

/* Prints a stack trace on *nix platforms if the engine crashes with SIGSEGV.
 * By Plombo, based on code at:
 * http://stackoverflow.com/questions/77005/how-to-generate-a-stacktrace-when-my-gcc-c-app-crashes/1925461#1925461
 * Date created: March 6, 2011
 */ 

#include "stacktrace.h"

#ifdef CUSTOM_SIGNAL_HANDLER

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <execinfo.h>
#include <ucontext.h>
#include "globals.h"
#undef exit // make sure "exit" refers to libc exit() and not borExit()

// This structure mirrors the one found in /usr/include/asm/ucontext.h
typedef struct _sig_ucontext {
	unsigned long     uc_flags;
	struct ucontext   *uc_link;
	stack_t           uc_stack;
	struct sigcontext uc_mcontext;
	sigset_t          uc_sigmask;
} sig_ucontext_t;

void addr2line(char* message)
{
	char command[1024];
	char output[1024];
	int i, len, lparen = 0;
	FILE* proc;
	
	snprintf(command, sizeof(command), "addr2line -e %s", message);
	len = strlen(command);
	for(i=0; i<len; i++)
	{
		if(command[i] == '[' || command[i] == ']') command[i] = ' ';
		if(command[i] == '(') lparen = 1;
		if(command[i] == ')') { command[i] = ' '; lparen = 0; }
		if(lparen) command[i] = ' ';
	}
	
	proc = popen(command, "r");
	if(proc == NULL) return;
	
	output[0] = '(';
	len = fread(output+1, 1, sizeof(output)-3, proc) + 1;
	if(strchr(output, '\n')) len = strchr(output, '\n') - output;
	output[len] = ')';
	output[len+1] = '\0';
	printf(output);
	pclose(proc);
}

void handleFatalSignal(int sig_num, siginfo_t * info, void * ucontext)
{
	void*              array[50];
	void*              caller_address;
	char**             messages;
	int                size, i;
	sig_ucontext_t*    uc;
	static int         recursive = 0;
	
	if(recursive++)
	{
		if(recursive == 2)
			printf("\nOuch, recursive signal handling!\n");
		exit(EXIT_FAILURE);
	}
	
	printf("\n****** A Serious Error Occurred *******"
		   "\n*            Shutting Down            *\n\n");

	uc = (sig_ucontext_t*)ucontext;

#ifndef AMD64
	// Get the address at the time the signal was raised from the EIP (x86)
	caller_address = (void*)uc->uc_mcontext.eip;   
#else
	caller_address = (void*)uc->uc_mcontext.rip;
#endif

	printf("Received signal %d (%s), address is %p from %p\n\n", sig_num, 
		strsignal(sig_num), info->si_addr, (void*)caller_address);

	size = backtrace(array, 50);

	// overwrite sigaction with caller's address
	array[1] = caller_address;

	messages = backtrace_symbols(array, size);
	printf("Backtrace:\n");
	
	// skip first stack frame (points here)
	for (i=1; i<size && messages!=NULL; i++)
	{
		printf("(%d) %s ", i, messages[i]);
		addr2line(messages[i]);
		printf("\n");
	}

	free(messages);
	
	exit(EXIT_FAILURE);
}

#endif

