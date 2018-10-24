#pragma once
#include "lua_main_incl.h"
#define ZIP_DISABLE_DEPRECATED
static const char * ZIP = "ZIP";
#pragma comment(lib, "zip/zip.lib")
#include "zip.h"

typedef struct LuaZIP {
	zip *z;
} LuaZIP;


int zip_deletefile(lua_State *L);
int zip_addfile(lua_State *L);
int zip_addbuffer(lua_State *L);
int zip_open(lua_State *L);
int zip_getfiles(lua_State *L);
int zip_getinfo(lua_State *L);
int zip_extract(lua_State *L);
int zip_close(lua_State *L);

LuaZIP * lua_pushzip(lua_State *L);
LuaZIP * lua_tozip(lua_State *L, int index);

int zip_gc(lua_State *L);
int zip_tostring(lua_State *L);