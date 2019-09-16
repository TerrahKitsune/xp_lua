#pragma once
#include "lua_main_incl.h"
#include "luakafka.h"
#include <Windows.h>
static const char* LUAKAFKATOPIC = "KAFKATOPIC";

typedef struct LuaKafkaTopic {

	rd_kafka_t* owner;
	rd_kafka_topic_t* topic;
	int partition;
	char * name;
} LuaKafkaTopic;


LuaKafkaTopic* lua_pushkafkatopic(lua_State* L, const char * name);
LuaKafkaTopic* lua_tokafkatopic(lua_State* L, int index);

int GetKafkaTopicOwnerId(lua_State* L);
int GetKafkaTopicInfo(lua_State* L);

int kafkatopic_gc(lua_State* L);
int kafkatopic_tostring(lua_State* L);