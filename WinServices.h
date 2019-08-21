#pragma once
#include "lua_main_incl.h"
#include <Windows.h>
static const char* WINSERVICE = "WINSERVICE";

typedef struct LuaWinService {

	SC_HANDLE hService;

} LuaWinService;


LuaWinService* lua_pushluawinservice(lua_State* L);
LuaWinService* lua_toluawinservice(lua_State* L, int index);

int getallservices(lua_State* L);
int openservice(lua_State* L);
int getstatus(lua_State* L);
int getconfig(lua_State* L);
int startservice(lua_State* L);
int stopservice(lua_State* L);

int luawinservice_gc(lua_State* L);
int luawinservice_tostring(lua_State* L);