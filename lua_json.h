#pragma once
#include "lua_main_incl.h"

typedef struct JsonContext {

	int resultReallocStep;
	size_t resultLength;
	size_t resultSize;
	char*result;

	size_t maxRecursion;
	size_t antiRecursionEmpty;
	size_t antiRecursionLength;
	size_t antiRecursionSize;
	unsigned int* antiRecursion;

	size_t readLine;
	size_t readPos;

	size_t readLength;
	size_t readCursor;
	const char * read;

} JsonContext;

int lua_jsonempty(lua_State *L);
int lua_jsonnull(lua_State *L);
int lua_jsonencode(lua_State *L);
int lua_jsondecode(lua_State *L);