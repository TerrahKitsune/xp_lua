#include "FileAsync.h"
#include "FileAsyncMain.h"

static const struct luaL_Reg fileasyncfunctions[] = {
	{ "Open",  OpenFileAsync },
	{ "Busy",  LuaIsBusy },
	{ "Tell",  LuaTell },
	{ "Seek",  LuaSeek },
	{ "Rewind",  LuaRewind },
	{ "EndOfFile",  LuaIsEof },
	{ "Read",  LuaRead },
	{ "BufferStatus",  LuaGetReadWriteBufferStatus },
	{ "EmptyBuffer",  LuaEmptyBuffer },
	{ "Close",  luafileasync_gc },
	{ NULL, NULL }
};

static const luaL_Reg fileasyncmeta[] = {

	{ "__gc",  luafileasync_gc },
	{ "__tostring",  luafileasync_tostring },
	{ NULL, NULL }
};

int luaopen_fileasync(lua_State* L) {

	luaL_newlibtable(L, fileasyncfunctions);
	luaL_setfuncs(L, fileasyncfunctions, 0);

	luaL_newmetatable(L, LUAFILEASYNC);
	luaL_setfuncs(L, fileasyncmeta, 0);

	lua_pushliteral(L, "__index");
	lua_pushvalue(L, -3);
	lua_rawset(L, -3);
	lua_pushliteral(L, "__metatable");
	lua_pushvalue(L, -3);
	lua_rawset(L, -3);

	lua_pop(L, 1);
	return 1;
}