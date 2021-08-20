#pragma once
#include "lua_main_incl.h"
#include <Windows.h>
static const char* ODBC = "ODBC";

#include <sql.h>
#include <sqlext.h>

typedef struct LuaOdbc {

	wchar_t* ConnectionString;
	SQLHENV env;
	SQLHDBC dbc;
	SQLHSTMT stmt;
	unsigned int paramnumber;

	void** params;
	unsigned int numbparams;

} LuaOdbc;

int ODBCDriverConnect(lua_State* L);
int ODBCGetAllDrivers(lua_State* L);
int ODBCPrepare(lua_State* L);
int ODBCBind(lua_State* L);
int ODBCExecute(lua_State* L);
int ODBCFetch(lua_State* L);
int ODBCGetRow(lua_State* L);
int ODBCGetResultColumns(lua_State* L);
int ODBCToggleAutoCommit(lua_State* L);
int ODBCBegin(lua_State* L);
int ODBCCommit(lua_State* L);
int ODBCRollback(lua_State* L);
int ODBCTables(lua_State* L);
int ODBCColumns(lua_State* L);
int ODBCForeignKeys(lua_State* L);
int ODBCPrimaryKeys(lua_State* L);
int ODBCProcedures(lua_State* L);
int ODBCProcedureColumns(lua_State* L);
int ODBCSpecialColumns(lua_State* L);

LuaOdbc* lua_pushodbc(lua_State* L);
LuaOdbc* lua_toodbc(lua_State* L, int index);

int odbc_gc(lua_State* L);
int odbc_tostring(lua_State* L);