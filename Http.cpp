#include "http.h"
#include <string.h>
#define WIN32_LEAN_AND_MEAN
#include "Http.h"
#include "networking.h"
#define PACKETSIZE 1500
static HttpResult * mustdieonpanic = NULL;
lua_CFunction panic;

char PACKET[PACKETSIZE];

char * FindInStringNoCase(char * data, size_t dataLen, const char * substr, size_t substrLen) {

	if (dataLen <= 0 || substrLen <= 0 || !data || !substr) {
		return NULL;
	}

	size_t found = 0;
	char * ptr = NULL;
	char a, b;

	for (size_t i = 0; i < dataLen; i++)
	{
		if (isalpha(data[i])) {
			a = tolower(data[i]);
		}
		else {
			a = data[i];
		}

		if (isalpha(substr[found])) {
			b = tolower(substr[found]);
		}
		else {
			b = substr[found];
		}

		if (a == b) {

			if (!ptr) {
				ptr = &data[i];
			}

			if (++found == substrLen) {
				return ptr;
			}
		}
		else if (ptr || found > 0) {
			found = 0;
			ptr = NULL;
		}
	}

	return NULL;
}

void StartCounter(LuaHttp* luahttp)
{
	LARGE_INTEGER li;
	if (!QueryPerformanceFrequency(&li))
		puts("QueryPerformanceFrequency failed!");

	luahttp->PCFreq = double(li.QuadPart) / 1000.0;

	QueryPerformanceCounter(&li);
	luahttp->CounterStart = li.QuadPart;
}
double GetCounter(LuaHttp* luahttp)
{
	LARGE_INTEGER li;
	QueryPerformanceCounter(&li);
	return double(li.QuadPart - luahttp->CounterStart) / luahttp->PCFreq;
}

void Destroy(HttpResult * result) {
	if (result) {

		if (result->result) {
			Destroy(result->result);
			result->result = NULL;
		}
		if (result->error) {
			gff_free(result->error);
			result->error = NULL;
		}

		gff_free(result);
	}
}

HttpResult * CreateResult(const char * error, Buffer * b = NULL) {

	HttpResult * result = (HttpResult *)gff_calloc(1, sizeof(HttpResult));

	if (error) {
		result->error = (char*)gff_calloc(strlen(error) + 1, sizeof(char));
		strcpy(result->error, error);
	}

	result->result = b;

	return result;
}

int GetUrls(const char * url, char * ip, char * page, char * proto) {

	int port = -1;
	int iResult;
	proto[0] = '\0';
	page[0] = '\0';
	ip[0] = '\0';
	iResult = sscanf(url, "%99[^:]://%99[^:]:%99d/%1023[^\n]", proto, ip, &port, page);
	if (iResult != 4) {
		iResult = sscanf(url, "%99[^:]://%99[^/]/%1023[^\n]", proto, ip, page);
		if (iResult != 3) {
			iResult = sscanf(url, "%99[^:]://%99[^:]:%i[^\n]", proto, ip, &port);
		}
	}

	if (ip[strlen(ip) - 1] == '/')
		ip[strlen(ip) - 1] = '\0';

	return port;
}

Buffer * GetHeaders(lua_State *L, int idx, size_t contentlen, const char * host) {

	Buffer * b = New();
	if (!b)
		return NULL;

	if (lua_type(L, idx) != LUA_TTABLE) {
		if (!BufferFormat(b, "Host: %s\r\n", host))
			goto bad;
		if (!BufferFormat(b, "Content-Length: %u\r\n", contentlen))
			goto bad;
	}
	else {

		bool hashost = false;
		bool hasclen = false;
		const char * key;
		lua_pushvalue(L, idx);
		lua_pushnil(L);

		while (lua_next(L, -2) != 0) {

			key = lua_tostring(L, -2);

			if (_stricmp(key, "host") == 0) {
				hashost = true;
			}
			else if (_stricmp(key, "content-length") == 0) {
				lua_pop(L, 1);
				continue;
			}

			if (!BufferFormat(b, "%s: %s\r\n", key, lua_tostring(L, -1))) {
				lua_pop(L, 2);
				goto bad;
			}
			lua_pop(L, 1);
		}
		lua_pop(L, 1);
		if (!BufferFormat(b, "Content-Length: %u\r\n", contentlen))
			goto bad;
		if (!hashost) {
			if (!BufferFormat(b, "Host: %s\r\n", host))
				goto bad;
		}
	}

	return b;
bad:
	Destroy(b);
	return NULL;
}

Buffer * CreateRequest(const char * method, const char * page, Buffer * headers, const void * data, size_t len)
{
	Buffer * b = New();
	if (!b)
		goto bad;

	if (!BufferFormat(b, "%s /%s HTTP/1.0\r\n", method, page))
		goto bad;
	if (!BufferAdd(b, headers))
		goto bad;
	if (!BufferAdd(b, "\r\n", 2))
		goto bad;
	if (data && len > 0) {
		if (!BufferAdd(b, data, len))
			goto bad;
	}

	return b;
bad:
	Destroy(b);
	return NULL;
}

SOCKET Connect(const char * ip, int port) {

	int iResult;
	struct addrinfo *result = NULL,
		*ptr = NULL,
		hints;
	SOCKET ConnectSocket = INVALID_SOCKET;

	ZeroMemory(&hints, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;

	char str[15];
	sprintf(str, "%d", port);

	iResult = getaddrinfo(ip, str, &hints, &result);
	if (iResult != 0) {

		return INVALID_SOCKET;
	}

	for (ptr = result; ptr != NULL; ptr = ptr->ai_next) {

		// Create a SOCKET for connecting to server
		ConnectSocket = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol);

		if (ConnectSocket == INVALID_SOCKET) {

			freeaddrinfo(result);
			return INVALID_SOCKET;
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

		return INVALID_SOCKET;
	}

	return ConnectSocket;
}

bool CheckTimeout(LuaHttp* luahttp, size_t total, size_t processed, bool recv) {

	if (recv) {
		luahttp->recv = processed;
	}
	else {
		luahttp->sent = processed;
	}

	if (!luahttp->alive) {
		return true;
	}
	else if (luahttp->Timeout <= 0) {
		return false;
	}

	double ms = GetCounter(luahttp);

	return ms < luahttp->Timeout;
}

bool IsBlocking(SSL* ssl, int ret) {

	if (ssl) {
		return SSL_get_error(ssl, ret) == SSL_ERROR_WANT_READ;
	}

	return ret == -1 && WSAGetLastError() == WSAEWOULDBLOCK;
}

int GetContentLength(char * header, size_t totalsize) {
	
	const char * end;
	char * contentlen;
	int ret = -1;

	contentlen = FindInStringNoCase(header, totalsize, "content-length: ", 16);
	if (contentlen) {

		contentlen = contentlen + 16;

		end = FindInStringNoCase(contentlen, totalsize - (contentlen - header), "\r\n", 2);
		if (end) {
			ret = atoi(contentlen);
		}
	}

	return ret;
}


HttpResult * SendRecv(LuaHttp* luahttp, SOCKET ConnectSocket, SSL* ssl) {

	int result;
	int tosend = -1;
	int contentlen = 0;
	u_long flag = 1;
	ioctlsocket(ConnectSocket, FIONBIO, &flag);
	const char * endofheader = NULL;
	size_t total = 0;

	StartCounter(luahttp);

	do {

		if (!CheckTimeout(luahttp, luahttp->request->length, total, false)) {
			return CreateResult("Request timed out");
		}

		result = ssl == NULL ? send(ConnectSocket, &luahttp->request->data[total], luahttp->request->length - total, 0) : SSL_write(ssl, &luahttp->request->data[total], luahttp->request->length - total);

		if (result <= 0) {

			if (IsBlocking(ssl, result))
			{
				Sleep(1);
				continue;
			}
			else
				break;
		}
		else {
			total += result;
		}

	} while (total < luahttp->request->length);

	Buffer * b = New(luahttp->packet);
	if (!b)
		return NULL;

	do {

		if (!CheckTimeout(luahttp, max(tosend, 0), max(b->length, b->filelength), true)) {
			Destroy(b);
			return CreateResult("Request timed out");
		}

		result = ssl == NULL ? recv(ConnectSocket, luahttp->packet, PACKETSIZE, 0) : SSL_read(ssl, luahttp->packet, PACKETSIZE);

		if (result <= 0) {

			if (IsBlocking(ssl, result))
			{
				Sleep(1);
				continue;
			}
			else
				return CreateResult(NULL, b);
		}
		else {
			if (!BufferAdd(b, luahttp->packet, result)) {
				Destroy(b);
				return CreateResult("Failed to allocate memory for message buffer");
			}
			if (!endofheader) {
				endofheader = FindInStringNoCase(c_str(b), b->length, "\r\n\r\n", 4);
				if (endofheader) {
					tosend = GetContentLength(c_str(b), b->length);
					if (tosend != -1 && !PreAlloc(b, tosend + (endofheader - c_str(b)) + 2))
					{
						Destroy(b);
						return CreateResult("Failed to allocate memory for message buffer");
					}
				}
			}
		}

	} while (tosend == -1 || (int)max(b->length, b->filelength) < tosend);

	return CreateResult(NULL, b);
}

void ShutdownSSL(SSL *ssl, SSL_CTX * ctx)
{
	if (ctx)
		SSL_CTX_free(ctx);

	if (ssl) {
		SSL_shutdown(ssl);
		SSL_free(ssl);
	}
}


HttpResult * DoHttp(LuaHttp* luahttp, SOCKET ConnectSocket) {

	HttpResult * result = SendRecv(luahttp, ConnectSocket, NULL);
	closesocket(ConnectSocket);

	if (!result) {
		return CreateResult("Unable to allocate buffer for response");
	}

	return result;
}

HttpResult * DoHttps(LuaHttp* luahttp, SOCKET ConnectSocket) {

	SSL_CTX* ctx;
	SSL* ssl;
	int result;
	int total = 0;

	ctx = SSL_CTX_new(TLS_client_method());
	if (ctx == NULL) {
		closesocket(ConnectSocket);
		return CreateResult("Unable to open ctx client method");
	}

	ssl = SSL_new(ctx);
	SSL_set_fd(ssl, ConnectSocket);
	result = SSL_connect(ssl);

	if (result != 1) {
		closesocket(ConnectSocket);
		ShutdownSSL(ssl, ctx);

		return CreateResult("TLS handshake failed");
	}

	HttpResult * resp = SendRecv(luahttp, ConnectSocket, ssl);

	closesocket(ConnectSocket);

	ShutdownSSL(ssl, ctx);

	if (!resp) {
		return CreateResult("Unable to allocate memory for response");
	}

	return resp;
}

void TaskProcess(LuaHttp* luahttp) {

	if (!luahttp->packet) {
		luahttp->result = CreateResult("Unable to allocate message buffer");
		return;
	}

	SOCKET ConnectSocket = Connect(luahttp->ip, luahttp->port);

	if (ConnectSocket == INVALID_SOCKET) {
		luahttp->result = CreateResult("Unable to connect to server");
		return;
	}

	luahttp->result = luahttp->ssl ? DoHttps(luahttp, ConnectSocket) : DoHttp(luahttp, ConnectSocket);
}

int Start(lua_State *L) {

	char ip[100];
	int port = -1;
	char page[1024];
	char proto[100];
	const char * method = luaL_checkstring(L, 1);

	port = GetUrls(luaL_checkstring(L, 2), ip, page, proto);

	bool ssl = _stricmp(proto, "https") == 0;

	if (port <= 0) {
		if (ssl)
			port = 443;
		else
			port = 80;
	}

	size_t len;
	const char * content = lua_tolstring(L, 3, &len);

	Buffer * h = GetHeaders(L, 4, len, ip);

	if (!h) {
		lua_pop(L, lua_gettop(L));
		lua_pushnil(L);
		lua_pushstring(L, "Unable to allocate buffer for headers");
		return 2;
	}

	Buffer * b = CreateRequest(method, page, h, content, len);

	Destroy(h);

	lua_pop(L, lua_gettop(L));

	if (!b) {
		lua_pushnil(L);
		lua_pushstring(L, "Unable to allocate buffer for request");
		return 2;
	}

	LuaHttp* luahttp = lua_pushhttp(L);

	luahttp->ip = (char*)gff_calloc(strlen(ip) + 1, sizeof(char));
	strcpy(luahttp->ip, ip);
	luahttp->port = port;
	luahttp->request = b;
	luahttp->alive = true;
	luahttp->ssl = ssl;

	luahttp->tHandle = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)TaskProcess, luahttp, 0, NULL);

	return 1;
}

static int Panic_PushHttpResultToLua(lua_State *L) {
	Destroy(mustdieonpanic);
	mustdieonpanic = NULL;
	lua_atpanic(L, panic);
	return panic(L);
}

static int PushHttpResultToLua(lua_State *L, HttpResult * b, const char * file) {

	mustdieonpanic = b;
	panic = lua_atpanic(L, Panic_PushHttpResultToLua);
	char * endofheader = NULL;
	if (b->result->file) {
		rewind(b->result->file);
		int read = fread(PACKET, 1, PACKETSIZE, b->result->file);
		endofheader = FindInStringNoCase(PACKET, read, "\r\n\r\n", 4);
	}
	else {
		endofheader = FindInStringNoCase(b->result->data, b->result->length, "\r\n\r\n", 4);
	}

	lua_pop(L, lua_gettop(L));
	if (!b || b->result->length < 15)
		goto bad;

	if (_strnicmp(b->result->data, "http/", 5) != 0)
		goto bad;

	char * version = &b->result->data[5];
	if (_strnicmp(version, "1.0", 3) != 0 && _strnicmp(version, "1.1", 3) != 0)
		goto bad;

	if (!endofheader)
		goto bad;

	memset(&endofheader[2], 0, 2);
	endofheader += 4;

	char * cursor = FindInStringNoCase(b->result->data, b->result->length, " ", 1);
	if (!cursor)
		goto bad;
	cursor++;

	lua_pushinteger(L, atoi(cursor));
	cursor = FindInStringNoCase(cursor, b->result->length - (cursor - b->result->data), " ", 1);
	if (!cursor)
		goto bad;
	cursor++;

	char * end = FindInStringNoCase(cursor, b->result->length - (cursor - b->result->data), "\r\n", 2);
	if (!end)
		goto bad;

	lua_pushlstring(L, cursor, end - cursor);

	int data;
	if (b->result->file) {
		data = endofheader - b->result->data;
		rewind(b->result->file);
		fseek(b->result->file, data, SEEK_CUR);

		Buffer * real = New(file);
		if (!real || !real->file) {
			Destroy(real);
			goto bad;
		}
		size_t read;
		while ((read = fread(PACKET, 1, PACKETSIZE, b->result->file))>0) {
			BufferAdd(real, PACKET, read);
		}
		Destroy(real);
		lua_pushnil(L);
	}
	else {
		data = b->result->length - (endofheader - b->result->data);
		data = max(data, 0);
		lua_pushlstring(L, endofheader, data);
	}

	char * key;
	cursor = end + 2;
	lua_newtable(L);
	while (cursor) {
		end = FindInStringNoCase(cursor, b->result->length - (cursor - b->result->data), "\r\n", 2);
		if (!end)
			break;
		key = FindInStringNoCase(cursor, b->result->length - (cursor - b->result->data), ": ", 2);
		if (!key)
			break;

		lua_pushlstring(L, cursor, key - cursor);
		key += 2;
		lua_pushlstring(L, key, end - key);
		lua_settable(L, -3);
		cursor = end + 2;
	}

	if (b->result->file)
	{
		fclose(b->result->file);
		b->result->file = NULL;
		sprintf(PACKET, "%s.temp", file);
		remove(PACKET);
	}

	Destroy(b);
	mustdieonpanic = NULL;
	lua_atpanic(L, panic);
	return 4;

bad:

	if (b->result->file)
	{
		fclose(b->result->file);
		b->result->file = NULL;
		sprintf(PACKET, "%s.temp", file);
		remove(PACKET);
	}

	lua_pushnil(L);
	lua_pushstring(L, "http response malformed");
	Destroy(b);
	mustdieonpanic = NULL;
	lua_atpanic(L, panic);
	return 2;
}

int SetHttpTimeout(lua_State *L) {

	LuaHttp * luahttp = luaL_checkhttp(L, 1);
	luahttp->Timeout = luaL_checknumber(L, 2);
	lua_pop(L, lua_gettop(L));
	return 0;
}

int GetResult(lua_State *L) {

	LuaHttp * luahttp = luaL_checkhttp(L, 1);

	if (!luahttp->alive) {
		lua_pop(L, lua_gettop(L));
		lua_pushnil(L);
		lua_pushstring(L, "Request not alive");
		return 2;
	}

	if (luahttp->tHandle != INVALID_HANDLE_VALUE) {
		WaitForSingleObject(luahttp->tHandle, INFINITE);
	}

	luahttp->alive = false;

	lua_pop(L, lua_gettop(L));

	HttpResult* result = luahttp->result;

	luahttp->result = NULL;

	if (result->error) {
		lua_pop(L, lua_gettop(L));
		lua_pushnil(L);
		lua_pushstring(L, result->error);
		Destroy(result);
		return 2;
	}

	int lreturn = PushHttpResultToLua(L, result, NULL);

	return lreturn;
}

int GetStatus(lua_State *L) {

	LuaHttp * luahttp = luaL_checkhttp(L, 1);
	DWORD code = 0;

	if (luahttp->tHandle != INVALID_HANDLE_VALUE) {
		GetExitCodeThread(luahttp->tHandle, &code);
	}

	bool isrunnning = code == STILL_ACTIVE;
	double elapsed = GetCounter(luahttp);
	size_t recv = luahttp->recv;
	size_t sent = luahttp->sent;

	lua_pop(L, lua_gettop(L));
	lua_pushboolean(L, isrunnning);
	lua_pushnumber(L, elapsed);
	lua_pushinteger(L, sent);
	lua_pushinteger(L, recv);

	return 4;
}

LuaHttp * lua_tohttp(lua_State *L, int index) {

	LuaHttp * luahttp = (LuaHttp*)lua_touserdata(L, index);
	if (luahttp == NULL)
		luaL_error(L, "parameter is not a %s", LUAHTTP);
	return luahttp;
}

LuaHttp * luaL_checkhttp(lua_State *L, int index) {

	LuaHttp * luahttp = (LuaHttp*)luaL_checkudata(L, index, LUAHTTP);
	if (luahttp == NULL)
		luaL_error(L, "parameter is not a %s", LUAHTTP);

	return luahttp;
}

LuaHttp * lua_pushhttp(lua_State *L) {

	LuaHttp * luahttp = (LuaHttp*)lua_newuserdata(L, sizeof(LuaHttp));
	if (luahttp == NULL)
		luaL_error(L, "Unable to create http");

	luaL_getmetatable(L, LUAHTTP);
	lua_setmetatable(L, -2);
	memset(luahttp, 0, sizeof(LuaHttp));

	luahttp->Timeout = 10000.0;
	luahttp->packet = (char*)gff_calloc(PACKETSIZE, sizeof(char));

	return luahttp;
}

int luahttp_gc(lua_State *L) {

	LuaHttp * luahttp = (LuaHttp*)lua_tohttp(L, 1);

	luahttp->alive = false;

	if (luahttp->tHandle != INVALID_HANDLE_VALUE) {

		WaitForSingleObject(luahttp->tHandle, INFINITE);
		CloseHandle(luahttp->tHandle);
		luahttp->tHandle = INVALID_HANDLE_VALUE;
	}

	if (luahttp->result) {

		Destroy(luahttp->result);
		luahttp->result = NULL;
	}

	if (luahttp->request) {

		Destroy(luahttp->request);
		luahttp->request = NULL;
	}

	if (luahttp->ip) {
		gff_free(luahttp->ip);
		luahttp->ip = NULL;
	}

	if (luahttp->packet) {
		gff_free(luahttp->packet);
		luahttp->packet = NULL;
	}

	return 0;
}

int luahttp_tostring(lua_State *L) {

	LuaHttp * sq = lua_tohttp(L, 1);
	char my[500];
	sprintf(my, "Http: 0x%08X", sq);
	lua_pushstring(L, my);
	return 1;
}