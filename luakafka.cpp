#include "luakafka.h"
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <stdlib.h> 
#include <windows.h> 

#define kafka_error_buffer_len 1024
#define kafka_last_error_buffer_len 10240
char errorbuffer[kafka_error_buffer_len];
char lasterror[kafka_last_error_buffer_len] = {0};

static void logger(const rd_kafka_t* rk, int level, const char* fac, const char* buf) {

	size_t len = strlen(lasterror);
	_snprintf(&lasterror[len], kafka_last_error_buffer_len - len - 1, "[%08X] [%d] [%s] [%s]\n", rk, level, fac, buf);
}

void ClearLast() {
	lasterror[0] = '\0';
}

int GetLastLogs(lua_State* L) {

	lua_pushstring(L, lasterror);
	return 1;
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

void lua_pushkafkapartition(lua_State* L, const rd_kafka_metadata_partition* partition) {

	lua_createtable(L, 0, 5);

	lua_pushstring(L, "Error");
	lua_pushstring(L, rd_kafka_err2str(partition->err));
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

	lua_createtable(L, 0, 4);

	lua_pushstring(L, "Error");
	lua_pushstring(L, rd_kafka_err2str(topic->err));
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

int GetMetadata(lua_State* L) {

	ClearLast();

	LuaKafka* luak = lua_tokafka(L, 1);
	int timeout = luaL_optinteger(L, 2, 10000);
	const struct rd_kafka_metadata* metadata;

	rd_kafka_resp_err_t err = rd_kafka_metadata(luak->rd, 1, NULL, &metadata, timeout);

	lua_pop(L, lua_gettop(L));

	if (err) {

		lua_pushnil(L);
		lua_pushstring(L, rd_kafka_err2str(err));

		return 2;
	}

	lua_createtable(L, 0, 4);

	lua_pushstring(L, "BrokerId");
	lua_pushinteger(L, metadata->orig_broker_id);
	lua_settable(L, -3);

	lua_pushstring(L, "BrokerName");
	lua_pushstring(L, metadata->orig_broker_name);
	lua_settable(L, -3);

	lua_pushstring(L, "Brokers");
	lua_createtable(L, metadata->broker_cnt, 0);
	for (size_t i = 0; i < metadata->broker_cnt; i++)
	{
		lua_pushkafkabroker(L, &metadata->brokers[i]);

		lua_rawseti(L, -2, i + 1);
	}
	lua_settable(L, -3);

	lua_pushstring(L, "Topics");
	lua_createtable(L, metadata->topic_cnt, 0);
	for (size_t i = 0; i < metadata->topic_cnt; i++)
	{
		lua_pushkafkatopic(L, &metadata->topics[i]);
		lua_rawseti(L, -2, i + 1);
	}
	lua_settable(L, -3);

	rd_kafka_metadata_destroy(metadata);

	return 1;
}

int AddBroker(lua_State* L) {

	ClearLast();

	LuaKafka* luak = lua_tokafka(L, 1);
	const char* addr = luaL_checkstring(L, 2);

	lua_pop(L, lua_gettop(L));

	lua_pushboolean(L, rd_kafka_brokers_add(luak->rd, addr) != 0);

	return 1;
}

int DescribeGroups(lua_State* L) {

	ClearLast();

	LuaKafka* luak = lua_tokafka(L, 1);
	const char* group = luaL_optstring(L, 2, NULL);
	int timeout = luaL_optinteger(L, 3, 10000);

	rd_kafka_resp_err_t err;
	const struct rd_kafka_group_list* grplist;
	int i;

	err = rd_kafka_list_groups(luak->rd, group, &grplist, timeout);

	lua_pop(L, lua_gettop(L));

	if (err) {

		lua_pushnil(L);
		lua_pushstring(L, rd_kafka_err2str(err));

		return 2;
	}

	lua_createtable(L, grplist->group_cnt, 0);

	for (i = 0; i < grplist->group_cnt; i++) {

		const struct rd_kafka_group_info* gi = &grplist->groups[i];

		lua_createtable(L, 0, 7);

		lua_pushstring(L, "Broker");
		lua_pushstring(L, gi->broker.host);
		lua_pushkafkabroker(L, &gi->broker);
		lua_settable(L, -3);

		lua_pushstring(L, "Error");
		lua_pushstring(L, rd_kafka_err2str(gi->err));
		lua_settable(L, -3);

		lua_pushstring(L, "Group");
		lua_pushstring(L, gi->group);
		lua_settable(L, -3);

		lua_pushstring(L, "Protocol");
		lua_pushstring(L, gi->protocol);
		lua_settable(L, -3);

		lua_pushstring(L, "ProtocolType");
		lua_pushstring(L, gi->protocol_type);
		lua_settable(L, -3);

		lua_pushstring(L, "State");
		lua_pushstring(L, gi->state);
		lua_settable(L, -3);

		lua_pushstring(L, "Members");
		lua_createtable(L, gi->member_cnt, 0);

		for (size_t n = 0; n < gi->member_cnt; n++)
		{
			rd_kafka_group_member_info*  memberinfo = &gi->members[n];

			lua_createtable(L, 0, 5);

			lua_pushstring(L, "Host");
			lua_pushstring(L, memberinfo->client_host);
			lua_settable(L, -3);

			lua_pushstring(L, "Id");
			lua_pushstring(L, memberinfo->client_id);
			lua_settable(L, -3);

			lua_pushstring(L, "Assignment");
			lua_pushlstring(L, (const char*)memberinfo->member_assignment, memberinfo->member_assignment_size);
			lua_settable(L, -3);

			lua_pushstring(L, "MemberId");
			lua_pushstring(L, memberinfo->member_id);
			lua_settable(L, -3);

			lua_pushstring(L, "Metadata");
			lua_pushlstring(L, (const char*)memberinfo->member_metadata, memberinfo->member_metadata_size);
			lua_settable(L, -3);
		}

		lua_settable(L, -3);

		lua_rawseti(L, -2, i + 1);
	}

	rd_kafka_group_list_destroy(grplist);

	return 1;
}

int CreateConsumer(lua_State* L) {

	ClearLast();

	rd_kafka_conf_t* conf = rd_kafka_conf_new();
	rd_kafka_topic_conf_t* topic_conf = rd_kafka_topic_conf_new();

	rd_kafka_conf_set_log_cb(conf, logger);

	rd_kafka_t* rd = rd_kafka_new(RD_KAFKA_CONSUMER, conf, errorbuffer, kafka_error_buffer_len);

	lua_pop(L, lua_gettop(L));

	if (!rd) {

		rd_kafka_conf_destroy(conf);
		lua_pushnil(L);
		lua_pushstring(L, errorbuffer);

		return 2;
	}

	LuaKafka* luak = lua_pushkafka(L);
	luak->conf = conf;
	luak->rd = rd;
	luak->topic_conf = topic_conf;

	return 1;
}

LuaKafka* lua_pushkafka(lua_State* L) {
	LuaKafka* lkafka = (LuaKafka*)lua_newuserdata(L, sizeof(LuaKafka));
	if (lkafka == NULL)
		luaL_error(L, "Unable to push kafka");
	luaL_getmetatable(L, LUAKAFKA);
	lua_setmetatable(L, -2);
	memset(lkafka, 0, sizeof(LuaKafka));

	return lkafka;
}

LuaKafka* lua_tokafka(lua_State* L, int index) {
	LuaKafka* lkafka = (LuaKafka*)luaL_checkudata(L, index, LUAKAFKA);
	if (lkafka == NULL)
		luaL_error(L, "parameter is not a %s", LUAKAFKA);
	return lkafka;
}

int kafka_gc(lua_State* L) {

	LuaKafka* luak = lua_tokafka(L, 1);

	if (luak->rd) {
		rd_kafka_destroy(luak->rd);
		luak->rd = NULL;

		//rd_kafka_destroy also frees the conf
		luak->conf = NULL;
	}

	if (luak->conf) {
		rd_kafka_conf_destroy(luak->conf);
		luak->conf = NULL;
	}

	if (luak->topic_conf) {
		rd_kafka_topic_conf_destroy(luak->topic_conf);
		luak->topic_conf = NULL;
	}

	return 0;
}

int kafka_tostring(lua_State* L) {
	char tim[100];
	sprintf(tim, "Kafka: 0x%08X", lua_tokafka(L, 1));
	lua_pushfstring(L, tim);
	return 1;
}