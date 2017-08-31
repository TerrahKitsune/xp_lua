#include "LuaServerMain.h"

static const struct luaL_Reg luaserverfunctions[] = {
	{ "Start", luaserver_start },
	{ "Stop", luaserver_gc },
	{ "GetEvent", luaserver_getevent },
	{ "Disconnect", luaserver_disconnect },
	{ "Send", luaserver_send },
	{ "SetStartFunc", SetLuaIndexFunctionToRun },
	{ "GetClients", luaserver_getclients },
	{ NULL, NULL }
};

static const luaL_Reg luaservermeta[] = {
	{ "__gc", luaserver_gc },
	{ "__tostring", luaserver_tostring },
	{ NULL, NULL }
};

int luaopen_luaserver(lua_State *L) {

	lua_pushinteger(L, WSAEWOULDBLOCK);
	lua_setglobal(L, "WOULDBLOCK");

	luaL_newlibtable(L, luaserverfunctions);
	luaL_setfuncs(L, luaserverfunctions, 0);

	luaL_newmetatable(L, LUASERVER);
	luaL_setfuncs(L, luaservermeta, 0);

	lua_pushliteral(L, "__index");
	lua_pushvalue(L, -3);
	lua_rawset(L, -3);
	lua_pushliteral(L, "__metatable");
	lua_pushvalue(L, -3);
	lua_rawset(L, -3);

	lua_pop(L, 1);
	return 1;
}