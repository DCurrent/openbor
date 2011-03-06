#!/bin/bash

gcc -g -O0 -Wall pp_test.c ../pp_parser.c ../pp_lexer.c \
	../../scriptlib/List.c ../../tracelib/tracemalloc.c \
	-DPP_TEST \
	-I.. -I../.. -I../../scriptlib -I../../tracelib -I../../gamelib -I../../.. -I../../ramlib \
	-opp_test


