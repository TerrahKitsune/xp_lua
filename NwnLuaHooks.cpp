#include "NwnLuaHooks.h"
#include "../../detours/include/detours.h"
#include "lua_helper.h"

static lua_State * external_lua = NULL;

static int lua_boothook = -1;
int(__fastcall *OriginalDisconnectPlayer)(void * pThis, void*, unsigned long a1, unsigned long a2, int flag);

static int lua_attackmodifierversus = -1;

static int lua_weapondamagehook = -1;
int(__fastcall *OriginalResolveBonusCombatDamage)(void * pThis, void*, CNWSObject *target, int isOffhand);

int __fastcall DisconnectPlayerProc(void * pThis, void*nothing, unsigned long a1, unsigned long strref, int flag) {

	lua_State * L = external_lua;

	lua_pop(L, lua_gettop(L));

	lua_rawgeti(L, LUA_REGISTRYINDEX, lua_boothook);
	lua_pushinteger(L, strref);
	lua_pushinteger(L, flag);

	if (lua_pcall(L, 2, 1, NULL) == 0) {

		if (lua_type(L, -1) == LUA_TNUMBER) {
			strref = lua_tointeger(L, -1);
		}

		lua_pop(L, lua_gettop(L));
	}

	return OriginalDisconnectPlayer(pThis, nothing, a1, strref, flag);
}

int __fastcall ResolveBonusCombatDamageHookProc(void* pThis, void *nothing, CNWSObject*target, int isOffhand)
{
	int intendeddamage = OriginalResolveBonusCombatDamage(pThis, nothing, target, isOffhand);

	if (pThis && target) {

		lua_State * L = external_lua;

		lua_pop(L, lua_gettop(L));

		CNWSObject* obj = (CNWSObject*)*(DWORD*)(((DWORD)pThis) + 0xA4);

		lua_rawgeti(L, LUA_REGISTRYINDEX, lua_weapondamagehook);
		lua_pushobject(L, obj->GenericObj.ObjectId);
		lua_pushobject(L, target->GenericObj.ObjectId);
		lua_pushinteger(L, intendeddamage);
		lua_pushinteger(L, isOffhand);

		if (lua_pcall(L, 4, 1, NULL) == 0) {

			intendeddamage = lua_tointeger(L, -1);

			lua_pop(L, lua_gettop(L));
		}
	}

	return intendeddamage;
}

int HookDisconnectPlayer(lua_State*L) {

	if (lua_boothook != -1) {

		luaL_error(L, "Already hooked");
		return 0;
	}
	else if (lua_type(L, 1) != LUA_TFUNCTION) {

		luaL_error(L, "No hook function provided");
		return 0;
	}

	external_lua = L;
	lua_boothook = luaL_ref(L, LUA_REGISTRYINDEX);

	DetourTransactionBegin();
	DetourUpdateThread(GetCurrentThread());
	int detour_success;
	DWORD addr = 0x4FD170;

	*(DWORD*)&OriginalDisconnectPlayer = addr;
	DetourAttach(&(PVOID&)OriginalDisconnectPlayer, DisconnectPlayerProc) == 0;

	detour_success = DetourTransactionCommit() == 0;

	return 0;
}

int HookResolveBonusCombatDamage(lua_State*L) {

	if (lua_weapondamagehook != -1) {

		luaL_error(L, "Already hooked");
		return 0;
	}
	else if (lua_type(L, 1) != LUA_TFUNCTION) {

		luaL_error(L, "No hook function provided");
		return 0;
	}

	external_lua = L;
	lua_weapondamagehook = luaL_ref(L, LUA_REGISTRYINDEX);

	DetourTransactionBegin();
	DetourUpdateThread(GetCurrentThread());
	int detour_success;
	DWORD addr = 0x5A5F90;

	*(DWORD*)&OriginalResolveBonusCombatDamage = addr;
	DetourAttach(&(PVOID&)OriginalResolveBonusCombatDamage, ResolveBonusCombatDamageHookProc) == 0;

	detour_success = DetourTransactionCommit() == 0;

	return 0;
}