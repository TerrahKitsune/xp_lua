#pragma once
#include "lua_main_incl.h"
#include "BinaryTree.h"
static const char* BINARYTREE = "BINARYTREE";

typedef struct LuaBinaryTree {

	size_t count;
	BinaryTreeNode* root;

} LuaBinaryTree;

int BinaryTreeCreate(lua_State* L);
int BinaryTreeCount(lua_State* L);
int BinaryTreeAdd(lua_State* L);
int BinaryTreeIterate(lua_State* L);
int BinaryTreeGet(lua_State* L);
int BinaryTreeDelete(lua_State* L);

LuaBinaryTree* lua_pushbinarytree(lua_State* L);
LuaBinaryTree* lua_tobinarytree(lua_State* L, int index);

int binarytree_gc(lua_State* L);
int binarytree_tostring(lua_State* L);