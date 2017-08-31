#pragma once
#include "lua_main_incl.h"
#include "networking.h"
#include "List.h"
#include "Queue.h"
static const char * LUACLIENT = "LuaClient";

typedef struct LuaClientThread {

	int LastError;
	char * addr;
	int Port;
	volatile bool IsAlive;
	volatile bool IsConnected;
	HANDLE Thread;
	DWORD ThreadId;
	List * All;
	Queue * Events;
	Queue * Send;

} LuaClientThread;

typedef struct LuaClient {
	LuaClientThread * Thread;
} LuaClient;

LuaClient * lua_toluaclient(lua_State *L, int index);
LuaClient * lua_pushluaclient(lua_State *L);

int luaclient_connect(lua_State *L);
int luaclient_send(lua_State *L);
int luaclient_getevent(lua_State *L);
int luaclient_status(lua_State *L);

int luaclient_gc(lua_State *L);
int luaclient_tostring(lua_State *L);