#include "Sha256Main.h"
#include "LuaSha256.h"

static const struct luaL_Reg sha256functions[] = {
	{ "New", NewSHA256 },
	{ "Update", UpdateSHA256 },
	{ "Finish", FinalSHA256 },
	{ NULL, NULL }
};

static const luaL_Reg sha256meta[] = {
	{ "__gc", sha256_gc },
	{ "__tostring", sha256_tostring },
{ NULL, NULL }
};

int luaopen_sha256(lua_State *L) {

	luaL_newlibtable(L, sha256functions);
	luaL_setfuncs(L, sha256functions, 0);

	luaL_newmetatable(L, LUASHA256);
	luaL_setfuncs(L, sha256meta, 0);

	lua_pushliteral(L, "__index");
	lua_pushvalue(L, -3);
	lua_rawset(L, -3);
	lua_pushliteral(L, "__metatable");
	lua_pushvalue(L, -3);
	lua_rawset(L, -3);

	lua_pop(L, 1);
	return 1;
}