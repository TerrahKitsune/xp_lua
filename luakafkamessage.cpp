#include "luakafkamessage.h"
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <stdlib.h> 
#include <windows.h> 
#include <time.h> 

int GetKafkaMessageOwnerId(lua_State* L) {

	LuaKafkaMessage* kafkamsg = lua_tokafkamsg(L, 1);

	if (!kafkamsg->message) {
		luaL_error(L, "Kafka message is disposed");
		return 0;
	}

	lua_pop(L, lua_gettop(L));
	lua_pushinteger(L, (lua_Integer)kafkamsg->owner);

	return 1;
}

int GetKafkaMessageLatency(lua_State* L) {

	LuaKafkaMessage* kafkamsg = lua_tokafkamsg(L, 1);

	if (!kafkamsg->message) {
		luaL_error(L, "Kafka message is disposed");
		return 0;
	}

	lua_pop(L, lua_gettop(L));
	lua_pushinteger(L, rd_kafka_message_latency(kafkamsg->message));
	return 1;
}

int GetKafkaMessageTimestamp(lua_State* L) {

	LuaKafkaMessage* kafkamsg = lua_tokafkamsg(L, 1);

	if (!kafkamsg->message) {
		luaL_error(L, "Kafka message is disposed");
		return 0;
	}

	rd_kafka_timestamp_type_t type;

	lua_pop(L, lua_gettop(L));
	lua_pushinteger(L, rd_kafka_message_timestamp((rd_kafka_message_t*)kafkamsg->message, &type));
	lua_pushinteger(L, type);
	return 2;
}

int GetKafkaMessageEqual(lua_State* L) {

	LuaKafkaMessage* kafkamsg = lua_tokafkamsg(L, 1);

	if (lua_isnoneornil(L, 2) && !kafkamsg->message) {

		lua_pop(L, lua_gettop(L));
		lua_pushboolean(L, true);
		return 1;
	}

	LuaKafkaMessage* kafkamsgother = lua_tokafkamsg(L, 2);

	lua_pop(L, lua_gettop(L));
	lua_pushboolean(L, kafkamsg == kafkamsgother && kafkamsg->message == kafkamsgother->message && kafkamsg->owner == kafkamsgother->owner);
	return 1;
}

int GetKafkaMessageData(lua_State* L) {

	LuaKafkaMessage* kafkamsg = lua_tokafkamsg(L, 1);

	if (!kafkamsg->message) {
		luaL_error(L, "Kafka message is disposed");
		return 0;
	}

	const rd_kafka_message_t* message = kafkamsg->message;

	lua_pop(L, lua_gettop(L));
	lua_createtable(L, 0, 8);

	lua_pushstring(L, "Error");
	lua_pushstring(L, rd_kafka_err2str(message->err));
	lua_settable(L, -3);

	lua_pushstring(L, "ErrorCode");
	lua_pushinteger(L, message->err);
	lua_settable(L, -3);

	lua_pushstring(L, "Key");
	lua_pushlstring(L, (const char*)message->key, message->key_len);
	lua_settable(L, -3);

	lua_pushstring(L, "Offset");
	lua_pushinteger(L, message->offset);
	lua_settable(L, -3);

	lua_pushstring(L, "Partition");
	lua_pushinteger(L, message->partition);
	lua_settable(L, -3);

	lua_pushstring(L, "Payload");
	lua_pushlstring(L, (const char*)message->payload, message->len);
	lua_settable(L, -3);

	lua_pushstring(L, "Topic");
	lua_pushstring(L, message->rkt ? rd_kafka_topic_name(message->rkt) : NULL);
	lua_settable(L, -3);

	lua_pushstring(L, "Headers");

	rd_kafka_headers_t* headers;
	if (rd_kafka_message_headers(message, &headers) == RD_KAFKA_RESP_ERR_NO_ERROR) {

		size_t count = rd_kafka_header_cnt(headers);

		lua_createtable(L, 0, count);

		const char* name;
		const char* data;
		size_t datasize;

		for (size_t i = 0; i < count; i++)
		{
			if (rd_kafka_header_get_all(headers, i, &name, (const void**)&data, &datasize) == RD_KAFKA_RESP_ERR_NO_ERROR) {
				
				lua_pushstring(L, name);
				lua_pushlstring(L, data, datasize);
				lua_settable(L, -3);
			}
		}
	}
	else {
		lua_createtable(L, 0, 0);
	}

	lua_settable(L, -3);

	return 1;
}

LuaKafkaMessage* lua_pushkafkamsg(lua_State* L) {

	LuaKafkaMessage* lkafka = (LuaKafkaMessage*)lua_newuserdata(L, sizeof(LuaKafkaMessage));
	if (lkafka == NULL)
		luaL_error(L, "Unable to push kafka message");
	luaL_getmetatable(L, LUAKAFKAMESSAGE);
	lua_setmetatable(L, -2);
	memset(lkafka, 0, sizeof(LuaKafkaMessage));

	return lkafka;
}

LuaKafkaMessage* lua_tokafkamsg(lua_State* L, int index) {
	LuaKafkaMessage* lkafka = (LuaKafkaMessage*)luaL_checkudata(L, index, LUAKAFKAMESSAGE);
	if (lkafka == NULL)
		luaL_error(L, "parameter is not a %s", LUAKAFKAMESSAGE);
	return lkafka;
}

int kafkamsg_gc(lua_State* L) {

	LuaKafkaMessage* luak = lua_tokafkamsg(L, 1);

	if (luak->message) {
		rd_kafka_message_destroy(luak->message);
		luak->message = NULL;
	}

	luak->owner = NULL;

	return 0;
}

int kafkamsg_tostring(lua_State* L) {
	char tim[100];
	sprintf(tim, "Kafka Message: 0x%08X", lua_tokafkamsg(L, 1));
	lua_pushfstring(L, tim);
	return 1;
}