#include "lua_json.h"
#include "luajsonmain.h"

static const struct luaL_Reg jsonfunctions[] = {

	{ "DecodeFromFile",  lua_jsondecodefromfile },
	{ "Decode",  lua_jsondecodestring },
	{ "EncodeToFile",  lua_jsonencodetabletofile },
	{ "Encode",  lua_jsonencodetabletostring },
	{ "EncodeToFunction",  lua_jsonencodefunction },
	{ "DecodeFromFunction",  lua_jsondecodefunction },
	{ "Create",  lua_jsoncreate },
	{ "Dispose",  json_gc },
	{ NULL, NULL }
};

static const luaL_Reg jsonmeta[] = {
	{ "__gc",  json_gc },
	{ "__tostring",  json_tostring },
	{ NULL, NULL }
};

int luaopen_json(lua_State *L) {

	luaL_newlibtable(L, jsonfunctions);
	luaL_setfuncs(L, jsonfunctions, 0);

	luaL_newmetatable(L, LUAJSON);
	luaL_setfuncs(L, jsonmeta, 0);

	lua_pushliteral(L, "__index");
	lua_pushvalue(L, -3);
	lua_rawset(L, -3);
	lua_pushliteral(L, "__metatable");
	lua_pushvalue(L, -3);
	lua_rawset(L, -3);

	lua_pop(L, 1);
	return 1;
}