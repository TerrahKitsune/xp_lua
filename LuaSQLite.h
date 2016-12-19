#pragma once
#include "lua_main_incl.h"
#include "SQLite\sqlite3.h"
static const char * LUASQLITE = "SQLite";

typedef struct LuaSQLite {
	sqlite3 *db;
	sqlite3_stmt * stmt;
	char * file;
	int status;
	int busyhandler;
} LuaSQLite;

int SQLiteConnect(lua_State *L);
int SQLiteExecute(lua_State *L);
int SQLiteFetch(lua_State *L);
int SQLiteGetRow(lua_State *L);
int SQLiteExecuteWithCallback(lua_State *L);
int SQLiteSetBusyHandler(lua_State *L);

int SQLite_ToString(lua_State *L);
int SQLite_GC(lua_State *L);