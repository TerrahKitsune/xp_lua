#include "luakafka.h"
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <stdlib.h> 
#include <windows.h> 
#include <time.h> 
#include "luakafkamessage.h"
#include "luakafkatopic.h"
#include "kafkahelpers.h"

#define kafka_last_error_buffer_len 10240
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

int PollEvents(lua_State* L) {

	LuaKafka* luak = lua_tokafka(L, 1);

	if (!luak->rd) {
		luaL_error(L, "Kafka object not open");
		return 0;
	}

	rd_kafka_event_t * ev = rd_kafka_queue_poll(luak->evqueue, 0);
	lua_pop(L, lua_gettop(L));
	if (ev) {

		lua_createtable(L, 0, 4);

		lua_pushstring(L, "Name");
		lua_pushstring(L, rd_kafka_event_name(ev));
		lua_settable(L, -3);

		lua_pushstring(L, "Type");
		lua_pushinteger(L, rd_kafka_event_type(ev));
		lua_settable(L, -3);

		lua_pushstring(L, "Error");
		lua_pushstring(L, rd_kafka_event_error_string(ev));
		lua_settable(L, -3);

		lua_pushstring(L, "ErrorCode");
		lua_pushinteger(L, rd_kafka_event_error(ev));
		lua_settable(L, -3);
	}
	else {
		lua_pushnil(L);
	}

	return 1;
}

int PollMessage(lua_State* L) {

	LuaKafka* luak = lua_tokafka(L, 1);

	if (!luak->rd) {
		luaL_error(L, "Kafka object not open");
		return 0;
	}
	else if (luak->type != RD_KAFKA_CONSUMER) {
		luaL_error(L, "Only consumers may poll for messages");
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

	int timeout = luaL_optinteger(L, 3, 0);
	rd_kafka_message_t* rkmessage;

	rkmessage = rd_kafka_consume(topic->topic, topic->partition, timeout);

	lua_pop(L, lua_gettop(L));

	if (rkmessage) {
		LuaKafkaMessage* msg = lua_pushkafkamsg(L);
		msg->message = rkmessage;
		msg->owner = luak->rd;
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
	luak->evqueue = rd_kafka_queue_get_main(rd);

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

int DeleteTopic(lua_State* L) {

	LuaKafka* luak = lua_tokafka(L, 1);
	const char * topicname = luaL_checkstring(L, 2);
	int timeout = luaL_optinteger(L, 3, 10000);

	if (!luak->rd) {
		luaL_error(L, "Kafka object not open");
		return 0;
	}

	rd_kafka_DeleteTopic_t * topic = rd_kafka_DeleteTopic_new(topicname);
	rd_kafka_AdminOptions_t * adminopts = rd_kafka_AdminOptions_new(luak->rd, RD_KAFKA_ADMIN_OP_ANY);

	rd_kafka_resp_err_t err = rd_kafka_AdminOptions_set_request_timeout(adminopts, luaL_optinteger(L, 3, 10000), errorbuffer, kafka_error_buffer_len);
	if (err) {
		rd_kafka_DeleteTopic_destroy(topic);
		rd_kafka_AdminOptions_destroy(adminopts);
		lua_pop(L, lua_gettop(L));
		lua_pushboolean(L, false);
		lua_pushstring(L, errorbuffer);
		return 2;
	}

	rd_kafka_queue_t * queue = rd_kafka_queue_new(luak->rd);
	rd_kafka_DeleteTopics(luak->rd, &topic, 1, adminopts, queue);

	rd_kafka_event_t * event = rd_kafka_queue_poll(queue, timeout);
	err = rd_kafka_event_error(event);

	rd_kafka_event_destroy(event);
	rd_kafka_queue_destroy(queue);
	rd_kafka_DeleteTopic_destroy(topic);
	rd_kafka_AdminOptions_destroy(adminopts);

	lua_pop(L, lua_gettop(L));
	lua_pushboolean(L, !err);
	if (err)
	{
		lua_pushstring(L, rd_kafka_err2str(err));
		return 2;
	}

	return 1;
}

int GetConfig(lua_State* L) {

	LuaKafka* luak = lua_tokafka(L, 1);
	int configtype = luaL_checkinteger(L, 2);
	const char * name = luaL_checkstring(L, 3);
	
	if (!luak->rd) {
		luaL_error(L, "Kafka object not open");
		return 0;
	}

	rd_kafka_ResourceType_t restype = RD_KAFKA_RESOURCE_UNKNOWN;
	switch (configtype) {

	case 1:
		restype = RD_KAFKA_RESOURCE_ANY;
		break;
	case 2:
		restype = RD_KAFKA_RESOURCE_TOPIC;
		break;
	case 3:
		restype = RD_KAFKA_RESOURCE_GROUP;
		break;
	case 4:
		restype = RD_KAFKA_RESOURCE_BROKER;
		break;
	}

	rd_kafka_ConfigResource_t* conf = rd_kafka_ConfigResource_new(restype, name);
	size_t len;
	const rd_kafka_ConfigEntry_t ** configs = rd_kafka_ConfigResource_configs(conf, &len);
	const rd_kafka_ConfigEntry_t * keyvalue;

	lua_pop(L, lua_gettop(L));
	lua_createtable(L, 0, len);

	for (size_t i = 0; i < len; i++)
	{
		lua_pushstring(L, rd_kafka_ConfigEntry_name(configs[i]));
		lua_pushstring(L, rd_kafka_ConfigEntry_value(configs[i]));
		lua_settable(L, -3);
	}

	rd_kafka_ConfigResource_destroy(conf);

	return 1;
}

int AlterConfig(lua_State* L) {

	LuaKafka* luak = lua_tokafka(L, 1);
	int configtype = luaL_checkinteger(L, 2);
	const char * name = luaL_checkstring(L, 3);
	const char * configname = luaL_checkstring(L, 4);
	const char * configvalue = luaL_checkstring(L, 5);
	int timeout = luaL_optinteger(L, 6, 10000);

	if (!luak->rd) {
		luaL_error(L, "Kafka object not open");
		return 0;
	}

	rd_kafka_ResourceType_t restype = RD_KAFKA_RESOURCE_UNKNOWN;
	switch (configtype) {

	case 1:
		restype = RD_KAFKA_RESOURCE_ANY;
		break;
	case 2:
		restype = RD_KAFKA_RESOURCE_TOPIC;
		break;
	case 3:
		restype = RD_KAFKA_RESOURCE_GROUP;
		break;
	case 4:
		restype = RD_KAFKA_RESOURCE_BROKER;
		break;
	}

	rd_kafka_AdminOptions_t * adminopts = rd_kafka_AdminOptions_new(luak->rd, RD_KAFKA_ADMIN_OP_ANY);
	rd_kafka_resp_err_t err = rd_kafka_AdminOptions_set_request_timeout(adminopts, timeout, errorbuffer, kafka_error_buffer_len);
	if (err) {
		rd_kafka_AdminOptions_destroy(adminopts);
		lua_pop(L, lua_gettop(L));
		lua_pushboolean(L, false);
		lua_pushstring(L, errorbuffer);
		return 2;
	}

	rd_kafka_ConfigResource_t* conf = rd_kafka_ConfigResource_new(restype, name);

	err = rd_kafka_ConfigResource_set_config(conf, configname, configvalue);
	if (err) {
		rd_kafka_AdminOptions_destroy(adminopts);
		rd_kafka_ConfigResource_destroy(conf);
		lua_pop(L, lua_gettop(L));
		lua_pushboolean(L, false);
		lua_pushstring(L, rd_kafka_err2str(err));
		return 2;
	}

	rd_kafka_queue_t * queue = rd_kafka_queue_new(luak->rd);

	rd_kafka_AlterConfigs(luak->rd, &conf, 1, adminopts, queue);

	rd_kafka_event_t * ev = rd_kafka_queue_poll(queue, timeout);

	err = rd_kafka_event_error(ev);

	rd_kafka_event_destroy(ev);
	rd_kafka_ConfigResource_destroy(conf);
	rd_kafka_AdminOptions_destroy(adminopts);

	lua_pop(L, lua_gettop(L));
	lua_pushboolean(L, !err);
	if (err)
	{
		lua_pushstring(L, rd_kafka_err2str(err));
		return 2;
	}

	return 1;
}

int CreatePartition(lua_State* L) {

	LuaKafka* luak = lua_tokafka(L, 1);
	const char * topicname = luaL_checkstring(L, 2);
	int numbpartitions = luaL_optinteger(L, 3, 1);
	int timeout = luaL_optinteger(L, 4, 10000);

	if (!luak->rd) {
		luaL_error(L, "Kafka object not open");
		return 0;
	}

	rd_kafka_AdminOptions_t * adminopts = rd_kafka_AdminOptions_new(luak->rd, RD_KAFKA_ADMIN_OP_ANY);
	rd_kafka_resp_err_t err = rd_kafka_AdminOptions_set_request_timeout(adminopts, timeout, errorbuffer, kafka_error_buffer_len);
	if (err) {
		rd_kafka_AdminOptions_destroy(adminopts);
		lua_pop(L, lua_gettop(L));
		lua_pushboolean(L, false);
		lua_pushstring(L, errorbuffer);
		return 2;
	}

	rd_kafka_NewPartitions_t * parts = rd_kafka_NewPartitions_new(topicname, numbpartitions, errorbuffer, kafka_error_buffer_len);

	rd_kafka_queue_t * queue = rd_kafka_queue_new(luak->rd);
	rd_kafka_CreatePartitions(luak->rd, &parts, 1, adminopts, queue);

	rd_kafka_event_t * event = rd_kafka_queue_poll(queue, timeout);
	err = rd_kafka_event_error(event);

	rd_kafka_event_destroy(event);
	rd_kafka_queue_destroy(queue);
	rd_kafka_AdminOptions_destroy(adminopts);
	rd_kafka_NewPartitions_destroy(parts);

	lua_pop(L, lua_gettop(L));
	lua_pushboolean(L, !err);
	if (err)
	{
		lua_pushstring(L, rd_kafka_err2str(err));
		return 2;
	}

	return 1;
}

int CreateTopic(lua_State* L) {

	LuaKafka* luak = lua_tokafka(L, 1);
	const char * topicname = luaL_checkstring(L, 2);
	int numbpartitions = luaL_optinteger(L, 3, 1);
	int replicafactor = luaL_optinteger(L, 4, -1);

	if (!luak->rd) {
		luaL_error(L, "Kafka object not open");
		return 0;
	}

	rd_kafka_NewTopic_t * rkt = rd_kafka_NewTopic_new(topicname, numbpartitions, replicafactor, errorbuffer, kafka_error_buffer_len);

	if (!rkt) {
		rd_kafka_NewTopic_destroy(rkt);
		lua_pop(L, lua_gettop(L));
		lua_pushboolean(L, false);
		lua_pushstring(L, errorbuffer);
		return 2;
	}

	rd_kafka_AdminOptions_t * adminopts = rd_kafka_AdminOptions_new(luak->rd, RD_KAFKA_ADMIN_OP_ANY);
	rd_kafka_resp_err_t err;

	if (lua_type(L, 5) == LUA_TNUMBER)
	{
		err = rd_kafka_AdminOptions_set_broker(adminopts, lua_tointeger(L, 5), errorbuffer, kafka_error_buffer_len);
		if (err) {
			rd_kafka_NewTopic_destroy(rkt);
			rd_kafka_AdminOptions_destroy(adminopts);
			lua_pop(L, lua_gettop(L));
			lua_pushboolean(L, false);
			lua_pushstring(L, errorbuffer);
			return 2;
		}
	}

	err = rd_kafka_AdminOptions_set_request_timeout(adminopts, luaL_optinteger(L, 6, 10000), errorbuffer, kafka_error_buffer_len);
	if (err) {
		rd_kafka_NewTopic_destroy(rkt);
		rd_kafka_AdminOptions_destroy(adminopts);
		lua_pop(L, lua_gettop(L));
		lua_pushboolean(L, false);
		lua_pushstring(L, errorbuffer);
		return 2;
	}

	if (lua_type(L, 7) == LUA_TNUMBER)
	{
		err = rd_kafka_AdminOptions_set_operation_timeout(adminopts, lua_tointeger(L, 7), errorbuffer, kafka_error_buffer_len);
		if (err) {
			rd_kafka_NewTopic_destroy(rkt);
			rd_kafka_AdminOptions_destroy(adminopts);
			lua_pop(L, lua_gettop(L));
			lua_pushboolean(L, false);
			lua_pushstring(L, errorbuffer);
			return 2;
		}
	}

	rd_kafka_queue_t * queue = rd_kafka_queue_new(luak->rd);
	rd_kafka_CreateTopics(luak->rd, &rkt, 1, adminopts, queue);

	rd_kafka_event_t * event = rd_kafka_queue_poll(queue, luaL_optinteger(L, 6, 10000));
	err = rd_kafka_event_error(event);

	rd_kafka_event_destroy(event);
	rd_kafka_queue_destroy(queue);
	rd_kafka_AdminOptions_destroy(adminopts);
	rd_kafka_NewTopic_destroy(rkt);

	lua_pop(L, lua_gettop(L));
	lua_pushboolean(L, !err);
	if (err)
	{
		lua_pushstring(L, rd_kafka_err2str(err));
		return 2;
	}

	return 1;
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
	rd_kafka_topic_conf_t* conf = lua_tokafkatopicconf(L, 5);

	rd_kafka_topic_t* rkt = rd_kafka_topic_new(luak->rd, topic, conf);

	lua_pop(L, lua_gettop(L));

	if (rd_kafka_consume_start(rkt, partition, offset) == -1) {

		rd_kafka_resp_err_t err = rd_kafka_last_error();
		lua_pushnil(L);
		lua_pushstring(L, rd_kafka_err2str(err));
		rd_kafka_topic_destroy(rkt);
		return 2;
	}

	LuaKafkaTopic* ltopic = lua_pushkafkatopic(L, topic);
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

		rd_kafka_queue_destroy(luak->evqueue);
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