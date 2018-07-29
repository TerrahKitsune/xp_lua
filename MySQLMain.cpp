#include "MySQLMain.h"
#include "LuaMySQL.h"

static const struct luaL_Reg mysqlfunctions[] = {
	{ "EncodeString", DataToHex },
	{ "EscapeString", EscapeString },
	{ "GetRow", MySQLGetRow },
	{ "Fetch", MySQLFetch },
	{ "ChangeDatabase", MySQLChangeDatabase },
	{ "Connect", MySQLConnect },
	{ "Query", MySQLExecute },
	{ "SetTimeout", SetTimeout },
	{ "IsRunning", MySQLIsRunningAsync },
	{ "GetResult", MySQLGetAsyncResults },
	{ "ForkResult", MySQLForkResult },
	{ "ToggleAsString", MySQLSetAsString },
	{ NULL, NULL }
};

static const luaL_Reg mysqlmeta[] = {
	{ "__gc", luamysql_gc },
	{ "__tostring", luamysql_tostring },
	{ NULL, NULL }
};

static const struct luaL_Reg mysqlresultfunctions[] = {
	{ "GetRow", MySQLResultGetRow },
	{ "Fetch", MySQLResultFetch },
	{ "Count", MySQLResultNumbRows },
	{ "ToggleAsString", MySQLResultSetAsString },
{ NULL, NULL }
};

static const luaL_Reg mysqlresultmeta[] = {
	{ "__gc", luamysqlresult_gc },
	{ "__tostring", luamysqlresult_tostring },
{ NULL, NULL }
};

int luaopen_mysql(lua_State *L) {

	luaL_newlibtable(L, mysqlresultfunctions);
	luaL_setfuncs(L, mysqlresultfunctions, 0);

	luaL_newmetatable(L, LUAMYSQLRESULT);
	luaL_setfuncs(L, mysqlresultmeta, 0);

	lua_pushliteral(L, "__index");
	lua_pushvalue(L, -3);
	lua_rawset(L, -3);
	lua_pushliteral(L, "__metatable");
	lua_pushvalue(L, -3);
	lua_rawset(L, -3);

	lua_pop(L, 2);

	luaL_newlibtable(L, mysqlfunctions);
	luaL_setfuncs(L, mysqlfunctions, 0);

	luaL_newmetatable(L, LUAMYSQL);
	luaL_setfuncs(L, mysqlmeta, 0);

	lua_pushliteral(L, "__index");
	lua_pushvalue(L, -3);
	lua_rawset(L, -3);
	lua_pushliteral(L, "__metatable");
	lua_pushvalue(L, -3);
	lua_rawset(L, -3);

	lua_pop(L, 1);

	//lua_pushliteral(L, "ismeta");
	//lua_pushboolean(L, true);
	//lua_rawset(L, -3);

	return 1;
}