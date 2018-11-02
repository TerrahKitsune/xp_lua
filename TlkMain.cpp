#include "tlkmain.h"
#include "tlk.h"

static const struct luaL_Reg tlkfunctions[] = {
	{ "Create", tlk_create },
	{ "Open", tlk_open },
	{ "GetAll", tlk_getall },
	{ "Get", tlk_get },
	{ "SetSoundInfo", tlk_setsound },
	{ "Set", tlk_setstrref },
	{ "Defragment", tlk_defragment },
	{ "GetInfo", tlk_info },
	{ NULL, NULL }
}; 

static const luaL_Reg tlkmeta[] = {
	{ "__gc", tlk_gc },
	{ "__tostring", tlk_tostring },
	{ NULL, NULL }
};

int luaopen_tlk(lua_State *L) {

	luaL_newlibtable(L, tlkfunctions);
	luaL_setfuncs(L, tlkfunctions, 0);

	luaL_newmetatable(L, TLK);
	luaL_setfuncs(L, tlkmeta, 0);

	lua_pushliteral(L, "__index");
	lua_pushvalue(L, -3);
	lua_rawset(L, -3);
	lua_pushliteral(L, "__metatable");
	lua_pushvalue(L, -3);
	lua_rawset(L, -3);

	lua_pop(L, 1);
	return 1;
}