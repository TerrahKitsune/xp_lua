#include "customwindow.h"
#include "luawindow.h"
#include "customdrawing.h"
#include "customcombobox.h"
#include "luawchar.h"
#include <commctrl.h>

bool IsBox(LuaWindow* window) {
	return window->custom &&
		(window->custom->type == WINDOW_TYPE_COMBOBOX ||
			window->custom->type == WINDOW_TYPE_LISTBOX ||
			window->custom->type == WINDOW_TYPE_LISTVIEW);
}

void DoCustomComboBoxEvent(lua_State* L, LuaWindow* parent, LuaWindow* child, HWND hwnd, UINT Msg, WPARAM wParam, LPARAM lParam) {

	WORD type = HIWORD(wParam);

	if (type == CBN_SELCHANGE) {

		int ItemIndex = SendMessageW((HWND)lParam, (UINT)CB_GETCURSEL, (WPARAM)0, (LPARAM)0);

		if (!IsBox(child)) {
			return;
		}

		lua_rawgeti(L, LUA_REGISTRYINDEX, child->custom->eventRef);
		lua_pushvalue(L, -4);
		lua_pushvalue(L, -5);
		lua_pushinteger(L, ItemIndex + 1);

		if (lua_pcall(L, 3, 0, NULL)) {

			puts(lua_tostring(L, -1));
			lua_pop(L, 1);
		}
	}
}

void DoCustomListBoxEvent(lua_State* L, LuaWindow* parent, LuaWindow* child, HWND hwnd, UINT Msg, WPARAM wParam, LPARAM lParam) {

	WORD type = HIWORD(wParam);

	if (type == LBN_SELCHANGE) {

		int ItemIndex = SendMessageW((HWND)lParam, (UINT)LB_GETCURSEL, (WPARAM)0, (LPARAM)0);

		if (!IsBox(child)) {
			return;
		}

		lua_rawgeti(L, LUA_REGISTRYINDEX, child->custom->eventRef);
		lua_pushvalue(L, -4);
		lua_pushvalue(L, -5);
		lua_pushinteger(L, ItemIndex + 1);

		if (lua_pcall(L, 3, 0, NULL)) {

			puts(lua_tostring(L, -1));
			lua_pop(L, 1);
		}
	}
}

void DoCustomListViewEvent(lua_State* L, LuaWindow* parent, LuaWindow* child, HWND hwnd, UINT Msg, WPARAM wParam, LPARAM lParam) {

	NMHDR* nmh = (NMHDR*)lParam;
	NMLISTVIEW* nmlist = (NMLISTVIEW*)lParam;

	if (nmh && nmh->code == LVN_ITEMCHANGED && nmlist->uNewState & LVIS_SELECTED) {

		int ItemIndex = ListView_GetNextItem(child->handle, -1, LVNI_SELECTED);

		if (!IsBox(child)) {
			return;
		}

		lua_rawgeti(L, LUA_REGISTRYINDEX, child->custom->eventRef);
		lua_pushvalue(L, -4);
		lua_pushvalue(L, -5);
		lua_pushinteger(L, ItemIndex + 1);

		if (lua_pcall(L, 3, 0, NULL)) {

			puts(lua_tostring(L, -1));
			lua_pop(L, 1);
		}
	}
}

int SetSelectedIndex(lua_State* L) {

	LuaWindow* window = lua_tonwindow(L, 1);
	int index = luaL_optinteger(L, 2, 0) - 1;

	if (!IsBox(window)) {

		luaL_error(L, "Cannot add combobox strings to non combobox elements");
		return 0;
	}

	if (window->custom->type == WINDOW_TYPE_COMBOBOX) {
		SendMessageW(window->handle, CB_SETCURSEL, (WPARAM)index, (LPARAM)0);
	}
	else if (window->custom->type == WINDOW_TYPE_LISTVIEW) {
		ListView_SetItemState(window->handle, index, LVNI_SELECTED | LVNI_FOCUSED, LVNI_SELECTED | LVNI_FOCUSED);
	}
	else if (window->custom->type == WINDOW_TYPE_LISTBOX) {
		SendMessageW(window->handle, LB_SETCURSEL, (WPARAM)index, (LPARAM)0);
	}

	return 0;
}

int GetSelectedIndex(lua_State* L) {

	LuaWindow* window = lua_tonwindow(L, 1);

	if (!IsBox(window)) {

		luaL_error(L, "Cannot add combobox strings to non combobox elements");
		return 0;
	}

	int ItemIndex = -1;

	if (window->custom->type == WINDOW_TYPE_COMBOBOX) {
		ItemIndex = SendMessageW(window->handle, (UINT)CB_GETCURSEL, (WPARAM)0, (LPARAM)0);
	}
	else if (window->custom->type == WINDOW_TYPE_LISTVIEW) {
		ItemIndex = ListView_GetNextItem(window->handle, -1, LVNI_SELECTED);
	}
	else if (window->custom->type == WINDOW_TYPE_LISTBOX) {
		ItemIndex = SendMessageW(window->handle, (UINT)LB_GETCURSEL, (WPARAM)0, (LPARAM)0);
	}

	lua_pushinteger(L, ItemIndex + 1);

	return 1;
}

int DeleteBoxItem(lua_State* L) {

	LuaWindow* window = lua_tonwindow(L, 1);
	int index = (int)luaL_checkinteger(L, 2) - 1;

	if (!IsBox(window)) {

		luaL_error(L, "Cannot add combobox strings to non combobox elements");
		return 0;
	}

	if (window->custom->type == WINDOW_TYPE_COMBOBOX) {
		lua_pushboolean(L, SendMessageW(window->handle, (UINT)CB_DELETESTRING, (WPARAM)index, NULL) != CB_ERR);
	}
	else if (window->custom->type == WINDOW_TYPE_LISTVIEW) {
		lua_pushboolean(L, ListView_DeleteItem(window->handle, index) == TRUE);
	}
	else {
		lua_pushboolean(L, SendMessageW(window->handle, (UINT)LB_DELETESTRING, (WPARAM)index, NULL) != LB_ERR);
	}

	return 1;
}

int GetBoxItems(lua_State* L) {

	LuaWindow* window = lua_tonwindow(L, 1);

	if (!IsBox(window)) {

		luaL_error(L, "Cannot add combobox strings to non combobox elements");
		return 0;
	}

	size_t len, items;
	size_t bufferlen = 0;
	wchar_t* data = NULL;
	int column = luaL_optinteger(L, 2, 1) - 1;

	if (window->custom->type == WINDOW_TYPE_COMBOBOX) {
		items = SendMessageW(window->handle, (UINT)CB_GETCOUNT, (WPARAM)0, (LPARAM)0);
		lua_createtable(L, items, 0);
	}
	else if (window->custom->type == WINDOW_TYPE_LISTVIEW) {
		items = ListView_GetItemCount(window->handle);
		lua_createtable(L, items, 0);
	}
	else if (window->custom->type == WINDOW_TYPE_LISTBOX) {
		items = SendMessageW(window->handle, (UINT)LB_GETCOUNT, (WPARAM)0, (LPARAM)0);
		lua_createtable(L, items, 0);
	}
	else {
		return 0;
	}

	LVITEMW item;

	for (size_t i = 0; i < items; i++)
	{
		if (window->custom->type == WINDOW_TYPE_COMBOBOX) {
			len = SendMessageW(window->handle, (UINT)CB_GETLBTEXTLEN, (WPARAM)i, (LPARAM)0);
		}
		else if (window->custom->type == WINDOW_TYPE_LISTVIEW) {
			len = 1024;
		}
		else {
			return 1;
		}

		if (len > bufferlen) {
			if (data) {
				gff_free(data);
			}

			data = (wchar_t*)gff_calloc(len + 1, sizeof(wchar_t));
			if (!data) {
				luaL_error(L, "Out of memory");
				return 0;
			}
			else {
				bufferlen = len;
			}
		}

		if (window->custom->type == WINDOW_TYPE_COMBOBOX) {
			len = SendMessageW(window->handle, (UINT)CB_GETLBTEXT, (WPARAM)i, (LPARAM)data);
		}
		if (window->custom->type == WINDOW_TYPE_LISTVIEW) {
			item.iSubItem = column;
			item.pszText = data;
			item.cchTextMax = len;
			len = SendMessageW((window->handle), LVM_GETITEMTEXTW, (WPARAM)(i), (LPARAM)(LV_ITEM*)&item);
		}

		lua_pushwchar(L, data, len);
		lua_rawseti(L, -2, i + 1);
	}

	if (data) {
		gff_free(data);
	}

	return 1;
}

int ListviewSetItemText(lua_State* L) {

	LuaWindow* window = lua_tonwindow(L, 1);
	int row = luaL_checkinteger(L, 2) - 1;
	int column = luaL_checkinteger(L, 3) - 1;
	LuaWChar* data = lua_stringtowchar(L, 4);

	if (!IsBox(window) || window->custom->type != WINDOW_TYPE_LISTVIEW) {

		luaL_error(L, "Cannot modify non listview items");
		return 0;
	}

	LV_ITEMW item;

	item.iItem = row;
	item.iSubItem = column;
	item.pszText = data->str;
	item.cchTextMax = data->len;

	SendMessageW(window->handle, LVM_SETITEMTEXTW, row, (LPARAM)&item);

	return 0;
}

int SetViewlistColumnWidth(lua_State* L) {

	LuaWindow* window = lua_tonwindow(L, 1);
	int column = luaL_checkinteger(L, 2) - 1;
	int width = luaL_checkinteger(L, 3);

	if (!IsBox(window) || window->custom->type != WINDOW_TYPE_LISTVIEW) {

		luaL_error(L, "Cannot modify non listview items");
		return 0;
	}

	lua_pushboolean(L, ListView_SetColumnWidth(window->handle, column, width) == TRUE);

	return 1;
}

int AddBoxItem(lua_State* L) {

	LuaWindow* window = lua_tonwindow(L, 1);
	LuaWChar* data;

	if (!IsBox(window)) {

		luaL_error(L, "Cannot add combobox strings to non combobox elements");
		return 0;
	}

	size_t len;
	int ItemIndex = 0;

	if (window->custom->type == WINDOW_TYPE_COMBOBOX) {
		len = SendMessageW(window->handle, (UINT)CB_GETCOUNT, (WPARAM)0, (LPARAM)0);
		ItemIndex = SendMessageW(window->handle, (UINT)CB_GETCURSEL, (WPARAM)0, (LPARAM)0);
		SendMessageW(window->handle, CB_SETCURSEL, (WPARAM)len, (LPARAM)0);
	}
	else if (window->custom->type == WINDOW_TYPE_LISTVIEW) {
		len = ListView_GetItemCount(window->handle);
	}
	else {
		len = SendMessageW(window->handle, (UINT)LB_GETCOUNT, (WPARAM)0, (LPARAM)0);
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
	}
	else if (window->custom->type == WINDOW_TYPE_LISTVIEW) {

		if (lua_type(L, 2)) {

			lua_pushvalue(L, 2);

			LV_ITEMW item;
			ZeroMemory(&item, sizeof(LV_ITEMW));
			item.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM | WS_VISIBLE;
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

			lua_pop(L, 1);
		}
	}
	else {
		data = lua_stringtowchar(L, 2);
		SendMessageW(window->handle, (UINT)LB_ADDSTRING, (WPARAM)0, (LPARAM)data->str);
		SendMessageW(window->handle, LB_SETCURSEL, (WPARAM)ItemIndex, (LPARAM)0);
	}

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

	return 1;
}