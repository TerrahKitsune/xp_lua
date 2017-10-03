#include "LuaServer.h"
#include "Server.h"
#include "NetEvent.h"

#define SRV_BUFFERSIZE 1500

int SRV_FUNC_INDEX = LUA_REFNIL;
int SRV_LIST_INDEX = LUA_REFNIL;

int SetLuaIndexFunctionToRun(lua_State *L) {

	if (SRV_FUNC_INDEX != LUA_REFNIL) {
		luaL_unref(L, LUA_REGISTRYINDEX, SRV_FUNC_INDEX);
		SRV_FUNC_INDEX = LUA_REFNIL;
	}

	if (!lua_isfunction(L, 1)) {
		SRV_FUNC_INDEX = -1;
		return 0;
	}
	else
		SRV_FUNC_INDEX = luaL_ref(L, LUA_REGISTRYINDEX);

	return 0;
}

List * _SrvList(lua_State *L) {

	if (SRV_LIST_INDEX == LUA_REFNIL) {

		lua_pushlightuserdata(L, list_CreateList());
		SRV_LIST_INDEX = luaL_ref(L, LUA_REGISTRYINDEX);
	}

	lua_rawgeti(L, LUA_REGISTRYINDEX, SRV_LIST_INDEX);

	List * lst = (List *)lua_touserdata(L, -1);
	lua_pop(L, 1);
	return lst;
}

int luaserver_KillAll(lua_State *L) {

	List * lst = _SrvList(L);

	list_Enter(lst);

	for (int n = 0; n < lst->len; n++) {
		if (((LuaServerThread *)lst->data[n])) {
			((LuaServerThread *)lst->data[n])->IsAlive = false;
		}
	}

	list_Leave(lst);

	while (lst->len > 0) { Sleep(1); }
	luaL_unref(L, LUA_REGISTRYINDEX, SRV_LIST_INDEX);
	SRV_LIST_INDEX = LUA_REFNIL;
	list_Destroy(lst);

	return 0;
}

void Disconnect(LuaServerThread * self, SOCKET client) {

	EnterCriticalSection(&self->CriticalSection);
	LuaServerClient * srvclient;
	srvclient = NULL;
	int index = 0;

	for (int i = 0; i < self->NumbClients; i++) {
		if (self->Clients[i].Socket == client) {
			index = i;
			srvclient = &self->Clients[i];
			queue_Enqueue(self->Events, NetEvent_Create(client, NETEVENT_DISCONNECTED, srvclient->Address, strlen(srvclient->Address)));
			break;
		}
	}

	if (srvclient) {
		memmove(&self->Clients[index], &self->Clients[index + 1], (self->NumbClients - index) * sizeof(LuaServerClient));
		self->NumbClients--;
	}

	LeaveCriticalSection(&self->CriticalSection);
}

DWORD WINAPI SrvProc(LPVOID lpParam) {

	LuaServerThread * self = (LuaServerThread *)lpParam;

	void * temp;
	SOCKET client;
	Server * srv = CreateServer(self->Port);
	LuaServerClient * srvclient;
	char buffer[SRV_BUFFERSIZE];
	int disc;
	size_t read;
	size_t total;
	int index;
	NetEvent * ev;

	if (srv) {

		while (self->IsAlive) {

			ev = (NetEvent *)queue_Dequeue(self->Send);
			while (ev) {

				if (ev->type == NETEVENT_RECEIVE || ev->type == NETEVENT_SEND) {
					disc = 0;
					total = 0;

					do {

						read = ServerSend(srv, ev->s, &ev->data[total], ev->len - total, &disc);
						if (disc) {
							Disconnect(self, ev->s);
							break;
						}
						else {
							queue_Enqueue(self->Events, NetEvent_Create(ev->s, NETEVENT_SEND, &ev->data[total], ev->len - total));
							total += read;
						}

					} while (total < ev->len);
				}
				else if (ev->type == NETEVENT_DISCONNECTED) {
					ServerDisconnect(srv, ev->s);
					Disconnect(self, ev->s);
				}

				free(ev);
				ev = (NetEvent *)queue_Dequeue(self->Send);
			}

			client = ServerAccept(srv);

			if (client != INVALID_SOCKET) {

				EnterCriticalSection(&self->CriticalSection);

				temp = realloc(self->Clients, sizeof(LuaServerClient) * (self->NumbClients + 1));

				if (temp) {

					self->Clients = (LuaServerClient*)temp;

					srvclient = &self->Clients[self->NumbClients];

					memset(srvclient, 0, sizeof(LuaServerClient));

					srvclient->Socket = client;

					GetIP(client, srvclient->Address, MAX_ADDRESS_LEN - 1);

					self->NumbClients++;

					queue_Enqueue(self->Events, NetEvent_Create(client, NETEVENT_CONNECTED, srvclient->Address, strlen(srvclient->Address)));
				}
				else {
					ServerDisconnect(srv, client);
				}

				LeaveCriticalSection(&self->CriticalSection);
			}

			list_Enter(srv->Clients);
			for (int n = 0; n < srv->Clients->len; n++) {

				client = *(SOCKET*)srv->Clients->data[n];
				disc = 0;
				read = ServerReceive(srv, client, buffer, SRV_BUFFERSIZE, &disc);
				if (read > 0) {
					queue_Enqueue(self->Events, NetEvent_Create(client, NETEVENT_RECEIVE, buffer, read));
				}
				else if (disc) {
					Disconnect(self, client);
				}
			}
			list_Leave(srv->Clients);

			Sleep(10);
		}
	}

	ServerShutdown(srv);

	HANDLE thread = self->Thread;

	list_Remove(self->All, self);

	queue_Destroy(self->Events);
	queue_Destroy(self->Send);

	DeleteCriticalSection(&self->CriticalSection);

	free(self);

	CloseHandle(thread);

	return 0;
}

int luaserver_start(lua_State *L) {

	List * lst = _SrvList(L);

	LuaServerThread * thread = (LuaServerThread*)calloc(1, sizeof(LuaServerThread));

	InitializeCriticalSectionAndSpinCount(&thread->CriticalSection, 0x00000400);

	thread->Events = queue_Create();
	thread->Send = queue_Create();
	thread->Port = luaL_checkinteger(L, 1);

	lua_pop(L, lua_gettop(L));

	thread->All = lst;
	thread->IsAlive = true;
	thread->Thread = CreateThread(NULL, 0, SrvProc, thread, 0, &thread->ThreadId);
	if (!thread->Thread) {


		DeleteCriticalSection(&thread->CriticalSection);
		queue_Destroy(thread->Events);
		queue_Destroy(thread->Send);
		free(thread);

		lua_pop(L, lua_gettop(L));
		lua_pushnil(L);
		lua_pushstring(L, "Unable to start server thread");

		return 2;
	}

	LuaServer * srv = lua_pushluaserver(L);

	srv->thread = thread;

	list_Add(lst, thread);

	if (SRV_FUNC_INDEX != LUA_REFNIL) {

		lua_rawgeti(L, LUA_REGISTRYINDEX, SRV_FUNC_INDEX);
		lua_pushvalue(L, -2);

		if (lua_pcall(L, 1, 0, NULL) != 0) {
			thread->IsAlive = false;

			lua_pushnil(L);
			lua_pushvalue(L, -2);

			return 2;
		}
	}

	return 1;
}

int luaserver_getclients(lua_State *L) {

	LuaServer * server = lua_toluaserver(L, 1);
	LuaServerThread * thread;
	if (!server || !server->thread) {

		lua_pop(L, lua_gettop(L));
		lua_pushnil(L);
		return 1;
	}

	List * lst = _SrvList(L);
	list_Enter(lst);

	for (int n = 0; n < lst->len; n++) {
		if (((LuaServerThread *)lst->data[n]) == server->thread) {
			thread = (LuaServerThread *)lst->data[n];
			break;
		}
	}

	if (!thread) {

		server->thread = NULL;
		list_Leave(lst);

		lua_pop(L, lua_gettop(L));
		lua_pushnil(L);
		return 1;
	}
	else
		lua_pop(L, lua_gettop(L));

	EnterCriticalSection(&thread->CriticalSection);

	lua_createtable(L, 0, thread->NumbClients);
	for (int n = 0; n < thread->NumbClients; n++) {

		lua_pushinteger(L, thread->Clients[n].Socket);
		lua_pushstring(L, thread->Clients[n].Address);	
		lua_settable(L, -3);
	}

	LeaveCriticalSection(&thread->CriticalSection);

	list_Leave(lst);

	return 1;
}

int luaserver_getevent(lua_State *L) {

	LuaServer * server = lua_toluaserver(L, 1);
	LuaServerThread * thread;
	if (!server || !server->thread) {

		lua_pop(L, lua_gettop(L));
		lua_pushnil(L);
		return 1;
	}

	List * lst = _SrvList(L);
	list_Enter(lst);

	for (int n = 0; n < lst->len; n++) {
		if (((LuaServerThread *)lst->data[n]) == server->thread) {
			thread = (LuaServerThread *)lst->data[n];
			break;
		}
	}

	if (!thread) {

		server->thread = NULL;
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

int luaserver_send(lua_State *L) {

	size_t datalen;
	LuaServer * server = lua_toluaserver(L, 1);
	SOCKET s = luaL_checkinteger(L, 2);
	const char * data = luaL_checklstring(L, 3, &datalen);

	LuaServerThread * thread;
	if (!server || !server->thread || !data || datalen <= 0) {

		lua_pop(L, lua_gettop(L));
		lua_pushboolean(L, false);
		return 1;
	}

	List * lst = _SrvList(L);
	list_Enter(lst);

	for (int n = 0; n < lst->len; n++) {
		if (((LuaServerThread *)lst->data[n]) == server->thread) {
			thread = (LuaServerThread *)lst->data[n];
			break;
		}
	}

	if (!thread) {

		server->thread = NULL;
		list_Leave(lst);

		lua_pop(L, lua_gettop(L));
		lua_pushboolean(L, false);
		return 1;
	}

	queue_Enqueue(thread->Send, NetEvent_Create(s, NETEVENT_SEND, data, datalen));

	list_Leave(lst);
	lua_pop(L, lua_gettop(L));
	lua_pushboolean(L, true);
	return 1;
}


int luaserver_disconnect(lua_State *L) {

	LuaServer * server = lua_toluaserver(L, 1);
	SOCKET s = luaL_checkinteger(L, 2);
	LuaServerThread * thread;
	if (!server || !server->thread) {

		lua_pop(L, lua_gettop(L));
		lua_pushboolean(L, false);
		return 1;
	}

	List * lst = _SrvList(L);
	list_Enter(lst);

	for (int n = 0; n < lst->len; n++) {
		if (((LuaServerThread *)lst->data[n]) == server->thread) {
			thread = (LuaServerThread *)lst->data[n];
			break;
		}
	}

	if (!thread) {

		server->thread = NULL;
		list_Leave(lst);

		lua_pop(L, lua_gettop(L));
		lua_pushboolean(L, false);
		return 1;
	}

	queue_Enqueue(thread->Send, NetEvent_Create(s, NETEVENT_DISCONNECTED, NULL, 0));

	list_Leave(lst);
	lua_pop(L, lua_gettop(L));
	lua_pushboolean(L, true);
	return 1;
}

LuaServer * lua_toluaserver(lua_State *L, int index) {

	LuaServer * proc = (LuaServer*)lua_touserdata(L, index);
	if (proc == NULL)
		luaL_error(L, "parameter is not a %s", LUASERVER);
	return proc;
}

LuaServer * lua_pushluaserver(lua_State *L) {

	LuaServer * proc = (LuaServer*)lua_newuserdata(L, sizeof(LuaServer));
	if (proc == NULL)
		luaL_error(L, "Unable to create LuaServer");
	luaL_getmetatable(L, LUASERVER);
	lua_setmetatable(L, -2);
	memset(proc, 0, sizeof(LuaServer));

	return proc;
}

int luaserver_gc(lua_State *L) {

	LuaServer * srv = lua_toluaserver(L, 1);
	List * lst = _SrvList(L);

	if (srv->thread) {

		list_Enter(lst);

		for (int n = 0; n < lst->len; n++) {
			if (((LuaServerThread *)lst->data[n]) == srv->thread) {

				srv->thread->IsAlive = false;
				break;
			}
		}

		srv->thread = NULL;

		list_Leave(lst);
	}

	lua_pop(L, 1);
	return 0;
}

int luaserver_tostring(lua_State *L) {
	char tim[100];
	sprintf(tim, "LuaServer: 0x%08X", lua_toluaserver(L, 1));
	lua_pushfstring(L, tim);
	return 1;
}