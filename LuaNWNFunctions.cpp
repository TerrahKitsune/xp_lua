#include "LuaNWNFunctions.h"

nwn_objid_t GetObjID(lua_State*L, int idx){

	int type = lua_type(L, idx);
	if (type == LUA_TNUMBER){

		return (nwn_objid_t)lua_tointeger(L, idx);
	}
	else if (type == LUA_TSTRING){

		const char * hex = lua_tostring(L, idx);
		if (hex == NULL)
			return OJBECT_INVALID;

		nwn_objid_t result;
		if (sscanf(hex, "%x", &result) == 1)
			return result;

		return OJBECT_INVALID;
	}
	else{
		return OJBECT_INVALID;
	}
}

int GetCreature(lua_State*L){

	nwn_objid_t objid = GetObjID(L, 1);
	void * obj = GetCreatureByGameObjectID(objid);
	lua_pop(L, 1);

	if (obj){
		char result[10];
		sprintf(result, "%08X", obj);	
		lua_pushstring(L, result);
	}
	else{
		lua_pushnil(L);
	}

	return 1;
}

int LRunScript(lua_State*L){

	const char * script = luaL_checkstring(L, 1);
	nwn_objid_t objid = GetObjID(L, 2);
	
	if (objid == OJBECT_INVALID)
		objid = OBJECT_MODULE;

	RunScript(script, objid);

	lua_pop(L, lua_gettop(L));

	return 0;
}

int Test(lua_State*L){

	void * test = NWN2_Malloc(10000000);

	char result[10];
	sprintf(result, "%08X", test);
	lua_pushstring(L, result);

	if (test)
		NWN2_Free(test);

	return 1;
}