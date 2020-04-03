#pragma once
#include "lua_main_incl.h"
#include "networking.h"
static const char* LUASOCKET = "SOCKET";

typedef struct LuaSocket {

	bool IsServer;
	SOCKET s;
	char* buf;
	size_t bufsize;

} LuaSocket;


LuaSocket* lua_pushluasocket(lua_State* L);
LuaSocket* lua_toluasocket(lua_State* L, int index);

int LuaSocketWrite(lua_State* L);
int LuaSocketReadData(lua_State* L);
int LuaSocketHasData(lua_State* L);
int LuaSocketSelect(lua_State* L);
int LuaSocketOpen(lua_State* L);
int LuaSocketOpenListener(lua_State* L);
int LuaSocketAccept(lua_State* L);
int LuaSocketGetInfo(lua_State* L);

int luasocket_gc(lua_State* L);
int luasocket_tostring(lua_State* L);