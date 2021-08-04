#include "macro.h"
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <stdlib.h> 
#include <windows.h> 

int CreateMacro(lua_State* L) {

	if (!lua_istable(L, 1)) {
		lua_pop(L, lua_gettop(L));
		lua_pushnil(L);
		return 1;
	}

	lua_pop(L, lua_gettop(L) - 1);

	size_t len = lua_rawlen(L, 1);

	if (len == 0) {
		lua_pop(L, lua_gettop(L));
		lua_pushnil(L);
		return 1;
	}

	LuaMacro* macro = lua_pushmacro(L);

	lua_pushvalue(L, 1);

	macro->inputs = (INPUT*)gff_calloc(len, sizeof(INPUT));

	if (!macro->inputs) {
		lua_pop(L, 1);
		lua_pushnil(L);
		return 1;
	}
	else {
		macro->length = len;
	}

	for (size_t i = 0; i < len; i++)
	{
		lua_pushinteger(L, i + 1);

		lua_gettable(L, -2);

		if (lua_type(L, -1) != LUA_TTABLE) {

			lua_pop(L, lua_gettop(L));
			lua_pushinteger(L, 0);
			return 1;
		}
		else {

			lua_pushstring(L, "Type");
			lua_gettable(L, -2);
			if (lua_isnumber(L, -1)) {
				macro->inputs[i].type = lua_tointeger(L, -1);
			}
			else {
				luaL_error(L, "Invalid Type in macro input");
				return 0;
			}
			lua_pop(L, 1);

			if (macro->inputs[i].type == INPUT_KEYBOARD)
			{
				lua_pushstring(L, "Key");
				lua_gettable(L, -2);
				if (lua_isnumber(L, -1)) {
					macro->inputs[i].ki.wVk = lua_tointeger(L, -1);
				}
				lua_pop(L, 1);

				lua_pushstring(L, "Flags");
				lua_gettable(L, -2);
				if (lua_isnumber(L, -1)) {
					macro->inputs[i].ki.dwFlags = lua_tointeger(L, -1);
				}
				lua_pop(L, 1);

				lua_pushstring(L, "ExtraInfo");
				lua_gettable(L, -2);
				if (lua_isnumber(L, -1)) {
					macro->inputs[i].ki.dwExtraInfo = lua_tointeger(L, -1);
				}
				lua_pop(L, 1);

				lua_pushstring(L, "Time");
				lua_gettable(L, -2);
				if (lua_isnumber(L, -1)) {
					macro->inputs[i].ki.time = lua_tointeger(L, -1);
				}
				lua_pop(L, 1);

				lua_pushstring(L, "Scan");
				lua_gettable(L, -2);
				if (lua_isnumber(L, -1)) {
					macro->inputs[i].ki.wScan = lua_tointeger(L, -1);
				}
				lua_pop(L, 1);
			}
			else if (macro->inputs[i].type == INPUT_MOUSE) {

				lua_pushstring(L, "ExtraInfo");
				lua_gettable(L, -2);
				if (lua_isnumber(L, -1)) {
					macro->inputs[i].mi.dwExtraInfo = lua_tointeger(L, -1);
				}
				lua_pop(L, 1);

				lua_pushstring(L, "Flags");
				lua_gettable(L, -2);
				if (lua_isnumber(L, -1)) {
					macro->inputs[i].mi.dwFlags = lua_tointeger(L, -1);
				}
				lua_pop(L, 1);

				lua_pushstring(L, "X");
				lua_gettable(L, -2);
				if (lua_isnumber(L, -1)) {
					macro->inputs[i].mi.dx = (LONG)lua_tonumber(L, -1);
				}
				lua_pop(L, 1);

				lua_pushstring(L, "Y");
				lua_gettable(L, -2);
				if (lua_isnumber(L, -1)) {
					macro->inputs[i].mi.dy = (LONG)lua_tonumber(L, -1);
				}
				lua_pop(L, 1);

				lua_pushstring(L, "Data");
				lua_gettable(L, -2);
				if (lua_isnumber(L, -1)) {
					macro->inputs[i].mi.mouseData = lua_tointeger(L, -1);
				}
				lua_pop(L, 1);

				lua_pushstring(L, "Time");
				lua_gettable(L, -2);
				if (lua_isnumber(L, -1)) {
					macro->inputs[i].mi.time = lua_tointeger(L, -1);
				}
				lua_pop(L, 1);
			}
			else {
				luaL_error(L, "Invalid Type in macro input");
				return 0;
			}
		}

		lua_pop(L, 1);
	}

	lua_pop(L, 1);

	return 1;
}

int ScreenToMouse(lua_State* L) {

	lua_Number x = luaL_checknumber(L, 1);
	lua_Number y = luaL_checknumber(L, 2);

	lua_pop(L, lua_gettop(L));

	lua_pushinteger(L, (lua_Integer)((x * 65536) / GetSystemMetrics(SM_CXSCREEN)));
	lua_pushinteger(L, (lua_Integer)((y * 65536) / GetSystemMetrics(SM_CYSCREEN)));

	return 2;
}

int SendMacro(lua_State* L) {

	LuaMacro* macro = lua_tomacro(L, 1);
	lua_pushinteger(L, (lua_Integer)SendInput(macro->length, macro->inputs, sizeof(INPUT)));
	return 1;
}

int GetInputs(lua_State* L) {

	LuaMacro* macro = lua_tomacro(L, 1);

	lua_createtable(L, macro->length, 0);

	for (size_t i = 0; i < macro->length; i++)
	{
		if (macro->inputs[i].type == INPUT_KEYBOARD) {

			lua_createtable(L, 0, 6);

			lua_pushstring(L, "Type");
			lua_pushinteger(L, macro->inputs[i].type);
			lua_settable(L, -3);

			lua_pushstring(L, "ExtraInfo");
			lua_pushinteger(L, macro->inputs[i].ki.dwExtraInfo);
			lua_settable(L, -3);

			lua_pushstring(L, "Flags");
			lua_pushinteger(L, macro->inputs[i].ki.dwFlags);
			lua_settable(L, -3);

			lua_pushstring(L, "Time");
			lua_pushinteger(L, macro->inputs[i].ki.time);
			lua_settable(L, -3);

			lua_pushstring(L, "Scan");
			lua_pushinteger(L, macro->inputs[i].ki.wScan);
			lua_settable(L, -3);

			lua_pushstring(L, "Key");
			lua_pushinteger(L, macro->inputs[i].ki.wVk);
			lua_settable(L, -3);
		}
		else if (macro->inputs[i].type == INPUT_MOUSE) {

			lua_createtable(L, 0, 7);

			lua_pushstring(L, "Type");
			lua_pushinteger(L, macro->inputs[i].type);
			lua_settable(L, -3);

			lua_pushstring(L, "ExtraInfo");
			lua_pushinteger(L, macro->inputs[i].mi.dwExtraInfo);
			lua_settable(L, -3);

			lua_pushstring(L, "Flags");
			lua_pushinteger(L, macro->inputs[i].mi.dwFlags);
			lua_settable(L, -3);

			lua_pushstring(L, "Time");
			lua_pushinteger(L, macro->inputs[i].mi.time);
			lua_settable(L, -3);

			lua_pushstring(L, "Data");
			lua_pushinteger(L, macro->inputs[i].mi.mouseData);
			lua_settable(L, -3);

			lua_pushstring(L, "X");
			lua_pushinteger(L, macro->inputs[i].mi.dx);
			lua_settable(L, -3);

			lua_pushstring(L, "Y");
			lua_pushinteger(L, macro->inputs[i].mi.dy);
			lua_settable(L, -3);
		}
		else {
			lua_createtable(L, 0, 1);

			lua_pushstring(L, "Type");
			lua_pushinteger(L, macro->inputs[i].type);
			lua_settable(L, -3);
		}

		lua_rawseti(L, -2, i + 1);
	}

	DumpStack(L);

	return 1;
}

LuaMacro* lua_pushmacro(lua_State* L) {
	LuaMacro* macro = (LuaMacro*)lua_newuserdata(L, sizeof(LuaMacro));
	if (macro == NULL)
		luaL_error(L, "Unable to push macro");
	luaL_getmetatable(L, MACRO);
	lua_setmetatable(L, -2);
	memset(macro, 0, sizeof(LuaMacro));

	return macro;
}

LuaMacro* lua_tomacro(lua_State* L, int index) {
	LuaMacro* macro = (LuaMacro*)luaL_checkudata(L, index, MACRO);
	if (macro == NULL)
		luaL_error(L, "parameter is not a %s", MACRO);
	return macro;
}

int macro_gc(lua_State* L) {

	LuaMacro* macro = lua_tomacro(L, 1);

	if (macro->inputs) {
		gff_free(macro->inputs);
	}

	ZeroMemory(macro, sizeof(LuaMacro));

	return 0;
}

int macro_tostring(lua_State* L) {
	char tim[100];
	sprintf(tim, "Macro: 0x%08X", lua_tomacro(L, 1));
	lua_pushfstring(L, tim);
	return 1;
}