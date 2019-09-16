#include "luakafka.h"
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <stdlib.h> 
#include <windows.h> 
#include <time.h> 
#include "luakafkamessage.h"
#include "luakafkatopic.h"

#define kafka_error_buffer_len 1024
#define kafka_last_error_buffer_len 10240
char errorbuffer[kafka_error_buffer_len];
char lasterror[kafka_last_error_buffer_len] = { 0 };
int CurrentlyOpenKafkas = 0;
FILE* KafkaLogFile = NULL;

static void logger(const rd_kafka_t* rk, int level, const char* fac, const char* buf) {

	time_t rawtime;
	struct tm* timeinfo;
	time(&rawtime);
	timeinfo = localtime(&rawtime);
	char timestamp[100];
	strftime(timestamp, 100, "%x %X", timeinfo);

	if (KafkaLogFile) {

		fprintf(KafkaLogFile, "[%s] [%08X] [%d] [%s]: %s\n", timestamp, rk, level, fac, buf);
		fflush(KafkaLogFile);
	}

	size_t len = strlen(lasterror);
	_snprintf(&lasterror[len], kafka_last_error_buffer_len - len - 1, "[%s] [%08X] [%d] [%s]: %s\n", timestamp, rk, level, fac, buf);
}

int GetLastLogs(lua_State* L) {

	const char* logfile = luaL_optstring(L, lua_type(L, 1) == LUA_TSTRING ? 1 : 2, NULL);

	if (logfile) {

		if (KafkaLogFile) {
			fclose(KafkaLogFile);
			KafkaLogFile = NULL;
		}

		if (logfile[0] != '\0') {

			KafkaLogFile = fopen(logfile, "a");
		}
	}

	lasterror[0] = '\0';
	lua_pushstring(L, lasterror);
	return 1;
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

void lua_pushkafkamessage(lua_State* L, rd_kafka_message_t* message, rd_kafka_t* owner) {

	LuaKafkaMessage* msg = lua_pushkafkamsg(L);
	msg->message = message;
	msg->owner = owner;
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

int CommitMessage(lua_State* L) {

	LuaKafka* luak = lua_tokafka(L, 1);

	if (!luak->rd) {
		luaL_error(L, "Kafka object not open");
		return 0;
	}

	LuaKafkaMessage* kafkamsg = lua_tokafkamsg(L, 2);

	if (!kafkamsg->message) {
		luaL_error(L, "Kafka message is disposed");
		return 0;
	}
	else if (kafkamsg->owner != luak->rd) {
		luaL_error(L, "Kafka message owner missmatch %d != %d", kafkamsg->owner, luak->rd);
		return 0;
	}

	rd_kafka_resp_err_t err = rd_kafka_commit_message(luak->rd, kafkamsg->message, luaL_optinteger(L, 3, 0));

	lua_pop(L, lua_gettop(L));
	lua_pushboolean(L, !err);

	if (err) {

		lua_pushstring(L, rd_kafka_err2str(err));
		return 2;
	}

	return 1;
}

int PollMessage(lua_State* L) {

	LuaKafka* luak = lua_tokafka(L, 1);

	if (!luak->rd) {
		luaL_error(L, "Kafka object not open");
		return 0;
	}

	LuaKafkaTopic* topic = lua_tokafkatopic(L, 2);

	if (!topic->topic) {
		luaL_error(L, "Topic object not open");
		return 0;
	}
	else if (topic->owner != luak->rd) {
		luaL_error(L, "Invalid kafka owner for topic");
		return 0;
	}

	int timeout = luaL_optinteger(L, 3, 1000);
	rd_kafka_message_t* rkmessage;

	rd_kafka_poll(luak->rd, 0);

	rkmessage = rd_kafka_consume(topic->topic, topic->partition, timeout);

	lua_pop(L, lua_gettop(L));

	if (rkmessage) {
		lua_pushkafkamessage(L, rkmessage, luak->rd);
	}
	else {
		lua_pushnil(L);
	}

	return 1;
}

int GetMetadata(lua_State* L) {

	LuaKafka* luak = lua_tokafka(L, 1);

	if (!luak->rd) {
		luaL_error(L, "Kafka object not open");
		return 0;
	}

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

	LuaKafka* luak = lua_tokafka(L, 1);

	if (!luak->rd) {
		luaL_error(L, "Kafka object not open");
		return 0;
	}

	const char* addr = luaL_checkstring(L, 2);

	lua_pop(L, lua_gettop(L));

	lua_pushboolean(L, rd_kafka_brokers_add(luak->rd, addr) != 0);

	return 1;
}

int DescribeGroups(lua_State* L) {

	LuaKafka* luak = lua_tokafka(L, 1);

	if (!luak->rd) {
		luaL_error(L, "Kafka object not open");
		return 0;
	}

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

		lua_createtable(L, 0, 8);

		lua_pushstring(L, "Broker");
		lua_pushkafkabroker(L, &gi->broker);
		lua_settable(L, -3);

		lua_pushstring(L, "Error");
		lua_pushstring(L, rd_kafka_err2str(gi->err));
		lua_settable(L, -3);

		lua_pushstring(L, "ErrorCode");
		lua_pushinteger(L, gi->err);
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
			rd_kafka_group_member_info* memberinfo = &gi->members[n];

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

			lua_rawseti(L, -2, n + 1);
		}
		lua_settable(L, -3);

		lua_rawseti(L, -2, i + 1);
	}

	rd_kafka_group_list_destroy(grplist);

	return 1;
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

int GetKafkaId(lua_State* L) {

	LuaKafka* luak = lua_tokafka(L, 1);

	if (!luak->rd) {
		luaL_error(L, "Kafka object not open");
		return 0;
	}

	lua_pop(L, lua_gettop(L));
	lua_pushinteger(L, (lua_Integer)luak->rd);

	return 1;
}

int CreateConsumer(lua_State* L) {

	rd_kafka_conf_t* conf = lua_tokafkaconf(L, 1, "LUAC");

	lua_pop(L, lua_gettop(L));

	rd_kafka_conf_set_log_cb(conf, logger);

	rd_kafka_t* rd = rd_kafka_new(RD_KAFKA_CONSUMER, conf, errorbuffer, kafka_error_buffer_len);

	if (!rd) {

		rd_kafka_conf_destroy(conf);
		lua_pushnil(L);
		lua_pushstring(L, errorbuffer);

		return 2;
	}

	LuaKafka* luak = lua_pushkafka(L);
	luak->rd = rd;
	luak->type = RD_KAFKA_CONSUMER;

	return 1;
}

int QueryHighLow(lua_State* L) {

	LuaKafka* luak = lua_tokafka(L, 1);

	if (!luak->rd) {
		luaL_error(L, "Kafka object not open");
		return 0;
	}

	const char* topic = luaL_checkstring(L, 2);
	int partition = luaL_checkinteger(L, 3);
	int timeout = luaL_optinteger(L, 4, 10000);
	int64_t low;
	int64_t high;

	rd_kafka_resp_err_t err = rd_kafka_query_watermark_offsets(luak->rd, topic, partition, &low, &high, timeout);
	lua_pop(L, lua_gettop(L));
	lua_pushboolean(L, !err);

	if (err) {
		lua_pushstring(L, rd_kafka_err2str(err));
		return 2;
	}

	lua_pushinteger(L, low);
	lua_pushinteger(L, high);

	return 3;
}

int SubscribeToTopic(lua_State* L) {

	LuaKafka* luak = lua_tokafka(L, 1);

	if (!luak->rd) {
		luaL_error(L, "Kafka object not open");
		return 0;
	}

	const char* topic = luaL_checkstring(L, 2);
	int partition = luaL_checkinteger(L, 3);
	int64_t offset = luaL_checkinteger(L, 4);
	rd_kafka_topic_conf_t* conf = NULL; // rd_kafka_topic_conf_new();

	rd_kafka_topic_t* rkt = rd_kafka_topic_new(luak->rd, topic, conf);

	lua_pop(L, lua_gettop(L));

	if (rd_kafka_consume_start(rkt, partition, offset) == -1) {

		rd_kafka_resp_err_t err = rd_kafka_last_error();
		lua_pushnil(L);
		lua_pushstring(L, rd_kafka_err2str(err));
		rd_kafka_topic_destroy(rkt);
		return 2;
	}

	LuaKafkaTopic* ltopic = lua_pushkafkatopic(L);
	ltopic->owner = luak->rd;
	ltopic->partition = partition;
	ltopic->topic = rkt;

	return 1;
}

LuaKafka* lua_pushkafka(lua_State* L) {
	LuaKafka* lkafka = (LuaKafka*)lua_newuserdata(L, sizeof(LuaKafka));
	if (lkafka == NULL)
		luaL_error(L, "Unable to push kafka");
	luaL_getmetatable(L, LUAKAFKA);
	lua_setmetatable(L, -2);
	memset(lkafka, 0, sizeof(LuaKafka));

	CurrentlyOpenKafkas++;

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

		if (luak->type == RD_KAFKA_CONSUMER) {
			rd_kafka_consumer_close(luak->rd);
		}

		rd_kafka_destroy(luak->rd);
		luak->rd = NULL;

		if (--CurrentlyOpenKafkas <= 0) {

			if (KafkaLogFile) {
				fclose(KafkaLogFile);
				KafkaLogFile = NULL;
			}
		}
	}

	return 0;
}

int kafka_tostring(lua_State* L) {
	char tim[100];
	sprintf(tim, "Kafka: 0x%08X", lua_tokafka(L, 1));
	lua_pushfstring(L, tim);
	return 1;
}