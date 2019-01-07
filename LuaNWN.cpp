#include "LuaNWN.h"
#include "LuaNWNFunctions.h"
#include "MainLoop.h"
#include "NwnLuaHooks.h"

static const struct luaL_Reg nwnfuncs[] = {
	{ "GetCreature", GetCreature },
	{ "GetObject", GetObject },
	{ "GetEffectData", GetEffectData },
	{ "EffectSetEffectInt", EffectSetEffectInt },
	{ "EffectSetExposed", EffectSetExposed },
	{ "SetEffectString", SetEffectString },
	{ "SetEffectObject", SetEffectObject },
	{ "CopyEffectIdsToEffectInts", CopyEffectIdsToEffectInts },
	{ "GetLocalVariables", GetLocalVariables },
	{ "RunScript", LRunScript },
	{ "GetABVs", GetABVs },
	{ "HookDisconnectPlayer", HookDisconnectPlayer },
	{ "HookMainloop", HookMainLoop },
	{ "HookResolveBonusCombatDamage", HookResolveBonusCombatDamage },
	{ "Test", Test },
	{ NULL, NULL }
};


int luaopen_nwnfunctions(lua_State *L) {

	SetStateToUse(L);

	luaL_newlibtable(L, nwnfuncs);
	luaL_setfuncs(L, nwnfuncs, 0);

	return 1;
}