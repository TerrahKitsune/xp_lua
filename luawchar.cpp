#include "luawchar.h"
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <stdlib.h> 
#include <windows.h> 

LuaWChar* lua_pushwchar(lua_State* L, wchar_t* str) {
	return lua_pushwchar(L, str, wcslen(str));
}

LuaWChar* lua_pushwchar(lua_State* L, wchar_t* str, size_t len) {

	LuaWChar* wchar = lua_pushwchar(L);

	wchar->str = (WCHAR*)gff_calloc(len + 1, sizeof(WCHAR));

	if (!wchar->str) {
		luaL_error(L, "out of memory");
		return 0;
	}

	memcpy(wchar->str, str, len * sizeof(wchar_t));

	wchar->len = len;

	return wchar;
}

int FromAnsi(lua_State* L) {

	if (lua_gettop(L) < 1) {
		return 0;
	}

	size_t len;
	const char* data = luaL_tolstring(L, 1, &len);
	lua_pop(L, 1);

	LuaWChar* wchar = lua_pushwchar(L);

	wchar->str = (WCHAR*)gff_calloc(len + 1, sizeof(WCHAR));

	if (!wchar->str) {
		luaL_error(L, "out of memory");
		return 0;
	}

	wchar->len = mbstowcs(wchar->str, data, len);

	return 1;
}


size_t to_narrow(const wchar_t* src, char* dest, size_t dest_len) {
	
	size_t i;
	wchar_t code;

	i = 0;

	for (; i < dest_len; i++) {
		code = src[i];
		if (code < 128)
			dest[i] = char(code);
		else {
			dest[i] = '?';
			if (code >= 0xD800 && code <= 0xD8FF)
				// lead surrogate, skip the next code unit, which is the trail
				i++;
		}
	}

	return i;
}

int ToWide(lua_State* L) {

	LuaWChar* wchar = lua_towchar(L, 1);

	if (!wchar->str) {
		lua_pushstring(L, "");
	}
	else {
		lua_pushlstring(L, (const char*)wchar->str, wchar->len * sizeof(wchar_t));
	}

	return 1;
}

int ToAnsi(lua_State* L) {

	LuaWChar* wchar = lua_towchar(L, 1);

	if (!wchar->str) {

		lua_pushstring(L, "");
		return 1;
	}

	char* real = (char*)gff_calloc(wchar->len + 1, sizeof(char));

	if (!real) {
		luaL_error(L, "out of memory");
		return 0;
	}

	size_t len = to_narrow(wchar->str, real, wchar->len);

	lua_pushlstring(L, (const char*)real, len * sizeof(char));

	gff_free(real);

	return 1;
}

LuaWChar* lua_pushwchar(lua_State* L) {
	LuaWChar* wchar = (LuaWChar*)lua_newuserdata(L, sizeof(LuaWChar));
	if (wchar == NULL)
		luaL_error(L, "Unable to push namedpipe");
	luaL_getmetatable(L, LUAWCHAR);
	lua_setmetatable(L, -2);
	memset(wchar, 0, sizeof(LuaWChar));

	return wchar;
}

LuaWChar* lua_towchar(lua_State* L, int index) {
	LuaWChar* wchar = (LuaWChar*)luaL_checkudata(L, index, LUAWCHAR);
	if (wchar == NULL)
		luaL_error(L, "parameter is not a %s", LUAWCHAR);
	return wchar;
}

int wchar_gc(lua_State* L) {

	LuaWChar* wchar = lua_towchar(L, 1);

	if (wchar->str) {
		gff_free(wchar->str);
	}

	ZeroMemory(wchar, sizeof(LuaWChar));

	return 0;
}

int wchar_tostring(lua_State* L) {
	return ToAnsi(L);
}

int wchar_concat(lua_State* L) {

	LuaWChar* a = lua_towchar(L, 1);
	LuaWChar* result;

	if (luaL_testudata(L, 2, LUAWCHAR)) {
		
		LuaWChar * b = lua_towchar(L, 2);

		result = lua_pushwchar(L);

		result->str = (wchar_t*)gff_calloc(a->len + b->len + 1, sizeof(wchar_t));

		if (!result->str) {
			luaL_error(L, "out of memory");
			return 0;
		}

		memcpy(result->str , a->str, a->len * sizeof(wchar_t));
		memcpy(&result->str[a->len], b->str, b->len * sizeof(wchar_t));
		result->len = a->len + b->len;
	}
	else {

		size_t len;
		const char* data = luaL_tolstring(L, 2, &len);
		lua_pop(L, 1);

		result = lua_pushwchar(L);

		result->str = (wchar_t*)gff_calloc(a->len + len + 1, sizeof(wchar_t));

		if (!result->str) {
			luaL_error(L, "out of memory");
			return 0;
		}

		memcpy(result->str, a->str, a->len * sizeof(wchar_t));	
		result->len = a->len + mbstowcs(&result->str[a->len], data, len);
	}

	return 1;
}