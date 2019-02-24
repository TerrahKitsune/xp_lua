#pragma once
#include "lua_main_incl.h"
#include <Windows.h>
static const char * IMAGE = "IMAGE";

typedef struct LuaImage {

	DWORD Width;
	DWORD Height;
	DWORD StartX;
	DWORD StartY;

	int screen = 0;
	BYTE *Data;
	size_t DataSize;
} LuaImage;


LuaImage * lua_pushimage(lua_State *L);
LuaImage * lua_toimage(lua_State *L, int index);

int lua_savetofile(lua_State *L);
int lua_screenshot(lua_State *L);
int lua_getpixels(lua_State *L);
int lua_setpixels(lua_State *L);
int lua_loadfromfile(lua_State *L);
int lua_getsize(lua_State *L);
int lua_createimage(lua_State *L);
int lua_crop(lua_State *L);
int lua_getpixelmatrix(lua_State *L);
int lua_setpixelmatrix(lua_State *L);

int image_gc(lua_State *L);
int image_tostring(lua_State *L);