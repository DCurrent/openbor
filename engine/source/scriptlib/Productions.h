/*
 * OpenBOR - http://www.LavaLit.com
 * -----------------------------------------------------------------------
 * Licensed under the BSD license, see LICENSE in OpenBOR root for details.
 *
 * Copyright (c) 2004 - 2011 OpenBOR Team
 */

#ifndef PRODUCTION_H
#define PRODUCTION_H
#define COMPAREPRODUCTION( x ) if (!strcmp( #x, theProduction)) return x

typedef enum PRODUCTION {
   start,
   external_decl,
   decl_spec,
   decl,
   funcDecl,
   funcDecl1,
   initializer,
   parm_decl,
   param_list,
   param_list2,
   decl_list,
   decl_list2,
   stmt_list,
   stmt_list2,
   stmt,
   expr_stmt,
   comp_stmt,
   comp_stmt2,
   comp_stmt3,
   select_stmt,
   opt_else,
   iter_stmt,
   opt_expr_stmt,
   defer_expr_stmt,
   jump_stmt,
   opt_expr,
   expr,
   assignment_op,
   assignment_expr,
   assignment_expr2,
   const_expr,
   log_or_expr,
   log_or_expr2,
   log_and_expr,
   log_and_expr2,
   eq_op,
   equal_expr,
   equal_expr2,
   rel_op,
   rel_expr,
   rel_expr2,
   add_op,
   add_expr,
   add_expr2,
   mult_op,
   mult_expr,
   mult_expr2,
   unary_expr,
   postfix_expr,
   postfix_expr2,
   arg_expr_list,
   arg_expr_list2,
   primary_expr,
   constant,
   error,
}PRODUCTION;
#define    NUMPRODUCTIONS error

#endif

