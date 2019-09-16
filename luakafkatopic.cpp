#include "luakafkatopic.h"
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <stdlib.h> 
#include <windows.h> 
#include <time.h> 

int GetKafkaTopicOwnerId(lua_State* L) {

	LuaKafkaTopic* kafkamsg = lua_tokafkatopic(L, 1);

	if (!kafkamsg->topic) {
		luaL_error(L, "Kafka topic is disposed");
		return 0;
	}

	lua_pop(L, lua_gettop(L));
	lua_pushinteger(L, (lua_Integer)kafkamsg->owner);

	return 1;
}

LuaKafkaTopic* lua_pushkafkatopic(lua_State* L) {

	LuaKafkaTopic* lkafka = (LuaKafkaTopic*)lua_newuserdata(L, sizeof(LuaKafkaTopic));
	if (lkafka == NULL)
		luaL_error(L, "Unable to push kafka topic");
	luaL_getmetatable(L, LUAKAFKATOPIC);
	lua_setmetatable(L, -2);
	memset(lkafka, 0, sizeof(LuaKafkaTopic));

	return lkafka;
}

LuaKafkaTopic* lua_tokafkatopic(lua_State* L, int index) {
	LuaKafkaTopic* lkafka = (LuaKafkaTopic*)luaL_checkudata(L, index, LUAKAFKATOPIC);
	if (lkafka == NULL)
		luaL_error(L, "parameter is not a %s", LUAKAFKATOPIC);
	return lkafka;
}

int kafkatopic_gc(lua_State* L) {

	LuaKafkaTopic* luak = lua_tokafkatopic(L, 1);

	if (luak->topic) {
		rd_kafka_consume_stop(luak->topic, luak->partition);
		rd_kafka_topic_destroy(luak->topic);
		luak->topic = NULL;
	}

	luak->owner = NULL;

	return 0;
}

int kafkatopic_tostring(lua_State* L) {
	char tim[100];
	sprintf(tim, "Kafka Topic: 0x%08X", lua_tokafkatopic(L, 1));
	lua_pushfstring(L, tim);
	return 1;
}