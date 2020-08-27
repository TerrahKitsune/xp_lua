#include "LuaBinaryTree.h"
#include <string.h>
#include <wtypes.h>

int BinaryTreeCreate(lua_State* L) {

	LuaBinaryTree* btree = lua_pushbinarytree(L);

	return 1;
}

void lua_PushBinaryTreeValue(lua_State* L, BinaryTreeNode* node) {

	if (!node || !node->data) {
		lua_pushnil(L);
		return;
	}

	BYTE* data = (BYTE*)node->data;

	if (data[0] == 1) {
		lua_Number number;
		memcpy(&number, &data[1], sizeof(lua_Number));
		lua_pushnumber(L, number);
	}
	else if (data[0] == 2) {
		int b;
		memcpy(&b, &data[1], sizeof(int));
		lua_pushboolean(L, b);
	}
	else if (data[0] == 3) {
		size_t len;
		memcpy(&len, &data[1], sizeof(size_t));
		lua_pushlstring(L, (const char*)&data[1 + sizeof(size_t)], len);
	}
	else {
		lua_pushnil(L);
		return;
	}
}

int BinaryTreeGet(lua_State* L) {

	LuaBinaryTree* btree = lua_tobinarytree(L, 1);
	long long key = luaL_checkinteger(L, 2);

	BinaryTreeNode * node = GetInTree(btree->root, key);
	if (!node) {
		lua_pushnil(L);
	}
	else {
		lua_PushBinaryTreeValue(L, node);
	}

	return 1;
}

#pragma region ITERATORS

int _IteratePreorder(lua_State* L, BinaryTreeNode* node) {

	if (!node) {
		return 0;
	}

	lua_pushvalue(L, 2);
	lua_pushinteger(L, node->key);
	lua_PushBinaryTreeValue(L, node);

	if (lua_pcall(L, 2, 1, 0) != 0) {
		lua_error(L);
		return 1;
	}

	if (!lua_isnoneornil(L, -1)) {
		return 1;
	}
	else {
		lua_pop(L, 1);
	}

	if (_IteratePreorder(L, node->Left)) {
		return 1;
	}

	if (_IteratePreorder(L, node->Right)) {
		return 1;
	}

	return 0;
}

int _IterateInorder(lua_State* L, BinaryTreeNode* node) {

	if (!node) {
		return 0;
	}

	if (_IterateInorder(L, node->Left)) {
		return 1;
	}

	lua_pushvalue(L, 2);
	lua_pushinteger(L, node->key);
	lua_PushBinaryTreeValue(L, node);

	if (lua_pcall(L, 2, 1, 0) != 0) {
		lua_error(L);
		return 1;
	}

	if (!lua_isnoneornil(L, -1)) {
		return 1;
	}
	else {
		lua_pop(L, 1);
	}

	if (_IterateInorder(L, node->Right)) {
		return 1;
	}

	return 0;
}

int _IteratePostorder(lua_State* L, BinaryTreeNode* node) {

	if (!node) {
		return 0;
	}

	if (_IteratePostorder(L, node->Left)) {
		return 1;
	}

	if (_IteratePostorder(L, node->Right)) {
		return 1;
	}

	lua_pushvalue(L, 2);
	lua_pushinteger(L, node->key);
	lua_PushBinaryTreeValue(L, node);

	if (lua_pcall(L, 2, 1, 0) != 0) {
		lua_error(L);
		return 1;
	}

	if (!lua_isnoneornil(L, -1)) {
		return 1;
	}
	else {
		lua_pop(L, 1);
	}

	return 0;
}

#pragma endregion

int BinaryTreeIterate(lua_State* L) {

	LuaBinaryTree* btree = lua_tobinarytree(L, 1);
	luaL_checktype(L, 2, LUA_TFUNCTION);
	int type = luaL_optinteger(L, 3, 0);

	if (type == 1) {
		if (!_IterateInorder(L, btree->root)) {
			lua_pushnil(L);
		}
	}
	else if (type == 2) {
		if (!_IteratePostorder(L, btree->root)) {
			lua_pushnil(L);
		}
	}
	else {
	
		if (!_IteratePreorder(L, btree->root)) {
			lua_pushnil(L);
		}
	}

	return 1;
}

int BinaryTreeAdd(lua_State* L) {

	LuaBinaryTree* btree = lua_tobinarytree(L, 1);
	long long key = luaL_checkinteger(L, 2);
	int type = lua_type(L, 3);
	BYTE* data = NULL;

	if (type == LUA_TNUMBER) {

		lua_Number number = lua_tonumber(L, 3);
		data = (BYTE*)gff_malloc(sizeof(lua_Number) + 1);
		if (!data) {
			luaL_error(L, "Out of memory");
			return 0;
		}
		data[0] = 1;
		memcpy(&data[1], &number, sizeof(lua_Number));
	}
	else if (type == LUA_TBOOLEAN) {

		int b = lua_toboolean(L, 3);
		data = (BYTE*)gff_malloc(sizeof(int) + 1);
		if (!data) {
			luaL_error(L, "Out of memory");
			return 0;
		}
		data[0] = 2;
		memcpy(&data[1], &b, sizeof(int));
	}
	else if (type == LUA_TSTRING) {

		size_t len;
		const char* str = lua_tolstring(L, 3, &len);

		data = (BYTE*)gff_malloc(sizeof(size_t) + 1 + len);
		if (!data) {
			luaL_error(L, "Out of memory");
			return 0;
		}
		data[0] = 3;
		memcpy(&data[1], &len, sizeof(size_t));
		memcpy(&data[1 + sizeof(size_t)], str, len);
	}
	else {
		luaL_error(L, "Invalid value type");
		return 0;
	}

	BinaryTreeNode* result = AddToTree(btree->root, key, data);

	if (!result) {

		if (data) {
			gff_free(data);
		}

		lua_pushboolean(L, false);
	}
	else {
		
		lua_pushboolean(L, true);
		btree->count++;

		if (!btree->root) {
			btree->root = result;
		}
	}
	
	return 1;
}

int BinaryTreeCount(lua_State* L) {

	LuaBinaryTree* btree = lua_tobinarytree(L, 1);

	lua_settop(L, lua_gettop(L));
	lua_pushinteger(L, (lua_Integer)btree->count);

	return 1;
}

LuaBinaryTree* lua_pushbinarytree(lua_State* L) {
	LuaBinaryTree* btree = (LuaBinaryTree*)lua_newuserdata(L, sizeof(LuaBinaryTree));
	if (btree == NULL)
		luaL_error(L, "Unable to push btree");
	luaL_getmetatable(L, BINARYTREE);
	lua_setmetatable(L, -2);
	memset(btree, 0, sizeof(LuaBinaryTree));
	return btree;
}

LuaBinaryTree* lua_tobinarytree(lua_State* L, int index) {
	LuaBinaryTree* btree = (LuaBinaryTree*)luaL_checkudata(L, index, BINARYTREE);
	if (btree == NULL)
		luaL_error(L, "parameter is not a %s", BINARYTREE);
	return btree;
}

int binarytree_gc(lua_State* L) {

	LuaBinaryTree* btree = lua_tobinarytree(L, 1);

	if (btree->root) {
		btree->count = 0;
		DestroyTree(btree->root);
		btree->root = NULL;
	}

	return 0;
}

int binarytree_tostring(lua_State* L) {
	char tim[100];
	sprintf(tim, "BinaryTree: 0x%08X", lua_tobinarytree(L, 1));
	lua_pushfstring(L, tim);
	return 1;
}