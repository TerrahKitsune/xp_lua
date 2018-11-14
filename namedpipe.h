#pragma once
#include "lua_main_incl.h"
#include <Windows.h>
static const char * NAMEDPIPE = "NAMEDPIPE";

typedef struct LuaNamedPipe {

	HANDLE Pipe;

} LuaNamedPipe;


LuaNamedPipe * lua_pushnamedpipe(lua_State *L);
LuaNamedPipe * lua_tonamedpipe(lua_State *L, int index);

int ReadPipe(lua_State *L);
int WritePipe(lua_State *L);
int OpenNamedPipe(lua_State *L);
int CreateNamedPipe(lua_State *L);

int namedpipe_gc(lua_State *L);
int namedpipe_tostring(lua_State *L);