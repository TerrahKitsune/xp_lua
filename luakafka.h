#pragma once
#include "lua_main_incl.h"
#include "librdkafka/rdkafka.h"
#include <Windows.h>
static const char* LUAKAFKA = "KAFKA";

typedef struct LuaKafka {

	rd_kafka_type_t type;
	rd_kafka_t* rd;
	rd_kafka_topic_partition_list_t * subscribelist;
} LuaKafka;


LuaKafka* lua_pushkafka(lua_State* L);
LuaKafka* lua_tokafka(lua_State* L, int index);

int CreateConsumer(lua_State* L);
int AddBroker(lua_State* L);
int DescribeGroups(lua_State* L);
int GetLastLogs(lua_State* L);
int GetMetadata(lua_State* L);
int PollMessage(lua_State* L);
int SubscribeToTopic(lua_State* L);

int kafka_gc(lua_State* L);
int kafka_tostring(lua_State* L);