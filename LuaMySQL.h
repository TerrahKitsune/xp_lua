#pragma once
#pragma warning (disable: 4514 4786)
#pragma warning( push, 3 )
//#define HAVE_STRUCT_TIMESPEC
#include "lua_main_incl.h"
#include <my_global.h>
#include <mysql.h>

static const char * LUAMYSQL = "LuaMySQL";

typedef struct LuaMySQL {
	MYSQL mysql;
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
} LuaMySQL;

LuaMySQL * lua_tomysql(lua_State *L, int index);
LuaMySQL * luaL_checkmysql(lua_State *L, int index);
LuaMySQL * lua_pushmysql(lua_State *L);

int luamysql_gc(lua_State *L);
int luamysql_tostring(lua_State *L);
int DataToHex(lua_State *L);
int EscapeString(lua_State *L);

int MySQLConnect(lua_State *L);
int MySQLExecute(lua_State *L);
int MySQLFetch(lua_State *L);
int MySQLGetRow(lua_State *L);

bool Reconnect(LuaMySQL *luamysql);