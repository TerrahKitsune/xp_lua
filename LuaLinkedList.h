#pragma once
#include "lua_main_incl.h"
#include <Windows.h>
#include "LinkedList.h"
static const char* LLINEDLIST = "LINKEDLIST";

typedef struct LuaLinkedList {

	LLNode* head;
	const LLNode* iterator;
	int pos;
	bool forwards;

} LuaLinkedList;

typedef struct LinkedListData {

	int type;
	lua_State* L;
	BYTE data[sizeof(size_t)+sizeof(void*)];

} LinkedListData;

LuaLinkedList* lua_pushlinkedlist(lua_State* L);
LuaLinkedList* lua_tolinkedlist(lua_State* L, int index);

int LuaGet(lua_State* L);
int LuaBackward(lua_State* L);
int LuaForward(lua_State* L);
int LuaAddFirst(lua_State* L);
int LuaAddLast(lua_State* L);
int Create(lua_State* L);
int LuaCount(lua_State* L);
int LuaRemove(lua_State* L);
int LuaInsert(lua_State* L);

int linkedlist_gc(lua_State* L);
int linkedlist_tostring(lua_State* L);