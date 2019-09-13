#pragma once
#include "lua_main_incl.h"
#include "librdkafka/rdkafka.h"
#include <Windows.h>
static const char* LUAKAFKA = "KAFKA";

typedef struct LuaKafka {

} LuaKafka;


LuaKafka* lua_pushkafka(lua_State* L);
LuaKafka* lua_tokafka(lua_State* L, int index);

int kafka_gc(lua_State* L);
int kafka_tostring(lua_State* L);