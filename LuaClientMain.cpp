#include "LuaClientMain.h"
#include "LuaClient.h"

static const struct luaL_Reg luaclientfunctions[] = {
	{ "Connect", CliConnect },
	{ "Recv", CliRecv },
	{ "Send", CliSend },
	{ NULL, NULL }
};

static const luaL_Reg luaclientmeta[] = {
	{ "__gc", luaclient_gc },
	{ "__tostring", luaclient_tostring },
	{ NULL, NULL }
};

int luaopen_luaclient(lua_State *L) {

	lua_pushinteger(L, WSAEWOULDBLOCK);
	lua_setglobal(L, "WOULDBLOCK");

	luaL_newlibtable(L, luaclientfunctions);
	luaL_setfuncs(L, luaclientfunctions, 0);

	luaL_newmetatable(L, LUACLIENT);
	luaL_setfuncs(L, luaclientmeta, 0);

	lua_pushliteral(L, "__index");
	lua_pushvalue(L, -3);
	lua_rawset(L, -3);
	lua_pushliteral(L, "__metatable");
	lua_pushvalue(L, -3);
	lua_rawset(L, -3);

	lua_pop(L, 1);
	return 1;
}