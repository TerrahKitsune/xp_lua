#include "LuaFrame.h"
#include <stdlib.h>
#include <string.h>
#define MAX(x, y) (((x) > (y)) ? (x) : (y))
#define MIN(x, y) (((x) < (y)) ? (x) : (y))

LuaFrame * lua_toframe(lua_State *L, int idx, const char * method){

	if (!lua_istable(L, idx))
		luaL_error(L, "lua_toframe index %d is not a table", idx);

	size_t keylen = 0;
	size_t valuelen = 0;
	size_t size = 0;
	int entries = 0;
	int valuetype;
	lua_pushvalue(L, idx);
	lua_pushnil(L);
	while (lua_next(L, -2) != 0) {

		/* uses 'key' (at index -2) and 'value' (at index -1) */
		if (lua_type(L, -2) != LUA_TSTRING){
			lua_pop(L, 1);
			continue;
		}

		valuelen = 0;
		valuetype = lua_type(L, -1);

		if (valuetype == LUA_TNUMBER)
			valuelen = sizeof(LUA_NUMBER);
		else if (valuetype == LUA_TBOOLEAN)
			valuelen = 1;
		else if(valuetype == LUA_TSTRING)
			lua_tolstring(L, -1, &valuelen);
		else{
			lua_pop(L, 1);
			continue;
		}


		if (!lua_tolstring(L, -2, &keylen)){
			lua_pop(L, 1);
			continue;
		}

		size += (valuelen + keylen);
		entries++;

		lua_pop(L, 1);
	}

	size_t trailing = (sizeof(LuaFrame) + (sizeof(LuaFrameKey)*entries));
	size += trailing;
	char * data = (char*)malloc(size);
	char * tail;
	LuaFrameKey * keys;
	LuaFrame * frame = (LuaFrame*)data;
	const char * key;
	if (!frame){
		lua_pop(L, 1);
		return NULL;
	}
	tail = &data[trailing];
	keys = &frame->keys[0];

	strncpy(frame->method, method, 16);
	frame->length = size;
	frame->numbkeys = entries;
	LUA_NUMBER number;

	int dataoffset = 0;

	const char * valuestring;
	entries = 0;
	lua_pushnil(L);
	while (lua_next(L, -2) != 0) {

		/* uses 'key' (at index -2) and 'value' (at index -1) */
		valuetype = lua_type(L, -1);
		if (entries == frame->numbkeys || lua_type(L, -2) != LUA_TSTRING ||
			(valuetype != LUA_TSTRING &&
			valuetype != LUA_TNUMBER &&
			valuetype != LUA_TBOOLEAN)){
			lua_pop(L, 1);
			continue;
		}

		key = lua_tolstring(L, -2, &keylen);
		if (!key){
			lua_pop(L, 1);
			continue;
		}

		keys[entries].luatype = valuetype;
		keys[entries].keyoffset = dataoffset;
		keys[entries].keylength = keylen;
		memcpy(&tail[dataoffset], key, keylen);
		dataoffset += keylen;

		if (valuetype == LUA_TNUMBER){
			keys[entries].datalength = sizeof(LUA_NUMBER);
			keys[entries].dataoffset = dataoffset;
			number = lua_tonumber(L, -1);
			memcpy(&tail[dataoffset], &number, sizeof(LUA_NUMBER));
			dataoffset += sizeof(LUA_NUMBER);
		}
		else if (valuetype == LUA_TBOOLEAN){
			keys[entries].datalength = sizeof(char);
			keys[entries].dataoffset = dataoffset;
			tail[dataoffset] = lua_toboolean(L, -1) ? 1 : 0;
			dataoffset += sizeof(char);
		}
		else if (valuetype == LUA_TSTRING){
			valuestring = lua_tolstring(L, -1, &valuelen);
			keys[entries].datalength = valuelen;
			keys[entries].dataoffset = dataoffset;
			memcpy(&tail[dataoffset], valuestring, valuelen);
			dataoffset += valuelen;
		}
		else{
			lua_pop(L, 1);
			continue;
		}

		entries++;

		lua_pop(L, 1);
	}

	lua_pop(L, 1);
	return frame;
}

void lua_pushframe(lua_State *L, LuaFrame * frame){

	char * data = (char*)frame;
	data = &data[sizeof(LuaFrame) + (sizeof(LuaFrameKey)*frame->numbkeys)];
	LUA_NUMBER number;
	char boolean;
	lua_createtable(L, 0, frame->numbkeys);
	for (int n = 0; n < frame->numbkeys; n++){
		if (frame->keys[n].datalength + frame->keys[n].dataoffset >= frame->length ||
			frame->keys[n].keyoffset + frame->keys[n].keylength >= frame->length)
			continue;
		lua_pushlstring(L, &data[frame->keys[n].keyoffset], frame->keys[n].keylength);

		switch (frame->keys[n].luatype){
		case LUA_TNUMBER:
			memcpy(&number, &data[frame->keys[n].dataoffset], MIN(sizeof(LUA_NUMBER), frame->keys[n].datalength));
			lua_pushnumber(L, number);
			break;
		case LUA_TBOOLEAN:
			boolean = data[frame->keys[n].dataoffset];
			lua_pushboolean(L, boolean>0);
			break;
		case LUA_TSTRING:
			lua_pushlstring(L, &data[frame->keys[n].dataoffset], frame->keys[n].datalength);
			break;
		default:
			lua_pushnil(L);
			break;
		}

		lua_settable(L, -3);
	}
}

void FreeLuaFrame(LuaFrame * frame){
	free(frame);
}