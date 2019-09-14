#include "luakafka.h"
#include "luakafkamain.h"

static const struct luaL_Reg kafkafunctions[] = {

	{ "NewConsumer",  CreateConsumer },
	{ "AddBroker",  AddBroker },
	{ "GetGroups",  DescribeGroups },
	{ "GetMetadata",  GetMetadata },
	{ "Logs",  GetLastLogs },
	{ "Poll",  PollMessage },
	{ "Subscribe",  SubscribeToTopic },
	{ NULL, NULL }
}; 

static const luaL_Reg kafkameta[] = {
	{ "__gc",  kafka_gc },
	{ "__tostring",  kafka_tostring },
{ NULL, NULL }
};

int luaopen_kafka(lua_State* L) {

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