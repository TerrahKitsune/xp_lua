#include "customwindow.h"
#include "luawindow.h"
#include "customdrawing.h"
#include "custombutton.h"
#include "customtextbox.h"
#include "luawchar.h"
#include "customcombobox.h"
#include "resource.h"

#define WINDOW_EVENT_RESIZE 1

lua_State* LuaStateCallback;
int msgcount;
int windowdrawMeta = LUA_REFNIL;
int windowTable = LUA_REFNIL;
int messageLoopThread = LUA_REFNIL;

void CleanUp(LuaCustomWindow* custom) {

	if (custom->className) {
		gff_free(custom->className);
	}

	if (custom->title) {
		gff_free(custom->title);
	}

	gff_free(custom);
}

int LuaSetCustomWindowDrawFunction(lua_State* L) {

	LuaWindow* window = lua_tonwindow(L, 1);

	if (!window->custom) {
		return 0;
	}

	if (lua_gettop(L) == 1) {
		lua_pushnil(L);
	}

	if (windowdrawMeta == LUA_REFNIL) {
		luaopen_windowdrawing(L);
		windowdrawMeta = luaL_ref(L, LUA_REGISTRYINDEX);
	}

	if (window->custom->customDrawingRef == LUA_REFNIL) {
		CreateCustomDrawing(L);
		window->custom->customDrawingRef = luaL_ref(L, LUA_REGISTRYINDEX);
	}

	lua_rawgeti(L, LUA_REGISTRYINDEX, window->custom->customDrawingRef);

	LuaCustomDrawing* drawing = lua_tonwindowdrawing(L, -1);

	lua_pushvalue(L, 2);

	if (drawing->paintFunctionRef != LUA_REFNIL) {
		luaL_unref(L, LUA_REGISTRYINDEX, drawing->paintFunctionRef);
		drawing->paintFunctionRef = LUA_REFNIL;
	}

	if (lua_type(L, -1) == LUA_TFUNCTION) {
		drawing->paintFunctionRef = luaL_ref(L, LUA_REGISTRYINDEX);
	}

	return 0;
}

LuaCustomWindow* CreateCustomWindowStruct() {

	LuaCustomWindow* custom = (LuaCustomWindow*)gff_calloc(1, sizeof(LuaCustomWindow));

	if (!custom) {
		return NULL;
	}

	custom->threadRef = LUA_REFNIL;
	custom->customDrawingRef = LUA_REFNIL;
	custom->childRef = LUA_REFNIL;
	custom->eventRef = LUA_REFNIL;
	custom->parentRef = LUA_REFNIL;

	return custom;
}

void RemoveWindowFromTable(lua_State* L, HWND hwnd) {

	if (windowTable == LUA_REFNIL) {
		return;
	}

	lua_rawgeti(L, LUA_REGISTRYINDEX, windowTable);
	lua_pushhwnd(L, hwnd);
	lua_pushnil(L);
	lua_settable(L, -3);
	
	lua_pop(L, 1);
}

int GetActiveMainWindows(lua_State* L) {
	
	int count = 0;
	LuaWindow* window;

	if (windowTable == LUA_REFNIL) {
		return count;
	}

	lua_rawgeti(L, LUA_REGISTRYINDEX, windowTable);
	lua_pushnil(L);

	while (lua_next(L, -2) != 0) {
		
		window = lua_tonwindow(L, -1);

		if (window && window->custom && window->custom->type != WINDOW_TYPE_INVALID) {
			count++;
		}

		lua_pop(L, 1);
	}

	lua_pop(L, 1);

	return count;
}

void AddWindowToTable(lua_State* L, int idx) {

	LuaWindow* window = lua_tonwindow(L, idx);

	lua_rawgeti(L, LUA_REGISTRYINDEX, windowTable);
	lua_pushhwnd(L, window->handle);
	lua_pushvalue(L, idx);
	lua_settable(L, -3);
	lua_pop(L, 1);
}

size_t GetLuaChildrenCount(lua_State* L, LuaCustomWindow* window) {

	if (window->childRef == LUA_REFNIL) {
		lua_newtable(L);
		window->childRef = luaL_ref(L, LUA_REGISTRYINDEX);
	}

	lua_rawgeti(L, LUA_REGISTRYINDEX, window->childRef);
	size_t len = lua_rawlen(L, -1);
	lua_pop(L, 1);

	return len;
}

void RemoveLuaTableChild(lua_State* L, LuaWindow* window) {

	if (!window->custom || window->custom->childRef == LUA_REFNIL) {
		return;
	}

	lua_newtable(L);
	lua_rawgeti(L, LUA_REGISTRYINDEX, window->custom->childRef);
	size_t len = lua_rawlen(L, -1);
	LuaWindow* sub;

	int nth = 0;
	for (size_t i = 0; i < len; i++)
	{
		lua_pushinteger(L, i + 1);
		lua_gettable(L, -2);

		sub = lua_tonwindow(L, -1);

		if (sub != window) {
			lua_rawseti(L, -3, ++nth);
		}
		else {
			lua_pop(L, 1);
		}
	}

	luaL_unref(L, LUA_REGISTRYINDEX, window->custom->childRef);
	lua_pop(L, 1);
	window->custom->childRef = luaL_ref(L, LUA_REGISTRYINDEX);
	lua_pop(L, 1);
}

LuaWindow* GetLuaTableChild(lua_State* L, LuaCustomWindow* window, HMENU menuid) {

	if (window->childRef == LUA_REFNIL) {
		lua_pushnil(L);
		return NULL;
	}

	lua_rawgeti(L, LUA_REGISTRYINDEX, window->childRef);
	size_t len = lua_rawlen(L, -1);
	LuaWindow* sub;

	for (size_t i = 0; i < len; i++)
	{
		lua_pushinteger(L, i + 1);
		lua_gettable(L, -2);

		sub = lua_tonwindow(L, -1);

		if (sub->custom && sub->custom->hmenu == menuid) {

			lua_copy(L, -1, -2);
			lua_pop(L, 1);
			return sub;
		}

		lua_pop(L, 1);
	}

	lua_pop(L, 1);
	lua_pushnil(L);

	return NULL;
}

void AddLuaTableChild(lua_State* L, LuaCustomWindow* window) {

	if (window->childRef == LUA_REFNIL) {
		lua_newtable(L);
		window->childRef = luaL_ref(L, LUA_REGISTRYINDEX);
	}

	lua_rawgeti(L, LUA_REGISTRYINDEX, window->childRef);
	size_t len = lua_rawlen(L, -1);
	lua_pushvalue(L, -2);
	lua_rawseti(L, -2, len + 1);
	lua_pop(L, 1);
}

int RemoveCustomWindow(lua_State* L) {

	LuaWindow* window = lua_tonwindow(L, 1);

	if (!window->custom) {
		return 0;
	}

	if (window->custom && window->custom->parentRef != LUA_REFNIL) {

		lua_rawgeti(L, LUA_REGISTRYINDEX, window->custom->parentRef);
		RemoveLuaTableChild(L, lua_tonwindow(L, -1));
		lua_pop(L, 1);
	}

	LuaWindow* parent = window;

	while (parent->custom && parent->custom->parentRef != LUA_REFNIL) {

		lua_rawgeti(L, LUA_REGISTRYINDEX, parent->custom->parentRef);
		parent = lua_tonwindow(L, -1);
		lua_pop(L, 1);
	}

	BOOL result = PostMessageW(parent->handle, WM_LUA_DESTROY, (WPARAM)window->handle, 0);

	return 0;
}

LuaWindow* GetSuperParent(lua_State* L, LuaWindow* window) {

	LuaWindow* parent = window;

	while (parent->custom && parent->custom->parentRef != LUA_REFNIL) {

		lua_rawgeti(L, LUA_REGISTRYINDEX, parent->custom->parentRef);
		parent = lua_tonwindow(L, -1);
		lua_pop(L, 1);
	}

	return parent;
}

int MoveCustomWindow(lua_State* L) {

	LuaWindow* window = lua_tonwindow(L, 1);

	if (!window->custom) {
		return 0;
	}

	lua_createtable(L, 0, 4);

	lua_pushstring(L, "X");
	lua_pushinteger(L, luaL_checkinteger(L, 2));
	lua_settable(L, -3);

	lua_pushstring(L, "Y");
	lua_pushinteger(L, luaL_checkinteger(L, 3));
	lua_settable(L, -3);

	lua_pushstring(L, "Width");
	lua_pushinteger(L, luaL_checkinteger(L, 4));
	lua_settable(L, -3);

	lua_pushstring(L, "Height");
	lua_pushinteger(L, luaL_checkinteger(L, 5));
	lua_settable(L, -3);

	PostMessageW(GetSuperParent(L, window)->handle, WM_LUA_MOVE, (WPARAM)window->handle, luaL_ref(L, LUA_REGISTRYINDEX));

	return 0;
}

int DoCustomComboBoxEvent(lua_State* L, LuaWindow* parent, HWND hwnd, UINT Msg, WPARAM wParam, LPARAM lParam) {

	if (parent &&
		parent->custom &&
		parent->custom->type == WINDOW_TYPE_CUSTOM &&
		parent->custom->eventRef != LUA_REFNIL) {

		bool trigger = false;

		if (Msg == WM_SIZE) {

			trigger = true;
			lua_createtable(L, 0, 4);

			lua_pushstring(L, "Event");
			lua_pushinteger(L, WINDOW_EVENT_RESIZE);
			lua_settable(L, -3);

			lua_pushstring(L, "Type");
			lua_pushinteger(L, wParam);
			lua_settable(L, -3);

			lua_pushstring(L, "Height");
			lua_pushinteger(L, HIWORD(lParam));
			lua_settable(L, -3);

			lua_pushstring(L, "Width");
			lua_pushinteger(L, LOWORD(lParam));
			lua_settable(L, -3);
		}

		if (trigger) {
			lua_rawgeti(L, LUA_REGISTRYINDEX, parent->custom->eventRef);
			lua_pushvalue(L, -2);
			if (lua_pcall(L, 1, 0, NULL)) {

				puts(lua_tostring(L, -1));
				lua_pop(L, 1);
			}
			lua_pop(L, 1);
		}
	}

	return DefWindowProcW(hwnd, Msg, wParam, lParam);;
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{
	lua_State* L = LuaStateCallback;
	LuaWindow* window = NULL;
	LuaWindow* child = NULL;
	LuaWChar* wchar;
	BOOL result;
	int x, y, h, w;

	if (L && windowTable != LUA_REFNIL) {

		lua_pushvalue(L, 1);
		lua_rawgeti(L, LUA_REGISTRYINDEX, windowTable);
		lua_pushhwnd(L, hwnd);
		lua_gettable(L, -2);
		lua_copy(L, -1, 1);
		lua_pop(L, lua_gettop(L) - 2);

		window = lua_type(L, -2) == LUA_TUSERDATA ? lua_tonwindow(L, -2) : NULL;

		if (window && window->custom) {

			lua_createtable(L, 0, 4);

			lua_pushstring(L, "Message");
			lua_pushinteger(L, Msg);
			lua_settable(L, -3);

			lua_pushstring(L, "WParam");
			lua_pushinteger(L, wParam);
			lua_settable(L, -3);

			lua_pushstring(L, "LParam");
			lua_pushinteger(L, lParam);
			lua_settable(L, -3);

			lua_pushstring(L, "ID");
			lua_pushinteger(L, (lua_Integer)hwnd);
			lua_settable(L, -3);

			lua_rawseti(L, -2, ++msgcount);
		}
	}

	switch (Msg)
	{
	case WM_LUA_SETCONTENT:

		lua_rawgeti(L, LUA_REGISTRYINDEX, lParam);
		wchar = lua_towchar(L, -1);
		luaL_unref(L, LUA_REGISTRYINDEX, lParam);
		SetWindowTextW((HWND)wParam, wchar->str);
		lua_pop(L, 1);

		break;

	case WM_LUA_MOVE:

		lua_rawgeti(L, LUA_REGISTRYINDEX, lParam);

		lua_pushstring(L, "X");
		lua_gettable(L, -2);
		x = lua_tointeger(L, -1);
		lua_pop(L, 1);

		lua_pushstring(L, "Y");
		lua_gettable(L, -2);
		y = lua_tointeger(L, -1);
		lua_pop(L, 1);

		lua_pushstring(L, "Height");
		lua_gettable(L, -2);
		h = lua_tointeger(L, -1);
		lua_pop(L, 1);

		lua_pushstring(L, "Width");
		lua_gettable(L, -2);
		w = lua_tointeger(L, -1);
		lua_pop(L, 1);

		result = SetWindowPos((HWND)wParam, HWND_TOP, x, y, w, h, 0);
		luaL_unref(L, LUA_REGISTRYINDEX, lParam);
		lua_pop(L, 1);
		break;

	case WM_LUA_DESTROY:

		RemoveWindowFromTable(L, (HWND)wParam);
		DestroyWindow((HWND)wParam);
		break;
	case WM_LUA_TOGGLESHOW:

		ShowWindow((HWND)wParam, (int)lParam);
		break;
	case WM_LUA_UPDATE:

		UpdateWindow((HWND)wParam);
		break;
	case WM_LUA_TOGGLEENABLE:

		EnableWindow((HWND)wParam, (int)lParam);
		break;
	case WM_NCDESTROY:
		if (window && window->custom) {
			window->custom->type = WINDOW_TYPE_INVALID;
		}
		RemoveWindowFromTable(L, hwnd);
		break;

	case WM_DESTROY:

		PostQuitMessage(WM_QUIT);
		break;

	case WM_SIZE:
		if (window) {
			return DoCustomComboBoxEvent(L, window, hwnd, Msg, wParam, lParam);
		}
		break;

	case WM_NOTIFY:
		if (window) {
			child = GetLuaTableChild(L, window->custom, (HMENU)LOWORD(wParam));
			if (!child || !child->custom) {
				lua_pop(L, 1);
				return DefWindowProc(hwnd, Msg, wParam, lParam);
			}
			else if (child->custom->type == WINDOW_TYPE_LISTVIEW) {
				DoCustomListViewEvent(L, window, child, hwnd, Msg, wParam, lParam);
			}
			lua_pop(L, 1);
		}
		break;

	case WM_COMMAND:

		if (window) {

			child = GetLuaTableChild(L, window->custom, (HMENU)LOWORD(wParam));

			if (!child || !child->custom) {
				lua_pop(L, 1);
				return DefWindowProc(hwnd, Msg, wParam, lParam);
			}
			else if (child->custom->type == WINDOW_TYPE_BUTTON) {
				DoCustomButtonEvent(L, window, child, hwnd, Msg, wParam, lParam);
			}
			else if (child->custom->type == WINDOW_TYPE_TEXTBOX) {
				DoCustomTextboxEvent(L, window, child, hwnd, Msg, wParam, lParam);
			}
			else if (child->custom->type == WINDOW_TYPE_COMBOBOX) {
				DoCustomComboBoxEvent(L, window, child, hwnd, Msg, wParam, lParam);
			}
			else if (child->custom->type == WINDOW_TYPE_LISTBOX) {
				DoCustomListBoxEvent(L, window, child, hwnd, Msg, wParam, lParam);
			}
			else {
				puts("test");
			}

			lua_pop(L, 1);
		}

		break;


	case WM_PAINT:

		if (L) {
			lua_pushvalue(L, -2);
			CustomDrawEvent(L);
			lua_pop(L, 1);
		}
		else {
			PAINTSTRUCT ps;
			HDC         hdc;
			hdc = BeginPaint(window->handle, &ps);
			EndPaint(window->handle, &ps);
		}
		break;

	default:
		return DefWindowProcW(hwnd, Msg, wParam, lParam);
	}

	return 0;
}


int LuaGetContent(lua_State* L) {

	lua_State* prev = LuaStateCallback;
	LuaStateCallback = NULL;

	LuaWindow* window = lua_tonwindow(L, 1);

	size_t len = GetWindowTextLengthW(window->handle);

	wchar_t* data = (wchar_t*)gff_calloc(len + 1, sizeof(wchar_t));

	if (!data) {
		LuaStateCallback = prev;
		luaL_error(L, "Out of memory");
		return 0;
	}

	int ret = GetWindowTextW(window->handle, data, len + 1);

	lua_pushwchar(L, data);

	gff_free(data);

	LuaStateCallback = prev;

	return 1;
}

int LuaEnableCustomWindow(lua_State* L) {

	LuaWindow* window = lua_tonwindow(L, 1);
	int show = lua_toboolean(L, 2);

	if (!window->custom) {
		return 0;
	}

	LuaWindow* parent = GetSuperParent(L, window);

	PostMessageW(parent->handle, WM_LUA_TOGGLEENABLE, (WPARAM)window->handle, show);
	PostMessageW(parent->handle, WM_LUA_UPDATE, (WPARAM)window->handle, 0);

	return 0;
}

int LuaSetContent(lua_State* L) {

	LuaWindow* window = lua_tonwindow(L, 1);
	LuaWChar* content = lua_stringtowchar(L, 2);

	LuaWindow* parent = GetSuperParent(L, window);
	lua_pushwchar(L, content->str, content->len);
	PostMessageW(parent->handle, WM_LUA_SETCONTENT, (WPARAM)window->handle, luaL_ref(L, LUA_REGISTRYINDEX));

	return 0;
}

int LuaShowCustomWindow(lua_State* L) {

	LuaWindow* window = lua_tonwindow(L, 1);
	int show = lua_toboolean(L, 2);

	if (!window->custom) {
		return 0;
	}

	LuaWindow* parent = GetSuperParent(L, window);

	PostMessageW(parent->handle, WM_LUA_TOGGLESHOW, (WPARAM)window->handle, show);
	PostMessageW(parent->handle, WM_LUA_UPDATE, (WPARAM)window->handle, 0);

	return 0;
}

bool ContainsMessage(lua_State* L, UINT Msg) {

	size_t len = lua_rawlen(L, -1);

	if (len == 0) {
		return false;
	}

	for (size_t i = 0; i < len; i++) {

		lua_pushinteger(L, i + 1);
		lua_gettable(L, -2);

		lua_pushstring(L, "Message");
		lua_gettable(L, -2);

		if (lua_tointeger(L, -1) == Msg) {
			lua_pop(L, 2);
			return true;
		}

		lua_pop(L, 2);
	}

	return false;
}

bool CheckHasMessage(LuaWindow* window) {

	if (window->handle == NULL || window->custom == NULL) {
		return false;
	}

	MSG Msg;
	lua_State* prev = LuaStateCallback;
	LuaStateCallback = NULL;

	bool hasMessage = PeekMessageW(&Msg, NULL, 0, 0, 0) != 0;

	LuaStateCallback = prev;

	return hasMessage;
}

int lua_customcoroutineiterator(lua_State* L, int status, lua_KContext ctx) {

	lua_newtable(L);
	msgcount = 0;
	lua_State* prev = LuaStateCallback;
	LuaStateCallback = L;

	MSG Msg;

	do {
		if (GetMessageW(&Msg, NULL, 0, 0))
		{
			TranslateMessage(&Msg);
			DispatchMessageW(&Msg);
		}
	} while (PeekMessageW(&Msg, NULL, 0, 0, 0));
	
	LuaStateCallback = prev;

	lua_copy(L, -1, 1);
	lua_pop(L, lua_gettop(L) - 1);

	if (GetActiveMainWindows(L) > 0) {
		lua_yieldk(L, 1, ctx, lua_customcoroutineiterator);
	}
	else {
		luaL_unref(L, LUA_REGISTRYINDEX, messageLoopThread);
		messageLoopThread = LUA_REFNIL;
		luaL_unref(L, LUA_REGISTRYINDEX, windowTable);
		windowTable = LUA_REFNIL;
	}

	return 1;
}

int lua_customwindowloop(lua_State* L) {
	return lua_yieldk(L, 0, 0, lua_customcoroutineiterator);
}

int CreateLuaCustomWindow(lua_State* L) {

	if (windowTable == LUA_REFNIL) {
		lua_newtable(L);
		windowTable = luaL_ref(L, LUA_REGISTRYINDEX);
	}

	LuaCustomWindow* custom = CreateCustomWindowStruct();

	if (!custom) {
		luaL_error(L, "out of memory");
		return 0;
	}

	HWND parent = lua_type(L, 1) == LUA_TUSERDATA ? lua_tonwindow(L, 1)->handle : NULL;
	LuaWChar* classname = lua_stringtowchar(L, 2);
	LuaWChar* title = lua_stringtowchar(L, 3);
	int x = (int)luaL_checkinteger(L, 4);
	int y = (int)luaL_checkinteger(L, 5);
	int width = (int)luaL_checkinteger(L, 6);
	int height = (int)luaL_checkinteger(L, 7);

	custom->className = (wchar_t*)gff_calloc(classname->len + 1, sizeof(wchar_t));
	if (!custom->className) {
		CleanUp(custom);
		luaL_error(L, "out of memory");
		return 0;
	}
	memcpy(custom->className, classname->str, classname->len * sizeof(wchar_t));

	custom->title = (wchar_t*)gff_calloc(title->len + 1, sizeof(wchar_t));
	if (!custom->title) {
		CleanUp(custom);
		luaL_error(L, "out of memory");
		return 0;
	}
	memcpy(custom->title, title->str, title->len * sizeof(wchar_t));

	WNDCLASSEXW  WndClsEx = { 0 };
	HINSTANCE hInstance = GetModuleHandle(NULL);

	WndClsEx.cbSize = sizeof(WNDCLASSEXW);
	WndClsEx.style = CS_HREDRAW | CS_VREDRAW;
	WndClsEx.lpfnWndProc = WndProc;
	WndClsEx.hInstance = hInstance;
	WndClsEx.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
	WndClsEx.lpszClassName = custom->className;
	WndClsEx.hIconSm = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_ICON1));

	RegisterClassExW(&WndClsEx);

	LuaStateCallback = NULL;

	HWND hwnd = CreateWindowExW(
		0,
		custom->className,
		custom->title,
		(DWORD)luaL_optinteger(L, 8, WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX | WS_MAXIMIZEBOX | WS_CLIPCHILDREN),
		x,
		y,
		width,
		height,
		parent,
		NULL,
		hInstance,
		NULL);

	if (lua_type(L, 9) == LUA_TFUNCTION) {
		lua_pushvalue(L, 9);
		custom->eventRef = luaL_ref(L, LUA_REGISTRYINDEX);
	}

	lua_pop(L, lua_gettop(L));

	if (hwnd == NULL) {

		CleanUp(custom);
		lua_pushnil(L);
		return 1;
	}

	LuaWindow* window = lua_pushwindow(L);
	window->handle = hwnd;
	window->custom = custom;
	window->custom->type = WINDOW_TYPE_CUSTOM;

	if (messageLoopThread == LUA_REFNIL) 
	{
		lua_State* T = lua_newthread(L);
		lua_pushvalue(L, -1);
		messageLoopThread = luaL_ref(L, LUA_REGISTRYINDEX);
		lua_pushcfunction(T, lua_customwindowloop);
		lua_resume(T, L, 0);
	}
	else {
		lua_rawgeti(L, LUA_REGISTRYINDEX, messageLoopThread);
	}

	window->custom->threadRef = messageLoopThread;

	AddWindowToTable(L, 1);

	return 2;
}