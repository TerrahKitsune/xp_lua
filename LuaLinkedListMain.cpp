#include "LuaLinkedList.h"
#include "LuaLinkedListMain.h"

static const struct luaL_Reg linkedlistfunctions[] = {
	{ "New",  Create },
	{ "AddFirst",  LuaAddFirst },
	{ "AddLast",  LuaAddLast },
	{ "Clear",  linkedlist_gc },
	{ "Forward",  LuaForward },
	{ "Backward",  LuaBackward },
	{ "Get",  LuaGet },
	{ "Remove",  LuaRemove },
	{ "Insert",  LuaInsert },
	{ "Count",  LuaCount },
	{ NULL, NULL }
}; 

static const luaL_Reg linkedlistmeta[] = {
	{ "__gc",  linkedlist_gc },
	{ "__tostring",  linkedlist_tostring },
{ NULL, NULL }
};

int luaopen_linkedlist(lua_State* L) {

	luaL_newlibtable(L, linkedlistfunctions);
	luaL_setfuncs(L, linkedlistfunctions, 0);

	luaL_newmetatable(L, LLINEDLIST);
	luaL_setfuncs(L, linkedlistmeta, 0);

	lua_pushliteral(L, "__index");
	lua_pushvalue(L, -3);
	lua_rawset(L, -3);
	lua_pushliteral(L, "__metatable");
	lua_pushvalue(L, -3);
	lua_rawset(L, -3);

	lua_pop(L, 1);
	return 1;
}