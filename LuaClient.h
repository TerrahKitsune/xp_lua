#pragma once
#include "lua_main_incl.h"
#include "networking.h"
static const char * LUACLIENT = "LuaClient";

typedef struct LuaClient {
	SOCKET client;
} LuaClient;

LuaClient * lua_toluaclient(lua_State *L, int index);
LuaClient * lua_pushluaclient(lua_State *L);

int CliConnect(lua_State *L);
int CliSend(lua_State *L);
int CliRecv(lua_State *L);

int luaclient_gc(lua_State *L);
int luaclient_tostring(lua_State *L);