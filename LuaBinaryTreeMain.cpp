#include "LuaBinaryTreeMain.h"
#include "LuaBinaryTree.h"

static const struct luaL_Reg binarytreefunctions[] = {
	{ "Get", BinaryTreeGet},
	{ "Iterate", BinaryTreeIterate},
	{ "Add", BinaryTreeAdd},
	{ "Count", BinaryTreeCount},
	{ "Create", BinaryTreeCreate },
	{ NULL, NULL }
};

static const luaL_Reg binarytreemeta[] = {
	{ "__gc", binarytree_gc },
	{ "__tostring", binarytree_tostring },
	{ NULL, NULL }
};

int luaopen_binarytree(lua_State* L) {

	luaL_newlibtable(L, binarytreefunctions);
	luaL_setfuncs(L, binarytreefunctions, 0);

	luaL_newmetatable(L, BINARYTREE);
	luaL_setfuncs(L, binarytreemeta, 0);

	lua_pushliteral(L, "__index");
	lua_pushvalue(L, -3);
	lua_rawset(L, -3);
	lua_pushliteral(L, "__metatable");
	lua_pushvalue(L, -3);
	lua_rawset(L, -3);

	lua_pop(L, 1);
	return 1;
}