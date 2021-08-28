#include "customtextbox.h"
#include "customwindow.h"
#include "luawchar.h"

void DoCustomTextboxEvent(lua_State* L, LuaWindow* parent, LuaWindow* child, HWND hwnd, UINT Msg, WPARAM wParam, LPARAM lParam) {

	if (child->custom->eventRef == LUA_REFNIL) {
		return;
	}

	WORD code = HIWORD(wParam);

	if (code == EN_CHANGE) {

		lua_rawgeti(L, LUA_REGISTRYINDEX, child->custom->eventRef);
		lua_pushvalue(L, -2);
		lua_pushvalue(L, -5);

		if (lua_pcall(L, 2, 0, NULL)) {

			puts(lua_tostring(L, -1));
			lua_pop(L, 1);
		}
	}
}

int CreateStaticTextField(lua_State* L) {

	LuaWindow* window = lua_tonwindow(L, 1);

	if (!window->custom) {

		lua_pushnil(L);
		return 1;
	}

	LuaCustomWindow* custom = CreateCustomWindowStruct();

	if (!custom) {
		luaL_error(L, "out of memory");
		return 0;
	}

	DWORD style = WS_VISIBLE | WS_CHILD;

	switch ((int)luaL_optnumber(L, 7, 0))
	{
	case 1:
		style |= SS_CENTER;
		break;
	case 2:
		style |= SS_RIGHT;
		break;
	default:
		style |= SS_LEFT;
		break;
	}

	custom->hmenu = (HMENU)(++window->custom->nextId);

	LuaWChar* content = lua_stringtowchar(L, 2);

	HWND hwndButton = CreateWindowW(
		L"STATIC",
		content->str,
		style,
		(int)luaL_optnumber(L, 3, 0),
		(int)luaL_optnumber(L, 4, 0),
		(int)luaL_optnumber(L, 5, 0),
		(int)luaL_optnumber(L, 6, 0),
		window->handle,
		custom->hmenu,
		(HINSTANCE)GetWindowLongPtr(window->handle, GWLP_HINSTANCE),
		NULL);

	lua_pushvalue(L, 1);
	int refParent = luaL_ref(L, LUA_REGISTRYINDEX);
	lua_pop(L, lua_gettop(L));

	LuaWindow* button = lua_pushwindow(L);
	button->handle = hwndButton;
	button->custom = custom;
	button->custom->type = WINDOW_TYPE_STATICTEXT;
	button->custom->parentRef = refParent;

	AddLuaTableChild(L, window->custom);

	return 1;
}

int CreateTextField(lua_State* L) {

	LuaWindow* window = lua_tonwindow(L, 1);

	if (!window->custom) {

		lua_pushnil(L);
		return 1;
	}

	LuaCustomWindow* custom = CreateCustomWindowStruct();

	if (!custom) {
		luaL_error(L, "out of memory");
		return 0;
	}

	DWORD style = WS_CHILD | WS_VISIBLE | ES_LEFT | WS_BORDER;

	if (lua_toboolean(L, 7)) {
		style |= ES_MULTILINE;
	}

	if (lua_toboolean(L, 8)) {
		style |= WS_VSCROLL | ES_AUTOVSCROLL;
	}

	custom->hmenu = (HMENU)(++window->custom->nextId);

	LuaWChar* content = lua_stringtowchar(L, 2);

	HWND hwndButton = CreateWindowW(
		L"EDIT",
		content->str,
		style,
		(int)luaL_optnumber(L, 3, 0),
		(int)luaL_optnumber(L, 4, 0),
		(int)luaL_optnumber(L, 5, 0),
		(int)luaL_optnumber(L, 6, 0),
		window->handle,
		custom->hmenu,
		(HINSTANCE)GetWindowLongPtr(window->handle, GWLP_HINSTANCE),
		NULL);

	lua_pushvalue(L, 1);
	int refParent = luaL_ref(L, LUA_REGISTRYINDEX);
	int refEvent = LUA_REFNIL;

	if (lua_type(L, 9) == LUA_TFUNCTION) {

		lua_pushvalue(L, 9);
		refEvent = luaL_ref(L, LUA_REGISTRYINDEX);
	}

	lua_pop(L, lua_gettop(L));

	LuaWindow* button = lua_pushwindow(L);
	button->handle = hwndButton;
	button->custom = custom;
	button->custom->type = WINDOW_TYPE_TEXTBOX;
	button->custom->parentRef = refParent;
	button->custom->eventRef = refEvent;

	AddLuaTableChild(L, window->custom);

	return 1;
}