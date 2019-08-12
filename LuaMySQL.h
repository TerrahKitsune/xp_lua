#pragma once
#include "mysql_main_incl.h"
#include <ppltasks.h>
#include "LuaMySQLResult.h"

using namespace concurrency;

static const char * LUAMYSQL = "LuaMySQL";

typedef struct LuaAsyncResult {

	bool Ok;
	char * Error;
	int Rows;

}LuaAsyncResult;

typedef struct LuaMySQL {
	MYSQL* mysql;
	MYSQL* connection;
	MYSQL_RES* result;
	MYSQL_FIELD * columns;
	int fields;
	MYSQL_ROW row;

	char * server;
	char * user;
	char * password;
	char * schema;
	unsigned int port;
	int timeout;
	bool asstring;

	char * lastError;

	volatile bool isRunningAsync;
	bool hasTask;
	task<LuaAsyncResult*> task;
	char * query;
} LuaMySQL;

LuaMySQL * lua_tomysql(lua_State *L, int index);
LuaMySQL * luaL_checkmysql(lua_State *L, int index);
LuaMySQL * lua_pushmysql(lua_State *L);

int MySQLSetAsString(lua_State *L);
int MySQLChangeDatabase(lua_State *L);
int luamysql_gc(lua_State *L);
int luamysql_tostring(lua_State *L);
int DataToHex(lua_State *L);
int EscapeString(lua_State *L);
int SetTimeout(lua_State *L);
int MySQLConnect(lua_State *L);
int MySQLExecute(lua_State *L);
int MySQLFetch(lua_State *L);
int MySQLGetRow(lua_State *L);
int MySQLIsRunningAsync(lua_State *L);
int MySQLGetAsyncResults(lua_State *L);
int MySQLForkResult(lua_State *L);

bool Reconnect(LuaMySQL *luamysql);