#pragma once
#include "lua_main_incl.h"
#include "networking.h"
static const char* LUAFTP = "FTP";

typedef struct LuaFTP {

	SOCKET s;
	char* passive_ip;
	int passive_port;
	int log;
	char endline[5];
} LuaFTP;

LuaFTP* lua_pushluaftp(lua_State* L);
LuaFTP* lua_toluaftp(lua_State* L, int index);

int GetMessageLog(lua_State* L);
int LuaConnect(lua_State* L);
int LuaLogin(lua_State* L);
int LuaCommand(lua_State* L);
int LuaPassive(lua_State* L);
int LuaOpenDataChannel(lua_State* L);
int LuaSetTimeout(lua_State* L);
int LuaGetConnectionStatus(lua_State* L);
int LuaSetEndline(lua_State* L);

int luaftp_gc(lua_State* L);
int luaftp_tostring(lua_State* L);