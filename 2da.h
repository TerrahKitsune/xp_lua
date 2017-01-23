#pragma once
#include "lua_main_incl.h"
static const char * TWODA = "2DA";

typedef struct Lua2da {
	char FileType[4];
	char FileVersion[4];
	int numbcols;
	int numbrows;
	char ** columns;
	char *** rows;
} Lua2da;


int twoda_open(lua_State *L);
int twoda_get2dastring(lua_State *L);
int twoda_get2darow(lua_State *L);
int twoda_get2dainfo(lua_State *L);

Lua2da * lua_pushtwoda(lua_State *L);
Lua2da * lua_totwoda(lua_State *L, int index);

int twoda_gc(lua_State *L);
int twoda_tostring(lua_State *L);