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
	{ "ToggleWidechar", SQLiteSetUseWidechar },
	{ NULL, NULL }
};

static const luaL_Reg luasqlmeta[] = {
	{ "__gc", SQLite_GC },
	{ "__tostring", SQLite_ToString },
	{ NULL, NULL }
};

static struct sqlite3_mem_methods sqlitemalloc;

void * sqlite_malloc(int size) {
	return gff_malloc(size);
}

void sqlite_free(void * ptr) {
	gff_free(ptr);
}

void * sqlite_realloc(void * ptr, int size) {
	return gff_realloc(ptr, size);
}

int luaopen_sqlite(lua_State *L) {

	sqlite3_config(SQLITE_CONFIG_MEMSTATUS, 0);
	sqlite3_config(SQLITE_CONFIG_GETMALLOC, &sqlitemalloc);
	sqlitemalloc.xMalloc = sqlite_malloc;
	sqlitemalloc.xFree = sqlite_free;
	sqlitemalloc.xRealloc = sqlite_realloc;
	sqlite3_config(SQLITE_CONFIG_MALLOC, &sqlitemalloc);

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