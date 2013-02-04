// Quick program to test the preprocessor.
// Compile using build.sh.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "pp_lexer.h"
#include "pp_parser.h"
#undef printf

List includes;

bool lexFile(char* filename)
{
	int length;
	char* buffer;
	pp_lexer lexer;
	pp_token token;

	// Open the file and read it into a memory buffer
	FILE* fp = fopen(filename, "rb");
	if(fp == NULL) return false;
	fseek(fp, 0, SEEK_END);
	length = ftell(fp);
	fseek(fp, 0, SEEK_SET);
	fprintf(stderr, "Length is %i\n", length);

	buffer = malloc(length + 1);
	memset(buffer, 0, length + 1);
	if(fread(buffer, 1, length, fp) != length) return false;
	fclose(fp);

	TEXTPOS position = {0,0};
	pp_lexer_Init(&lexer, buffer, position);

	do {
		if(FAILED(pp_lexer_GetNextToken(&lexer, &token))) { fprintf(stderr, "Fail.\n"); return false; }
		printf("%s", token.theSource);
	} while(token.theType != PP_TOKEN_EOF);

	pp_lexer_Clear(&lexer);
	free(buffer);

	return true;
}

bool parseFile(char* filename)
{
	int length;
	char* buffer;
	bool success = true;
	FILE* fp;
	pp_parser parser;
	pp_context ctx;
	TEXTPOS initialPosition = {1, 0};

	// Open the file and determine its size
	fp = fopen(filename, "rb");
	if(fp == NULL) return false;
	fseek(fp, 0, SEEK_END);
	length = ftell(fp);
	fseek(fp, 0, SEEK_SET);

	// Read the file into a memory buffer; return false if this fails for some reason
	buffer = malloc(length + 1);
	memset(buffer, 0, length + 1);
	if(fread(buffer, 1, length, fp) != length)
		success = false;
	fclose(fp);
	if(!success) return false;

	// Initialize the preprocessor
	pp_context_init(&ctx);
	pp_parser_init(&parser, &ctx, filename, buffer, initialPosition);

	// Parse the file
	//pp_parser_parse(&parser);
	pp_token* token;
	while((token = pp_parser_emit_token(&parser)) != NULL && token->theType != PP_TOKEN_EOF)
	{
		printf("%s", token->theSource);
	}

	// Don't forget to free the buffer!
	free(buffer);

	// Print out all imports
	int size, i;
	fprintf(stderr, "Imports:\n");
	FOREACH(ctx.imports, fprintf(stderr, "%s\n", List_GetName(&ctx.imports)););

	//printf("%s", ctx.tokens);
	pp_context_destroy(&ctx);

	return success;
}

// called from pp_parser.c
char namebuf[256];
char* get_full_path(char* filename)
{
	int i;
	int numIncludes = List_GetSize(&includes);
	FILE* fp;
	
	List_Reset(&includes);
	for(i=0; i<numIncludes; i++)
	{
		strcpy(namebuf, List_GetName(&includes));
		strcat(namebuf, "/");
		strcat(namebuf, filename);
		if((fp = fopen(namebuf, "r")) != NULL)
		{
			fclose(fp);
			//fprintf(stderr, "including '%s'\n", namebuf);
			return namebuf;
		}
		List_GotoNext(&includes);
	}
	return filename;
}

int main(int argc, char** argv)
{
	char* filename;
	if(argc != 2)
	{
		printf("Usage: %s filename\n", argv[0]);
		return 1;
	}

	List_Init(&includes);
	List_InsertAfter(&includes, NULL, ".");
	List_InsertAfter(&includes, NULL, "/usr/include");
	List_InsertAfter(&includes, NULL, "/usr/include/i386-linux-gnu");
	List_InsertAfter(&includes, NULL, "/usr/lib/gcc/i686-linux-gnu/4.6/include");
	filename = argv[1];
	//bool success = lexFile(filename);
	bool success = parseFile(filename);
	return !success;
}


