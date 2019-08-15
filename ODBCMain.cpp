#include "odbc.h"
#include "ODBCMain.h"

static const struct luaL_Reg odbcfunctions[] = {

	{ "DriverConnect",  ODBCDriverConnect },
	{ "GetAllDrivers",  ODBCGetAllDrivers },
	{ "Prepare",  ODBCPrepare },
	{ "Bind",  ODBCBind },
	{ "Execute",  ODBCExecute },
	{ "Fetch",  ODBCFetch },
	{ "GetRow",  ODBCGetRow },
	{ "GetRowColumnTypes",  ODBCGetResultColumns },
	{ "ToggleAutoCommit",  ODBCToggleAutoCommit },
	{ "Begin",  ODBCBegin },
	{ "Commit",  ODBCCommit },
	{ "Rollback",  ODBCRollback },
	{ "Disconnect",  odbc_gc },
	{ NULL, NULL }
}; 

static const luaL_Reg odbcemeta[] = {
	{ "__gc",  odbc_gc },
	{ "__tostring",  odbc_tostring },
{ NULL, NULL }
};

int luaopen_odbc(lua_State* L) {

	luaL_newlibtable(L, odbcfunctions);
	luaL_setfuncs(L, odbcfunctions, 0);

	luaL_newmetatable(L, ODBC);
	luaL_setfuncs(L, odbcemeta, 0);

	lua_pushliteral(L, "__index");
	lua_pushvalue(L, -3);
	lua_rawset(L, -3);
	lua_pushliteral(L, "__metatable");
	lua_pushvalue(L, -3);
	lua_rawset(L, -3);

	lua_pop(L, 1);
	return 1;
}