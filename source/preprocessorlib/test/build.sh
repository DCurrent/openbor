#!/bin/bash

if [ `basename "$PWD"` != 'test' ]; then cd test; fi

gcc -g -O0 -Wall pp_test.c \
	../pp_expr.c ../pp_lexer.c ../pp_parser.c ../../scriptlib/List.c \
	-DPP_TEST \
	-I.. -I../.. -I../../scriptlib -I../../tracelib -I../../gamelib -I../.. -I../../ramlib \
	-o../pp_test || exit 1

gcc -g -O0 -Wall infixparser.c \
	../pp_expr.c ../pp_lexer.c ../pp_parser.c ../../scriptlib/List.c \
	-DPP_TEST \
	-I.. -I../.. -I../../scriptlib -I../../tracelib -I../../gamelib -I../.. -I../../ramlib \
	-o../infixparser || exit 1

gcc -g -O0 -Wall calculator.c \
	../pp_expr.c ../pp_lexer.c ../pp_parser.c ../../scriptlib/List.c \
	-DPP_TEST \
	-I.. -I../.. -I../../scriptlib -I../../tracelib -I../../gamelib -I../../.. -I../../ramlib \
	-o../calculator || exit 1

gcc -g -O0 -Wall tokendiff.c \
	../pp_lexer.c ../../scriptlib/List.c \
	-DPP_TEST \
	-I.. -I../.. -I../../scriptlib -I../../tracelib -I../../gamelib -I../.. -I../../ramlib \
	-o../tokendiff || exit 1

