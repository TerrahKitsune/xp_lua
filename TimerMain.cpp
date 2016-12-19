#include "TimerMain.h"
#include "Timer.h"

static const struct luaL_Reg timerfunctions[] = {
	{ "New", TimerNew },
	{ "IsRunning", TimerIsRunning },
	{ "Reset", TimerReset },
	{ "Start", TimerStart },
	{ "Stop", TimerStop },
	{ "Elapsed", TimerGetElapsed },
	{ NULL, NULL }
};

static const luaL_Reg timermeta[] = {
	{ "__gc", Timer_gc },
	{ "__tostring", Timer_tostring },
	{ NULL, NULL }
};

int luaopen_timer(lua_State *L) {

	luaL_newlibtable(L, timerfunctions);
	luaL_setfuncs(L, timerfunctions, 0);

	luaL_newmetatable(L, TIMER);
	luaL_setfuncs(L, timermeta, 0);

	lua_pushliteral(L, "__index");
	lua_pushvalue(L, -3);
	lua_rawset(L, -3);
	lua_pushliteral(L, "__metatable");
	lua_pushvalue(L, -3);
	lua_rawset(L, -3);

	lua_pop(L, 1);
	return 1;
}