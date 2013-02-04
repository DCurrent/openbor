/**
 * Calculates the value of the constant integer expressions that are used
 * in #if and #elif directives in the C preprocessor.
 *
 * @author Plombo
 * @date 6 March 2012
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <assert.h>
#include "pp_parser.h"
#include "pp_expr.h"

char* get_full_path(char* filename) { return filename; }

// for testing infixparser and calculator
int main()
{
	char buffer[1024];
	pp_expr* expression;
	pp_context ctx;
	pp_parser parser;
	TEXTPOS startpos = {0,0};
	
	while(1)
	{
		// get the user's input (i.e. the expression to evaluate)
		printf(">> ");
		fgets(buffer, sizeof(buffer), stdin);
		
		// end the program if the user enters a blank line, a null char, or Ctrl+D
		if(buffer[0] == '\r' || buffer[0] == '\n' || buffer[0] == '\0')
			break;
		else if(feof(stdin))
		{
			printf("\n");
			break;
		}
		
		// parse the input and correct the parsed tree for operator precedence
		pp_context_init(&ctx);
		pp_parser_init(&parser, &ctx, "<stdin>", buffer, startpos);
		expression = pp_expr_parse(&parser, false);
		if(expression == NULL) { pp_context_destroy(&ctx); continue; }
		pp_expr_fix_precedence(expression);
		
		// evaluate the expression and display the result
		printf("%i\n", pp_expr_eval(expression));
		pp_expr_destroy(expression);
		pp_context_destroy(&ctx);
	}
	
	return 0;
}

