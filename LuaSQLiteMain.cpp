#include "LuaSQLiteMain.h"
#include "LuaSQLite.h"

static const struct luaL_Reg lasqlitefunctions[] = {
	{ "Open", SQLiteConnect },
	{ "Close", SQLite_GC },
	{ "Query", SQLiteExecute },	
	{ "Execute", SQLiteExecuteWithCallback },
	{ "Fetch", SQLiteFetch },
	{ "GetRow", SQLiteGetRow },
	{ "SetBusyHandler", SQLiteSetBusyHandler },
	{ NULL, NULL }
};

static const luaL_Reg luasqlmeta[] = {
	{ "__gc", SQLite_GC },
	{ "__tostring", SQLite_ToString },
	{ NULL, NULL }
};

int luaopen_sqlite(lua_State *L) {

	luaL_newlibtable(L, lasqlitefunctions);
	luaL_setfuncs(L, lasqlitefunctions, 0);

	luaL_newmetatable(L, LUASQLITE);
	luaL_setfuncs(L, luasqlmeta, 0);

	lua_pushliteral(L, "__index");
	lua_pushvalue(L, -3);
	lua_rawset(L, -3);
	lua_pushliteral(L, "__metatable");
	lua_pushvalue(L, -3);
	lua_rawset(L, -3);

	lua_pop(L, 1);
	return 1;
}