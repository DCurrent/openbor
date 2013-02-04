// Quick program to test the preprocessor.
// Compile using build.sh.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "pp_lexer.h"
#undef printf

bool lexFiles(char* filename, char* filename2)
{
	int length;
	char* buffer, *buffer2;
	FILE* fp;
	pp_lexer lexer, lexer2;
	pp_token token, token2;
	TEXTPOS position = {0,0};

	// Open the first file and read it into a memory buffer
	fp = fopen(filename, "rb");
	if(fp == NULL) return false;
	fseek(fp, 0, SEEK_END);
	length = ftell(fp);
	fseek(fp, 0, SEEK_SET);
	fprintf(stderr, "Length of %s is %i\n", filename, length);

	buffer = malloc(length + 1);
	memset(buffer, 0, length + 1);
	if(fread(buffer, 1, length, fp) != length) return false;
	fclose(fp);
	pp_lexer_Init(&lexer2, buffer, position);

	// Open the second file and read it into a memory buffer
	fp = fopen(filename2, "rb");
	if(fp == NULL) return false;
	fseek(fp, 0, SEEK_END);
	length = ftell(fp);
	fseek(fp, 0, SEEK_SET);
	fprintf(stderr, "Length of %s is %i\n", filename2, length);
	
	buffer2 = malloc(length + 1);
	memset(buffer2, 0, length + 1);
	if(fread(buffer2, 1, length, fp) != length) return false;
	fclose(fp);
	pp_lexer_Init(&lexer, buffer2, position);

	do {
		do {
			if(FAILED(pp_lexer_GetNextToken(&lexer, &token)))  { fprintf(stderr, "Lexical error (file 1)\n"); return false; }
		} while(token.theType == PP_TOKEN_WHITESPACE || token.theType == PP_TOKEN_NEWLINE);
		do {
			if(FAILED(pp_lexer_GetNextToken(&lexer2, &token2))) { fprintf(stderr, "Lexical error (file 2)\n"); return false; }
		} while(token2.theType == PP_TOKEN_WHITESPACE || token2.theType == PP_TOKEN_NEWLINE);
		if(token.theType != token2.theType || strcmp(token.theSource, token2.theSource))
		{
			printf("Different tokens:\n");
			printf("\tFile 1, line %4d col %3d: %s\n", token.theTextPosition.row+1, token.theTextPosition.col+1, token.theSource);
			printf("\tFile 2, line %4d col %3d: %s\n", token2.theTextPosition.row+1, token2.theTextPosition.col+1, token2.theSource);
			pp_lexer_Clear(&lexer);
			pp_lexer_Clear(&lexer2);
			free(buffer);
			free(buffer2);
			return true;
		}
		//printf("%s", token.theSource);
	} while(token.theType != PP_TOKEN_EOF && token2.theType != PP_TOKEN_EOF);

	pp_lexer_Clear(&lexer);
	pp_lexer_Clear(&lexer2);
	free(buffer);
	free(buffer2);

	printf("Files are identical\n");
	return true;
}

int main(int argc, char** argv)
{
	if(argc != 3)
	{
		printf("Usage: %s filename filename2\n", argv[0]);
		return 1;
	}

	bool success = lexFiles(argv[1], argv[2]);
	return !success;
}


