#include "KeyBifMain.h"
#include "keybif.h"

static const struct luaL_Reg keybiffunctions[] = {

	{ "Create", CreateKeyBif},
	{ NULL, NULL }
};

static const luaL_Reg keybifmeta[] = {
	{ "__gc", keybif_gc },
	{ "__tostring", keybif_tostring },
	{ NULL, NULL }
};

int luaopen_keybif(lua_State* L) {

	luaL_newlibtable(L, keybiffunctions);
	luaL_setfuncs(L, keybiffunctions, 0);

	luaL_newmetatable(L, KEYBIF);
	luaL_setfuncs(L, keybifmeta, 0);

	lua_pushliteral(L, "__index");
	lua_pushvalue(L, -3);
	lua_rawset(L, -3);
	lua_pushliteral(L, "__metatable");
	lua_pushvalue(L, -3);
	lua_rawset(L, -3);

	lua_pop(L, 1);
	return 1;
}