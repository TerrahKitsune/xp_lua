#include "LuaNWNFunctions.h"
#include "NwnLuaHooks.h"
#include "lua_helper.h"

nwn_objid_t GetObjID(lua_State*L, int idx) {

	int type = lua_type(L, idx);
	if (type == LUA_TNUMBER) {

		return (nwn_objid_t)lua_tointeger(L, idx);
	}
	else if (type == LUA_TSTRING) {

		const char * hex = lua_tostring(L, idx);
		if (hex == NULL)
			return OJBECT_INVALID;

		nwn_objid_t result;
		if (sscanf(hex, "%x", &result) == 1)
			return result;

		return OJBECT_INVALID;
	}
	else {
		return OJBECT_INVALID;
	}
}

int GetABVs(lua_State*L) {

	nwn_objid_t objid = GetObjID(L, 1);
	nwn_objid_t objidtarget = GetObjID(L, 2);

	if (objid == OJBECT_INVALID || objidtarget == OJBECT_INVALID) {

		lua_pop(L,lua_gettop(L));
		lua_pushinteger(L, 0);
		return 1;
	}

	CNWSObject * object = (CNWSObject*)GetObjectByGameObjectID(objid);

	if (!object) {
		lua_pop(L, lua_gettop(L));
		lua_pushinteger(L, 0);
		return 1;
	}

	CNWSObject * target = (CNWSObject*)GetObjectByGameObjectID(objidtarget);

	if (!target) {
		lua_pop(L, lua_gettop(L));
		lua_pushinteger(L, 0);
		return 1;
	}

	CNWSCreatureStats* stats = GetCreatureStats(object);

	if (!stats) {
		lua_pop(L, lua_gettop(L));
		lua_pushinteger(L, 0);
		return 1;
	}

	int result = OriginalGetAttackModifierVersus(stats, NULL, target);

	lua_pop(L, lua_gettop(L));
	lua_pushinteger(L, result);
	return 1;
}

bool ApplyEffectId(CGameEffect * effect) {

	if (effect->NumbEffectInts < 11) {
		int * temp = (int*)NWN2_Malloc(sizeof(int) * 11);
		if (!temp) {
			return false;
		}
		memset(temp, 0, sizeof(int) * 11);
		memcpy(temp, effect->effectInts, sizeof(int) * effect->NumbEffectInts);
		NWN2_Free(effect->effectInts);
		effect->effectInts = temp;
		effect->NumbEffectInts = 11;
		effect->AllocEffectInts = 11;
	}

	effect->effectInts[10] = (int)effect;

	return true;
}

CGameEffect * GetEffect(lua_State*L) {

	nwn_objid_t objid = GetObjID(L, 1);
	int effectid = lua_tointeger(L, 2);

	if (objid == OJBECT_INVALID) {

		return NULL;
	}

	CNWSObject * object = (CNWSObject*)GetObjectByGameObjectID(objid);

	if (!object) {
		return NULL;
	}

	int effects = object->effects_len;
	CGameEffect * effect = NULL;

	for (size_t i = 0; i < effects; i++)
	{
		if ((int)object->effects[i] == effectid) {

			effect = object->effects[i];
			break;
		}
	}

	return effect;
}

void lua_pusheffectdata(lua_State*L, CGameEffect * effect) {

	if (!effect) {
		lua_pushnil(L);
		return;
	}

	lua_createtable(L, 0, 15);

	lua_pushstring(L, "ID");
	lua_pushinteger(L, (int)effect);
	lua_settable(L, -3);

	lua_pushstring(L, "Creator");
	lua_pushobject(L, effect->Creator);
	lua_settable(L, -3);

	lua_pushstring(L, "Duration");
	lua_pushnumber(L, effect->Duration);
	lua_settable(L, -3);

	lua_pushstring(L, "ExpireDay");
	lua_pushinteger(L, effect->ExpireDay);
	lua_settable(L, -3);

	lua_pushstring(L, "ExpireTime");
	lua_pushinteger(L, effect->ExpireTime);
	lua_settable(L, -3);

	lua_pushstring(L, "Exposed");
	lua_pushboolean(L, effect->IsExposed);
	lua_settable(L, -3);

	lua_pushstring(L, "ShowIcon");
	lua_pushboolean(L, effect->ShowIcon);
	lua_settable(L, -3);

	lua_pushstring(L, "SpellID");
	lua_pushinteger(L, effect->SpellId);
	lua_settable(L, -3);

	lua_pushstring(L, "CasterLevel");
	lua_pushinteger(L, effect->CasterLevel);
	lua_settable(L, -3);

	lua_pushstring(L, "LinkID");
	lua_pushinteger(L, effect->LinkId);
	lua_settable(L, -3);

	lua_pushstring(L, "Type");
	lua_pushinteger(L, effect->Type);
	lua_settable(L, -3);

	lua_pushstring(L, "SubType");
	lua_pushinteger(L, effect->SubType);
	lua_settable(L, -3);

	lua_pushstring(L, "EffectStrings");
	lua_createtable(L, 6, 0);
	for (size_t i = 0; i < 6; i++)
	{
		if (effect->EffectString[i].text) {
			lua_pushstring(L, effect->EffectString[i].text);
		}
		else {
			lua_pushstring(L, "");
		}

		lua_rawseti(L, -2, i + 1);
	}
	lua_settable(L, -3);

	lua_pushstring(L, "EffectObjects");
	lua_createtable(L, 4, 0);
	for (size_t i = 0; i < 4; i++)
	{
		lua_pushobject(L, effect->EffectObjects[i]);
		lua_rawseti(L, -2, i + 1);
	}
	lua_settable(L, -3);

	lua_pushstring(L, "EffectInts");
	lua_createtable(L, effect->NumbEffectInts, 0);
	for (size_t i = 0; i < effect->NumbEffectInts; i++)
	{
		lua_pushinteger(L, effect->effectInts[i]);
		lua_rawseti(L, -2, i + 1);
	}
	lua_settable(L, -3);
}

int SetEffectString(lua_State*L) {

	size_t len;
	CGameEffect * effect = GetEffect(L);
	int idx = lua_tointeger(L, 3);
	const char * str = lua_tolstring(L, 4, &len);

	if (!effect) {
		lua_pop(L, lua_gettop(L));
		lua_pushboolean(L, 0);
		return 1;
	}
	else if (idx < 0 || idx >= 6) {

		luaL_error(L, "SetEffectString index must be between 0 and 5");
		return 0;
	}

	if (effect->EffectString[idx].text) {
		NWN2_Free(effect->EffectString[idx].text);
		effect->EffectString[idx].text = NULL;
		effect->EffectString[idx].len = 0;
	}
	
	if (str && len > 0) {
		effect->EffectString[idx].text = (char*)NWN2_Malloc(len + 1);
		if (!effect->EffectString[idx].text) {
			lua_pop(L, lua_gettop(L));
			lua_pushboolean(L, 0);
			return 1;
		}
		memcpy(effect->EffectString[idx].text, str, len);
		effect->EffectString[idx].text[len] = '\0';
	}

	lua_pop(L, lua_gettop(L));
	lua_pushboolean(L, 1);
	return 1;
}

int GetCreature(lua_State*L) {

	nwn_objid_t objid = GetObjID(L, 1);
	void * obj = GetCreatureByGameObjectID(objid);
	lua_pop(L, 1);

	if (obj) {
		char result[10];
		sprintf(result, "%08X", obj);
		lua_pushstring(L, result);
	}
	else {
		lua_pushnil(L);
	}

	return 1;
}

int GetObject(lua_State*L) {

	nwn_objid_t objid = GetObjID(L, 1);
	void * obj = GetObjectByGameObjectID(objid);
	lua_pop(L, 1);

	if (obj) {
		char result[10];
		sprintf(result, "%08X", obj);
		lua_pushstring(L, result);
	}
	else {
		lua_pushnil(L);
	}

	return 1;
}

int LRunScript(lua_State*L) {

	const char * script = luaL_checkstring(L, 1);
	nwn_objid_t objid = GetObjID(L, 2);

	if (objid == OJBECT_INVALID)
		objid = OBJECT_MODULE;

	RunScript(script, objid);

	lua_pop(L, lua_gettop(L));

	return 0;
}

int EffectSetExposed(lua_State*L) {

	CGameEffect * effect = GetEffect(L);

	if (effect) {
		effect->IsExposed = lua_toboolean(L, 3);
		effect->ShowIcon = effect->IsExposed;
	}

	return 0;
}

int SetEffectObject(lua_State*L) {

	CGameEffect * effect = GetEffect(L);
	int idx = luaL_checkinteger(L, 3);
	nwn_objid_t newobject = GetObjID(L, 4);
	lua_pop(L, lua_gettop(L));

	if (!effect || idx < 0 || idx >= 4) {
		lua_pushboolean(L, false);
		return 1;
	}

	effect->EffectObjects[idx] = newobject;

	lua_pushboolean(L, true);
	return 1;
}

int EffectSetEffectInt(lua_State*L) {

	CGameEffect * effect = GetEffect(L);
	int idx = luaL_checkinteger(L, 3);
	int newint = luaL_checkinteger(L, 4);
	lua_pop(L, lua_gettop(L));

	if (!effect || idx < 0 || idx >= 10) {
		lua_pushboolean(L, false);
		return 1;
	}

	if (!ApplyEffectId(effect)) {
		lua_pushboolean(L, false);
		return 1;
	}

	effect->effectInts[idx] = newint;

	lua_pushboolean(L, true);
	return 1;
}

int GetEffectData(lua_State*L) {

	if (lua_gettop(L) > 1) {

		CGameEffect * effect = GetEffect(L);

		lua_pop(L, lua_gettop(L));

		lua_pusheffectdata(L, effect);
	}
	else {

		CNWSObject * object = (CNWSObject*)GetObjectByGameObjectID(GetObjID(L, 1));
		lua_pop(L, lua_gettop(L));
		if (!object) {
			lua_pushnil(L);
			return 1;
		}

		int effects = object->effects_len;
		lua_createtable(L, effects, 0);
		for (size_t i = 0; i < effects; i++)
		{
			CGameEffect * effect = object->effects[i];

			ApplyEffectId(effect);
			lua_pusheffectdata(L, effect);

			lua_rawseti(L, -2, i + 1);
		}
	}

	return 1;
}

int CopyEffectIdsToEffectInts(lua_State*L) {

	nwn_objid_t objid = GetObjID(L, 1);

	if (objid == OJBECT_INVALID) {

		lua_pop(L, lua_gettop(L));
		lua_pushnil(L);
		return 1;
	}
	else
		lua_pop(L, lua_gettop(L));

	CNWSObject * object = (CNWSObject*)GetObjectByGameObjectID(objid);

	if (!object) {
		lua_pushboolean(L, false);
		return 1;
	}

	int effects = object->effects_len;

	for (size_t i = 0; i < effects; i++)
	{
		CGameEffect * effect = object->effects[i];

		ApplyEffectId(effect);
	}

	lua_pushboolean(L, true);
	return 1;
}

int ClearLocalVariables(lua_State*L) {

	nwn_objid_t objid = GetObjID(L, 1);

	if (objid == OJBECT_INVALID) {

		lua_pop(L, lua_gettop(L));
		return 0;
	}
	else
		lua_pop(L, lua_gettop(L));

	CNWSObject * object = (CNWSObject*)GetObjectByGameObjectID(objid);

	if (!object) {
		return 0;
	}

	ClearVariables(&object->vartable);

	return 0;
}

int GetLocalVariable(lua_State*L) {

	nwn_objid_t objid = GetObjID(L, 1);
	const char * name = luaL_checkstring(L, 2);
	int type = luaL_optinteger(L, 3, -1);

	if (objid == OJBECT_INVALID) {

		lua_pop(L, lua_gettop(L));
		lua_pushnil(L);
		return 1;
	}
	else
		lua_pop(L, lua_gettop(L));

	CNWSObject * object = (CNWSObject*)GetObjectByGameObjectID(objid);

	if (!object) {

		lua_pushnil(L);
		return 1;
	}

	for (int n = 0; n < object->vartable.vartable_len; n++) {

		CScriptVariable * var = &object->vartable.vartable[n];

		if ((type == -1 || type == var->Type) && strcmp(var->Name.text, name) == 0) {
			
			lua_pop(L, lua_gettop(L));

			switch (var->Type)
			{
			case 3: //string
				lua_pushcexostring(L, (CExoString*)var->Data);
				break;
			case 2: //float
				float numb;
				memcpy(&numb, &var->Data, sizeof(float));
				lua_pushnumber(L, numb);
				break;
			case 5: //location
				lua_pushlocation(L, *(Location*)var->Data);
				break;
			case 4: //object
				lua_pushobject(L, var->Data);
				break;
			case 1: //int
				lua_pushinteger(L, (int)var->Data);
				break;
			default:
				lua_pushnil(L);
				break;
			}

			return 1;
		}
	}

	lua_pop(L, lua_gettop(L));
	lua_pushnil(L);

	return 1;
}

int GetLocalVariables(lua_State*L) {

	nwn_objid_t objid = GetObjID(L, 1);

	if (objid == OJBECT_INVALID) {

		lua_pop(L, lua_gettop(L));
		lua_pushnil(L);
		return 1;
	}
	else
		lua_pop(L, lua_gettop(L));

	CNWSObject * object = (CNWSObject*)GetObjectByGameObjectID(objid);

	if (!object) {

		lua_pushnil(L);
		return 1;
	}

	lua_createtable(L, object->vartable.vartable_len, 0);

	for (int n = 0; n < object->vartable.vartable_len; n++) {

		lua_createtable(L, 0, 3);

		CScriptVariable * var = &object->vartable.vartable[n];

		lua_pushstring(L, "Name");
		lua_pushcexostring(L, &var->Name);
		lua_settable(L, -3);

		lua_pushstring(L, "Type");
		switch (var->Type)
		{
		case 3: //string
			lua_pushstring(L, "string");
			break;
		case 2: //float
			lua_pushstring(L, "float");
			break;
		case 5: //location
			lua_pushstring(L, "location");
			break;
		case 4: //object
			lua_pushstring(L, "object");
			break;
		case 1: //int
			lua_pushstring(L, "int");
			break;
		default:
			lua_pushstring(L, "unknown");
			break;
		}
		lua_settable(L, -3);

		lua_pushstring(L, "Data");
		switch (var->Type)
		{
		case 3: //string
			lua_pushcexostring(L, (CExoString*)var->Data);
			break;
		case 2: //float
			float numb;
			memcpy(&numb, &var->Data, sizeof(float));
			lua_pushnumber(L, numb);
			break;
		case 5: //location
			lua_pushlocation(L, *(Location*)var->Data);
			break;
		case 4: //object
			lua_pushobject(L, var->Data);
			break;
		case 1: //int
			lua_pushinteger(L, (int)var->Data);
			break;
		default:
			lua_pushnil(L);
			break;
		}
		lua_settable(L, -3);

		lua_rawseti(L, -2, n + 1);
	}

	return 1;
}

int Test(lua_State*L) {

	void * test = NWN2_Malloc(10000000);

	char result[10];
	sprintf(result, "%08X", test);
	lua_pushstring(L, result);

	if (test)
		NWN2_Free(test);

	return 1;
}