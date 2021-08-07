#pragma once
#include "lua_main_incl.h"
#include "luawindow.h"
#include <Windows.h>

static const char* LUAWINDOWDRAW = "LUAWINDOWDRAW";

typedef struct LuaCustomDrawing {

	PAINTSTRUCT *ps;
	HDC         *hdc;
	HWND		window;

	int paintFunctionRef;

} LuaCustomDrawing;

int luaopen_windowdrawing(lua_State* L);

LuaCustomDrawing* lua_pushwindowdrawing(lua_State* L);
LuaCustomDrawing* lua_tonwindowdrawing(lua_State* L, int index);

int DrawCustomText(lua_State* L);
int CreateCustomDrawing(lua_State* L);
int DrawSetTextColor(lua_State* L);
int DrawSetBackgroundColor(lua_State* L);
int DrawSetBackgroundMode(lua_State* L);
int RgbToHex(lua_State* L);
int HexToRgb(lua_State* L);
int DrawSetPixel(lua_State* L);
int DrawGetSize(lua_State* L);
int DrawCalcTextSize(lua_State* L);

int windowdrawing_gc(lua_State* L);
int windowdrawing_tostring(lua_State* L);