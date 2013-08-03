/*
 * OpenBOR - http://www.LavaLit.com
 * -----------------------------------------------------------------------
 * All rights reserved, see LICENSE in OpenBOR root for details.
 *
 * Copyright (c) 2004 - 2013 OpenBOR Team
 */

/**
 * This is the parser for the script preprocessor.  Its purpose is to emit the
 * preprocessed source code for use by scriptlib.  It is not derived from the
 * parser in scriptlib because it does something entirely different.
 *
 * @author Plombo
 * @date 15 October 2010
 */

#ifndef PP_PARSER_H
#define PP_PARSER_H

#include "pp_lexer.h"
#include "List.h"
#include "types.h"

#define MACRO_CONTENTS_SIZE		512

/*
 * The current "state" of a single conditional (#ifdef/#else/#endif) sequence:
 * true, false, already completed (i.e. not eligible for #elif), or non-existent.
 */
enum conditional_state
{
    cs_none = 0,
    cs_true = 1,
    cs_false = 2,
    cs_done = 3
};

/**
 * Stack of conditional directives.  The preprocessor can handle up to 16 nested
 * conditionals.  The stack is implemented as a 32-bit integer.
 */
typedef union
{
    u64 all;
    struct
    {
        u64 top: 2;
        u64 others: 62;
    };
} conditional_stack;

typedef struct pp_context
{
    List macros;                       // list of currently defined non-function macros
    List func_macros;                  // list of currently defined function-style macros
    List imports;                      // list of files for the interpreter to "import"
    conditional_stack conditionals;    // the conditional stack
    int num_conditionals;              // current size of the conditional stack
} pp_context;

typedef enum
{
    PP_ROOT,
    PP_INCLUDE,
    PP_NORMAL_MACRO,
    PP_FUNCTION_MACRO,
    PP_CONCATENATE,
    PP_CONDITIONAL
} pp_parser_type;

typedef struct pp_parser
{
    pp_parser_type type;
    pp_context *ctx;
    pp_lexer lexer;
    const char *filename;
    char *sourceCode;
    int numParams;                     // parameter macros defined for a function macro parser
    char *macroName;
    bool freeFilename;
    bool freeSourceCode;
    pp_token token;
    pp_token last_token;
    struct pp_parser *parent;
    struct pp_parser *child;
    bool newline;
    bool overread;
} pp_parser;

void pp_context_init(pp_context *self);
void pp_context_destroy(pp_context *self);

void pp_parser_init(pp_parser *self, pp_context *ctx, const char *filename, char *sourceCode, TEXTPOS initialPosition);
pp_parser *pp_parser_alloc(pp_parser *parent, const char *filename, char *sourceCode, pp_parser_type type);
pp_parser *pp_parser_alloc_macro(pp_parser *parent, char *macroContents, int numParams, pp_parser_type type);

pp_token *pp_parser_emit_token(pp_parser *self);
HRESULT pp_parser_lex_token(pp_parser *self, bool skip_whitespace);
HRESULT pp_parser_readline(pp_parser *self, char *buf, int bufsize);
HRESULT pp_parser_stringify(pp_parser *self);
void pp_parser_concatenate(pp_parser *self, const char *token1, const char *token2);
HRESULT pp_parser_parse_directive(pp_parser *self);
HRESULT pp_parser_include(pp_parser *self, char *filename);
HRESULT pp_parser_define(pp_parser *self, char *name);
HRESULT pp_parser_conditional(pp_parser *self, PP_TOKEN_TYPE directive);
HRESULT pp_parser_eval_conditional(pp_parser *self, PP_TOKEN_TYPE directive, int *result);
void pp_parser_insert_macro(pp_parser *self, char *name);
HRESULT pp_parser_insert_function_macro(pp_parser *self, char *name);
bool pp_is_builtin_macro(const char *name);
void pp_parser_insert_builtin_macro(pp_parser *self, const char *name);
bool pp_parser_is_defined(pp_parser *self, const char *name);

HRESULT pp_error(pp_parser *self, char *format, ...);
void pp_warning(pp_parser *self, char *format, ...);

#endif

