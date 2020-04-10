#pragma once
#include "lua_json.h"
#include <stdlib.h> 
#include <string.h>
#include <ctype.h>
#include <stdio.h>
#include "math.h"
#include "utf8.h"

void json_seekbuffer(JsonContext* context, int posmod);
void json_append(const char * data, size_t len, lua_State *L, JsonContext* context);
void json_bail(lua_State *L, JsonContext* context, const char * err);
unsigned int table_crc32(const unsigned char* data, int size);
bool json_addtoantirecursion(unsigned int id, JsonContext* context);
bool json_existsinantirecursion(unsigned int id, JsonContext* context);
void json_removefromantirecursion(unsigned int id, JsonContext* context);