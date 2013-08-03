/*
 * OpenBOR - http://www.LavaLit.com
 * -----------------------------------------------------------------------
 * All rights reserved, see LICENSE in OpenBOR root for details.
 *
 * Copyright (c) 2004 - 2013 OpenBOR Team
 */

/**
 * This is a parser and evaluator for the infix expressions that are used in
 * the #if and #elif preprocessor directives.
 *
 * About 50% written in March 2012, with the other 50% done in January 2013 and
 * some finishing touches done in February 2013.
 *
 * @author Plombo
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <stdbool.h>
#include <assert.h>
#include "pp_parser.h"
#include "pp_expr.h"

#undef printf
#define tokendisplay(tok) ((tok).theType == PP_TOKEN_EOF ? "E" : (tok).theSource)
#define FIXTREE_DEBUG 0
#define LEXTREE_DEBUG 0

/**
 * @return the type of a token - binary operator, unary operator, multipurpose
 *         operator (+ and - can be binary or unary), or operand (leaf).
 */
int tokentype(PP_TOKEN_TYPE id)
{
    switch(id)
    {
        // BITWISE_AND and MUL would be MULTI in full C, but we don't need pointer operations for preprocessing
    case PP_TOKEN_LEFT_OP:
    case PP_TOKEN_RIGHT_OP:
    case PP_TOKEN_LT:
    case PP_TOKEN_GT:
    case PP_TOKEN_LE_OP:
    case PP_TOKEN_GE_OP:
    case PP_TOKEN_EQ_OP:
    case PP_TOKEN_NE_OP:
    case PP_TOKEN_AND_OP:
    case PP_TOKEN_OR_OP:
    case PP_TOKEN_BITWISE_AND:
    case PP_TOKEN_BITWISE_OR:
    case PP_TOKEN_XOR:
    case PP_TOKEN_MUL:
    case PP_TOKEN_DIV:
    case PP_TOKEN_MOD:
        return BINARY;
    case PP_TOKEN_BOOLEAN_NOT:
    case PP_TOKEN_BITWISE_NOT:
    case PP_TOKEN_LPAREN:
    case PP_TOKEN_RPAREN:
    case PP_TOKEN_EOF:
        return UNARY;
    case PP_TOKEN_ADD:
    case PP_TOKEN_SUB:
        return MULTI;
    case PP_TOKEN_DEFINED: // see pp_expr_parse for how defined works as a "leaf"
    case PP_TOKEN_INTCONSTANT:
    case PP_TOKEN_HEXCONSTANT:
    case PP_TOKEN_IDENTIFIER:
    case PP_TOKEN_INCLUDE:
    case PP_TOKEN_IMPORT:
    case PP_TOKEN_DEFINE:
    case PP_TOKEN_UNDEF:
    case PP_TOKEN_PRAGMA:
    case PP_TOKEN_ELIF:
    case PP_TOKEN_IFDEF:
    case PP_TOKEN_IFNDEF:
    case PP_TOKEN_ENDIF:
    case PP_TOKEN_WARNING:
    case PP_TOKEN_ERROR_TEXT:
        return LEAF;
    default:
        return -1;
    }
}

/**
 * @return the precedence from 1 (lowest) to 10 (highest) of a binary operator,
 *         or 0 if the operation is not binary
 */
int precedence(PP_TOKEN_TYPE op)
{
    switch(op)
    {
    case PP_TOKEN_MUL:
    case PP_TOKEN_DIV:
    case PP_TOKEN_MOD:
        return 10;
    case PP_TOKEN_ADD:
    case PP_TOKEN_SUB:
        return 9;
    case PP_TOKEN_LEFT_OP:
    case PP_TOKEN_RIGHT_OP:
        return 8;
    case PP_TOKEN_LT:
    case PP_TOKEN_GT:
    case PP_TOKEN_LE_OP:
    case PP_TOKEN_GE_OP:
        return 7;
    case PP_TOKEN_EQ_OP:
    case PP_TOKEN_NE_OP:
        return 6;
    case PP_TOKEN_BITWISE_AND:
        return 5;
    case PP_TOKEN_XOR:
        return 4;
    case PP_TOKEN_BITWISE_OR:
        return 3;
    case PP_TOKEN_AND_OP:
        return 2;
    case PP_TOKEN_OR_OP:
        return 1;
    default:
        return 0;
    }
}

/**
 * Initializes the fields of a tree.
 * @param self the tree
 * @param info the node
 * @param left the left subtree
 * @param right the right subtree
 */
void pp_expr_init(pp_expr *self, nodedata *info, pp_expr *left, pp_expr *right)
{
    self->info = info;
    self->left = left;
    self->right = right;
}

/**
 * Recursively frees a pp_expr node and its children.
 */
void pp_expr_destroy(pp_expr *node)
{
    if(node->info)
    {
        free(node->info);
    }
    if(node->left)
    {
        pp_expr_destroy(node->left);
    }
    if(node->right)
    {
        pp_expr_destroy(node->right);
    }
    free(node);
}

/**
 * Rearranges a parsed tree so that the operations will be done in order of
 * precedence.
 */
void pp_expr_fix_precedence(pp_expr *self)
{
#if FIXTREE_DEBUG
    static int iteration = 0;
    ++iteration;

    printf("Iteration %i\n", iteration);
    tree_display2(self);
    printf("\n");
#endif

    if(self->info->type == LEAF)
    {
        assert(!self->left && !self->right);
    }
    else if(self->info->type == UNARY)
    {
        assert(self->left && !self->right);
        pp_expr_fix_precedence(self->left);
    }
    else
    {
        assert(self->info->type == BINARY);
        assert(self->left && self->right);
        pp_expr_fix_precedence(self->right);
        if(self->right->info->type == BINARY &&
                precedence(self->info->theToken.theType) >= precedence(self->right->info->theToken.theType))
        {
            pp_expr *ll = self->left->left;
            pp_expr *r = self->right;
#if FIXTREE_DEBUG
            printf("Swap %s with %s (iteration %i)\n", tokendisplay(self->info->theToken), tokendisplay(self->right->info->theToken), iteration);
#endif
            self->left->left = malloc(sizeof(pp_expr));
            pp_expr_init(self->left->left, self->left->info, ll ? ll : NULL, self->left->right);
            pp_expr_init(self->left, self->info, self->left->left, self->right->left);
            pp_expr_init(self, self->right->info, self->left, self->right->right);
            free(r);
        }
        pp_expr_fix_precedence(self->left);
    }

#if FIXTREE_DEBUG
    printf("return from %i\n", iteration);
    --iteration;
#endif
}

/**
 * Constructs an abstract syntax tree from the rest of the remaining line of
 * the parser.  The returned pointer should be freed with pp_expr_destroy()
 * when it's no longer needed.
 *
 * @param parser the parser to get the token stream from
 * @param paren true if parsing a parenthetical expression, false otherwise
 * @return the tree constructed from the input, or NULL on error
 */
pp_expr *pp_expr_parse(pp_parser *parser, bool paren)
{
    pp_expr *root = malloc(sizeof(pp_expr));
    pp_expr *current = root;
    pp_expr *previous = NULL;
    pp_expr *leftleaf = NULL;
    nodedata *currentdata = NULL;
    pp_token *token;

    memset(root, 0, sizeof(pp_expr));

    do
    {
        int type;
        token = pp_parser_emit_token(parser);
        type = tokentype(token->theType);

        // determine whether a MULTI operator is being used as unary or binary
        if(type == MULTI)
        {
            if(!leftleaf)
            {
                type = UNARY;
            }
            else
            {
                pp_expr *bottomnode = leftleaf;

                while(bottomnode && bottomnode->left)
                {
                    bottomnode = bottomnode->left;
                }
                type = (bottomnode->info->type == LEAF) ? BINARY : UNARY;
            }
        }

        currentdata = malloc(sizeof(nodedata));
        memcpy(&currentdata->theToken, token, sizeof(pp_token));
        currentdata->type = type;

        if(token->theType == PP_TOKEN_EOF || token->theType == PP_TOKEN_RPAREN)
        {
            if (token->theType == PP_TOKEN_RPAREN && (!paren || !leftleaf))
            {
                if(paren)
                {
                    pp_error(parser, "no expression between '(' and ')'");
                }
                else
                {
                    pp_error(parser, "')' without matching '('");
                }
                goto error;
            }
            else if (token->theType == PP_TOKEN_EOF && (paren || !leftleaf))
            {
                if(paren)
                {
                    pp_error(parser, "expected ')' before end of conditional expression");
                }
                else
                {
                    pp_error(parser, "#if with no expression");
                }
                goto error;
            }

            pp_expr_init(current, currentdata, leftleaf, NULL);
            if(previous)
            {
                previous->right = current;
            }
            currentdata = NULL;
            break;
        }
        else if(token->theType == PP_TOKEN_WHITESPACE ||
                token->theType == PP_TOKEN_NEWLINE) // skip over whitespace
        {
            free(currentdata);
            currentdata = NULL;
        }
        else if(type == LEAF || type == UNARY)
        {
            pp_expr *treedata;
            pp_expr *bottomnode = leftleaf;
            pp_expr *subtree = NULL;

            // find the bottom of the left tree (there can be any number of unary operators stacked there)
            if(leftleaf)
                while(bottomnode->left)
                {
                    bottomnode = bottomnode->left;
                }

            if(leftleaf && bottomnode->info->type != UNARY)
            {
                pp_error(parser, "expected an operator, got '%s'", token->theSource);
                goto error;
            }

            if(token->theType == PP_TOKEN_LPAREN)
            {
                subtree = pp_expr_parse(parser, true);
                if(subtree == NULL)
                {
                    goto error;
                }
            }
            else if(token->theType == PP_TOKEN_DEFINED)
            {
                int lparen = 0;
                pp_parser_lex_token(parser, true);
                if(parser->token.theType == PP_TOKEN_LPAREN)
                {
                    lparen = 1;
                    pp_parser_lex_token(parser, true);
                }
                if(parser->token.theType != PP_TOKEN_IDENTIFIER)
                {
                    pp_error(parser, "bad 'defined' syntax");
                    goto error;
                }
                currentdata->theToken.theType = PP_TOKEN_INTCONSTANT;
                strcpy(currentdata->theToken.theSource,
                       pp_parser_is_defined(parser, parser->token.theSource) ? "1" : "0");
                if(lparen)
                {
                    pp_parser_lex_token(parser, true);
                    if(parser->token.theType != PP_TOKEN_RPAREN)
                    {
                        pp_error(parser, "bad 'defined' syntax");
                        goto error;
                    }
                }
            }

            treedata = malloc(sizeof(pp_expr));
            pp_expr_init(treedata, currentdata, subtree, NULL);
            currentdata = NULL;

            // insert at the bottom of the left tree
            if(leftleaf)
            {
                bottomnode->left = treedata;
            }
            else
            {
                leftleaf = treedata;
            }
        }
        else if(type == BINARY)
        {
            if(!leftleaf)
            {
                pp_error(parser, "expected an operand, got '%s'", token->theSource);
                goto error;
            }

            pp_expr_init(current, currentdata, leftleaf, NULL);
            leftleaf = NULL;
            currentdata = NULL;

            if(previous)
            {
                previous->right = current;
            }

            previous = current;
            current = malloc(sizeof(pp_expr));

#if LEXTREE_DEBUG
            printf("%s %s\n", previous->left->info->theToken.theSource, token->theSource);
#endif
        }
        else
        {
            pp_error(parser, "'%s' is not a valid token in an #if/#elif condition\n",
                     token->theSource);
            goto error;
        }
    }
    while(1);

    return root;

error:
    if(root->info)
    {
        pp_expr_destroy(root);
    }
    if(leftleaf)
    {
        pp_expr_destroy(leftleaf);
    }
    if(current != root)
    {
        free(current);
    }
    if(currentdata)
    {
        free(currentdata);
    }
    return NULL;
}

int pp_expr_eval(pp_expr *self)
{
    int left, right;

    switch(self->info->type)
    {
    case LEAF:
        switch(self->info->theToken.theType)
        {
        case PP_TOKEN_INTCONSTANT:
            if(self->info->theToken.theSource[0] == '0') // octal
            {
                return strtol(self->info->theToken.theSource, NULL, 8);
            }
            else
            {
                return strtol(self->info->theToken.theSource, NULL, 10);
            }
        case PP_TOKEN_HEXCONSTANT:
            assert(!strncmp(self->info->theToken.theSource, "0x", 2) ||
                   !strncmp(self->info->theToken.theSource, "0X", 2));
            return strtol(self->info->theToken.theSource + 2, NULL, 16);
        case PP_TOKEN_IDENTIFIER:
        case PP_TOKEN_INCLUDE:
        case PP_TOKEN_IMPORT:
        case PP_TOKEN_DEFINE:
        case PP_TOKEN_UNDEF:
        case PP_TOKEN_PRAGMA:
        case PP_TOKEN_ELIF:
        case PP_TOKEN_IFDEF:
        case PP_TOKEN_IFNDEF:
        case PP_TOKEN_ENDIF:
        case PP_TOKEN_WARNING:
        case PP_TOKEN_ERROR_TEXT:
            // identifiers here indicate undefined macros and evaluate to 0
            return 0;
        default:
            assert(!"invalid token type for leaf node");
        }
        break;
    case UNARY:
        assert(self->left);
        left = pp_expr_eval(self->left);
        switch(self->info->theToken.theType)
        {
        case PP_TOKEN_EOF:
        case PP_TOKEN_LPAREN:
        case PP_TOKEN_RPAREN:
        case PP_TOKEN_ADD:
            return left;
        case PP_TOKEN_SUB:
            return -left;
        case PP_TOKEN_BOOLEAN_NOT:
            return !left;
        case PP_TOKEN_BITWISE_NOT:
            return ~left;
        default:
            assert(!"invalid token type for unary operator");
        }
        break;
    case BINARY:
        assert(self->left && self->right);
        left = pp_expr_eval(self->left);
        right = pp_expr_eval(self->right);
        switch(self->info->theToken.theType)
        {
        case PP_TOKEN_MUL:
            return left * right;
        case PP_TOKEN_DIV:
            return left / right;
        case PP_TOKEN_MOD:
            return left % right;
        case PP_TOKEN_ADD:
            return left + right;
        case PP_TOKEN_SUB:
            return left - right;
        case PP_TOKEN_LEFT_OP:
            return left << right;
        case PP_TOKEN_RIGHT_OP:
            return left >> right;
        case PP_TOKEN_LT:
            return left < right;
        case PP_TOKEN_GT:
            return left > right;
        case PP_TOKEN_LE_OP:
            return left <= right;
        case PP_TOKEN_GE_OP:
            return left >= right;
        case PP_TOKEN_EQ_OP:
            return left == right;
        case PP_TOKEN_NE_OP:
            return left != right;
        case PP_TOKEN_BITWISE_AND:
            return left & right;
        case PP_TOKEN_XOR:
            return left ^ right;
        case PP_TOKEN_BITWISE_OR:
            return left | right;
        case PP_TOKEN_AND_OP:
            return left && right;
        case PP_TOKEN_OR_OP:
            return left || right;
        default:
            assert(!"invalid token type for binary operator");
        }
        break;
    case MULTI:
    default:
        assert(!"invalid node type");
    }

    assert(!"should not reach end of function");
    return 0;
}

/**
 * Parses an #if/#elif boolean expression from the parser and evaluates it.
 * @param lexer a lexer where the next token is the start of the expression
 * @return 1 if result is nonzero, 0 if result is zero, -1 on error
 */
int pp_expr_eval_expression(pp_parser *parser)
{
    pp_expr *expression = pp_expr_parse(parser, false);
    if(expression == NULL)
    {
        return -1;
    }
    pp_expr_fix_precedence(expression);
    return pp_expr_eval(expression) ? 1 : 0;
}

