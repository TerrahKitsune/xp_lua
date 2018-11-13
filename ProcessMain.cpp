#include "ProcessMain.h"
#include "LuaProcess.h"

static const struct luaL_Reg processfunctions[] = {
	{ "All", GetAllProcesses },
	{ "Open", LuaOpenProcess },
	{ "Start", StartNewProcess },
	{ "Stop", StopProcess },
	{ "GetID", GetProcId },
	{ "GetName", GetProcName },
	{ "GetExitCode", GetExitCode }, 
	{ "Priority", GetSetPriority },
	{ "GetCPU", GetCPU },
	{ "GetRAM", GetMemory },
	{ "ReadFromPipe", ReadFromPipe },
	{ "WriteToPipe", WriteToPipe },
	{ "ReadErrorFromPipe", ErrorFromPipe },
	{ NULL, NULL }
}; 

static const luaL_Reg processmeta[] = {
	{ "__gc", process_gc },
	{ "__tostring", process_tostring },
	{ NULL, NULL }
};

int luaopen_process(lua_State *L) {

	luaL_newlibtable(L, processfunctions);
	luaL_setfuncs(L, processfunctions, 0);

	luaL_newmetatable(L, LUAPROCESS);
	luaL_setfuncs(L, processmeta, 0);

	lua_pushliteral(L, "__index");
	lua_pushvalue(L, -3);
	lua_rawset(L, -3);
	lua_pushliteral(L, "__metatable");
	lua_pushvalue(L, -3);
	lua_rawset(L, -3);

	lua_pop(L, 1);
	return 1;
}