#pragma once
#include "lua_main_incl.h"
#include "librdkafka/rdkafka.h"
#include <Windows.h>
static const char* LUAKAFKA = "KAFKA";

#define kafka_error_buffer_len 1024
static char errorbuffer[kafka_error_buffer_len];

typedef struct LuaKafka {

	rd_kafka_type_t type;
	rd_kafka_t* rd;
	rd_kafka_queue_t* evqueue;

} LuaKafka;


LuaKafka* lua_pushkafka(lua_State* L);
LuaKafka* lua_tokafka(lua_State* L, int index);

int AlterConfig(lua_State* L);
int DeleteTopic(lua_State* L);
int CreateTopic(lua_State* L);
int QueryHighLow(lua_State* L);
int CommitMessage(lua_State* L);
int GetKafkaId(lua_State* L);
int CreateConsumer(lua_State* L);
int AddBroker(lua_State* L);
int DescribeGroups(lua_State* L);
int GetLastLogs(lua_State* L);
int GetMetadata(lua_State* L);
int PollMessage(lua_State* L);
int SubscribeToTopic(lua_State* L);
int CreatePartition(lua_State* L);
int PollEvents(lua_State* L);
int GetConfig(lua_State* L);

int kafka_gc(lua_State* L);
int kafka_tostring(lua_State* L);