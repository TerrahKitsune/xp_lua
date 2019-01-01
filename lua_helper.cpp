#include "lua_helper.h"

void lua_pushobject(lua_State*L, nwn_objid_t obj) {

	char t[10];
	sprintf(t, "%x", obj);
	lua_pushstring(L, t);
}

void lua_pushcexostring(lua_State*L, CExoString* str) {

	if (!str || !str->text || str->len <= 0) {
		lua_pushlstring(L, "", 0);
	}

	size_t len = str->len;

	if (str->text[len - 1] == '\0') {
		len--;
	}

	lua_pushlstring(L, str->text, len);
}

void lua_pushlocation(lua_State*L, Location loc) {

	lua_createtable(L, 0, 3);

	lua_pushstring(L, "Position");
	lua_pushvector(L, loc.Position);
	lua_settable(L, -3);

	lua_pushstring(L, "Facing");
	lua_pushvector(L, loc.Facing);
	lua_settable(L, -3);

	lua_pushstring(L, "Area");
	lua_pushobject(L, loc.AreaId);
	lua_settable(L, -3);
}

void lua_pushvector(lua_State*L, Vector vec) {

	lua_createtable(L, 0, 3);

	lua_pushstring(L, "x");
	lua_pushnumber(L, vec.x);
	lua_settable(L, -3);

	lua_pushstring(L, "y");
	lua_pushnumber(L, vec.y);
	lua_settable(L, -3);

	lua_pushstring(L, "z");
	lua_pushnumber(L, vec.z);
	lua_settable(L, -3);
}