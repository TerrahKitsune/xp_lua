#include "customwindow.h"
#include "luawindow.h"
#include "customdrawing.h"

int DrawCustomText(lua_State* L) {

	LuaCustomDrawing* window = lua_tonwindowdrawing(L, 1);
	size_t len;
	const char* data = luaL_checklstring(L, 2, &len);

	RECT rc;
	GetClientRect(window->window, &rc);

	rc.left = (LONG)luaL_optnumber(L, 3, 0);
	rc.top = (LONG)luaL_optnumber(L, 4, 0);

	lua_pop(L, lua_gettop(L));

	lua_pushinteger(L,DrawText(*window->hdc, data, len, &rc, (UINT)luaL_optinteger(L, 5, 0)));

	return 1;
}

int DrawCalcTextSize(lua_State* L) {

	LuaCustomDrawing* window = lua_tonwindowdrawing(L, 1);
	size_t len;
	const char* data = luaL_checklstring(L, 2, &len);

	SIZE size;

	if (GetTextExtentPoint32A(*window->hdc, data, len, &size)) {

		lua_pushinteger(L, size.cx);
		lua_pushinteger(L, size.cy);
	}
	else {
		return 0;
	}

	return 2;
}

int DrawSetPixel(lua_State* L) {

	LuaCustomDrawing* window = lua_tonwindowdrawing(L, 1);

	if (lua_type(L, 4) != LUA_TNUMBER) {
		lua_pushinteger(L,GetPixel(*window->hdc, (int)luaL_optnumber(L, 2, 0), (int)luaL_optnumber(L, 3, 0)));
	}
	else {
		lua_pushinteger(L, SetPixel(*window->hdc, (int)luaL_optnumber(L, 2, 0), (int)luaL_optnumber(L, 3, 0), (int)luaL_optnumber(L, 4, 0)));
	}

	return 1;
}

int DrawGetSize(lua_State* L) {

	LuaCustomDrawing* window = lua_tonwindowdrawing(L, 1);
	RECT rc;
	GetClientRect(window->window, &rc);

	lua_pushinteger(L, rc.right);
	lua_pushinteger(L, rc.bottom);

	return 2;
}

int RgbToHex(lua_State* L) {

	LuaCustomDrawing* window = lua_tonwindowdrawing(L, 1);
	lua_pushinteger(L, RGB((int)luaL_optnumber(L, 2, 0), (int)luaL_optnumber(L, 3, 0), (int)luaL_optnumber(L, 4, 0)));
	return 1;
}

int HexToRgb(lua_State* L) {

	LuaCustomDrawing* window = lua_tonwindowdrawing(L, 1);
	lua_Integer hex = luaL_checkinteger(L, 2);

	lua_pushinteger(L, GetRValue(hex));
	lua_pushinteger(L, GetGValue(hex));
	lua_pushinteger(L, GetBValue(hex));

	return 3;
}

int DrawSetBackgroundMode(lua_State* L) {

	LuaCustomDrawing* window = lua_tonwindowdrawing(L, 1);
	lua_pushinteger(L, (lua_Integer)SetBkMode(*window->hdc, (int)luaL_checkinteger(L, 2)));
	return 1;
}

int DrawSetBackgroundColor(lua_State* L) {

	LuaCustomDrawing* window = lua_tonwindowdrawing(L, 1);
	lua_pushinteger(L, SetBkColor(*window->hdc, (COLORREF)luaL_checknumber(L, 2)));
	return 1;
}

int DrawSetTextColor(lua_State* L) {

	LuaCustomDrawing* window = lua_tonwindowdrawing(L, 1);
	lua_pushinteger(L, SetTextColor(*window->hdc, (COLORREF)luaL_checknumber(L, 2)));
	return 1;
}

int CreateCustomDrawing(lua_State* L) {

	LuaCustomDrawing* draw = lua_pushwindowdrawing(L);

	return 1;
}

LuaCustomDrawing* lua_pushwindowdrawing(lua_State* L) {

	LuaCustomDrawing* window = (LuaCustomDrawing*)lua_newuserdata(L, sizeof(LuaCustomDrawing));
	if (window == NULL)
		luaL_error(L, "Unable to push windowdraw");
	luaL_getmetatable(L, LUAWINDOWDRAW);
	lua_setmetatable(L, -2);
	memset(window, 0, sizeof(LuaCustomDrawing));

	window->paintFunctionRef = LUA_REFNIL;

	return window;
}

LuaCustomDrawing* lua_tonwindowdrawing(lua_State* L, int index) {

	LuaCustomDrawing* window = (LuaCustomDrawing*)luaL_checkudata(L, index, LUAWINDOWDRAW);
	if (window == NULL)
		luaL_error(L, "parameter is not a %s", LUAWINDOWDRAW);
	return window;
}

int windowdrawing_gc(lua_State* L) {

	LuaCustomDrawing* window = lua_tonwindowdrawing(L, 1);

	if (window->paintFunctionRef != LUA_REFNIL) {
		luaL_unref(L, LUA_REGISTRYINDEX, window->paintFunctionRef);
	}

	ZeroMemory(window, sizeof(LuaCustomDrawing));

	return 0;
}

int windowdrawing_tostring(lua_State* L) {
	char tim[100];
	sprintf(tim, "WindowDrawing: 0x%08X", lua_tonwindowdrawing(L, 1));
	lua_pushfstring(L, tim);
	return 1;
}

static const struct luaL_Reg windowdrawingfunctions[] = {

	{ "GetSize", DrawGetSize },
	{ "Pixel", DrawSetPixel },
	{ "RgbToHex", RgbToHex },
	{ "HexToRgb", HexToRgb },
	{ "SetBackgroundMode", DrawSetBackgroundMode },
	{ "SetBackgroundColor", DrawSetBackgroundColor},
	{ "SetTextColor", DrawSetTextColor},
	{ "Text", DrawCustomText },
	{ "CalcTextSize", DrawCalcTextSize },
	{ NULL, NULL }
};

static const luaL_Reg windowdrawingmeta[] = {
	{ "__gc",  windowdrawing_gc },
	{ "__tostring",  windowdrawing_tostring },
	{ NULL, NULL }
};

int luaopen_windowdrawing(lua_State* L) {

	luaL_newlibtable(L, windowdrawingfunctions);
	luaL_setfuncs(L, windowdrawingfunctions, 0);

	luaL_newmetatable(L, LUAWINDOWDRAW);
	luaL_setfuncs(L, windowdrawingmeta, 0);

	lua_pushliteral(L, "__index");
	lua_pushvalue(L, -3);
	lua_rawset(L, -3);
	lua_pushliteral(L, "__metatable");
	lua_pushvalue(L, -3);
	lua_rawset(L, -3);

	lua_pop(L, 1);
	return 1;
}