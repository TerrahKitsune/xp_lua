#include "Timer.h"
#include <string.h>
#include <windows.h>

Timer * lua_totimer(lua_State *L, int index) {

	Timer * timer = (Timer*)lua_touserdata(L, index);
	if (timer == NULL)
		luaL_error(L, "paramter is not a %s", TIMER);
	return timer;
}

Timer * luaL_checktimer(lua_State *L, int index) {

	Timer * timer = (Timer*)luaL_checkudata(L, index, TIMER);
	if (timer == NULL)
		luaL_error(L, "paramter is not a %s", TIMER);
	return timer;
}

Timer * lua_pushtimer(lua_State *L) {

	Timer * timer = (Timer*)lua_newuserdata(L, sizeof(Timer));
	if (timer == NULL)
		luaL_error(L, "Unable to create timer");
	luaL_getmetatable(L, TIMER);
	lua_setmetatable(L, -2);
	memset(timer, 0, sizeof(Timer));
	return timer;
}

int TimerNew(lua_State *L) {
	lua_pushtimer(L);
	return 1;
}

int TimerIsRunning(lua_State *L) {

	Timer * timer = luaL_checktimer(L, 1);
	int started = timer->CounterStart > 0 && timer->CounterStop <= 0;
	lua_pop(L, 1);
	lua_pushboolean(L, started);
	return 1;
}

int TimerReset(lua_State *L) {

	Timer * timer = luaL_checktimer(L, 1);

	LARGE_INTEGER li;
	if (!QueryPerformanceFrequency(&li))
		luaL_error(L, "QueryPerformanceFrequency failed!");

	timer->CounterStart = 0;
	timer->CounterStop = 0;
	timer->StoredTime = 0;

	lua_pop(L, 1);
	return 0;
}

int TimerStart(lua_State *L) {
	Timer * timer = luaL_checktimer(L, 1);

	LARGE_INTEGER li;
	if (!QueryPerformanceFrequency(&li))
		luaL_error(L, "QueryPerformanceFrequency failed!");

	if (timer->CounterStart > 0){
		timer->StoredTime += (double((timer->CounterStop <= 0 ? li.QuadPart : timer->CounterStop) - timer->CounterStart) / timer->PCFreq);
	}

	timer->PCFreq = double(li.QuadPart) / 1000.0;

	QueryPerformanceCounter(&li);
	timer->CounterStart = li.QuadPart;
	
	timer->CounterStop = 0;

	lua_pop(L, 1);

	return 0;
}

int TimerStop(lua_State *L) {

	Timer * timer = luaL_checktimer(L, 1);

	if (timer->CounterStop <= 0) {
		LARGE_INTEGER li;
		QueryPerformanceCounter(&li);
		timer->CounterStop = li.QuadPart;
	}

	lua_pop(L, 1);
	lua_pushnumber(L, double(timer->CounterStop - timer->CounterStart) / timer->PCFreq);

	return 1;
}

int TimerGetElapsed(lua_State *L) {

	Timer * timer = luaL_checktimer(L, 1);

	if (timer->CounterStart <= 0){
		lua_pop(L, 1);
		lua_pushnumber(L, 0);
	}
	else if (timer->CounterStop <= 0) {
		LARGE_INTEGER li;
		if (!QueryPerformanceCounter(&li))
			luaL_error(L, "QueryPerformanceFrequency failed!");


		lua_pop(L, 1);
		lua_pushnumber(L, timer->StoredTime + (double(li.QuadPart - timer->CounterStart) / timer->PCFreq));
	}
	else {
		lua_pop(L, 1);
		lua_pushnumber(L, timer->StoredTime + (double(timer->CounterStop - timer->CounterStart) / timer->PCFreq));
	}
	return 1;
}

int Timer_gc(lua_State *L) {
	return 0;
}

int Timer_tostring(lua_State *L) {

	char tim[100];
	sprintf(tim, "Timer: 0x%08X", lua_totimer(L, 1));
	lua_pushfstring(L, tim);
	return 1;
}