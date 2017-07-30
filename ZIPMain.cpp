#include "ZIPMain.h"
#include "luazip.h"

static const struct luaL_Reg zipfunctions[] = {
	
	{ "Delete", zip_deletefile },
	{ "AddFile", zip_addfile },
	{ "AddData", zip_addbuffer },
	{ "Extract", zip_extract },
	{ "GetInfo", zip_getinfo },
	{ "GetFiles", zip_getfiles},
	{ "Open", zip_open },
	{ "Close", zip_gc },
	{ NULL, NULL }
};

static const luaL_Reg zipmeta[] = {
	{ "__gc", zip_gc },
	{ "__tostring", zip_tostring },
	{ NULL, NULL }
};
int luaopen_zip(lua_State *L) {

	luaL_newlibtable(L, zipfunctions);
	luaL_setfuncs(L, zipfunctions, 0);

	luaL_newmetatable(L, ZIP);
	luaL_setfuncs(L, zipmeta, 0);

	lua_pushliteral(L, "__index");
	lua_pushvalue(L, -3);
	lua_rawset(L, -3);
	lua_pushliteral(L, "__metatable");
	lua_pushvalue(L, -3);
	lua_rawset(L, -3);

	lua_pop(L, 1);
	return 1;
}