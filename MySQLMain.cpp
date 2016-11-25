#include "MySQLMain.h"
#include "LuaMySQL.h"

static const struct luaL_Reg mysqlfunctions[] = {	
	{ "EncodeString", DataToHex },
	{ "EscapeString", EscapeString },
	{ "GetRow", MySQLGetRow },
	{ "Fetch", MySQLFetch },
	{ "Connect", MySQLConnect },
	{ "Query", MySQLExecute },
	{ NULL, NULL }
};

static const luaL_Reg mysqlmeta[] = {
	{ "__gc", luamysql_gc },
	{ "__tostring", luamysql_tostring },
	{ NULL, NULL }
};

int luaopen_mysql(lua_State *L) {

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