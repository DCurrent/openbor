/**
 * Early test used for debugging the expression parser.
 *
 * @author Plombo
 * @date 5 March 2012
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include "pp_parser.h"
#include "pp_expr.h"

#define tokendisplay(tok) ((tok).theType == PP_TOKEN_EOF ? "E" : (tok).theSource)

// text representation of a tree for debugging purposes
void pp_expr_display(pp_expr* self)
{
	if(self->info->type == UNARY && self->info->theToken.theType != PP_TOKEN_RPAREN)
		printf("%s", self->info->theToken.theSource);
	if(self->left) pp_expr_display(self->left);
	if(self->info->type != UNARY || self->info->theToken.theType == PP_TOKEN_RPAREN)
		printf("%s", self->info->theToken.theSource);
	if(self->right) pp_expr_display(self->right);
}

// another text representation of a tree for debugging purposes
void pp_expr_display2(pp_expr* self)
{
	if(self->info->type == BINARY)
	{
		printf("%s: %s %s\n", tokendisplay(self->info->theToken), tokendisplay(self->left->info->theToken), tokendisplay(self->right->info->theToken));
		pp_expr_display2(self->left);
		pp_expr_display2(self->right);
	}
	else if(self->info->type == UNARY)
	{
		printf("%s: %s\n", tokendisplay(self->info->theToken), tokendisplay(self->left->info->theToken));
		pp_expr_display2(self->left);
	}
	else
		printf("%s\n", tokendisplay(self->info->theToken));
}

char* get_full_path(char* filename) { return filename; }

int main()
{
	//pp_lexer lexer;
	pp_expr* expression;
	pp_context ctx;
	pp_parser parser;
	TEXTPOS startPos = {0,0};
	
	//pp_lexer_Init(&lexer, "a*(b+c)/2 + 2", startPos);
	//pp_lexer_Init(&lexer, "a*b+c", startPos);
	//pp_lexer_Init(&lexer, "a*(b+c)", startPos);
	//pp_lexer_Init(&lexer, "~a*+(b+!-c)/2", startPos);
	//pp_lexer_Init(&lexer, "48/4/3/2", startPos);
	//pp_lexer_Init(&lexer, "48/4/3/2/1", startPos);
	//pp_lexer_Init(&lexer, "~10 + 17", startPos);
	pp_context_init(&ctx);
	//pp_parser_init(&parser, &ctx, "infixparser main", "~10 + 17", startPos);
	pp_parser_init(&parser, &ctx, "infixparser main", "(3)", startPos);
	
	expression = pp_expr_parse(&parser, false);
	if(expression == NULL) return 1;
	
	pp_expr_display(expression);
	printf("\n\n");
	pp_expr_display2(expression);
	printf("\n");
	
	pp_expr_fix_precedence(expression);
	pp_expr_display(expression);
	printf("\n\n");
	pp_expr_display2(expression);
	printf("\n");
	
	printf("Expression evaluates to %i\n", pp_expr_eval(expression));
	
	pp_expr_destroy(expression);
	pp_context_destroy(&ctx);
	
	return 0;
}

