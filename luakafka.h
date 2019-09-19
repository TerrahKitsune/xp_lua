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

int GetCommitted(lua_State* L);
int ProduceMessage(lua_State* L);
int AlterConfig(lua_State* L);
int DeleteTopic(lua_State* L);
int CreateTopic(lua_State* L);
int QueryHighLow(lua_State* L);
int GetKafkaId(lua_State* L);
int CreateConsumer(lua_State* L);
int AddBroker(lua_State* L);
int DescribeGroups(lua_State* L);
int GetLastLogs(lua_State* L);
int GetMetadata(lua_State* L);
int ConsumeMessage(lua_State* L);
int StartTopicConsumer(lua_State* L);
int CreatePartition(lua_State* L);
int PollEvents(lua_State* L);
int PausePartition(lua_State* L);
int ResumePartition(lua_State* L);
int CreateProducer(lua_State* L);
int GetConfig(lua_State* L);
int CommitMessage(lua_State* L);
int Subscribe(lua_State* L);
int PollMessages(lua_State* L);
int Assign(lua_State* L);

int kafka_gc(lua_State* L);
int kafka_tostring(lua_State* L);