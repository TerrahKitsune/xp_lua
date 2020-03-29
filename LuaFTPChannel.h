#pragma once
#include "lua_main_incl.h"
#include "networking.h"
static const char* FTPCHANNEL = "FTPCHANNEL";

typedef struct LuaFTPChannel {

	SOCKET s;
	char* ip;
	int port;

	char* buffer;
	size_t buffersize;

} LuaFTPChannel;

LuaFTPChannel* lua_pushluaftpchannel(lua_State* L);
LuaFTPChannel* lua_toluaftpchannel(lua_State* L, int index);

int LuaFtpChannelGetConnectionStatus(lua_State* L);
int LuaFtpChannelRecv(lua_State* L);
int LuaFtpChannelSend(lua_State* L);

int luaftpchannel_gc(lua_State* L);
int luaftpchannel_tostring(lua_State* L);