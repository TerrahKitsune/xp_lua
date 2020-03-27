#include "LuaFTP.h"
#include "LuaFTPMain.h"

static const struct luaL_Reg ftpfunctions[] = {
	{ "Open",  LuaConnect },
	{ "Login",  LuaLogin },
	{ "Command",  LuaCommand },
	{ "Passive",  LuaPassive },
	{ "SetTimeout",  LuaSetTimeout },
	{ "OpenDataChannel",  LuaOpenDataChannel },
	{ "GetMessages",  GetMessageLog },
	{ "Close",  luaftp_gc },
	{ NULL, NULL }
}; 

static const luaL_Reg ftpmeta[] = {
	
	{ "__gc",  luaftp_gc },
	{ "__tostring",  luaftp_tostring },
	{ NULL, NULL }
};

int luaopen_ftp(lua_State* L) {

	luaL_newlibtable(L, ftpfunctions);
	luaL_setfuncs(L, ftpfunctions, 0);

	luaL_newmetatable(L, LUAFTP);
	luaL_setfuncs(L, ftpmeta, 0);

	lua_pushliteral(L, "__index");
	lua_pushvalue(L, -3);
	lua_rawset(L, -3);
	lua_pushliteral(L, "__metatable");
	lua_pushvalue(L, -3);
	lua_rawset(L, -3);

	lua_pop(L, 1);
	return 1;
}