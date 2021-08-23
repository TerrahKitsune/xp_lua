#include "customwindow.h"
#include "luawindow.h"
#include "customdrawing.h"
#include "customcombobox.h"
#include "luawchar.h"
#include <commctrl.h>

void DoCustomComboBoxEvent(lua_State* L, LuaWindow* parent, LuaWindow* child, HWND hwnd, UINT Msg, WPARAM wParam, LPARAM lParam) {

	WORD type = HIWORD(wParam);

	if (type == CBN_SELCHANGE) {

		int ItemIndex = SendMessageW((HWND)lParam, (UINT)CB_GETCURSEL, (WPARAM)0, (LPARAM)0);

		if (child->custom->eventRef == LUA_REFNIL || child->custom->boxItemsRef == LUA_REFNIL) {
			return;
		}

		lua_rawgeti(L, LUA_REGISTRYINDEX, child->custom->boxItemsRef);
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

void DoCustomListBoxEvent(lua_State* L, LuaWindow* parent, LuaWindow* child, HWND hwnd, UINT Msg, WPARAM wParam, LPARAM lParam) {

	WORD type = HIWORD(wParam);

	if (type == LBN_SELCHANGE) {

		int ItemIndex = SendMessageW((HWND)lParam, (UINT)LB_GETCURSEL, (WPARAM)0, (LPARAM)0);

		if (child->custom->eventRef == LUA_REFNIL || child->custom->boxItemsRef == LUA_REFNIL) {
			return;
		}

		lua_rawgeti(L, LUA_REGISTRYINDEX, child->custom->boxItemsRef);
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

void DoCustomListViewEvent(lua_State* L, LuaWindow* parent, LuaWindow* child, HWND hwnd, UINT Msg, WPARAM wParam, LPARAM lParam) {

	NMHDR * nmh = (NMHDR*)lParam;
	NMLISTVIEW* nmlist = (NMLISTVIEW*)lParam;

	if (nmh && nmh->code == LVN_ITEMCHANGED && nmlist->uNewState & LVIS_SELECTED) {

		int ItemIndex = ListView_GetNextItem(child->handle, -1, LVNI_SELECTED);

		if (child->custom->eventRef == LUA_REFNIL || child->custom->boxItemsRef == LUA_REFNIL) {
			return;
		}

		lua_rawgeti(L, LUA_REGISTRYINDEX, child->custom->boxItemsRef);
		size_t len = luaL_len(L, -1);
		lua_pop(L, 1);

		ItemIndex = len - ItemIndex - 1;

		lua_rawgeti(L, LUA_REGISTRYINDEX, child->custom->boxItemsRef);
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

int DeleteBoxItem(lua_State* L) {

	LuaWindow* window = lua_tonwindow(L, 1);
	int index = (int)luaL_checkinteger(L, 2) - 1;

	if (!window->custom || window->custom->boxItemsRef == LUA_REFNIL) {

		luaL_error(L, "Cannot add combobox strings to non combobox elements");
		return 0;
	}

	lua_newtable(L);
	lua_rawgeti(L, LUA_REGISTRYINDEX, window->custom->boxItemsRef);
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

	luaL_unref(L, LUA_REGISTRYINDEX, window->custom->boxItemsRef);
	lua_pop(L, 1);
	window->custom->boxItemsRef = luaL_ref(L, LUA_REGISTRYINDEX);
	lua_pop(L, 1);

	if (window->custom->type == WINDOW_TYPE_COMBOBOX) {
		SendMessageW(window->handle, (UINT)CB_DELETESTRING, (WPARAM)index, NULL);
	}
	else if (window->custom->type == WINDOW_TYPE_LISTVIEW) {
		ListView_DeleteItem(window->handle, (len - index - 1));
	}
	else {
		SendMessageW(window->handle, (UINT)LB_DELETESTRING, (WPARAM)index, NULL);
	}

	return 0;
}

int GetBoxItems(lua_State* L) {

	LuaWindow* window = lua_tonwindow(L, 1);

	if (!window->custom || window->custom->boxItemsRef == LUA_REFNIL) {

		luaL_error(L, "Cannot add combobox strings to non combobox elements");
		return 0;
	}

	lua_rawgeti(L, LUA_REGISTRYINDEX, window->custom->boxItemsRef);

	return 1;
}

int AddBoxItem(lua_State* L) {

	LuaWindow* window = lua_tonwindow(L, 1);
	LuaWChar* data;

	if (!window->custom || window->custom->boxItemsRef == LUA_REFNIL) {

		luaL_error(L, "Cannot add combobox strings to non combobox elements");
		return 0;
	}

	lua_rawgeti(L, LUA_REGISTRYINDEX, window->custom->boxItemsRef);
	size_t len = lua_rawlen(L, -1);
	int ItemIndex = 0;

	if (window->custom->type == WINDOW_TYPE_COMBOBOX) {
		ItemIndex = SendMessageW(window->handle, (UINT)CB_GETCURSEL, (WPARAM)0, (LPARAM)0);
		SendMessageW(window->handle, CB_SETCURSEL, (WPARAM)len, (LPARAM)0);		
	}
	else if (window->custom->type == WINDOW_TYPE_LISTVIEW) {
		ItemIndex = ListView_GetNextItem(window->handle, -1, LVNI_SELECTED);
		ListView_SetItemState(window->handle, len, LVNI_SELECTED | LVNI_FOCUSED, LVNI_SELECTED | LVNI_FOCUSED);	
	}
	else {
		ItemIndex = SendMessageW(window->handle, (UINT)LB_GETCURSEL, (WPARAM)0, (LPARAM)0);
		SendMessageW(window->handle, LB_SETCURSEL, (WPARAM)len, (LPARAM)0);		
	}

	if (ItemIndex < 0) {
		ItemIndex = 0;
	}

	if (window->custom->type == WINDOW_TYPE_COMBOBOX) {
		data = lua_stringtowchar(L, 2);
		SendMessageW(window->handle, (UINT)CB_ADDSTRING, (WPARAM)0, (LPARAM)data->str);
		SendMessageW(window->handle, CB_SETCURSEL, (WPARAM)ItemIndex, (LPARAM)0);
		lua_pushwchar(L, data->str, data->len);
	}
	else if (window->custom->type == WINDOW_TYPE_LISTVIEW) {

		if (lua_type(L, 2)) {
			lua_pushvalue(L, 2);
			LV_ITEMW item;
			ZeroMemory(&item, sizeof(LV_ITEMW));
			item.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM | WS_VISIBLE;
			item.iGroupId = len;
			SendMessageW(window->handle, LVM_INSERTITEMW, 0, (LPARAM)&item);

			size_t sublen = luaL_len(L, -1);
			for (size_t i = 0; i < sublen; i++)
			{
				lua_rawgeti(L, -1, i + 1);

				data = lua_stringtowchar(L, -1);
				item.iSubItem = i;
				item.pszText = data->str;
				item.cchTextMax = data->len;

				SendMessageW(window->handle, LVM_SETITEMTEXTW, 0, (LPARAM)&item);

				lua_pop(L, 1);
			}
		}

		ListView_SetItemState(window->handle, ItemIndex, LVNI_SELECTED | LVNI_FOCUSED, LVNI_SELECTED | LVNI_FOCUSED);
	}
	else {
		data = lua_stringtowchar(L, 2);
		SendMessageW(window->handle, (UINT)LB_ADDSTRING, (WPARAM)0, (LPARAM)data->str);
		SendMessageW(window->handle, LB_SETCURSEL, (WPARAM)ItemIndex, (LPARAM)0);
		lua_pushwchar(L, data->str, data->len);
	}

	lua_rawseti(L, -2, len + 1);
	lua_pop(L, 1);

	return 0;
}

int CreateCustomLuaListView(lua_State* L) {

	LuaWindow* window = lua_tonwindow(L, 1);

	if (!window->custom) {

		lua_pushnil(L);
		return 1;
	}
	else if (lua_type(L, 6) != LUA_TTABLE) {

		luaL_error(L, "Missing function parameter");
		return 0;
	}
	else if (lua_type(L, 7) != LUA_TFUNCTION) {

		luaL_error(L, "Missing function parameter");
		return 0;
	}

	LuaCustomWindow* custom = CreateCustomWindowStruct();

	if (!custom) {
		luaL_error(L, "out of memory");
		return 0;
	}

	custom->hmenu = (HMENU)(++window->custom->nextId);

	HWND hwnd = CreateWindowExW(
		0,
		WC_LISTVIEWW,
		L"",
		WS_VISIBLE | WS_CHILD | WS_BORDER | LVS_SHOWSELALWAYS | LVS_REPORT | LVS_NOSORTHEADER | LVS_SINGLESEL,
		(int)luaL_optnumber(L, 2, 0),
		(int)luaL_optnumber(L, 3, 0),
		(int)luaL_optnumber(L, 4, 0),
		(int)luaL_optnumber(L, 5, 0),
		window->handle,
		custom->hmenu,
		(HINSTANCE)GetWindowLongPtr(window->handle, GWLP_HINSTANCE),
		NULL);

	lua_pushvalue(L, 7);
	int ref = luaL_ref(L, LUA_REGISTRYINDEX);
	lua_pushvalue(L, 1);
	int refParent = luaL_ref(L, LUA_REGISTRYINDEX);

	LVCOLUMNW lvc;
	lvc.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM | WS_VISIBLE;

	LuaWChar* data;
	lua_pushvalue(L, 6);
	size_t len = luaL_len(L, -1);
	for (size_t i = 0; i < len; i++)
	{
		lua_rawgeti(L, -1, i + 1);
		data = lua_stringtowchar(L, -1);
		lvc.iSubItem = i;
		lvc.pszText = data->str;
		lvc.cchTextMax = data->len;
		lvc.cx = luaL_optnumber(L, 4, 0) / len;
		lvc.fmt = LVCFMT_LEFT;

		SendMessageW((hwnd), LVM_INSERTCOLUMNW, (WPARAM)(int)(i), (LPARAM)(const LV_COLUMN*)(&lvc));
		lua_pop(L, 1);
	}

	lua_pop(L, lua_gettop(L));

	LuaWindow* button = lua_pushwindow(L);
	button->handle = hwnd;
	button->custom = custom;
	button->custom->type = WINDOW_TYPE_LISTVIEW;
	button->custom->eventRef = ref;
	button->custom->parentRef = refParent;

	AddLuaTableChild(L, window->custom);

	lua_newtable(L);
	button->custom->boxItemsRef = luaL_ref(L, LUA_REGISTRYINDEX);

	return 1;
}

int CreateCustomLuaListbox(lua_State* L) {

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

	HWND hwnd = CreateWindowW(
		L"LISTBOX",
		L"",
		WS_CHILD | WS_VISIBLE | WS_VSCROLL | ES_AUTOVSCROLL | LBS_NOTIFY,
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
	button->handle = hwnd;
	button->custom = custom;
	button->custom->type = WINDOW_TYPE_LISTBOX;
	button->custom->eventRef = ref;
	button->custom->parentRef = refParent;

	AddLuaTableChild(L, window->custom);

	lua_newtable(L);
	button->custom->boxItemsRef = luaL_ref(L, LUA_REGISTRYINDEX);

	return 1;
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

	HWND hwnd = CreateWindowW(
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
	button->handle = hwnd;
	button->custom = custom;
	button->custom->type = WINDOW_TYPE_COMBOBOX;
	button->custom->eventRef = ref;
	button->custom->parentRef = refParent;

	AddLuaTableChild(L, window->custom);

	lua_newtable(L);
	button->custom->boxItemsRef = luaL_ref(L, LUA_REGISTRYINDEX);

	return 1;
}