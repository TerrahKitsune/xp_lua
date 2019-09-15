#pragma once
#include "lua_main_incl.h"
#include "luakafka.h"
#include <Windows.h>
static const char* LUAKAFKAMESSAGE = "KAFKAMSG";

typedef struct LuaKafkaMessage {

	rd_kafka_message_t* message;
	rd_kafka_t* owner;

} LuaKafkaMessage;


LuaKafkaMessage* lua_pushkafkamsg(lua_State* L);
LuaKafkaMessage* lua_tokafkamsg(lua_State* L, int index);

int GetKafkaMessageOwnerId(lua_State* L);
int GetKafkaMessageData(lua_State* L);

int kafkamsg_gc(lua_State* L);
int kafkamsg_tostring(lua_State* L);