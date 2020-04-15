#include "kafkahelpers.h"

rd_kafka_topic_conf_t* lua_tokafkatopicconf(lua_State* L, int idx) {

	rd_kafka_topic_conf_t * conf = rd_kafka_topic_conf_new();

	const char* confname;
	const char* confvalue;

	if (lua_type(L, idx) == LUA_TTABLE) {

		lua_pushnil(L);
		while (lua_next(L, -2) != 0) {

			confname = luaL_checkstring(L, -2);
			confvalue = luaL_checkstring(L, -1);

			if (rd_kafka_topic_conf_set(conf, confname, confvalue, errorbuffer, kafka_error_buffer_len) != RD_KAFKA_CONF_OK) {

				rd_kafka_topic_conf_destroy(conf);
				luaL_error(L, "Unable to set conf %s = %s: %s", confname, confvalue, errorbuffer);
			}

			lua_pop(L, 1);
		}
	}

	return conf;
}

rd_kafka_conf_t* lua_tokafkaconf(lua_State* L, int idx, const char* defaultgroup) {

	rd_kafka_conf_t* conf = rd_kafka_conf_new();
	bool didGroup = false;

	const char* confname;
	const char* confvalue;

	if (lua_type(L, idx) == LUA_TTABLE) {

		lua_pushnil(L);
		while (lua_next(L, -2) != 0) {

			confname = luaL_checkstring(L, -2);
			confvalue = luaL_checkstring(L, -1);

			if (!didGroup && strcmp(confname, "group.id") == 0) {
				didGroup = true;
			}

			if (rd_kafka_conf_set(conf, confname, confvalue, errorbuffer, kafka_error_buffer_len) != RD_KAFKA_CONF_OK) {

				rd_kafka_conf_destroy(conf);
				luaL_error(L, "Unable to set conf %s = %s: %s", confname, confvalue, errorbuffer);
			}

			//printf("%s = %s\n", confname, confvalue);
			lua_pop(L, 1);
		}
	}

	if (!didGroup) {
		if (rd_kafka_conf_set(conf, "group.id", defaultgroup, errorbuffer, kafka_error_buffer_len) != RD_KAFKA_CONF_OK) {

			rd_kafka_conf_destroy(conf);
			luaL_error(L, "Unable to set conf %s = %s: %s", "group.id", defaultgroup, errorbuffer);
		}
	}

	return conf;
}

void lua_pushkafkapartition(lua_State* L, const rd_kafka_metadata_partition* partition) {

	lua_createtable(L, 0, 6);

	lua_pushstring(L, "Error");
	lua_pushstring(L, rd_kafka_err2str(partition->err));
	lua_settable(L, -3);

	lua_pushstring(L, "ErrorCode");
	lua_pushinteger(L, partition->err);
	lua_settable(L, -3);

	lua_pushstring(L, "Id");
	lua_pushinteger(L, partition->id);
	lua_settable(L, -3);

	lua_pushstring(L, "Leader");
	lua_pushinteger(L, partition->leader);
	lua_settable(L, -3);

	lua_pushstring(L, "InSyncReplicas");
	lua_createtable(L, partition->isr_cnt, 0);
	for (size_t i = 0; i < partition->isr_cnt; i++)
	{
		lua_pushinteger(L, partition->isrs[i]);
		lua_rawseti(L, -2, i + 1);
	}
	lua_settable(L, -3);

	lua_pushstring(L, "Replicas");
	lua_createtable(L, partition->replica_cnt, 0);
	for (size_t i = 0; i < partition->replica_cnt; i++)
	{
		lua_pushinteger(L, partition->replicas[i]);
		lua_rawseti(L, -2, i + 1);
	}
	lua_settable(L, -3);
}

void lua_pushkafkatopic(lua_State* L, const rd_kafka_metadata_topic* topic) {

	lua_createtable(L, 0, 5);

	lua_pushstring(L, "Error");
	lua_pushstring(L, rd_kafka_err2str(topic->err));
	lua_settable(L, -3);

	lua_pushstring(L, "ErrorCode");
	lua_pushinteger(L, topic->err);
	lua_settable(L, -3);

	lua_pushstring(L, "Name");
	lua_pushstring(L, topic->topic);
	lua_settable(L, -3);

	lua_pushstring(L, "Partitions");
	lua_createtable(L, topic->partition_cnt, 0);
	for (size_t i = 0; i < topic->partition_cnt; i++)
	{
		lua_pushkafkapartition(L, &topic->partitions[i]);
		lua_rawseti(L, -2, i + 1);
	}
	lua_settable(L, -3);
}

void lua_pushkafkaptopicpartition(lua_State* L, const rd_kafka_topic_partition_t* topic) {

	lua_createtable(L, 0, 6);

	lua_pushstring(L, "Error");
	lua_pushstring(L, rd_kafka_err2str(topic->err));
	lua_settable(L, -3);

	lua_pushstring(L, "ErrorCode");
	lua_pushinteger(L, topic->err);
	lua_settable(L, -3);

	lua_pushstring(L, "Metadata");
	lua_pushlstring(L, (const char*)topic->metadata, topic->metadata_size);
	lua_settable(L, -3);

	lua_pushstring(L, "Offset");
	lua_pushinteger(L, topic->offset);
	lua_settable(L, -3);

	lua_pushstring(L, "Partition");
	lua_pushinteger(L, topic->partition);
	lua_settable(L, -3);

	lua_pushstring(L, "Topic");
	lua_pushstring(L, topic->topic);
	lua_settable(L, -3);
}

void lua_pushkafkabroker(lua_State* L, const rd_kafka_metadata_broker* broker) {

	lua_createtable(L, 0, 3);

	lua_pushstring(L, "Host");
	lua_pushstring(L, broker->host);
	lua_settable(L, -3);

	lua_pushstring(L, "Id");
	lua_pushinteger(L, broker->id);
	lua_settable(L, -3);

	lua_pushstring(L, "Port");
	lua_pushinteger(L, broker->port);
	lua_settable(L, -3);
}