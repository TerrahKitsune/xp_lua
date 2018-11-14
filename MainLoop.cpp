#include "MainLoop.h"
#include <windows.h>

int hook = -1;
lua_State * external_lua = NULL;
DWORD hook_address;

void __cdecl lua_hook_sleep(DWORD duration) {

	if (hook == -1 || !external_lua)
		return;

	lua_State * L = external_lua;

	lua_pop(L, lua_gettop(L));

	lua_rawgeti(L, LUA_REGISTRYINDEX, hook);
	lua_pushinteger(L, duration);

	//function(duration) return true/false; end
	if (lua_pcall(L, 1, 1, NULL) != 0) {
		Sleep(duration);
		return;
	}

	int top = lua_gettop(L);

	if (lua_isboolean(L, 1) && lua_toboolean(L, 1)) {
		lua_pop(L, top);
		return;
	}
	else {
		lua_pop(L, top);
		Sleep(duration);
	}
}

void SetStateToUse(lua_State*L) {
	external_lua = L;
}

int HookMainLoop(lua_State*L) {

	if (!lua_isfunction(L, 1))
		luaL_error(L, "Parameter is not a function");

	if (hook == -1) {

		lua_hook_sleep(0);

		//0x0040B148 CALL DS:PTR[msvcr80_sleep]
		DWORD address = 0x0040B148;
		DWORD old;
		hook_address = (DWORD)&lua_hook_sleep;
		DWORD target = (DWORD)&hook_address;


		VirtualProtect((LPVOID)address, 8, PAGE_EXECUTE_READWRITE, &old);

		unsigned char* proc = (unsigned char*)address;

		//CALL DS:PTR[lua_hook_sleep]
		proc[0] = 0xff; 
		proc[1] = 0x15;

		proc[2] = target & 0xff;
		proc[3] = (target >> 8) & 0xff;
		proc[4] = (target >> 16) & 0xff;
		proc[5] = (target >> 24) & 0xff;

		VirtualProtect((LPVOID)address, 8, old, NULL);
	}
	else {
		luaL_unref(L, LUA_REGISTRYINDEX, hook);
	}

	hook = luaL_ref(L, LUA_REGISTRYINDEX);

	return 0;
}

