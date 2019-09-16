#pragma once
#include "luakafka.h"

void lua_pushkafkabroker(lua_State* L, const rd_kafka_metadata_broker* broker);
void lua_pushkafkatopic(lua_State* L, const rd_kafka_metadata_topic* topic);
rd_kafka_conf_t* lua_tokafkaconf(lua_State* L, int idx, const char* defaultgroup);
rd_kafka_topic_conf_t* lua_tokafkatopicconf(lua_State* L, int idx);