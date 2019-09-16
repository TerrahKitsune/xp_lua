#include "luakafka.h"
#include "luakafkamain.h"
#include "luakafkamessage.h"
#include "luakafkatopic.h"

static const struct luaL_Reg kafkafunctions[] = {

	{ "Events",  PollEvents },
	{ "NewConsumer",  CreateConsumer },
	{ "AddBroker",  AddBroker },
	{ "GetGroups",  DescribeGroups },
	{ "GetMetadata",  GetMetadata },
	{ "Logs",  GetLastLogs },
	{ "Poll",  PollMessage },
	{ "Subscribe",  SubscribeToTopic },
	{ "GetOffsets",  QueryHighLow },
	{ "CreateTopic",  CreateTopic },
	{ "DeleteTopic",  DeleteTopic },
	{ "Close",  kafka_gc },
	{ "GetId",  GetKafkaId },
	{ "Commit",  CommitMessage },
	{ "AlterConfig",  AlterConfig },
	{ "SetPartitions",  CreatePartition },
	{ "GetConfig",  GetConfig },
	{ NULL, NULL }
}; 

static const luaL_Reg kafkameta[] = {
	{ "__gc",  kafka_gc },
	{ "__tostring",  kafka_tostring },
{ NULL, NULL }
};

static const struct luaL_Reg kafkamessagefunctions[] = {
	{ "GetData",  GetKafkaMessageData },
	{ "GetOwnerId",  GetKafkaMessageOwnerId },
	{ "Dispose",  kafkamsg_gc },
	{ NULL, NULL }
};

static const luaL_Reg kafkamessagemeta[] = {
	{ "__gc",  kafkamsg_gc },
	{ "__tostring",  kafkamsg_tostring },
	{ NULL, NULL }
};

static const struct luaL_Reg kafkatopicfunctions[] = {
	{ "GetOwnerId",  GetKafkaTopicOwnerId },
	{ "GetInfo",  GetKafkaTopicInfo },
	{ "Dispose",  kafkatopic_gc },
	{ NULL, NULL }
};

static const luaL_Reg kafkatopicmeta[] = {
	{ "__gc",  kafkatopic_gc },
	{ "__tostring",  kafkatopic_tostring },
	{ NULL, NULL }
};

int luaopen_kafka(lua_State* L) {

	luaL_newlibtable(L, kafkatopicfunctions);
	luaL_setfuncs(L, kafkatopicfunctions, 0);

	luaL_newmetatable(L, LUAKAFKATOPIC);
	luaL_setfuncs(L, kafkatopicmeta, 0);

	lua_pushliteral(L, "__index");
	lua_pushvalue(L, -3);
	lua_rawset(L, -3);
	lua_pushliteral(L, "__metatable");
	lua_pushvalue(L, -3);
	lua_rawset(L, -3);

	lua_pop(L, 2);

	luaL_newlibtable(L, kafkamessagefunctions);
	luaL_setfuncs(L, kafkamessagefunctions, 0);

	luaL_newmetatable(L, LUAKAFKAMESSAGE);
	luaL_setfuncs(L, kafkamessagemeta, 0);

	lua_pushliteral(L, "__index");
	lua_pushvalue(L, -3);
	lua_rawset(L, -3);
	lua_pushliteral(L, "__metatable");
	lua_pushvalue(L, -3);
	lua_rawset(L, -3);

	lua_pop(L, 2);

	luaL_newlibtable(L, kafkafunctions);
	luaL_setfuncs(L, kafkafunctions, 0);

	luaL_newmetatable(L, LUAKAFKA);
	luaL_setfuncs(L, kafkameta, 0);

	lua_pushliteral(L, "__index");
	lua_pushvalue(L, -3);
	lua_rawset(L, -3);
	lua_pushliteral(L, "__metatable");
	lua_pushvalue(L, -3);
	lua_rawset(L, -3);

	lua_pop(L, 1);

	return 1;
}