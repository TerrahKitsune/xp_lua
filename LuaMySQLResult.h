#pragma once

#include "mysql_main_incl.h"

static const char * LUAMYSQLRESULT = "LuaMySQLResult";

typedef struct LuaMySQLResult {
	MYSQL_RES* result;
	MYSQL_FIELD * columns;
	int fields;
	MYSQL_ROW row;
	int rows;
	bool asstring;
}LuaMySQLResult;

LuaMySQLResult * lua_tomysqlresult(lua_State *L, int index);
LuaMySQLResult * luaL_checkmysqlresult(lua_State *L, int index);
LuaMySQLResult * lua_pushmysqlresult(lua_State *L);

int MySQLResultNumbRows(lua_State *L);
int MySQLResultFetch(lua_State *L);
int MySQLResultGetRow(lua_State *L);
int MySQLResultSetAsString(lua_State *L);

int luamysqlresult_gc(lua_State *L);
int luamysqlresult_tostring(lua_State *L);