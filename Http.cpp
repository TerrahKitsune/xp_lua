#define WIN32_LEAN_AND_MEAN
#include "Http.h"
#include "networking.h"
#define PACKETSIZE 1048576

static char PACKET[PACKETSIZE];
static char BFILE[_MAX_PATH] = { 0 };
static int Function = -1;
static double PCFreq = 0.0;
static __int64 CounterStart = 0;
static Buffer * mustdieonpanic = NULL;
lua_CFunction panic;

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
	if (data && len > 0){
		if (!BufferAdd(b, data, len))
			goto bad;
	}

	return b;
bad:
	Destroy(b);
	return NULL;
}

static Buffer * GetHeaders(lua_State *L, int idx, size_t contentlen, const char * host){

	Buffer * b = New();
	if (!b)
		return NULL;

	if (lua_type(L, idx) != LUA_TTABLE){
		if (!BufferFormat(b, "Host: %s\r\n", host))
			goto bad;
		if (!BufferFormat(b, "Content-Length: %u\r\n", contentlen))
			goto bad;
	}
	else{

		bool hashost = false;
		bool hasclen = false;
		const char * key;
		lua_pushvalue(L, idx);
		lua_pushnil(L);

		while (lua_next(L, -2) != 0) {

			key = lua_tostring(L, -2);

			if (_stricmp(key, "host") == 0){
				hashost = true;
			}
			else if (_stricmp(key, "content-length") == 0){
				lua_pop(L, 1);
				continue;
			}

			if (!BufferFormat(b, "%s: %s\r\n", key, lua_tostring(L, -1))){
				lua_pop(L, 2);
				goto bad;
			}
			lua_pop(L, 1);
		}
		lua_pop(L, 1);
		if (!BufferFormat(b, "Content-Length: %u\r\n", contentlen))
			goto bad;
		if (!hashost){
			if (!BufferFormat(b, "Host: %s\r\n", host))
				goto bad;
		}
	}

	return b;
bad:
	Destroy(b);
	return NULL;
}

static void ShutdownSSL(SSL *ssl, SSL_CTX * ctx)
{
	if (ctx)
		SSL_CTX_free(ctx);

	if (ssl){
		SSL_shutdown(ssl);
		SSL_free(ssl);
	}
}

static bool IsBlocking(SSL* ssl, int ret){

	if (ssl){
		return SSL_get_error(ssl, ret) == SSL_ERROR_WANT_READ;
	}

	return ret == -1 && WSAGetLastError() == WSAEWOULDBLOCK;
}

static int Panic_PushHttpResultToLua(lua_State *L){
	Destroy(mustdieonpanic);
	mustdieonpanic = NULL;
	return panic(L);
}

static int PushHttpResultToLua(lua_State *L, Buffer * b){

	mustdieonpanic = b;
	panic = lua_atpanic(L, Panic_PushHttpResultToLua);
	char * endofheader = NULL;
	if (b->file){
		rewind(b->file);
		fread(PACKET, 1, PACKETSIZE, b->file);
		endofheader = strstr(b->data, "\r\n\r\n");
	}
	else{
		endofheader = strstr(b->data, "\r\n\r\n");
	}

	lua_pop(L, lua_gettop(L));
	if (!b || b->length < 15)
		goto bad;

	if (_strnicmp(b->data, "http/", 5) != 0)
		goto bad;

	char * version = &b->data[5];
	if (_strnicmp(version, "1.0", 3) != 0 && _strnicmp(version, "1.1", 3) != 0)
		goto bad;

	if (!endofheader)
		goto bad;

	memset(&endofheader[2], 0, 2);
	endofheader += 4;

	char * cursor = strstr(b->data, " ");
	if (!cursor)
		goto bad;
	cursor++;

	lua_pushinteger(L, atoi(cursor));
	cursor = strstr(cursor, " ");
	if (!cursor)
		goto bad;
	cursor++;

	char * end = strstr(cursor, "\r\n");
	if (!end)
		goto bad;

	lua_pushlstring(L, cursor, end - cursor);
	
	int data;
	if (b->file){
		data = endofheader - b->data;
		rewind(b->file);
		fseek(b->file, data, SEEK_CUR);
		
		Buffer * real = New(BFILE);
		if (!real || !real->file){
			Destroy(real);
			goto bad;
		}
		size_t read;
		while ((read=fread(PACKET, 1, PACKETSIZE, b->file))>0){
			BufferAdd(real, PACKET, read);
		}
		Destroy(real);
		lua_pushnil(L);
	}
	else{
		data = b->length - (endofheader - b->data);
		data = max(data, 0);
		lua_pushlstring(L, endofheader, data);
	}

	char * key;
	cursor = end + 2;
	lua_newtable(L);
	while (cursor){
		end = strstr(cursor, "\r\n");
		if (!end)
			break;
		key = strstr(cursor, ": ");
		if (!key)
			break;

		lua_pushlstring(L, cursor, key - cursor);
		key += 2;
		lua_pushlstring(L, key, end - key);
		lua_settable(L, -3);
		cursor = end + 2;
	}

	if (b->file)
	{
		fclose(b->file);
		b->file = NULL;
		sprintf(PACKET, "%s.temp", BFILE);
		remove(PACKET);
	}

	Destroy(b);
	mustdieonpanic = NULL;
	lua_atpanic(L, panic);
	return 4;

bad:

	if (b->file)
	{
		fclose(b->file);
		b->file = NULL;
		sprintf(PACKET, "%s.temp", BFILE);
		remove(PACKET);
	}

	lua_pushnil(L);
	lua_pushstring(L, "http response malformed");
	Destroy(b);
	mustdieonpanic = NULL;
	lua_atpanic(L, panic);
	return 2;
}

static int GetContentLength(const char * header){
	char * end;
	char * contentlen;
	char * head = (char*)malloc(strlen(header) + 1);
	int ret = -1;
	if (!head)
		return -1;

	strcpy(head, header);
	int n = 0;
	while (head[n]){
		head[n] = tolower(head[n]);
		n++;
	}

	contentlen = strstr(head, "content-length: ");
	if (contentlen){
		contentlen = contentlen + 16;
		end = strstr(contentlen, "\r\n");
		if (end){
			end[0] = '\0';
			ret = atoi(contentlen);
		}
	}

	free(head);
	return ret;
}

static bool CheckTimeout(lua_State *L, size_t total, size_t processed, bool recv){

	double ms = GetCounter();

	if (Function == -1){
		return ms <= 10000.0;
	}

	lua_rawgeti(L, LUA_REGISTRYINDEX, Function);
	lua_pushnumber(L, ms);
	lua_pushinteger(L, total);
	lua_pushinteger(L, processed);
	lua_pushboolean(L, recv);
	if (lua_pcall(L, 4, 1, NULL)){
		return false;
	}
	else{
		int result = lua_toboolean(L, -1);
		lua_pop(L, 1);
		return result;
	}

	return true;
}

static Buffer * SendRecv(lua_State *L, Buffer * request, SOCKET ConnectSocket, SSL* ssl){

	int result;
	int tosend = -1;
	int contentlen = 0;
	u_long flag = 1;
	ioctlsocket(ConnectSocket, FIONBIO, &flag);
	char * endofheader = NULL;
	size_t total = 0;

	StartCounter();

	do{

		if (!CheckTimeout(L, request->length, total, false)){
			return NULL;
		}

		result = ssl == NULL ? send(ConnectSocket, &request->data[total], request->length - total, 0) : SSL_write(ssl, &request->data[total], request->length - total);

		if (result <= 0){

			if (IsBlocking(ssl, result))
			{
				Sleep(1);
				continue;
			}
			else
				break;
		}
		else{
			total += result;
		}

	} while (total < request->length);

	if (BFILE[0] != '\0')
		sprintf(PACKET, "%s.temp", BFILE);
	else
		PACKET[0] = '\0';
	Buffer * b = New(PACKET);
	if (!b)
		return NULL;

	do{

		if (!CheckTimeout(L, max(tosend, 0), max(b->length, b->filelength), true)){
			Destroy(b);
			return NULL;
		}

		result = ssl == NULL ? recv(ConnectSocket, PACKET, PACKETSIZE, 0) : SSL_read(ssl, PACKET, PACKETSIZE);

		if (result <= 0){

			if (IsBlocking(ssl, result))
			{
				Sleep(1);
				continue;
			}
			else
				return b;
		}
		else{
			if (!BufferAdd(b, PACKET, result)){
				Destroy(b);
				return NULL;
			}
			if (!endofheader){
				endofheader = strstr(c_str(b), "\r\n\r\n");
				if (endofheader){
					tosend = GetContentLength(c_str(b));
					if (tosend != -1 && !PreAlloc(b, tosend + (endofheader - c_str(b)) + 2))
					{
						Destroy(b);
						return NULL;
					}
				}
			}
		}

	} while (tosend == -1 || max(b->length,b->filelength) < tosend);

	return b;
}

static int HTTPS(lua_State *L, SOCKET ConnectSocket, Buffer * b) {

	SSL_CTX* ctx;
	SSL* ssl;
	X509* server_cert;
	int result;
	int total = 0;

	ctx = SSL_CTX_new(TLS_client_method());
	if (ctx == NULL){
		closesocket(ConnectSocket);
		lua_pop(L, lua_gettop(L));
		lua_pushnil(L);
		lua_pushstring(L, "Unable to open ctx client method");
		return 2;
	}

	ssl = SSL_new(ctx);
	SSL_set_fd(ssl, ConnectSocket);
	result = SSL_connect(ssl);

	if (result != 1){
		closesocket(ConnectSocket);
		ShutdownSSL(ssl, ctx);
		lua_pop(L, lua_gettop(L));
		lua_pushnil(L);
		lua_pushstring(L, "TLS handshake failed");
		return 2;
	}

	Buffer * resp = SendRecv(L, b, ConnectSocket, ssl);

	closesocket(ConnectSocket);

	ShutdownSSL(ssl, ctx);

	if (!resp){
		lua_pop(L, lua_gettop(L));
		lua_pushnil(L);
		lua_pushstring(L, "Unable to allocate memory for response");
		return 2;
	}

	return PushHttpResultToLua(L, resp);
}

static int GetUrls(const char * url, char * ip, char * page, char * proto){

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

static SOCKET Connect(const char * ip, int port){

	int iResult;
	SOCKET s;
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
		ConnectSocket = socket(ptr->ai_family, ptr->ai_socktype,
			ptr->ai_protocol);
		if (ConnectSocket == INVALID_SOCKET) {
			
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

int SetTimeoutFunction(lua_State *L){

	if (mustdieonpanic){
		Destroy(mustdieonpanic);
		mustdieonpanic = NULL;
	}

	if (Function != -1)
		luaL_unref(L, LUA_REGISTRYINDEX, Function);

	if (lua_isfunction(L, 1))
		Function = luaL_ref(L, LUA_REGISTRYINDEX);
	else
		Function = -1;

	return 0;
}

int SetFile(lua_State *L){
	size_t len;
	const char * file = luaL_checklstring(L, 1, &len);
	if (len + 5 >= _MAX_PATH)
		luaL_error(L, "File path too long");
	else if (file == NULL || file[0] == '\0'){
		BFILE[0] = '\0';
	}
	else{
		memcpy(BFILE, file, len);
		BFILE[len] = '\0';
	}
	lua_pop(L, 1);
	return 0;
}

int HTTP(lua_State *L)
{
	if (mustdieonpanic){
		Destroy(mustdieonpanic);
		mustdieonpanic = NULL;
	}

	int iResult;
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

	SOCKET ConnectSocket = Connect(ip, port);
	if (ConnectSocket == INVALID_SOCKET){
		lua_pop(L, lua_gettop(L));
		lua_pushnil(L);
		lua_pushstring(L, "Unable to connect to server");
		return 2;
	}

	size_t len;
	const char * content = lua_tolstring(L, 3, &len);

	Buffer * h = GetHeaders(L, 4, len, ip);
	if (!h){
		lua_pop(L, lua_gettop(L));
		lua_pushnil(L);
		lua_pushstring(L, "Unable to allocate buffer for headers");
		iResult = 2;
		goto end;
	}

	Buffer * b = CreateRequest(method, page, h, content, len);
	if (!b){
		lua_pop(L, lua_gettop(L));
		lua_pushnil(L);
		lua_pushstring(L, "Unable to allocate buffer for request");
		iResult = 2;
		goto end;
	}

	if (ssl){
		return HTTPS(L, ConnectSocket, b);
	}
	else{
		Buffer * resp = SendRecv(L, b, ConnectSocket, NULL);
		if (!resp){
			lua_pop(L, lua_gettop(L));
			lua_pushnil(L);
			lua_pushstring(L, "Unable to allocate buffer for response");
			iResult = 2;
			goto end;
		}
		else{
			iResult = PushHttpResultToLua(L, resp);
		}
	}

end:
	Destroy(h);
	Destroy(b);
	closesocket(ConnectSocket);

	return iResult;
}