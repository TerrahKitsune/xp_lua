#include "customwindow.h"
#include "luawindow.h"
#include "customdrawing.h"
#include "custombutton.h"

lua_State* LuaStateCallback;
int msgcount;
int emptytableref = LUA_REFNIL;
int windowdrawMeta = LUA_REFNIL;

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

	if (!window->custom || window->custom->type != WINDOW_TYPE_CUSTOM) {
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

	RedrawWindow(window->handle, NULL, NULL, RDW_INVALIDATE);

	return 0;
}

LuaCustomWindow* CreateCustomWindowStruct() {

	LuaCustomWindow * custom = (LuaCustomWindow*)gff_calloc(1, sizeof(LuaCustomWindow));

	if (!custom) {
		return NULL;
	}

	custom->threadRef = LUA_REFNIL;
	custom->customDrawingRef = LUA_REFNIL;
	custom->childRef = LUA_REFNIL;
	custom->eventRef = LUA_REFNIL;

	return custom;
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

LRESULT CALLBACK WndProc(HWND hwnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{
	lua_State* L = LuaStateCallback;
	LuaWindow* window = NULL;
	LuaWindow* child = NULL;

	if (L) {

		window = lua_type(L, 1) == LUA_TUSERDATA ? lua_tonwindow(L, 1) : NULL;

		if (!window || !window->custom) {
			return DefWindowProc(hwnd, Msg, wParam, lParam);
		}

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

		//printf("%u %u", hwnd, Msg);
		//DumpStack(L);

		lua_rawseti(L, -2, ++msgcount);
	}
	else {
		return DefWindowProc(hwnd, Msg, wParam, lParam);
	}

	switch (Msg)
	{

	case WM_NCDESTROY:

		break;

	case WM_DESTROY:

		PostQuitMessage(WM_QUIT);		
		break;

	case WM_COMMAND:

		child = GetLuaTableChild(L, window->custom, (HMENU)LOWORD(wParam));

		if (!child || !child->custom) {
			lua_pop(L, 1);
			return DefWindowProc(hwnd, Msg, wParam, lParam);
		}
		else if (child->custom->type == WINDOW_TYPE_BUTTON) {
			DoCustomButtonEvent(L, window, child, hwnd, Msg, wParam, lParam);
		}

		lua_pop(L, 1);

		break;
	case WM_PAINT:

		PAINTSTRUCT ps;
		HDC         hdc;

		hdc = BeginPaint(hwnd, &ps);

		if (window->custom->customDrawingRef != LUA_REFNIL) {

			lua_rawgeti(L, LUA_REGISTRYINDEX, window->custom->customDrawingRef);
			LuaCustomDrawing* draw = lua_tonwindowdrawing(L, -1);

			if (draw->paintFunctionRef) {

				lua_rawgeti(L, LUA_REGISTRYINDEX, draw->paintFunctionRef);
				lua_pushvalue(L, -2);

				draw->hdc = &hdc;
				draw->ps = &ps;
				draw->window = hwnd;

				if (lua_pcall(L, 1, 0, NULL)) {

					draw->hdc = NULL;
					draw->ps = NULL;
					draw->window = NULL;

					EndPaint(hwnd, &ps);
					lua_error(L);
					return 0;
				}
				else {

					draw->hdc = NULL;
					draw->ps = NULL;
					draw->window = NULL;
				}
			}

			lua_pop(L, 1);
		}

		EndPaint(hwnd, &ps);

		break;

	default:
		return DefWindowProc(hwnd, Msg, wParam, lParam);
	}

	return 0;
}

bool ContainsMessage(lua_State* L, UINT Msg){

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

	if (window->handle == NULL || window->custom == NULL || window->custom->type != WINDOW_TYPE_CUSTOM) {
		return false;
	}

	MSG Msg;
	lua_State* prev = LuaStateCallback;
	LuaStateCallback = NULL;

	bool hasMessage = PeekMessage(&Msg, window->handle, 0, 0, 0);

	LuaStateCallback = prev;

	return hasMessage;
}

int lua_customcoroutineiterator(lua_State* L, int status, lua_KContext ctx) {

	LuaWindow* window = lua_tonwindow(L, 1);

	if (!CheckHasMessage(window)) {

		lua_rawgeti(L, LUA_REGISTRYINDEX, emptytableref);

		size_t len = lua_rawlen(L, -1);
		if (len > 0) {
			luaL_unref(L, LUA_REGISTRYINDEX, emptytableref);
			lua_pop(L, 1);
			lua_newtable(L);
			lua_pushvalue(L, -1);
			emptytableref = luaL_ref(L, LUA_REGISTRYINDEX);
		}

		lua_yieldk(L, 1, ctx, lua_customcoroutineiterator);
		return 1;
	}
	
	lua_newtable(L);

	msgcount = 0;
	lua_State* prev = LuaStateCallback;
	LuaStateCallback = L;
	MSG Msg;

	if (GetMessage(&Msg, window->handle, 0, 0))
	{
		TranslateMessage(&Msg);
		DispatchMessage(&Msg);
	}

	LuaStateCallback = prev;

	if (!ContainsMessage(L, WM_NCDESTROY)) {
		lua_yieldk(L, 1, ctx, lua_customcoroutineiterator);
	}

	return 1;
}

int lua_customwindowloop(lua_State* L) {
	return lua_yieldk(L, 0, 0, lua_customcoroutineiterator);
}

int CreateLuaCustomWindow(lua_State* L) {

	if (emptytableref == LUA_REFNIL) {
		lua_newtable(L);
		emptytableref = luaL_ref(L, LUA_REGISTRYINDEX);
	}

	LuaCustomWindow* custom = CreateCustomWindowStruct();

	if (!custom) {
		luaL_error(L, "out of memory");
		return 0;
	}

	HWND parent = lua_type(L, 1) == LUA_TUSERDATA ? lua_tonwindow(L, 1)->handle : NULL;
	size_t lenclassname;
	const char* classname = luaL_checklstring(L, 2, &lenclassname);
	size_t lentitle;
	const char* title = luaL_checklstring(L, 3, &lentitle);
	int x = (int)luaL_checkinteger(L, 4);
	int y = (int)luaL_checkinteger(L, 5);
	int width = (int)luaL_checkinteger(L, 6);
	int height = (int)luaL_checkinteger(L, 7);

	custom->className = (char*)gff_calloc(lenclassname + 1, sizeof(char));
	if (!custom->className) {
		CleanUp(custom);
		luaL_error(L, "out of memory");
		return 0;
	}
	memcpy(custom->className, classname, lenclassname);

	custom->title = (char*)gff_calloc(lentitle + 1, sizeof(char));
	if (!custom->title) {
		CleanUp(custom);
		luaL_error(L, "out of memory");
		return 0;
	}
	memcpy(custom->title, title, lentitle);

	WNDCLASSEX  WndClsEx = { 0 };
	HINSTANCE hInstance = GetModuleHandle(NULL);

	WndClsEx.cbSize = sizeof(WNDCLASSEX);
	WndClsEx.style = (UINT)luaL_optinteger(L, 8, CS_HREDRAW | CS_VREDRAW);
	WndClsEx.lpfnWndProc = WndProc;
	WndClsEx.hInstance = hInstance;
	WndClsEx.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
	WndClsEx.lpszClassName = custom->className;
	WndClsEx.hIconSm = LoadIcon(hInstance, IDI_APPLICATION);

	RegisterClassEx(&WndClsEx);

	LuaStateCallback = NULL;

	HWND hwnd = CreateWindowEx(
		(DWORD)luaL_optinteger(L, 9, WS_EX_OVERLAPPEDWINDOW),
		custom->className,
		custom->title,
		(DWORD)luaL_optinteger(L, 10, WS_OVERLAPPEDWINDOW),
		x,
		y,
		width,
		height,
		parent,
		NULL,
		hInstance,
		NULL);

	lua_pop(L, lua_gettop(L));

	if (hwnd == NULL) {

		CleanUp(custom);
		lua_pushnil(L);
		return 1;
	}

	LuaWindow* window = lua_pushwindow(L);
	window->handle = hwnd;
	window->custom = custom;

	lua_State* T = lua_newthread(L);
	lua_pushvalue(L, -1);
	window->custom->threadRef = luaL_ref(L, LUA_REGISTRYINDEX);
	lua_pushcfunction(T, lua_customwindowloop);
	lua_pushvalue(L, 1);
	lua_xmove(L, T, 1);
	lua_resume(T, L, 1);

	return 2;
}