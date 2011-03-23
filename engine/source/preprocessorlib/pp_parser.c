/*
 * OpenBOR - http://www.LavaLit.com
 * -----------------------------------------------------------------------
 * Licensed under the BSD license, see LICENSE in OpenBOR root for details.
 *
 * Copyright (c) 2004 - 2011 OpenBOR Team
 */

/**
 * This is the parser for the script preprocessor.  Its purpose is to emit the
 * preprocessed source code for use by scriptlib.  It is not related to the
 * parser in scriptlib because it does something entirely different.
 *
 * @author Plombo
 * @date 15 October 2010
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <malloc.h>
#include <errno.h>
#include "List.h"
#include "pp_parser.h"
#include "borendian.h"

#if PP_TEST // using pp_test.c to test the preprocessor functionality; OpenBOR functionality is not available
#undef printf
#define openpackfile(fname, pname)	((int)fopen(fname, "rb"))
#define readpackfile(hnd, buf, len)	fread(buf, 1, len, (FILE*)hnd)
#define seekpackfile(hnd, loc, md)	fseek((FILE*)hnd, loc, md)
#define tellpackfile(hnd)			ftell((FILE*)hnd)
#define closepackfile(hnd)			fclose((FILE*)hnd)
#define printf(msg, args...)		fprintf(stderr, msg, ##args)
#define shutdown(ret, msg, args...) { fprintf(stderr, msg, ##args); exit(ret); }
#else // otherwise, we can use OpenBOR functionality like tracemalloc and writeToLogFile
#include "openbor.h"
#include "globals.h"
#include "packfile.h"
#define tellpackfile(hnd)			seekpackfile(hnd, 0, SEEK_CUR)
#endif

/**
 * Initializes a preprocessor context.  Assumes that this context either hasn't
 * been initialized yet or has been destroyed since the last time it was initialized.
 */
void pp_context_init(pp_context* self)
{
	// initialize the macro lists
	List_Init(&self->macros);
	List_Init(&self->func_macros);

	// initialize the import list
	List_Init(&self->imports);

	// initialize the conditional stack
	self->conditionals.all = 0;
	self->num_conditionals = 0;
}

/**
 * Frees the memory associated with a preprocessor context.  This function can
 * safely be called multiple times on the same context with no negative
 * consequences.  However, it does assume that the context has been initialized
 * at least once.
 */
void pp_context_destroy(pp_context* self)
{
	// undefine and free all non-function macros
	List_Reset(&self->macros);
	while(self->macros.size > 0)
	{
		free(List_Retrieve(&self->macros));
		List_Remove(&self->macros);
	}
	List_Clear(&self->macros);

	// undefine and free all function-style macros
	List_Reset(&self->func_macros);
	while(self->func_macros.size > 0)
	{
		List* params = List_Retrieve(&self->func_macros);
		while(params->size > 0)
		{
			free(List_Retrieve(params));
			List_Remove(params);
		}
		List_Clear(params);
		free(params);
		List_Remove(&self->func_macros);
	}
	List_Clear(&self->func_macros);

	// free the import list
	List_Reset(&self->imports);
	while(self->imports.size > 0)
		List_Remove(&self->imports);
	List_Clear(&self->imports);
}

/**
 * Initializes a preprocessor parser (pp_parser) object.
 * @param self the object
 * @param ctx the shared context used by this parser
 * @param filename the name of the file to parse
 * @param sourceCode the source code to parse, in string form
 */
void pp_parser_init(pp_parser* self, pp_context* ctx, const char* filename, char* sourceCode, TEXTPOS initialPosition)
{
	pp_lexer_Init(&self->lexer, sourceCode, initialPosition);
	self->ctx = ctx;
	self->filename = filename;
	self->sourceCode = sourceCode;

	self->type = PP_ROOT;
	self->freeFilename = false;
	self->freeSourceCode = false;
	self->parent = NULL;
	self->child = NULL;
	self->numParams = 0;
	self->newline = true;
	self->overread = false;
}

/**
 * Allocates a subparser, used for including files and expanding macros.
 * @param parent the parent parser of this subparser
 * @param filename the name of the file to parse
 * @param sourceCode the source code to parse, in string form
 */
pp_parser* pp_parser_alloc(pp_parser* parent, const char* filename, char* sourceCode, pp_parser_type type)
{
	pp_parser* self = malloc(sizeof(pp_parser));
	TEXTPOS initialPos = {1, 0};

	pp_parser_init(self, parent->ctx, filename, sourceCode, initialPos);
	self->type = type;
	self->parent = parent;
	parent->child = self;

	return self;
}

/**
 * Allocates a subparser to expand a macro.  This is a convenience constructor.
 * @param self the object
 * @param parent the parent parser of
 * @param numParams number of macros to free from the start of the macro list
 *        when freeing this parser (0 for non-function macros)
 * @return the newly allocated parser
 */
pp_parser* pp_parser_alloc_macro(pp_parser* parent, char* macroContents, int numParams, pp_parser_type type)
{
	pp_parser* self = pp_parser_alloc(parent, parent->filename, macroContents, type);
	self->numParams = numParams;
	return self;
}

/**
 * Prints an error or warning message to the log file.
 * @param messageType the type of message ("error" or "warning")
 * @param message the actual message to display
 */
void pp_message(pp_parser* self, char* messageType, char* message)
{
	pp_parser* parser = self;
	char buf[1024] = {""}, *linePtr;
	int bufPos, i;

	printf("\n\n");

	// print a backtrace of #includes to make it clear exactly where this error/warning is occurring
	while(parser->parent) parser = parser->parent;
	while(parser->child && parser->child->type == PP_INCLUDE)
	{
		printf("In file included from %s, line %d:\n", parser->filename, parser->lexer.theTokenPosition.row);
		parser = parser->child;
	}

	// print the error/warning and its location
	printf("Script %s: %s, line %d: %s\n", messageType, parser->filename, parser->lexer.theTokenPosition.row, message);

	// find the start of the line that the error occurred on
	linePtr = parser->lexer.pcurChar - strlen(parser->lexer.theTokenSource);
	while(linePtr-- > parser->lexer.ptheSource)
		if(*linePtr == '\n' || *linePtr == '\r' || *linePtr == '\f') break;
	linePtr++;

	// write the line that the error/warning occurred on to the log file
	bufPos = 0;
	while(*linePtr != '\n' && *linePtr != '\r' && *linePtr != '\f' && *linePtr != '\0')
	{
		buf[bufPos++] = *linePtr++;
		if(bufPos >= sizeof(buf)-1) break;
	}
	buf[bufPos] = '\0';
	printf("\n%s\n", buf);

	// print a position marker to show where in the line the error/warning occurred, just for completeness :)
	for(i=0; i<parser->token.theTextPosition.col; i++)
		printf(" ");
	printf("^\n\n");
}

/**
 * Writes an error message to the log.
 * @return E_FAIL
 */
HRESULT pp_error(pp_parser* self, char* format, ...)
{
	char buf[1024] = {""};
	va_list arglist;

	va_start(arglist, format);
	vsprintf(buf, format, arglist);
	va_end(arglist);
	pp_message(self, "error", buf);

	return E_FAIL;
}

/**
 * Writes a warning message to the log.
 */
void pp_warning(pp_parser* self, char* format, ...)
{
	char buf[1024] = {""};
	va_list arglist;

	va_start(arglist, format);
	vsprintf(buf, format, arglist);
	va_end(arglist);
	pp_message(self, "warning", buf);
}

/**
 * Gets the next parsable token from the lexer.
 * @param skip_whitespace true to ignore whitespace, false otherwise
 */
HRESULT pp_parser_lex_token(pp_parser* self, bool skip_whitespace)
{
	bool success = true;

	while(success)
	{
		if(self->overread)
		{
			memcpy(&self->token, &self->last_token, sizeof(pp_token));
			self->overread = false;
			success = true;
		}
		else
			success = SUCCEEDED(pp_lexer_GetNextToken(&self->lexer, &self->token));

		if(!success) return E_FAIL;

		if(skip_whitespace && self->token.theType == PP_TOKEN_WHITESPACE) continue;
		else break;
	}

	memcpy(&self->last_token, &self->token, sizeof(pp_token));
	return S_OK;
}

/**
 * Gets the next parsable token from the lexer, using a token from a parent
 * lexer if necessary.  This is useful for expanding macros.
 * @param skip_whitespace true to ignore whitespace, false otherwise
 */
HRESULT pp_parser_lex_token_essential(pp_parser* self, bool skip_whitespace)
{
	pp_parser* parser = self;

	while(1)
	{
		if(FAILED(pp_parser_lex_token(parser, skip_whitespace)))
			return E_FAIL;

		if(parser->token.theType == PP_TOKEN_EOF && parser->parent)
		{
			parser->overread = true;
			parser = parser->parent;
			continue;
		}
		else break;
	}

	if(parser != self)
		memcpy(&self->token, &parser->token, sizeof(pp_token));

	return S_OK;
}

/**
 * Preprocesses the source file until it reaches a token that should be emitted.
 * @return the next token of the output
 */
pp_token* pp_parser_emit_token(pp_parser* self)
{
	bool emitme = false;
	bool success = true;
	pp_token token2, token3;
	pp_token* child_token;
	int i;
	char* param1, *param2;

	while(success && !emitme)
	{
		// get token from subparser ("child") if one exists
		if(self->child)
		{
			child_token = pp_parser_emit_token(self->child);

			if(child_token == NULL || child_token->theType == PP_TOKEN_EOF)
			{
				// free the parameters of function macros
				List_Reset(&self->ctx->macros);
				for(i=0; i<self->child->numParams; i++)
				{
					// the string is allocated by strdup(), so use free() instead of free()
					free(List_Retrieve(&self->ctx->macros));
					List_Remove(&self->ctx->macros);
				}

				// free the source code and filename if necessary
				if(self->child->freeFilename) free((void*)self->child->filename);
				if(self->child->freeSourceCode) free(self->child->sourceCode);

				free(self->child);
				self->child = NULL;

				if(child_token == NULL) return NULL;
				else continue;
			}
			else
			{
				memcpy(&self->token, child_token, sizeof(pp_token));
				emitme = true;
			}
		}

		if(emitme) // re-process tokens emitted by the subparser
			emitme = false;
		else // lex the next token if no token is obtained from the subparser
			if(FAILED(pp_parser_lex_token(self, false))) break;

		if(self->token.theType == PP_TOKEN_DIRECTIVE ||
		   !(self->ctx->conditionals.top == cs_false || self->ctx->conditionals.top == cs_done))
		{
			// handle token concatenation
			if(self->type == PP_FUNCTION_MACRO || self->type == PP_NORMAL_MACRO)
			{
				bool whitespace = false;

				memcpy(&token2, &self->token, sizeof(pp_token));
				success = SUCCEEDED(pp_parser_lex_token(self, false));
				while(success && self->token.theType == PP_TOKEN_WHITESPACE)
				{
					whitespace = true;
					memcpy(&token3, &self->token, sizeof(pp_token));
					success = SUCCEEDED(pp_parser_lex_token(self, false));
				}
				if(!success) break;

				if(self->token.theType == PP_TOKEN_CONCATENATE)
				{
					memcpy(&token3, &self->token, sizeof(pp_token));
					success = SUCCEEDED(pp_parser_lex_token(self, true));
					if(!success) break;

					if(self->token.theType == PP_TOKEN_EOF)
					{
						pp_error(self, "'##' at end of macro expansion");
						return NULL;
					}

					if(List_FindByName(&self->ctx->macros, token2.theSource) &&
						(List_GetIndex(&self->ctx->macros) < self->numParams))
					{
						param1 = (char*)List_Retrieve(&self->ctx->macros);
					}
					else
						param1 = token2.theSource;

					if(List_FindByName(&self->ctx->macros, self->token.theSource) &&
						(List_GetIndex(&self->ctx->macros) < self->numParams))
					{
						param2 = (char*)List_Retrieve(&self->ctx->macros);
					}
					else
						param2 = self->token.theSource;

					pp_parser_concatenate(self, param1, param2);
					emitme = false;
					continue;
				}
				else // oops, the next token is not a ##
				{
					if(whitespace && token2.theType != PP_TOKEN_DIRECTIVE)
					{
						// this is a silly and convoluted way to do this, but it works...
						pp_parser_concatenate(self, token2.theSource, token3.theSource);
						self->overread = true;
						continue;
					}
					else
					{
						memcpy(&self->token, &token2, sizeof(pp_token));
						self->overread = true;
					}
				}
			}

			switch(self->token.theType)
			{
				case PP_TOKEN_DIRECTIVE:
					if(self->type == PP_FUNCTION_MACRO)
					{
						success = SUCCEEDED(pp_parser_lex_token(self, true));
						if(!success) break;

						if(self->token.theType == PP_TOKEN_IDENTIFIER &&
							List_FindByName(&self->ctx->macros, self->token.theSource))
						{
							if(FAILED(pp_parser_stringify(self))) return NULL;
							emitme = true;
							break;
						}
						else
							self->overread = true;
					}
					else if(self->newline)
					{ /* only parse the "#" symbol when it's at the beginning of a
					   * line (ignoring whitespace) */
						if(FAILED(pp_parser_parse_directive(self))) return NULL;
						break;
					}

					// if none of the above cases are true, emit the token
					emitme = true;
					break;
				case PP_TOKEN_NEWLINE:
					emitme = true;
					self->newline = true;
					break;
				case PP_TOKEN_WHITESPACE:
					emitme = true;
					// whitespace doesn't affect the newline property
					break;
				case PP_TOKEN_IDENTIFIER:
					memcpy(&token2, &self->token, sizeof(pp_token));
					success = SUCCEEDED(pp_parser_lex_token(self, false));
					if(!success) break;

					if(self->token.theType == PP_TOKEN_LPAREN && List_FindByName(&self->ctx->func_macros, token2.theSource))
					{
						if(FAILED(pp_parser_insert_function_macro(self, token2.theSource))) return NULL;
						self->overread = false;
					}
					else if(List_FindByName(&self->ctx->macros, token2.theSource))
					{
						pp_parser_insert_macro(self, token2.theSource);
						self->overread = true;
					}
					else
					{
						memcpy(&self->token, &token2, sizeof(pp_token));
						emitme = true;
						self->overread = true;
					}
					break;
				default: // now includes EOF
					emitme = true;
					self->newline = false;
			}
		}
	}

	return success ? &self->token : NULL;
}

// self->token contains the first token of the macro/message if self->overread == true
HRESULT pp_parser_readline(pp_parser* self, char* buf, int bufsize)
{
	int total_length = 1;

	if(FAILED(pp_parser_lex_token(self, true))) return E_FAIL;

	while(1)
	{
		if(self->token.theType == PP_TOKEN_EOF)
		{
			self->overread = true;
			break;
		}
		else if(self->token.theType == PP_TOKEN_NEWLINE)
			break;

		if((total_length + strlen(self->token.theSource)) > bufsize)
		{
			// Prevent buffer overflow
			pp_error(self, "length of macro or message contents is too long; must be <= %i characters", bufsize);
			return E_FAIL;
		}

		strcat(buf, self->token.theSource);
		total_length += strlen(self->token.theSource);
		if(FAILED(pp_parser_lex_token(self, false))) return E_FAIL;
	}

	return S_OK;
}

/**
 * Implements the C "stringify" operator.
 */
HRESULT pp_parser_stringify(pp_parser* self)
{
	TEXTPOS lexerPosition = {1, 0};
	char* contents = (char*)List_Retrieve(&self->ctx->macros);
	pp_parser parser;
	pp_token* token;

	pp_token_Init(&self->token, PP_TOKEN_STRING_LITERAL, "\"",
		self->token.theTextPosition, 0);
	pp_parser_init(&parser, self->ctx, self->filename, contents, lexerPosition);

	while((token = pp_parser_emit_token(&parser)) && token->theType != PP_TOKEN_EOF)
	{
		char* source = token->theSource;
		bool in_string = false;
		while(*source)
		{
			if(*source == '"')
			{
				strncat(self->token.theSource, "\\\"", 2);
				in_string = !in_string;
			}
			else if(*source == '\\' && in_string)
				strncat(self->token.theSource, "\\\\", 2);
			else
				strncat(self->token.theSource, source, 1);

			if(strlen(self->token.theSource)+2 > MAX_TOKEN_LENGTH)
				return pp_error(self, "sequence is too long to stringify");

			source++;
		}
	}

	strncat(self->token.theSource, "\"", 1);
	return S_OK;
}

/**
 * Concatenates two tokens together, implementing the "##" operator.
 * @param token1 the contents of the first token
 * @param token2 the contents of the second token
 */
void pp_parser_concatenate(pp_parser* self, const char* token1, const char* token2)
{
	char* output = malloc(strlen(token1) + strlen(token2) + 1);
	pp_parser* outputParser;

	sprintf(output, "%s%s", token1, token2);
	outputParser = pp_parser_alloc(self, self->filename, output, PP_CONCATENATE);
	outputParser->freeSourceCode = true;
}

/**
 * Parses a C preprocessor directive.  When this function is called, the token
 * '#' has just been detected by the compiler.
 */
HRESULT pp_parser_parse_directive(pp_parser* self)
{
	if(FAILED(pp_parser_lex_token(self, true))) return E_FAIL;

	// most directives shouldn't be parsed if we're in the middle of a conditional false
	if(self->ctx->conditionals.top == cs_false || self->ctx->conditionals.top == cs_done)
	{
		if(self->token.theType != PP_TOKEN_ELIF &&
		   self->token.theType != PP_TOKEN_ELSE &&
		   self->token.theType != PP_TOKEN_ENDIF)
		{
			return S_OK;
		}
	}

	switch(self->token.theType)
	{
		case PP_TOKEN_INCLUDE:
		case PP_TOKEN_IMPORT:
		{
			char* filename;
			int type = self->token.theType;

			if(FAILED(pp_parser_lex_token(self, true))) return E_FAIL;

			if(self->token.theType != PP_TOKEN_STRING_LITERAL)
				return pp_error(self, "#include not followed by a path string");

			filename = self->token.theSource + 1; // trim first " mark
			filename[strlen(filename)-1] = '\0'; // trim last " mark


			if(type == PP_TOKEN_INCLUDE)
				return pp_parser_include(self, filename);
			else // PP_TOKEN_IMPORT
			{
				List_InsertAfter(&self->ctx->imports, NULL, filename);
				return S_OK;
			}
		}
		case PP_TOKEN_DEFINE:
		{
			char name[MAX_TOKEN_LENGTH];

			if(FAILED(pp_parser_lex_token(self, true))) return E_FAIL;
			if(self->token.theType != PP_TOKEN_IDENTIFIER)
			{
				// Macro must have at least a name
				return pp_error(self, "no macro name given in #define directive");
			}

			// Parse macro name and contents
			strcpy(name, self->token.theSource);
			return pp_parser_define(self, name);
		}
		case PP_TOKEN_UNDEF:
			if(FAILED(pp_parser_lex_token(self, true))) return E_FAIL;
			if(List_FindByName(&self->ctx->macros, self->token.theSource))
			{
				free(List_Retrieve(&self->ctx->macros));
				List_Remove(&self->ctx->macros);
			}
			if(List_FindByName(&self->ctx->func_macros, self->token.theSource))
			{
				List* params = List_Retrieve(&self->ctx->func_macros);
				while(params->size > 0)
				{
					free(List_Retrieve(params));
					List_Remove(params);
				}
				List_Clear(params);
				free(params);
				List_Remove(&self->ctx->func_macros);
			}

			break;
		case PP_TOKEN_IF:
		case PP_TOKEN_IFDEF:
		case PP_TOKEN_IFNDEF:
		case PP_TOKEN_ELIF:
		case PP_TOKEN_ELSE:
		case PP_TOKEN_ENDIF:
			return pp_parser_conditional(self, self->token.theType);
		case PP_TOKEN_WARNING:
		case PP_TOKEN_ERROR_TEXT:
		{
			char text[256] = {""};

			// "self->token" is about to be clobbered, so save whether this is a warning or error
			PP_TOKEN_TYPE msgType = self->token.theType;
			if(FAILED(pp_parser_readline(self, text, sizeof(text)))) return E_FAIL;

			if(msgType == PP_TOKEN_WARNING)
				pp_warning(self, "#warning %s", text);
			else
				return pp_error(self, "#error %s", text);
			break;
		}
		case PP_TOKEN_NEWLINE:
			// null directive - do nothing
			return S_OK;
		default:
			return pp_error(self, "unknown directive '#%s'", self->token.theSource);
	}

	return S_OK;
}

/**
 * Includes a source file specified with the #include directive.
 * @param filename the path to include
 */
HRESULT pp_parser_include(pp_parser* self, char* filename)
{
	char* buffer;
	int length;
	int bytesRead;
	int handle;
	pp_parser* includeParser;

	// Open the file
	handle = openpackfile(filename, packfile);
#ifdef PP_TEST
	if(!handle)
#else
	if(handle < 0)
#endif
	{
		return pp_error(self, "unable to open file '%s'", filename);
	}

	// Determine the file's size
	seekpackfile(handle, 0, SEEK_END);
	length = tellpackfile(handle);
	seekpackfile(handle, 0, SEEK_SET);

	// Allocate a buffer for the file's contents
	buffer = malloc(length + 1);
	memset(buffer, 0, length + 1);

	// Read the file into the buffer
	bytesRead = readpackfile(handle, buffer, length);
	closepackfile(handle);

	if(bytesRead != length)
	{
		free(buffer);
		return pp_error(self, "I/O error: %s", strerror(errno));
	}

	// Allocate a subparser for the included file
	includeParser = pp_parser_alloc(self, NAME(filename), buffer, PP_INCLUDE);
	includeParser->freeFilename = true;
	includeParser->freeSourceCode = true;

	return S_OK;
}

/**
 * Defines a macro specified with the #define directive.
 * The length of the contents is limited to MACRO_CONTENTS_SIZE (512) characters.
 * @param name the macro name
 */
HRESULT pp_parser_define(pp_parser* self, char* name)
{
	char* contents = NULL;
	bool is_function = false; // true if this is a function-style #define; false otherwise
	List* params = malloc(sizeof(List));

	List_Init(params);

	// emit a warning if the macro is already defined
	// note: this won't mess with function macro parameters since #define can't be used from inside a macro

	if(List_FindByName(&self->ctx->macros, name))
	{
		pp_warning(self, "'%s' redefined", name);
		free(List_Retrieve(&self->ctx->macros));
		List_Remove(&self->ctx->macros);
	}
	if(List_FindByName(&self->ctx->func_macros, name))
	{
		List* params = List_Retrieve(&self->ctx->func_macros);
		pp_warning(self, "'%s' redefined", name);
		while(params->size > 0)
		{
			free(List_Retrieve(params));
			List_Remove(params);
		}
		List_Clear(params);
		free(params);
		List_Remove(&self->ctx->func_macros);
	}

	// do NOT skip whitespace here - the '(' must come immediately after the name!
	if(FAILED(pp_parser_lex_token(self, false))) goto error;

	if(self->token.theType == PP_TOKEN_LPAREN) // function-style #define
	{
		is_function = true;
		if(FAILED(pp_parser_lex_token(self, true))) goto error;
		while(self->token.theType != PP_TOKEN_RPAREN)
		{
			switch(self->token.theType)
			{
				// All of the below types are technically valid macro names
				case PP_TOKEN_IDENTIFIER:
				case PP_TOKEN_INCLUDE:
				case PP_TOKEN_DEFINE:
				case PP_TOKEN_UNDEF:
				case PP_TOKEN_IFDEF:
				case PP_TOKEN_IFNDEF:
				case PP_TOKEN_ELIF:
				case PP_TOKEN_ENDIF:
				case PP_TOKEN_PRAGMA:
				case PP_TOKEN_WARNING:
				case PP_TOKEN_ERROR_TEXT:
					List_InsertAfter(params, NULL, self->token.theSource);
					if(FAILED(pp_parser_lex_token(self, true))) goto error;
					if(self->token.theType == PP_TOKEN_COMMA || self->token.theType == PP_TOKEN_RPAREN) break;
				default:
					return pp_error(self, "unexpected token '%s' in #define parameter list", self->token.theSource);
			}

			if(self->token.theType != PP_TOKEN_RPAREN) pp_parser_lex_token(self, true);
		}
		self->overread = false;
	}
	else self->overread = true;

	// Read macro contents
	contents = malloc(MACRO_CONTENTS_SIZE);
	contents[0] = '\0';
	if(FAILED(pp_parser_readline(self, contents, MACRO_CONTENTS_SIZE))) goto error;

	// Add macro to the correct list, either macros or func_macros
	if(is_function)
	{
		List_InsertAfter(params, contents, NULL);
		List_InsertAfter(&self->ctx->func_macros, params, name);
	}
	else
	{
		free(params);
		List_InsertAfter(&self->ctx->macros, contents, name);
	}

	return S_OK;

error:
	List_Reset(params);
	while(List_GetSize(params)) List_Remove(params);
	free(params);
	if(contents) free(contents);
	return E_FAIL;
}

/**
 * Handles conditional directives.
 * @param directive the type of conditional directive
 */
HRESULT pp_parser_conditional(pp_parser* self, PP_TOKEN_TYPE directive)
{
	switch(directive)
	{
		case PP_TOKEN_IF:
			return pp_error(self, "#if directive not supported");
		case PP_TOKEN_IFDEF:
		case PP_TOKEN_IFNDEF:
			if(self->ctx->num_conditionals++ > 16) return pp_error(self, "too many levels of nested conditional directives");
			self->ctx->conditionals.all <<= 2; // push a new conditional state onto the stack
			self->ctx->conditionals.top = pp_parser_eval_conditional(self, directive) ? cs_true : cs_false;
			break;
		case PP_TOKEN_ELIF:
			return pp_error(self, "#elif directive not supported");
#if 0
			if(self->ctx->conditionals.top == cs_done || self->ctx->conditionals.top == cs_true)
				self->ctx->conditionals.top = cs_done;
			else
				self->ctx->conditionals.top = pp_parser_eval_conditional(self, directive) ? cs_true : cs_false;
			break;
#endif
		case PP_TOKEN_ELSE:
			if(self->ctx->conditionals.top == cs_none) return pp_error(self, "stray #else");
			self->ctx->conditionals.top = (self->ctx->conditionals.top == cs_false) ? cs_true : cs_done;
			break;
		case PP_TOKEN_ENDIF:
			if(self->ctx->conditionals.top == cs_none || self->ctx->num_conditionals-- < 0) return pp_error(self, "stray #endif");
			self->ctx->conditionals.all >>= 2; // pop a conditional state from the stack
			break;
		default:
			return pp_error(self, "unknown conditional directive type (ID=%d)", directive);
	}

	return S_OK;
}

bool pp_parser_eval_conditional(pp_parser* self, PP_TOKEN_TYPE directive)
{
	// all directives have whitespace between the directive and the contents
	if(FAILED(pp_parser_lex_token(self, true))) return false; // we can deal with lexer errors later!

	switch(directive)
	{
		case PP_TOKEN_IFDEF:
			return List_FindByName(&self->ctx->macros, self->token.theSource);
		case PP_TOKEN_IFNDEF:
			return !List_FindByName(&self->ctx->macros, self->token.theSource);
		default:
			pp_warning(self, "unknown conditional directive (this is a bug; please report it)");
			return false;
	}
}

/**
 * Expands a regular (i.e. non-function) macro.
 * Pre: the macro is defined
 */
void pp_parser_insert_macro(pp_parser* self, char* name)
{
	// don't waste time searching under normal circumstances
	if(strcmp((char*)List_Retrieve(&self->ctx->macros), name))
		List_FindByName(&self->ctx->macros, name);

	pp_parser_alloc_macro(self, List_Retrieve(&self->ctx->macros), 0, PP_NORMAL_MACRO);
}

/**
 * Expands a macro.
 * Pre: the macro is defined in func_macros
 * Pre: the last token retrieved was a PP_TOKEN_LPAREN
 */
HRESULT pp_parser_insert_function_macro(pp_parser* self, char* name)
{
	int numParams, paramCount = 0, paramMacros = 0;
	List* params;
	char paramBuffer[1024] = "", *tail;

	// find macro and get number of parameters
	if(strcmp((char*)List_Retrieve(&self->ctx->func_macros), name))
		List_FindByName(&self->ctx->func_macros, name);
	params = List_Retrieve(&self->ctx->func_macros);
	numParams = List_GetSize(params) - 1;

	// read the parameter list and temporarily define a "simple" macro for each parameter
	List_Reset(params);
	List_Reset(&self->ctx->macros);
	do
	{
		if(FAILED(pp_parser_lex_token_essential(self, false))) return E_FAIL;

		switch(self->token.theType)
		{
			case PP_TOKEN_NEWLINE:
				if(paramBuffer[0] != '\0')
					return pp_error(self, "unexpected newline in parameter list for function '%s'", name);
			case PP_TOKEN_EOF:
				return pp_error(self, "unexpected end of file in parameter list for function '%s'", name);
			case PP_TOKEN_RPAREN:
			case PP_TOKEN_COMMA:
				if(!paramCount) break;

				// remove trailing whitespace
				tail = strchr(paramBuffer, '\0');
				while(--tail >= paramBuffer)
				{
					if(*tail == ' ' || *tail == '\t' || *tail == '\n')
						*tail = '\0';
					else break;
				}

				if(paramCount > numParams)
					return pp_error(self, "too many parameters to function '%s'", name);

				// no need to create a macro if passed variable name is the same as the parameter name
				if(strncmp(paramBuffer, List_GetName(params), sizeof(paramBuffer)) != 0)
				{
					// add the new macro to the beginning of the macro list
					paramMacros++;
					List_InsertBefore(&self->ctx->macros, strdup(paramBuffer), List_GetName(params));
				}

				List_GotoNext(params);
				paramBuffer[0] = '\0';
				break;
			case PP_TOKEN_WHITESPACE:
				if(paramBuffer[0] == '\0') break;
			default:
				if(paramBuffer[0] == '\0')
					paramCount++;

				if((strlen(paramBuffer) + strlen(self->token.theSource) + 1) > sizeof(paramBuffer))
					return pp_error(self, "parameter %d of function '%s' exceeds max length of %d characters", name, sizeof(paramBuffer)-1);

				strcat(paramBuffer, self->token.theSource);
		}
	} while(self->token.theType != PP_TOKEN_RPAREN);

	if(paramCount < numParams)
		return pp_error(self, "not enough parameters to function '%s'", name);

	// do the actual parsing
	List_GotoLast(params);
	pp_parser_alloc_macro(self, List_Retrieve(params), paramMacros, PP_FUNCTION_MACRO);

	return S_OK;
}

