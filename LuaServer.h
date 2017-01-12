#pragma once
#include "lua_main_incl.h"
#include "networking.h"
static const char * LUASERVER = "LuaServer";

typedef struct LuaServer {
	SOCKET Listener;
	unsigned int NumClients;
	SOCKET * Clients;
} LuaServer;

LuaServer * lua_toluaserver(lua_State *L, int index);
LuaServer * lua_pushluaserver(lua_State *L);

int SvrStartLuaServer(lua_State *L);
int SvrAcceptConnection(lua_State *L);
int SvrRecv(lua_State *L);
int SvrSend(lua_State *L);
int SvrGetClients(lua_State *L);
int SrvGetIP(lua_State *L);

int luaserver_gc(lua_State *L);
int luaserver_tostring(lua_State *L);