#include "LuaServerMain.h"
#include "LuaServer.h"

static const struct luaL_Reg luaserverfunctions[] = {
	{ "Start", SvrStartLuaServer },
	{ "Accept", SvrAcceptConnection },
	{ "Recv", SvrRecv },
	{ "Send", SvrSend },
	{ "Clients", SvrGetClients },
	{ "GetIP", SrvGetIP },
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