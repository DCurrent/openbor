#ifndef PP_EXPR_H
#define PP_EXPR_H

#include <stdbool.h>
#include "pp_parser.h"
#undef printf

typedef struct {
	pp_token theToken;
	enum {BINARY, UNARY, MULTI, LEAF} type; // MULTI refers to operators that can be unary or binary (+, -)
} nodedata;

// binary tree structure used for parsing and evaulating expressions
typedef struct node {
	nodedata* info;
	struct node* left;
	struct node* right;
} pp_expr;

pp_expr* pp_expr_parse(pp_parser* parser, bool paren);
void pp_expr_fix_precedence(pp_expr* self);
void pp_expr_destroy(pp_expr* node);
int pp_expr_eval(pp_expr* self);
int pp_expr_eval_expression(pp_parser* parser);

#endif

