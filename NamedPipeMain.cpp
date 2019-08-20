#include "namedpipe.h"
#include "NamedPipeMain.h"

static const struct luaL_Reg namedpipefunctions[] = {
	{ "WriteByte", WriteByte },
	{ "ReadByte", ReadByte },
	{ "Create", CreateNamedPipe },
	{ "Open", OpenNamedPipe },
	{ "Read", ReadPipe },
	{ "Write", WritePipe },
	{ "Close",  namedpipe_gc },
	{ "Available", CheckNamedPipe },
	{ NULL, NULL }
};

static const luaL_Reg namedpipemeta[] = {
	{ "__gc",  namedpipe_gc },
	{ "__tostring",  namedpipe_tostring },
{ NULL, NULL }
};

int luaopen_namedpipe(lua_State *L) {

	luaL_newlibtable(L, namedpipefunctions);
	luaL_setfuncs(L, namedpipefunctions, 0);

	luaL_newmetatable(L, NAMEDPIPE);
	luaL_setfuncs(L, namedpipemeta, 0);

	lua_pushliteral(L, "__index");
	lua_pushvalue(L, -3);
	lua_rawset(L, -3);
	lua_pushliteral(L, "__metatable");
	lua_pushvalue(L, -3);
	lua_rawset(L, -3);

	lua_pop(L, 1);
	return 1;
}