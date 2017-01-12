#pragma once
#include "lua_main_incl.h"
#include <Windows.h>
static const char * LUAPROCESS = "LuaProcess";

typedef struct LuaProcess {
	STARTUPINFO info;
	PROCESS_INFORMATION processInfo;
	ULARGE_INTEGER lastCPU, lastSysCPU, lastUserCPU;
	int numProcessors;
} LuaProcess;

LuaProcess * lua_toprocess(lua_State *L, int index);
LuaProcess * lua_pushprocess(lua_State *L);

int LuaOpenProcess(lua_State *L);
int GetAllProcesses(lua_State *L);
int StartNewProcess(lua_State *L);
int StopProcess(lua_State *L);
int GetExitCode(lua_State *L);
int GetProcId(lua_State *L);
int GetProcName(lua_State *L);
int GetSetPriority(lua_State *L);
int GetCPU(lua_State *L);
int GetMemory(lua_State *L);

int process_gc(lua_State *L);
int process_tostring(lua_State *L);