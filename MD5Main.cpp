#include "MD5Main.h"
#include "LuaMD5.h"

static const struct luaL_Reg md5functions[] = {
	{ "New", NewMD5 },
	{ "Update", UpdateMD5 },
	{ "Finish", FinalMD5 },
	{ NULL, NULL }
};

static const luaL_Reg md5meta[] = {
	{ "__gc", md5_gc },
	{ "__tostring", md5_tostring },
	{ NULL, NULL }
};

int luaopen_md5(lua_State *L) {

	luaL_newlibtable(L, md5functions);
	luaL_setfuncs(L, md5functions, 0);

	luaL_newmetatable(L, LUAMD5);
	luaL_setfuncs(L, md5meta, 0);

	lua_pushliteral(L, "__index");
	lua_pushvalue(L, -3);
	lua_rawset(L, -3);
	lua_pushliteral(L, "__metatable");
	lua_pushvalue(L, -3);
	lua_rawset(L, -3);

	lua_pop(L, 1);
	return 1;
}