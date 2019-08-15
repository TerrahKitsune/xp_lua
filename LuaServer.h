#pragma once
#include "lua_main_incl.h"
#include "networking.h"
#include "List.h"
#include "Queue.h"

static const char * LUASERVER = "LuaServer";

#define MAX_ADDRESS_LEN 70

typedef struct LuaServerClient {
	SOCKET Socket;
	char Address[MAX_ADDRESS_LEN];
}LuaServerClient;

typedef struct LuaServerThread {

	int Port;
	volatile bool IsAlive;
	HANDLE Thread;
	DWORD ThreadId;
	List * All;
	Queue * Events;
	Queue * Send;

	CRITICAL_SECTION CriticalSection;

	int NumbClients;
	LuaServerClient * Clients;

} LuaServerThread;

typedef struct LuaServer {

	LuaServerThread * thread;
} LuaServer;

int luaserver_KillAll(lua_State *L);
int SetLuaIndexFunctionToRun(lua_State *L);

LuaServer * lua_toluaserver(lua_State *L, int index);
LuaServer * lua_pushluaserver(lua_State *L);

int luaserver_start(lua_State *L);
int luaserver_getevent(lua_State *L);
int luaserver_send(lua_State *L);
int luaserver_disconnect(lua_State *L);
int luaserver_getclients(lua_State *L);

int luaserver_gc(lua_State *L);
int luaserver_tostring(lua_State *L);