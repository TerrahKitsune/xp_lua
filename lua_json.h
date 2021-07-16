#pragma once
#include "lua_main_incl.h"
static const char * LUAJSON = "LUAJSON";

#define JSONANTIRECURSIONINITSIZE 10
#define JSONINITBUFFERSIZE 1024
#define JSONFILEREADBUFFERSIZE 1048576

typedef struct JsonContext {

	int resultReallocStep;
	unsigned int * antiRecursion;
	size_t antiRecursionSize;

	int refWriteFunction;
	int refReadFunction;
	int refThreadInput;
	int refNullValue;

	int arrayKey;

	FILE * bufferFile;
	char * fileName;

	size_t bufferLength;
	size_t bufferSize;
	char * buffer;

	char prevFileChar[2];
	char * readFileBuffer;
	size_t readFileBufferSize;

	FILE * readFile;

	size_t readLine;
	size_t readPosition;

	size_t readCursor;
	size_t readSize;
	const char * read;

} JsonContext;

int lua_jsoniterator(lua_State *L);
int lua_jsoncreate(lua_State *L);
int lua_jsondecodestring(lua_State *L);
int lua_jsondecodefromfile(lua_State *L);
int lua_jsonencodetabletostring(lua_State *L);
int lua_jsonencodetabletofile(lua_State *L);
int lua_jsonencodefunction(lua_State *L);
int lua_jsondecodefunction(lua_State *L);
int lua_jsonsetnullvalue(lua_State* L);

JsonContext * lua_pushjson(lua_State *L);
JsonContext * lua_tojson(lua_State *L, int index);
int json_gc(lua_State *L);
int json_tostring(lua_State *L);

