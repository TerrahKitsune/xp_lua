#pragma once
#include "lua_json.h"
#include <stdlib.h> 
#include <string.h>
#include <ctype.h>
#include <stdio.h>
#include "math.h"
#include "utf8.h"

void json_encodenumber(lua_State *L, JsonContext* context);
void json_encodewidecharacter(const char * str, size_t* current, size_t len, lua_State *L, JsonContext* context);
void json_encodestring(lua_State* L, JsonContext* C);
void json_encodetable(lua_State* L, JsonContext* C, int* depth);
void json_encodevalue(lua_State *L, JsonContext* context, int* depth);

