#include "stream.h"
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <stdlib.h> 
#include <windows.h> 

const size_t MIN_STREAM_SIZE = 1024;

size_t AllocAddSize(lua_State* L, LuaStream* stream, size_t requestedsize) {

	if (stream && stream->allocfunc != LUA_NOREF) {

		lua_rawgeti(L, LUA_REGISTRYINDEX, stream->allocfunc);
		lua_pushinteger(L, requestedsize);

		if (lua_pcall(L, 1, 1, NULL) != 0) {

			lua_error(L);
			return 0;
		}

		if (lua_type(L, -1) == LUA_TNUMBER) {
			size_t t = (size_t)lua_tointeger(L, -1);
			lua_pop(L, 1);
			return t;
		}
	}

	return MIN_STREAM_SIZE;
}

bool CheckStreamSize(lua_State* L, LuaStream* stream, size_t requestedsize) {

	if (!stream->data) {
		return false;
	}
	else if (stream->pos + requestedsize > stream->len) {
		requestedsize = (stream->pos + requestedsize) - stream->len;
	}

	if (stream->len + requestedsize > stream->alloc) {

		size_t ne = AllocAddSize(L, stream, requestedsize);

		if (ne > requestedsize) {
			requestedsize = ne;
		}

		size_t newsize = (stream->len + requestedsize) - stream->alloc;

		if (ne == 0 && newsize < MIN_STREAM_SIZE) {
			newsize = MIN_STREAM_SIZE;
		}

		void* temp = realloc(stream->data, stream->alloc + newsize);
		if (!temp) {
			return false;
		}
		else {
			stream->data = (BYTE*)temp;
			stream->alloc = stream->alloc + newsize;
			return true;
		}
	}
	else {
		return true;
	}
}

bool StreamWrite(lua_State* L, LuaStream* stream, BYTE* data, size_t len) {

	if (!stream || !stream->data) {
		return false;
	}
	else if (!CheckStreamSize(L, stream, len)) {
		return false;
	}

	memcpy(&stream->data[stream->pos], data, len);

	long mod = (stream->pos + len) - stream->len;

	if (mod > 0) {
		stream->len += mod;
	}

	stream->pos += len;

	return true;
}

int StreamBuffer(lua_State* L) {

	LuaStream* stream = lua_toluastream(L, 1);
	size_t len;
	size_t current = stream->pos;
	const char* data = lua_tolstring(L, 2, &len);

	if (len <= 0 || !data) {
		lua_pop(L, lua_gettop(L));
		lua_pushinteger(L, 0);
		return 1;
	}

	stream->pos = stream->len;

	if (StreamWrite(L, stream, (BYTE*)data, len)) {
		lua_pop(L, lua_gettop(L));
		lua_pushinteger(L, len);
	}
	else {
		lua_pop(L, lua_gettop(L));
		lua_pushinteger(L, 0);
	}

	stream->pos = current;

	return 1;
}

const BYTE* ReadStream(LuaStream* stream, size_t len) {

	if (len <= 0 || !stream || !stream->data || (stream->pos + len) > stream->len) {
		return NULL;
	}

	const BYTE* result = &stream->data[stream->pos];
	stream->pos += len;

	return result;
}

int WriteFloat(lua_State* L) {

	LuaStream* stream = lua_toluastream(L, 1);
	float f = (float)lua_tonumber(L, 2);

	lua_pop(L, lua_gettop(L));

	lua_pushboolean(L, StreamWrite(L, stream, (BYTE*)& f, sizeof(float)));

	return 1;
}

int ReadFloat(lua_State* L) {

	LuaStream* stream = lua_toluastream(L, 1);

	const BYTE* raw = ReadStream(stream, sizeof(float));

	lua_pop(L, lua_gettop(L));

	if (!raw) {
		lua_pushnil(L);
	}
	else {
		float f;
		memcpy(&f, raw, sizeof(float));
		lua_pushnumber(L, f);
	}

	return 1;
}

int WriteDouble(lua_State* L) {

	LuaStream* stream = lua_toluastream(L, 1);
	double f = lua_tonumber(L, 2);

	lua_pop(L, lua_gettop(L));

	lua_pushboolean(L, StreamWrite(L, stream, (BYTE*)& f, sizeof(double)));

	return 1;
}

int ReadDouble(lua_State* L) {

	LuaStream* stream = lua_toluastream(L, 1);

	const BYTE* raw = ReadStream(stream, sizeof(double));

	lua_pop(L, lua_gettop(L));

	if (!raw) {
		lua_pushnil(L);
	}
	else {
		double f;
		memcpy(&f, raw, sizeof(double));
		lua_pushnumber(L, f);
	}

	return 1;
}

int WriteShort(lua_State* L) {

	LuaStream* stream = lua_toluastream(L, 1);
	short n = (short)lua_tointeger(L, 2);

	lua_pop(L, lua_gettop(L));

	lua_pushboolean(L, StreamWrite(L, stream, (BYTE*)& n, sizeof(short)));

	return 1;
}

int ReadShort(lua_State* L) {

	LuaStream* stream = lua_toluastream(L, 1);

	const BYTE* raw = ReadStream(stream, sizeof(short));

	lua_pop(L, lua_gettop(L));

	if (!raw) {
		lua_pushnil(L);
	}
	else {
		short f;
		memcpy(&f, raw, sizeof(short));
		lua_pushinteger(L, f);
	}

	return 1;
}

int WriteUShort(lua_State* L) {

	LuaStream* stream = lua_toluastream(L, 1);
	unsigned short n = (unsigned short)lua_tointeger(L, 2);

	lua_pop(L, lua_gettop(L));

	lua_pushboolean(L, StreamWrite(L, stream, (BYTE*)& n, sizeof(unsigned short)));

	return 1;
}

int ReadUShort(lua_State* L) {

	LuaStream* stream = lua_toluastream(L, 1);

	const BYTE* raw = ReadStream(stream, sizeof(unsigned short));

	lua_pop(L, lua_gettop(L));

	if (!raw) {
		lua_pushnil(L);
	}
	else {
		short f;
		memcpy(&f, raw, sizeof(unsigned short));
		lua_pushinteger(L, f);
	}

	return 1;
}

int WriteInt(lua_State* L) {

	LuaStream* stream = lua_toluastream(L, 1);
	int n = (int)lua_tointeger(L, 2);

	lua_pop(L, lua_gettop(L));

	lua_pushboolean(L, StreamWrite(L, stream, (BYTE*)& n, sizeof(int)));

	return 1;
}

int ReadInt(lua_State* L) {

	LuaStream* stream = lua_toluastream(L, 1);

	const BYTE* raw = ReadStream(stream, sizeof(int));

	lua_pop(L, lua_gettop(L));

	if (!raw) {
		lua_pushnil(L);
	}
	else {
		int f;
		memcpy(&f, raw, sizeof(int));
		lua_pushinteger(L, f);
	}

	return 1;
}

int WriteUInt(lua_State* L) {

	LuaStream* stream = lua_toluastream(L, 1);
	unsigned int n = (unsigned int)lua_tointeger(L, 2);

	lua_pop(L, lua_gettop(L));

	lua_pushboolean(L, StreamWrite(L, stream, (BYTE*)& n, sizeof(unsigned int)));

	return 1;
}

int ReadUInt(lua_State* L) {

	LuaStream* stream = lua_toluastream(L, 1);

	const BYTE* raw = ReadStream(stream, sizeof(unsigned int));

	lua_pop(L, lua_gettop(L));

	if (!raw) {
		lua_pushnil(L);
	}
	else {
		unsigned int f;
		memcpy(&f, raw, sizeof(unsigned int));
		lua_pushinteger(L, f);
	}

	return 1;
}

int ReadLong(lua_State* L) {

	LuaStream* stream = lua_toluastream(L, 1);

	const BYTE* raw = ReadStream(stream, sizeof(long long));

	lua_pop(L, lua_gettop(L));

	if (!raw) {
		lua_pushnil(L);
	}
	else {
		long long f;
		memcpy(&f, raw, sizeof(long long));
		lua_pushinteger(L, f);
	}

	return 1;
}

int WriteLong(lua_State* L) {

	LuaStream* stream = lua_toluastream(L, 1);
	long long n = lua_tointeger(L, 2);

	lua_pop(L, lua_gettop(L));

	lua_pushboolean(L, StreamWrite(L, stream, (BYTE*)& n, sizeof(long long)));

	return 1;
}

int WriteUnsignedLong(lua_State* L) {

	LuaStream* stream = lua_toluastream(L, 1);
	unsigned long long n = lua_tointeger(L, 2);

	lua_pop(L, lua_gettop(L));

	lua_pushboolean(L, StreamWrite(L, stream, (BYTE*)& n, sizeof(unsigned long long)));

	return 1;
}

int ReadUnsignedLong(lua_State* L) {

	LuaStream* stream = lua_toluastream(L, 1);

	const BYTE* raw = ReadStream(stream, sizeof(unsigned long long));

	lua_pop(L, lua_gettop(L));

	if (!raw) {
		lua_pushnil(L);
	}
	else {
		unsigned long long f;
		memcpy(&f, raw, sizeof(unsigned long long));
		lua_pushinteger(L, f);
	}

	return 1;
}

int StreamSetPos(lua_State* L) {

	LuaStream* stream = lua_toluastream(L, 1);
	size_t newpos = (size_t)luaL_optinteger(L, 2, 0);

	if (!stream->data) {
		lua_pop(L, lua_gettop(L));
		return 0;
	}

	if (newpos > stream->len) {
		newpos = stream->len;
	}

	if (newpos < 0) {
		newpos = 0;
	}

	stream->pos = newpos;

	lua_pop(L, lua_gettop(L));
	return 0;
}

int GetStreamInfo(lua_State* L) {

	LuaStream* stream = lua_toluastream(L, 1);

	if (!stream->data) {
		lua_pop(L, lua_gettop(L));
		lua_pushnil(L);
		return 1;
	}

	lua_pop(L, lua_gettop(L));

	lua_pushinteger(L, stream->pos);
	lua_pushinteger(L, stream->len);
	lua_pushinteger(L, stream->alloc);

	return 3;
}

int StreamShrink(lua_State* L) {

	LuaStream* stream = lua_toluastream(L, 1);

	if (stream->pos <= 0) {
		return 0;
	}
	else if (stream->pos >= stream->len) {
		stream->len = 0;
		stream->pos = 0;
	}
	else {
		size_t mod = stream->len - stream->pos;
		memcpy(&stream->data[0], &stream->data[stream->pos], mod);
		stream->len = mod;
		stream->pos = 0;
	}

	return 0;
}

int SetStreamByte(lua_State* L) {

	LuaStream* stream = lua_toluastream(L, 1);
	BYTE data = (BYTE)lua_tointeger(L, 2);
	size_t pos = (size_t)luaL_optinteger(L, 3, stream->pos);

	lua_pop(L, lua_gettop(L));

	if (pos >= stream->len || pos < 0) {
		stream->data[pos] = data;
	}

	return 0;
}

int PeekStreamByte(lua_State* L) {

	LuaStream* stream = lua_toluastream(L, 1);
	size_t pos = (size_t)luaL_optinteger(L, 2, stream->pos);

	lua_pop(L, lua_gettop(L));

	if (pos >= stream->len || pos < 0) {
		lua_pushinteger(L, -1);
	}
	else {
		lua_pushinteger(L, stream->data[pos]);
	}

	return 1;
}

int StreamLen(lua_State* L) {

	LuaStream* stream = lua_toluastream(L, 1);
	lua_pop(L, lua_gettop(L));
	lua_pushinteger(L, stream->len);
	return 1;
}

int StreamPos(lua_State* L) {

	LuaStream* stream = lua_toluastream(L, 1);
	lua_pop(L, lua_gettop(L));
	lua_pushinteger(L, stream->pos);
	return 1;
}

int ReadStreamByte(lua_State* L) {

	LuaStream* stream = lua_toluastream(L, 1);
	const BYTE* result = ReadStream(stream, 1);

	if (!result) {
		lua_pop(L, lua_gettop(L));
		lua_pushinteger(L, -1);
		return 1;
	}

	lua_pop(L, lua_gettop(L));
	lua_pushinteger(L, *result);

	return 1;
}

int ReadLuaStream(lua_State* L) {

	LuaStream* stream = lua_toluastream(L, 1);
	long len = (long)luaL_optinteger(L, 2, stream->len - stream->pos);
	const BYTE* result = ReadStream(stream, len);

	if (!result) {
		lua_pop(L, lua_gettop(L));
		lua_pushlstring(L, "", 0);
		return 1;
	}

	lua_pop(L, lua_gettop(L));
	lua_pushlstring(L, (char*)result, len);

	return 1;
}

int WriteLuaValue(lua_State* L) {

	LuaStream* stream = lua_toluastream(L, 1);
	size_t size = (size_t)luaL_optinteger(L, 3, 0);

	LUA_NUMBER number;
	BYTE boolean;
	void* p;
	LuaStream* other;

	BYTE* raw;
	size_t len;

	switch (lua_type(L, 2)) {

	case LUA_TNUMBER:
		number = lua_tonumber(L, 2);
		raw = (BYTE*)& number;
		len = sizeof(LUA_NUMBER);
		break;

	case LUA_TBOOLEAN:
		boolean = lua_toboolean(L, 2);
		raw = &boolean;
		len = 1;
		break;

	case LUA_TSTRING:
		raw = (BYTE*)lua_tolstring(L, 2, &len);
		break;

	case LUA_TUSERDATA:
		p = luaL_testudata(L, 2, STREAM);
		if (p && p != (void*)stream) {
			other = (LuaStream*)p;
			raw = &other->data[other->pos];
			len = other->len - other->pos;
		}
		else {
			raw = NULL;
			len = 0;
		}

		break;

	default:
		len = 0;
		raw = NULL;
		break;
	}

	lua_pop(L, lua_gettop(L));

	if (size > 0 && size < len) {
		len = size;
	}

	if (len <= 0 || !raw || !StreamWrite(L, stream, raw, len)) {
		lua_pushinteger(L, 0);
	}
	else {
		lua_pushinteger(L, len);
	}

	return 1;
}

int WriteStreamByte(lua_State* L) {

	LuaStream* stream = lua_toluastream(L, 1);
	int byte = (int)lua_tointeger(L, 2);

	if (byte > 255 || byte < 0) {

		lua_pop(L, lua_gettop(L));
		lua_pushboolean(L, false);
		lua_pushstring(L, "Byte out of range, must be between 0 and 255");
		return 2;
	}

	BYTE raw = byte;

	if (!StreamWrite(L, stream, &raw, 1)) {

		lua_pop(L, lua_gettop(L));
		lua_pushboolean(L, false);
		lua_pushstring(L, "Unable to allocate memory");
		return 2;
	}

	lua_pop(L, lua_gettop(L));
	lua_pushboolean(L, true);

	return 1;
}

int WriteToFile(lua_State* L) {
	
	LuaStream* stream = lua_toluastream(L, 1);
	const char * file = luaL_checkstring(L, 2);
	size_t pos = luaL_checkinteger(L, 3);
	size_t len = luaL_checkinteger(L, 4);

	const BYTE * data = ReadStream(stream, len);

	if (data == NULL) {
		luaL_error(L, "Stream out of bounds");
		return 0;
	}

	FILE * f = fopen(file, "r+b");

	if (!f) {
		luaL_error(L, "Unable to open file");
		return 0;
	}

	if (fseek(f, 0, SEEK_END) != 0) {
		fclose(f);
		luaL_error(L, "Unable to seek in file");
		return 0;
	}

	long size = ftell(f);

	if (pos > size) {

		while (pos > size) {

			if (fputc('\0', f) == EOF) {
				fclose(f);
				luaL_error(L, "Unable to write padding");
				return 0;
			}

			size++;
		}
	}
	else {
		if (fseek(f, pos, SEEK_SET) != 0) {
			fclose(f);
			luaL_error(L, "Unable to seek in file");
			return 0;
		}
	}

	fwrite(data, sizeof(BYTE), len, f);
	fflush(f);
	fclose(f);

	return 0;
}

int DumpToFile(lua_State* L) {

	LuaStream* stream = lua_toluastream(L, 1);
	const char * file = luaL_checkstring(L, 2);

	FILE * f = fopen(file, "wb");

	if (!f) {
		luaL_error(L, "Unable to open file");
		return 0;
	}

	fwrite(stream->data, sizeof(BYTE), stream->len, f);
	fflush(f);
	fclose(f);

	return 0;
}

int OpenFileToStream(lua_State* L) {

	const char * file = luaL_checkstring(L, 1);

	FILE * f = fopen(file, "rb");

	if (!f) {
		luaL_error(L, "Unable to open file");
		return 1;
	}

	if (fseek(f, 0, SEEK_END) != 0) {
		fclose(f);
		luaL_error(L, "Unable to seek in file");
		return 1;
	}

	long size = ftell(f);
	rewind(f);
	int alloc = size;

	lua_pop(L, lua_gettop(L));

	LuaStream* stream = lua_pushluastream(L);
	
	if (alloc < MIN_STREAM_SIZE) {
		alloc = MIN_STREAM_SIZE;
	}

	stream->data = (BYTE*)malloc(alloc);
	if (!stream->data) {
		fclose(f);
		luaL_error(L, "Unable to allocate memory");
		return 0;
	}

	stream->alloc = alloc;
	stream->allocfunc = LUA_NOREF;
	stream->pos = 0;

	if (size > 0) {
		stream->len = fread(stream->data, sizeof(BYTE), size, f);
	}
	else {		
		stream->len = 0;
	}

	fclose(f);

	return 1;
}

int NewStream(lua_State* L) {

	if (lua_type(L, 1) == LUA_TFUNCTION) {

		int ref = luaL_ref(L, LUA_REGISTRYINDEX);

		LuaStream* stream = lua_pushluastream(L);
		stream->allocfunc = ref;
		size_t size = AllocAddSize(L, stream, MIN_STREAM_SIZE);

		stream->data = (BYTE*)malloc(size);
		if (!stream->data) {
			luaL_error(L, "Unable to allocate memory");
			return 0;
		}
		stream->alloc = size;
		stream->len = 0;

		return 1;
	}

	int init = (int)luaL_optinteger(L, 1, 1048576);

	if (init <= 0) {
		init = MIN_STREAM_SIZE;
	}

	lua_pop(L, lua_gettop(L));

	LuaStream* stream = lua_pushluastream(L);

	stream->data = (BYTE*)malloc(init);
	if (!stream->data) {
		luaL_error(L, "Unable to allocate memory");
		return 0;
	}

	stream->alloc = init;
	stream->len = 0;
	stream->pos = 0;
	stream->allocfunc = LUA_NOREF;

	return 1;
}

LuaStream* lua_pushluastream(lua_State* L) {

	LuaStream* stream = (LuaStream*)lua_newuserdata(L, sizeof(LuaStream));

	if (stream == NULL)
		luaL_error(L, "Unable to push namedpipe");

	luaL_getmetatable(L, STREAM);
	lua_setmetatable(L, -2);

	memset(stream, 0, sizeof(LuaStream));

	return stream;
}

LuaStream* lua_toluastream(lua_State* L, int index) {

	LuaStream* pipe = (LuaStream*)luaL_checkudata(L, index, STREAM);

	if (pipe == NULL)
		luaL_error(L, "parameter is not a %s", STREAM);

	return pipe;
}

int luastream_gc(lua_State* L) {

	LuaStream* pipe = lua_toluastream(L, 1);

	if (pipe && pipe->alloc) {
		free(pipe->data);
		ZeroMemory(pipe, sizeof(LuaStream));
	}

	if (pipe && pipe->allocfunc != LUA_NOREF) {
		luaL_unref(L, LUA_REGISTRYINDEX, pipe->allocfunc);
		pipe->allocfunc = 0;
	}

	return 0;
}

int luastream_tostring(lua_State* L) {
	char tim[100];
	sprintf(tim, "Stream: 0x%08X", lua_toluastream(L, 1));
	lua_pushfstring(L, tim);
	return 1;
}