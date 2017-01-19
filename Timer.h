#pragma once
#include "lua_main_incl.h"
static const char * TIMER = "Timer";

typedef struct Timer {
	double PCFreq;
	__int64 CounterStart;
	__int64 CounterStop;
	double StoredTime;
} Timer;

Timer * lua_totimer(lua_State *L, int index);
Timer * luaL_checktimer(lua_State *L, int index);
Timer * lua_pushtimer(lua_State *L);
int TimerNew(lua_State *L);
int TimerIsRunning(lua_State *L);
int TimerReset(lua_State *L);
int TimerStart(lua_State *L);
int TimerStop(lua_State *L);
int TimerGetElapsed(lua_State *L);
int Timer_gc(lua_State *L);
int Timer_tostring(lua_State *L);