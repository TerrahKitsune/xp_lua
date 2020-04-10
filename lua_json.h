#pragma once
#include "lua_main_incl.h"
static const char * LUAJSON = "LUAJSON";

#define JSONANTIRECURSIONINITSIZE 10
#define JSONINITBUFFERSIZE 1024

typedef struct JsonContext {

	int resultReallocStep;
	unsigned int * antiRecursion;
	size_t antiRecursionSize;

	FILE * bufferFile;
	char * fileName;

	size_t bufferLength;
	size_t bufferSize;
	char * buffer;

	FILE * readFile;

	size_t readLine;
	size_t readPosition;

	size_t readCursor;
	size_t readSize;
	const char * read;

} JsonContext;

int lua_jsoncreate(lua_State *L);
int lua_jsondecodestring(lua_State *L);
int lua_jsondecodefromfile(lua_State *L);
int lua_jsonencodetabletostring(lua_State *L);
int lua_jsonencodetabletofile(lua_State *L);

JsonContext * lua_pushjson(lua_State *L);
JsonContext * lua_tojson(lua_State *L, int index);
int json_gc(lua_State *L);
int json_tostring(lua_State *L);

