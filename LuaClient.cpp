#include "LuaClient.h"
#include "Client.h"
#include "NetEvent.h"
#define CLI_BUFFERSIZE 1500

int CLI_LIST_INDEX = LUA_REFNIL;

DWORD WINAPI CliProc(LPVOID lpParam) {

	LuaClientThread * self = (LuaClientThread *)lpParam;
	Client * cli = NULL;
	int disc;
	size_t read;
	size_t total;
	char buffer[CLI_BUFFERSIZE];
	NetEvent * ev;

	while (self->IsAlive) {

		if (!cli) {
			cli = ClientConnect(self->addr, self->Port);
			if (!cli) {
				self->IsConnected = false;
				Sleep(1);
				continue;
			}
			else if (cli->s != INVALID_SOCKET) {
				self->IsConnected = true;
				queue_Enqueue(self->Events, NetEvent_Create(cli->s, NETEVENT_CONNECTED, NULL, 0));
			}
			else {
				self->LastError = cli->LastError;
				self->IsConnected = false;
				ClientDisconnect(cli);
				cli = NULL;
				Sleep(1);
				continue;
			}
		}

		if (cli->s == INVALID_SOCKET) {
			queue_Enqueue(self->Events, NetEvent_Create(cli->s, NETEVENT_DISCONNECTED, NULL, 0));
			self->LastError = cli->LastError;
			self->IsConnected = false;
			ClientDisconnect(cli);
			cli = NULL;
			continue;
		}

		ev = (NetEvent *)queue_Dequeue(self->Send);
		while (ev) {

			if (cli && cli->s != INVALID_SOCKET) {

				if (ev->type == NETEVENT_RECEIVE || ev->type == NETEVENT_SEND) {
					disc = 0;
					total = 0;

					do {

						read = ClientSend(cli, &ev->data[total], ev->len - total, NULL);
						if (cli->s == INVALID_SOCKET) {
							break;
						}
						else {
							queue_Enqueue(self->Events, NetEvent_Create(ev->s, NETEVENT_SEND, &ev->data[total], ev->len - total));
							total += read;
						}

					} while (total < (size_t)ev->len);
				}
				else if (ev->type == NETEVENT_DISCONNECTED) {
					queue_Enqueue(self->Events, NetEvent_Create(cli->s, NETEVENT_DISCONNECTED, NULL, 0));
					self->LastError = cli->LastError;
					self->IsConnected = false;
					ClientDisconnect(cli);
					cli = NULL;

					self->IsAlive = false;
				}
			}

			free(ev);
			ev = (NetEvent *)queue_Dequeue(self->Send);
		}

		if (!cli)
			continue;

		read = ClientReceive(cli, buffer, CLI_BUFFERSIZE, NULL);
		if (read > 0) {
			queue_Enqueue(self->Events, NetEvent_Create(cli->s, NETEVENT_RECEIVE, buffer, read));
		}

		Sleep(1);
	}

	ClientDisconnect(cli);

	HANDLE thread = self->Thread;

	list_Remove(self->All, self);

	queue_Destroy(self->Events);
	queue_Destroy(self->Send);

	free(self);

	CloseHandle(thread);

	return 0;
}

List * _CliList(lua_State *L) {

	if (CLI_LIST_INDEX == LUA_REFNIL) {

		lua_pushlightuserdata(L, list_CreateList());
		CLI_LIST_INDEX = luaL_ref(L, LUA_REGISTRYINDEX);
	}

	lua_rawgeti(L, LUA_REGISTRYINDEX, CLI_LIST_INDEX);

	List * lst = (List *)lua_touserdata(L, -1);
	lua_pop(L, 1);
	return lst;
}

LuaClient * lua_toluaclient(lua_State *L, int index) {

	LuaClient * proc = (LuaClient*)lua_touserdata(L, index);
	if (proc == NULL)
		luaL_error(L, "parameter is not a %s", LUACLIENT);
	return proc;
}

int luaclient_connect(lua_State *L) {

	const char * addr = luaL_checkstring(L, 1);
	int port = (int)luaL_checkinteger(L, 2);

	LuaClientThread * thread = (LuaClientThread*)calloc(1, sizeof(LuaClientThread));

	thread->addr = (char*)malloc(strlen(addr) + 1);
	strcpy(thread->addr, addr);

	thread->Events = queue_Create();
	thread->Send = queue_Create();
	thread->Port = port;

	lua_pop(L, lua_gettop(L));

	List * lst = _CliList(L);

	thread->All = lst;
	thread->IsAlive = true;
	thread->Thread = CreateThread(NULL, 0, CliProc, thread, 0, &thread->ThreadId);
	if (!thread->Thread) {

		free(thread->addr);
		free(thread);

		lua_pop(L, lua_gettop(L));
		lua_pushnil(L);
		lua_pushstring(L, "Unable to start client thread");

		return 2;
	}

	LuaClient * srv = lua_pushluaclient(L);

	srv->Thread = thread;

	list_Add(lst, thread);

	return 1;
}

int luaclient_getevent(lua_State *L) {

	LuaClient * cli = lua_toluaclient(L, 1);
	LuaClientThread * thread;
	if (!cli || !cli->Thread) {

		lua_pop(L, lua_gettop(L));
		lua_pushnil(L);
		return 1;
	}

	List * lst = _CliList(L);
	list_Enter(lst);

	for (unsigned int n = 0; n < lst->len; n++) {
		if (((LuaClientThread *)lst->data[n]) == cli->Thread) {
			thread = (LuaClientThread *)lst->data[n];
			break;
		}
	}

	if (!thread) {

		cli->Thread = NULL;
		list_Leave(lst);

		lua_pop(L, lua_gettop(L));
		lua_pushnil(L);
		return 1;
	}

	NetEvent * netevent = (NetEvent *)queue_Dequeue(thread->Events);

	if (!netevent) {
		list_Leave(lst);

		lua_pop(L, lua_gettop(L));
		lua_pushnil(L);
		return 1;
	}

	lua_pop(L, lua_gettop(L));

	lua_createtable(L, 0, 3);

	lua_pushstring(L, "socket");
	lua_pushinteger(L, netevent->s);
	lua_settable(L, -3);

	lua_pushstring(L, "type");
	lua_pushinteger(L, netevent->type);
	lua_settable(L, -3);

	lua_pushstring(L, "data");
	lua_pushlstring(L, netevent->data, netevent->len);
	lua_settable(L, -3);

	list_Leave(lst);

	free(netevent);

	return 1;
}

int luaclient_send(lua_State *L) {

	size_t datalen;
	LuaClient * cli = lua_toluaclient(L, 1);
	const char * data = luaL_checklstring(L, 2, &datalen);

	LuaClientThread * thread;
	if (!cli || !cli->Thread || !data || datalen <= 0) {

		lua_pop(L, lua_gettop(L));
		lua_pushboolean(L, false);
		return 1;
	}

	List * lst = _CliList(L);
	list_Enter(lst);

	for (unsigned int n = 0; n < lst->len; n++) {
		if (((LuaClientThread *)lst->data[n]) == cli->Thread) {
			thread = (LuaClientThread *)lst->data[n];
			break;
		}
	}

	if (!thread) {

		cli->Thread = NULL;
		list_Leave(lst);

		lua_pop(L, lua_gettop(L));
		lua_pushboolean(L, false);
		return 1;
	}

	queue_Enqueue(thread->Send, NetEvent_Create(INVALID_SOCKET, NETEVENT_SEND, data, datalen));

	list_Leave(lst);
	lua_pop(L, lua_gettop(L));
	lua_pushboolean(L, true);
	return 1;
}

int luaclient_status(lua_State *L) {

	LuaClient * cli = lua_toluaclient(L, 1);

	LuaClientThread * thread;
	if (!cli || !cli->Thread) {

		lua_pop(L, lua_gettop(L));
		lua_pushboolean(L, false);
		return 1;
	}

	List * lst = _CliList(L);
	list_Enter(lst);

	for (unsigned int n = 0; n < lst->len; n++) {
		if (((LuaClientThread *)lst->data[n]) == cli->Thread) {
			thread = (LuaClientThread *)lst->data[n];
			break;
		}
	}

	if (!thread) {

		cli->Thread = NULL;
		list_Leave(lst);

		lua_pop(L, lua_gettop(L));
		lua_pushboolean(L, false);
		lua_pushinteger(L, 0);
		return 2;
	}

	lua_pop(L, lua_gettop(L));
	lua_pushboolean(L, thread->IsConnected);
	lua_pushinteger(L, thread->LastError);
	list_Leave(lst);

	return 2;
}

LuaClient * lua_pushluaclient(lua_State *L) {

	LuaClient * proc = (LuaClient*)lua_newuserdata(L, sizeof(LuaClient));
	if (proc == NULL)
		luaL_error(L, "Unable to create luaclient");
	luaL_getmetatable(L, LUACLIENT);
	lua_setmetatable(L, -2);
	memset(proc, 0, sizeof(LuaClient));
	return proc;
}

int luaclient_gc(lua_State *L) {

	LuaClient * cli = lua_toluaclient(L, 1);

	List * lst = _CliList(L);

	if (cli->Thread) {

		list_Enter(lst);

		for (unsigned int n = 0; n < lst->len; n++) {
			if (((LuaClientThread *)lst->data[n]) == cli->Thread) {

				cli->Thread->IsAlive = false;
				break;
			}
		}

		cli->Thread = NULL;

		list_Leave(lst);
	}

	lua_pop(L, 1);
	return 0;

	return 0;
}

int luaclient_tostring(lua_State *L) {
	char tim[100];
	sprintf(tim, "LuaClient: 0x%08X", lua_toluaclient(L, 1));
	lua_pushfstring(L, tim);
	return 1;
}