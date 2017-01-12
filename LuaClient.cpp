#include "LuaClient.h"
#include "LuaFrame.h"

static double PCFreq = 0.0;
static __int64 CounterStart = 0;
static double timeout = 1000.0;

static void StartCounter()
{
	LARGE_INTEGER li;
	if (!QueryPerformanceFrequency(&li))
		puts("QueryPerformanceFrequency failed!");

	PCFreq = double(li.QuadPart) / 1000.0;

	QueryPerformanceCounter(&li);
	CounterStart = li.QuadPart;
}
static double GetCounter()
{
	LARGE_INTEGER li;
	QueryPerformanceCounter(&li);
	return double(li.QuadPart - CounterStart) / PCFreq;
}

static int CheckTimeout(lua_State *L){

	if (GetCounter() > timeout)
		return 1;

	return 0;
}

LuaClient * lua_toluaclient(lua_State *L, int index){

	LuaClient * proc = (LuaClient*)lua_touserdata(L, index);
	if (proc == NULL)
		luaL_error(L, "parameter is not a %s", LUACLIENT);
	return proc;
}

LuaClient * lua_pushluaclient(lua_State *L){

	LuaClient * proc = (LuaClient*)lua_newuserdata(L, sizeof(LuaClient));
	if (proc == NULL)
		luaL_error(L, "Unable to create luaclient");
	luaL_getmetatable(L, LUACLIENT);
	lua_setmetatable(L, -2);
	memset(proc, 0, sizeof(LuaClient));
	proc->client = INVALID_SOCKET;
	return proc;
}

int CliConnect(lua_State *L){

	const char * addr = luaL_checkstring(L, 1);
	int port = luaL_checkinteger(L, 2);

	SOCKET ConnectSocket = INVALID_SOCKET;
	struct addrinfo *result = NULL,
		*ptr = NULL,
		hints;

	ZeroMemory(&hints, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;

	char portstr[15];
	sprintf(portstr, "%d", port);

	int iResult = getaddrinfo(addr, portstr, &hints, &result);
	if (iResult != 0) {
		lua_pop(L, lua_gettop(L));
		lua_pushnil(L);
		lua_pushinteger(L, iResult);
		return 1;
	}

	for (ptr = result; ptr != NULL; ptr = ptr->ai_next) {

		// Create a SOCKET for connecting to server
		ConnectSocket = socket(ptr->ai_family, ptr->ai_socktype,
			ptr->ai_protocol);
		if (ConnectSocket == INVALID_SOCKET) {
			lua_pop(L, lua_gettop(L));
			lua_pushnil(L);
			lua_pushinteger(L, WSAGetLastError());
			return 2;
		}

		// Connect to server.
		iResult = connect(ConnectSocket, ptr->ai_addr, (int)ptr->ai_addrlen);
		if (iResult == SOCKET_ERROR) {
			closesocket(ConnectSocket);
			ConnectSocket = INVALID_SOCKET;
			continue;
		}
		break;
	}

	freeaddrinfo(result);

	if (ConnectSocket == INVALID_SOCKET) {
		lua_pop(L, lua_gettop(L));
		lua_pushnil(L);
		lua_pushinteger(L, WSAGetLastError());
		return 2;
	}

	lua_pop(L, lua_gettop(L));
	LuaClient * cli = lua_pushluaclient(L);
	if (!cli){
		lua_pop(L, lua_gettop(L));
		lua_pushnil(L);
		lua_pushinteger(L, 0);
		return 2;
	}
	else{
		u_long flag = 1;
		ioctlsocket(ConnectSocket, FIONBIO, &flag);
	}

	cli->client = ConnectSocket;
	return 1;
}

int CliRecv(lua_State *L){

	LuaClient * cli = lua_toluaclient(L, 1);
	int read;

	if (cli->client == INVALID_SOCKET){
		lua_pop(L, lua_gettop(L));
		lua_pushnil(L);
		lua_pushinteger(L, 1);
	}

	StartCounter();
	LuaFrame Frame;
	int result = recv(cli->client, (char*)&Frame, sizeof(LuaFrame), 0);
	if (result == 0){
	disconnect:
		shutdown(cli->client, SD_SEND);
		closesocket(cli->client);
		cli->client = INVALID_SOCKET;
		lua_pop(L, lua_gettop(L));
		lua_pushnil(L);
		lua_pushinteger(L, 1);
		return 2;
	}
	else if (result > 0){

		if (result < sizeof(LuaFrame) || Frame.length < sizeof(LuaFrame)){
			lua_pop(L, lua_gettop(L));
			lua_pushnil(L);
			lua_pushinteger(L, WSAEWOULDBLOCK);
			return 2;
		}

		char * data = (char*)malloc(Frame.length);
		if (!data){
			goto error;
		}

		memcpy(data, &Frame, result);
		read = result;
		while (read < Frame.length){
			result = recv(cli->client, &data[read], Frame.length - read, 0);
			if (result == 0){
				free(data);
				goto disconnect;
			}
			else if (result > 0){
				read += result;
			}
			else if (WSAGetLastError() == WSAEWOULDBLOCK){

				result = CheckTimeout(L);
				if (result == 0){
					Sleep(1);
					continue;
				}

				free(data);
				if (result != 1){
					shutdown(cli->client, SD_SEND);
					closesocket(cli->client);
					cli->client = INVALID_SOCKET;
				}
				lua_pop(L, lua_gettop(L));
				lua_pushnil(L);
				lua_pushinteger(L, result);
				return 2;
			}
			else{
				free(data);
				goto error;
			}
		}

		lua_pop(L, lua_gettop(L));
		for (int n = 0; n < 16; n++){
			if (n == 15 || ((LuaFrame*)data)->method[n] == '\0'){
				lua_pushlstring(L, ((LuaFrame*)data)->method, n);
				break;
			}
		}
		lua_pushframe(L, (LuaFrame*)data);
		free(data);
	}
	else{
	error:
		lua_pop(L, lua_gettop(L));
		lua_pushnil(L);
		lua_pushinteger(L, WSAGetLastError());
		if (WSAGetLastError() != WSAEWOULDBLOCK)
		{
			shutdown(cli->client, SD_SEND);
			closesocket(cli->client);
			cli->client = INVALID_SOCKET;
		}
		return 2;
	}

	return 2;
}

int CliSend(lua_State *L){

	LuaClient * cli = lua_toluaclient(L, 1);

	if (cli->client == INVALID_SOCKET){
		lua_pop(L, lua_gettop(L));
		lua_pushnil(L);
		lua_pushinteger(L, 1);
		return 2;
	}

	LuaFrame * frame = lua_toframe(L, 3, lua_tostring(L, 2));
	if (!frame){
		lua_pop(L, lua_gettop(L));
		lua_pushboolean(L, false);
		lua_pushinteger(L, 0);
		return 2;
	}
	int sent = 0;
	char* buffer = (char*)frame;

	int result;
	StartCounter();
	while (sent < frame->length){
		result = send(cli->client, &buffer[sent], frame->length - sent, 0);
		if (result == 0){
			free(frame);
			shutdown(cli->client, SD_SEND);
			closesocket(cli->client);
			cli->client = INVALID_SOCKET;
			lua_pop(L, lua_gettop(L));
			lua_pushboolean(L, false);
			lua_pushinteger(L, 1);
			return 2;
		}
		else if (result > 0){
			sent += result;
		}
		else if (WSAGetLastError() == WSAEWOULDBLOCK){

			result = CheckTimeout(L);
			if (result == 0){
				Sleep(1);
				continue;
			}

			free(frame);
			lua_pop(L, lua_gettop(L));
			lua_pushboolean(L, false);
			lua_pushinteger(L, result);
			if (result != 1){
				shutdown(cli->client, SD_SEND);
				closesocket(cli->client);
				cli->client = INVALID_SOCKET;
			}
			return 2;
		}
		else{
			free(frame);
			lua_pop(L, lua_gettop(L));
			lua_pushboolean(L, false);
			lua_pushinteger(L, WSAGetLastError());
			shutdown(cli->client, SD_SEND);
			closesocket(cli->client);
			cli->client = INVALID_SOCKET;
			return 2;
		}
	}

	free(frame);
	lua_pop(L, lua_gettop(L));
	lua_pushboolean(L, true);

	return 1;
}

int luaclient_gc(lua_State *L){

	LuaClient * cli = lua_toluaclient(L, 1);

	if (cli->client != INVALID_SOCKET){
		shutdown(cli->client, SD_SEND);
		closesocket(cli->client);
		cli->client = INVALID_SOCKET;
	}

	return 0;
}

int luaclient_tostring(lua_State *L){
	char tim[100];
	sprintf(tim, "LuaClient: 0x%08X", lua_toluaclient(L, 1));
	lua_pushfstring(L, tim);
	return 1;
}