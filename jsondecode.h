#pragma once
#include "lua_json.h"
#include <stdlib.h> 
#include <string.h>
#include <ctype.h>
#include <stdio.h>
#include "math.h"

void json_stepback(JsonContext* context);
char json_readnext(lua_State *L, JsonContext* context);
void json_unexpected(char c, lua_State *L, JsonContext* context);
void json_decodevalue(lua_State *L, JsonContext* context);
void json_decodecharacter(lua_State *L, JsonContext* context);
void json_decodestring(lua_State *L, JsonContext* context);
void json_advancewhitespace(lua_State *L, JsonContext* context);
void json_decodetable(lua_State *L, JsonContext* C);