#include "customwindow.h"
#include "luawindow.h"
#include "customdrawing.h"
#include "customcombobox.h"
#include "luawchar.h"

void DoCustomComboBoxEvent(lua_State* L, LuaWindow* parent, LuaWindow* child, HWND hwnd, UINT Msg, WPARAM wParam, LPARAM lParam) {

	WORD type = HIWORD(wParam);

	if (type == CBN_SELCHANGE) {

		int ItemIndex = SendMessageW((HWND)lParam, (UINT)CB_GETCURSEL, (WPARAM)0, (LPARAM)0);

		if (child->custom->eventRef == LUA_REFNIL || child->custom->comboBoxItemsRef == LUA_REFNIL) {
			return;
		}

		lua_rawgeti(L, LUA_REGISTRYINDEX, child->custom->comboBoxItemsRef);
		lua_rawgeti(L, -1, ItemIndex + 1);
		lua_rawgeti(L, LUA_REGISTRYINDEX, child->custom->eventRef);
		lua_pushvalue(L, -4);
		lua_pushvalue(L, -7);
		lua_pushvalue(L, -4);
		
		if (lua_pcall(L, 3, 0, NULL)) {

			puts(lua_tostring(L, -1));
			lua_pop(L, 1);
		}

		lua_pop(L, 2);
	}
}

int DeleteComboBoxItem(lua_State* L) {

	LuaWindow* window = lua_tonwindow(L, 1);
	int index = (int)luaL_checkinteger(L, 2) - 1;

	if (!window->custom || window->custom->type != WINDOW_TYPE_COMBOBOX || window->custom->comboBoxItemsRef == LUA_REFNIL) {

		luaL_error(L, "Cannot add combobox strings to non combobox elements");
		return 0;
	}

	DumpStack(L);

	lua_newtable(L);
	lua_rawgeti(L, LUA_REGISTRYINDEX, window->custom->comboBoxItemsRef);
	size_t len = lua_rawlen(L, -1);

	if (index > len || index < 0) {
		luaL_error(L, "Index out of bounds");
		return 0;
	}

	int nth = 0;
	for (size_t i = 0; i < len; i++)
	{
		lua_pushinteger(L, i + 1);
		lua_gettable(L, -2);

		if (index != i) {
			lua_rawseti(L, -3, ++nth);
		}
		else {
			lua_pop(L, 1);
		}
	}

	luaL_unref(L, LUA_REGISTRYINDEX, window->custom->comboBoxItemsRef);
	lua_pop(L, 1);
	window->custom->comboBoxItemsRef = luaL_ref(L, LUA_REGISTRYINDEX);
	lua_pop(L, 1);

	SendMessageW(window->handle, (UINT)CB_DELETESTRING, (WPARAM)index, NULL);
}

int GetComboBoxItems(lua_State* L) {

	LuaWindow* window = lua_tonwindow(L, 1);

	if (!window->custom || window->custom->type != WINDOW_TYPE_COMBOBOX || window->custom->comboBoxItemsRef == LUA_REFNIL) {

		luaL_error(L, "Cannot add combobox strings to non combobox elements");
		return 0;
	}

	lua_rawgeti(L, LUA_REGISTRYINDEX, window->custom->comboBoxItemsRef);

	return 1;
}

int AddComboBoxItem(lua_State* L) {

	LuaWindow* window = lua_tonwindow(L, 1);
	LuaWChar* data = lua_stringtowchar(L, 2);

	if (!window->custom || window->custom->type != WINDOW_TYPE_COMBOBOX || window->custom->comboBoxItemsRef == LUA_REFNIL) {

		luaL_error(L, "Cannot add combobox strings to non combobox elements");
		return 0;
	}

	lua_rawgeti(L, LUA_REGISTRYINDEX, window->custom->comboBoxItemsRef);
	size_t len = lua_rawlen(L, -1);

	SendMessageW(window->handle, CB_SETCURSEL, (WPARAM)len, (LPARAM)0);
	int ItemIndex = SendMessageW(window->handle, (UINT)CB_GETCURSEL, (WPARAM)0, (LPARAM)0);

	if (ItemIndex < 0) {
		ItemIndex = 0;
	}
	
	SendMessageW(window->handle, (UINT)CB_ADDSTRING, (WPARAM)0, (LPARAM)data->str);
	SendMessageW(window->handle, CB_SETCURSEL, (WPARAM)ItemIndex, (LPARAM)0);
	lua_pushwchar(L, data->str, data->len);
	lua_rawseti(L, -2, len + 1);
	lua_pop(L, 1);

	return 0;
}

int CreateCustomLuaComboBox(lua_State* L) {

	LuaWindow* window = lua_tonwindow(L, 1);

	if (!window->custom) {

		lua_pushnil(L);
		return 1;
	}
	else if (lua_type(L, 6) != LUA_TFUNCTION) {

		luaL_error(L, "Missing function parameter");
		return 0;
	}

	LuaCustomWindow* custom = CreateCustomWindowStruct();

	if (!custom) {
		luaL_error(L, "out of memory");
		return 0;
	}

	custom->hmenu = (HMENU)(++window->custom->nextId);

	HWND hwndButton = CreateWindowW(
		L"COMBOBOX",
		L"",
		CBS_DROPDOWN | CBS_HASSTRINGS | WS_CHILD | WS_OVERLAPPED | WS_VISIBLE,
		(int)luaL_optnumber(L, 2, 0),
		(int)luaL_optnumber(L, 3, 0),
		(int)luaL_optnumber(L, 4, 0),
		(int)luaL_optnumber(L, 5, 0),
		window->handle,
		custom->hmenu,
		(HINSTANCE)GetWindowLongPtr(window->handle, GWLP_HINSTANCE),
		NULL);

	lua_pushvalue(L, 6);
	int ref = luaL_ref(L, LUA_REGISTRYINDEX);
	lua_pushvalue(L, 1);
	int refParent = luaL_ref(L, LUA_REGISTRYINDEX);

	lua_pop(L, lua_gettop(L));

	LuaWindow* button = lua_pushwindow(L);
	button->handle = hwndButton;
	button->custom = custom;
	button->custom->type = WINDOW_TYPE_COMBOBOX;
	button->custom->eventRef = ref;
	button->custom->parentRef = refParent;

	AddLuaTableChild(L, window->custom);

	lua_newtable(L);
	button->custom->comboBoxItemsRef = luaL_ref(L, LUA_REGISTRYINDEX);

	return 1;
}