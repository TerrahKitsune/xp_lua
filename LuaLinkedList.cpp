#include "LuaLinkedList.h"
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <stdlib.h> 
#include <windows.h> 
#include "LinkedList.h"

void Delete(void* data) {
	LinkedListData* datanode = (LinkedListData*)data;
	if (datanode->type == LUA_TSTRING) {
		void* raw;
		memcpy(&raw, &datanode->data[sizeof(size_t)], sizeof(void*));
		if (raw) {
			free(raw);
		}
	}
	else if (datanode->type != LUA_TNUMBER) {

		int ref;
		memcpy(&ref, datanode->data, sizeof(int));

		luaL_unref(datanode->L, LUA_REGISTRYINDEX, ref);
	}

	free(data);
}

void PushNode(LinkedListData* datanode) {

	if (datanode->type == LUA_TSTRING) {
		void* raw;
		size_t size;
		memcpy(&size, datanode->data, sizeof(size_t));
		memcpy(&raw, &datanode->data[sizeof(size_t)], sizeof(void*));

		lua_pushlstring(datanode->L, (const char*)raw, size);
	}
	else if (datanode->type != LUA_TNUMBER) {

		int ref;
		memcpy(&ref, datanode->data, sizeof(int));

		lua_rawgeti(datanode->L, LUA_REGISTRYINDEX, ref);
	}
	else {
		lua_Number number;
		memcpy(&number, datanode->data, sizeof(lua_Number));
		lua_pushnumber(datanode->L, number);
	}
}

LinkedListData* CreateNode(lua_State* L, int index, int id) {

	LinkedListData* datanode = (LinkedListData*)calloc(1, sizeof(LinkedListData));

	if (!datanode) {
		return NULL;
	}

	datanode->L = L;
	datanode->id = id;
	datanode->type = lua_type(L, index);
	int ref;

	if (datanode->type == LUA_TSTRING) {

		size_t len;
		const char* data = lua_tolstring(L, index, &len);

		if (data) {
			void* raw = malloc(max(len, 1));
			if (!raw) {
				free(datanode);
				return NULL;
			}

			memcpy(datanode->data, &len, sizeof(size_t));
			memcpy(&datanode->data[sizeof(size_t)], &raw, sizeof(void*));
			memcpy(raw, data, len);
		}
	}
	else if (datanode->type == LUA_TNUMBER) {

		lua_Number number = lua_tonumber(L, index);

		memcpy(datanode->data, &number, sizeof(lua_Number));
	}
	else {

		ref = luaL_ref(L, LUA_REGISTRYINDEX);
		memcpy(datanode->data, &ref, sizeof(int));
	}

	return datanode;
}

int LuaIndexOf(lua_State* L) {

	LuaLinkedList* ll = lua_tolinkedlist(L, 1);
	int id = (int)luaL_checkinteger(L, 2);
	LLNode* itr = ll->head;
	int index = 0;
	for (; itr; itr = itr->next) {
		index++;
		if (((LinkedListData*)itr->data)->id == id) {
			lua_pop(L, lua_gettop(L));
			lua_pushinteger(L, index);
			return 1;
		}
	}

	lua_pop(L, lua_gettop(L));
	lua_pushnil(L);

	return 1;
}

int LuaAddAfter(lua_State* L) {

	LuaLinkedList* ll = lua_tolinkedlist(L, 1);
	int key = (int)luaL_checkinteger(L, 2);

	LLNode* find = NULL;
	LLNode* itr = ll->head;
	for (; itr; itr = itr->next) {
		if (((LinkedListData*)itr->data)->id == key) {
			find = itr;
			break;
		}
	}

	if (!find) {

		lua_pop(L, lua_gettop(L));
		lua_pushnil(L);
		return 1;
	}

	LinkedListData* datanode = CreateNode(L, 3, ++ll->nextid);

	if (!datanode) {
		luaL_error(L, "Unable to allocate memory");
		return 0;
	}

	find = AddAfter(find, datanode, (Dealloc*)& Delete);

	if (!find) {
		Delete(datanode);
		luaL_error(L, "Unable to allocate memory");
		return 0;
	}

	ll->head = find;

	lua_pop(L, lua_gettop(L));
	lua_pushinteger(L, datanode->id);

	return 1;
}

int LuaAddBefore(lua_State* L) {

	LuaLinkedList* ll = lua_tolinkedlist(L, 1);
	int key = (int)luaL_checkinteger(L, 2);

	LLNode* find = NULL;
	LLNode* itr = ll->head;
	for (; itr; itr = itr->next) {
		if (((LinkedListData*)itr->data)->id == key) {
			find = itr;
			break;
		}
	}

	if (!find) {

		lua_pop(L, lua_gettop(L));
		lua_pushnil(L);
		return 1;
	}

	LinkedListData* datanode = CreateNode(L, 3, ++ll->nextid);

	if (!datanode) {
		luaL_error(L, "Unable to allocate memory");
		return 0;
	}

	find = AddBefore(find, datanode, (Dealloc*)& Delete);

	if (!find) {
		Delete(datanode);
		luaL_error(L, "Unable to allocate memory");
		return 0;
	}

	ll->head = find;

	lua_pop(L, lua_gettop(L));
	lua_pushinteger(L, datanode->id);

	return 1;
}

int LuaGetDataFromKey(lua_State* L) {

	LuaLinkedList* ll = lua_tolinkedlist(L, 1);
	int id = (int)luaL_checkinteger(L, 2);
	LLNode* itr = ll->head;
	int index = 0;
	for (; itr; itr = itr->next) {
		index++;
		if (((LinkedListData*)itr->data)->id == id) {
			lua_pop(L, lua_gettop(L));
			PushNode((LinkedListData*)itr->data);
			return 1;
		}
	}

	lua_pop(L, lua_gettop(L));
	lua_pushnil(L);

	return 1;
}

int LuaGet(lua_State* L) {

	LuaLinkedList* ll = lua_tolinkedlist(L, 1);
	int index = (int)luaL_checkinteger(L, 2);

	LLNode* node = Get(ll->head, index - 1);
	lua_pop(L, lua_gettop(L));

	if (node) {
		PushNode((LinkedListData*)node->data);
	}
	else {
		lua_pushnil(L);
	}

	return 1;
}

int LuaGetKey(lua_State* L) {

	LuaLinkedList* ll = lua_tolinkedlist(L, 1);
	int index = (int)luaL_checkinteger(L, 2);

	LLNode* node = Get(ll->head, index - 1);
	lua_pop(L, lua_gettop(L));

	if (node) {
		lua_pushinteger(L, ((LinkedListData*)node->data)->id);
	}
	else {
		lua_pushnil(L);
	}

	return 1;
}

int LuaRemove(lua_State* L) {

	LuaLinkedList* ll = lua_tolinkedlist(L, 1);
	int index = (int)luaL_checkinteger(L, 2);

	LLNode* node = Get(ll->head, index - 1);
	lua_pop(L, lua_gettop(L));

	if (!node) {
		lua_pushnil(L);
	}
	else {
		PushNode((LinkedListData*)node->data);
		ll->head = Remove(node);
		ll->iterator = NULL;
		ll->pos = 0;
	}

	return 1;
}

int LuaInsert(lua_State* L) {

	LuaLinkedList* ll = lua_tolinkedlist(L, 1);
	int index = (int)luaL_checkinteger(L, 2);
	LinkedListData* datanode = CreateNode(L, 3, ++ll->nextid);

	if (!datanode) {
		luaL_error(L, "Unable to allocate memory");
		return 0;
	}

	LLNode* created = Insert(ll->head, datanode, (Dealloc*)& Delete, index - 1);

	if (!created) {
		Delete(datanode);
		luaL_error(L, "Unable to allocate memory");
		return 0;
	}

	ll->head = created;

	lua_pop(L, lua_gettop(L));
	lua_pushinteger(L, datanode->id);

	return 1;
}

int LuaCount(lua_State* L) {
	LuaLinkedList* ll = lua_tolinkedlist(L, 1);
	lua_pop(L, lua_gettop(L));
	lua_pushinteger(L, Count(ll->head));
	return 1;
}

int LuaFirstNext(lua_State* L) {

	LuaLinkedList* ll = lua_tolinkedlist(L, 1);

	if (!ll->head) {
		return 0;
	}

	if (ll->forwards) {

		if (lua_isnil(L, 2) || !ll->iterator) {
			ll->iterator = ll->head;
			ll->pos = 0;
		}
		else if (ll->iterator->next == NULL) {
			return 0;
		}
		else {
			ll->iterator = ll->iterator->next;
		}

		lua_pop(L, 1);
		lua_pushinteger(L, ++ll->pos);
		PushNode((LinkedListData*)ll->iterator->data);
		lua_pushinteger(L, ((LinkedListData*)ll->iterator->data)->id);
	}
	else {
		if (lua_isnil(L, 2) || !ll->iterator) {
			ll->pos = 1;
			for (ll->iterator = ll->head; ll->iterator->next; ll->iterator = ll->iterator->next) {
				ll->pos++;
			}
		}
		else if (ll->iterator->prev == NULL) {
			return 0;
		}
		else {
			ll->iterator = ll->iterator->prev;
		}

		lua_pop(L, 1);
		lua_pushinteger(L, ll->pos--);
		PushNode((LinkedListData*)ll->iterator->data);
		lua_pushinteger(L, ((LinkedListData*)ll->iterator->data)->id);
	}

	return 3;
}

int LuaForward(lua_State* L) {
	LuaLinkedList* ll = lua_tolinkedlist(L, 1);
	ll->forwards = true;
	lua_pushcfunction(L, LuaFirstNext);
	lua_pushvalue(L, 1);

	return 2;
}

int LuaBackward(lua_State* L) {
	LuaLinkedList* ll = lua_tolinkedlist(L, 1);
	ll->forwards = false;
	lua_pushcfunction(L, LuaFirstNext);
	lua_pushvalue(L, 1);

	return 2;
}

int LuaAddFirst(lua_State* L) {

	LuaLinkedList* ll = lua_tolinkedlist(L, 1);
	LinkedListData* datanode = CreateNode(L, 2, ++ll->nextid);

	if (!datanode) {
		luaL_error(L, "Unable to allocate memory");
		return 0;
	}

	LLNode* created = AddFirst(ll->head, datanode, (Dealloc*)& Delete);

	if (!created) {
		Delete(datanode);
		luaL_error(L, "Unable to allocate memory");
		return 0;
	}

	ll->head = created;

	lua_pop(L, lua_gettop(L));
	lua_pushinteger(L, datanode->id);

	return 1;
}

int LuaAddLast(lua_State* L) {
	LuaLinkedList* ll = lua_tolinkedlist(L, 1);
	LinkedListData* datanode = CreateNode(L, 2, ++ll->nextid);

	if (!datanode) {
		luaL_error(L, "Unable to allocate memory");
		return 0;
	}

	LLNode* created = AddLast(ll->head, datanode, (Dealloc*)& Delete);

	if (!created) {
		Delete(datanode);
		luaL_error(L, "Unable to allocate memory");
		return 0;
	}

	ll->head = created;

	lua_pop(L, lua_gettop(L));
	lua_pushinteger(L, datanode->id);

	return 1;
}

int Create(lua_State* L) {

	lua_pushlinkedlist(L);

	return 1;
}

LuaLinkedList* lua_pushlinkedlist(lua_State* L) {
	LuaLinkedList* ll = (LuaLinkedList*)lua_newuserdata(L, sizeof(LuaLinkedList));
	if (ll == NULL)
		luaL_error(L, "Unable to push namedpipe");
	luaL_getmetatable(L, LLINEDLIST);

	lua_setmetatable(L, -2);
	memset(ll, 0, sizeof(LuaLinkedList));

	return ll;
}

LuaLinkedList* lua_tolinkedlist(lua_State* L, int index) {
	LuaLinkedList* ll = (LuaLinkedList*)luaL_checkudata(L, index, LLINEDLIST);
	if (ll == NULL)
		luaL_error(L, "parameter is not a %s", LLINEDLIST);
	return ll;
}

int linkedlist_gc(lua_State* L) {

	LuaLinkedList* ll = lua_tolinkedlist(L, 1);

	if (ll->head) {
		FreeLinkedList(ll->head);
		ll->head = NULL;
	}

	return 0;
}

int linkedlist_tostring(lua_State* L) {
	char tim[100];
	sprintf(tim, "LinkedList: 0x%08X", lua_tolinkedlist(L, 1));
	lua_pushfstring(L, tim);
	return 1;
}