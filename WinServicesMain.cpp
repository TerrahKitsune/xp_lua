#include "WinServices.h"
#include "WinServicesMain.h"

static const struct luaL_Reg winservicefunctions[] = {

	{ "Stop",  stopservice },
	{ "Start",  startservice },
	{ "Config",  getconfig },
	{ "Status",  getstatus },
	{ "Open",  openservice },
	{ "All",  getallservices },
	{ NULL, NULL }
}; 

static const luaL_Reg winservicemeta[] = {
	
	{ "__gc",  luawinservice_gc },
	{ "__tostring",  luawinservice_tostring },
	{ NULL, NULL }
};

int luaopen_winservice(lua_State* L) {

	luaL_newlibtable(L, winservicefunctions);
	luaL_setfuncs(L, winservicefunctions, 0);

	luaL_newmetatable(L, WINSERVICE);
	luaL_setfuncs(L, winservicemeta, 0);

	lua_pushliteral(L, "__index");
	lua_pushvalue(L, -3);
	lua_rawset(L, -3);
	lua_pushliteral(L, "__metatable");
	lua_pushvalue(L, -3);
	lua_rawset(L, -3);

	lua_pop(L, 1);
	return 1;
}