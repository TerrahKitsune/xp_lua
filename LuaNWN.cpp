#include "LuaNWN.h"
#include "LuaNWNFunctions.h"
#include "MainLoop.h"

static const struct luaL_Reg nwnfuncs[] = {

	{ "GetCreature", GetCreature },
	{ "RunScript", LRunScript },
	{ "HookMainloop", HookMainLoop },
	{ "Test", Test },
	{ NULL, NULL }
};


int luaopen_nwnfunctions(lua_State *L){

	SetStateToUse(L);

	luaL_newlibtable(L, nwnfuncs);
	luaL_setfuncs(L, nwnfuncs, 0);

	return 1;
}