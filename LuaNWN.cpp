#include "LuaNWN.h"
#include "LuaNWNFunctions.h"

static const struct luaL_Reg nwnfuncs[] = {

	{ "GetCreature", GetCreature },
	{ "RunScript", LRunScript },
	{ "Test", Test },
	{ NULL, NULL }
};


int luaopen_nwnfunctions(lua_State *L){

	luaL_newlibtable(L, nwnfuncs);
	luaL_setfuncs(L, nwnfuncs, 0);

	return 1;
}