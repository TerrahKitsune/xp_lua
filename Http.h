#pragma once

#include "lua_main_incl.h"
#include <ppltasks.h>
#include "Buffer.h"

using namespace concurrency;

static const char * LUAHTTP = "LuaHTTP";

typedef struct HttpResult {
	char* error;
	Buffer * result;
} HttpResult;

typedef struct LuaHttp {
	
	size_t sent;
	size_t recv;
	char*packet;
	double Timeout;
	double PCFreq;
	__int64 CounterStart;
	char *ip;
	int port;
	bool ssl;
	Buffer * request;
	volatile bool alive;
	task<HttpResult*> task;
} LuaHttp;

LuaHttp * lua_tohttp(lua_State *L, int index);
LuaHttp * luaL_checkhttp(lua_State *L, int index);
LuaHttp * lua_pushhttp(lua_State *L);

int Start(lua_State *L);
int SetHttpTimeout(lua_State *L);
int GetResult(lua_State *L);
int GetStatus(lua_State *L);

int luahttp_gc(lua_State *L);
int luahttp_tostring(lua_State *L);