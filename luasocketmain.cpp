#include "luasocket.h"
#include "luasocketmain.h"

static const struct luaL_Reg socketfunctions[] = {
	{ "Close",  luasocket_gc },
	{ "Connect",  LuaSocketOpen },
	{ "Select",  LuaSocketSelect },
	{ "HasData",  LuaSocketHasData },
	{ "Read",  LuaSocketReadData },
	{ "Write",  LuaSocketWrite },
	{ "Listen",  LuaSocketOpenListener },
	{ "Accept",  LuaSocketAccept },
	{ "Info",  LuaSocketGetInfo },
	{ NULL, NULL }
};

static const luaL_Reg socketmeta[] = {
	{ "__gc",  luasocket_gc },
	{ "__tostring",  luasocket_tostring },
	{ NULL, NULL }
};

int luaopen_socket(lua_State* L) {

	luaL_newlibtable(L, socketfunctions);
	luaL_setfuncs(L, socketfunctions, 0);

	luaL_newmetatable(L, LUASOCKET);
	luaL_setfuncs(L, socketmeta, 0);

	lua_pushliteral(L, "__index");
	lua_pushvalue(L, -3);
	lua_rawset(L, -3);
	lua_pushliteral(L, "__metatable");
	lua_pushvalue(L, -3);
	lua_rawset(L, -3);

	lua_pop(L, 1);
	return 1;
}