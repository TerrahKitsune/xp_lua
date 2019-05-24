#include "stream.h"
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <stdlib.h> 
#include <windows.h> 

const size_t MIN_STREAM_SIZE = 1024;

bool CheckStreamSize(LuaStream* stream, size_t requestedsize) {

	if (!stream->data) {
		return false;
	}
	else if (stream->pos + requestedsize > stream->len) {
		requestedsize = (stream->pos + requestedsize) - stream->len;
	}

	if (stream->len + requestedsize > stream->alloc) {

		size_t newsize = (stream->len + requestedsize) - stream->alloc;

		if (newsize < MIN_STREAM_SIZE) {
			newsize = MIN_STREAM_SIZE;
		}

		void* temp = malloc(stream->alloc + newsize);
		if (!temp) {
			return false;
		}
		else {
			memcpy(temp, stream->data, stream->len);
			free(stream->data);
			stream->data = (BYTE*)temp;
			stream->alloc = stream->alloc + newsize;
			return true;
		}
	}
	else {
		return true;
	}
}

bool StreamWrite(LuaStream * stream, BYTE * data, size_t len) {

	if (!stream || !stream->data) {
		return false;
	}
	else if (!CheckStreamSize(stream, len)) {
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

const BYTE* ReadStream(LuaStream* stream, size_t len) {

	if (len <= 0 || !stream || !stream->data || (stream->pos+len) > stream->len) {
		return NULL;
	}

	const BYTE* result = &stream->data[stream->pos];
	stream->pos += len;

	return result;
}

int WriteFloat(lua_State* L) {

	LuaStream* stream = lua_toluastream(L, 1);
	float f = lua_tonumber(L, 2);

	lua_pop(L, lua_gettop(L));

	lua_pushboolean(L, StreamWrite(stream, (BYTE*)& f, sizeof(float)));

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

	lua_pushboolean(L, StreamWrite(stream, (BYTE*)& f, sizeof(double)));

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
	short n = lua_tointeger(L, 2);

	lua_pop(L, lua_gettop(L));

	lua_pushboolean(L, StreamWrite(stream, (BYTE*)& n, sizeof(short)));

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
	unsigned short n = lua_tointeger(L, 2);

	lua_pop(L, lua_gettop(L));

	lua_pushboolean(L, StreamWrite(stream, (BYTE*)& n, sizeof(unsigned short)));

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
	int n = lua_tointeger(L, 2);

	lua_pop(L, lua_gettop(L));

	lua_pushboolean(L, StreamWrite(stream, (BYTE*)& n, sizeof(int)));

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
	unsigned int n = lua_tointeger(L, 2);

	lua_pop(L, lua_gettop(L));

	lua_pushboolean(L, StreamWrite(stream, (BYTE*)& n, sizeof(unsigned int)));

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

	const BYTE* raw = ReadStream(stream, sizeof(long));

	lua_pop(L, lua_gettop(L));

	if (!raw) {
		lua_pushnil(L);
	}
	else {
		long f;
		memcpy(&f, raw, sizeof(long));
		lua_pushinteger(L, f);
	}

	return 1;
}

int WriteLong(lua_State* L) {

	LuaStream* stream = lua_toluastream(L, 1);
	long n = lua_tointeger(L, 2);

	lua_pop(L, lua_gettop(L));

	lua_pushboolean(L, StreamWrite(stream, (BYTE*)& n, sizeof(long)));

	return 1;
}

int WriteUnsignedLong(lua_State* L) {

	LuaStream* stream = lua_toluastream(L, 1);
	unsigned long n = lua_tointeger(L, 2);

	lua_pop(L, lua_gettop(L));

	lua_pushboolean(L, StreamWrite(stream, (BYTE*)& n, sizeof(unsigned long)));

	return 1;
}

int ReadUnsignedLong(lua_State* L) {

	LuaStream* stream = lua_toluastream(L, 1);

	const BYTE* raw = ReadStream(stream, sizeof(unsigned long));

	lua_pop(L, lua_gettop(L));

	if (!raw) {
		lua_pushnil(L);
	}
	else {
		unsigned long f;
		memcpy(&f, raw, sizeof(unsigned long));
		lua_pushinteger(L, f);
	}

	return 1;
}

int StreamSetPos(lua_State * L) {

	LuaStream* stream = lua_toluastream(L, 1);
	int newpos = luaL_optinteger(L, 2, 0);

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

int GetStreamInfo(lua_State * L) {

	LuaStream* stream = lua_toluastream(L, 1);

	if (!stream->data) {
		lua_pop(L, lua_gettop(L));
		lua_pushnil(L);
		return 1;
	}

	lua_pop(L, lua_gettop(L));

	lua_pushinteger(L, stream->pos);
	lua_pushinteger(L, stream->len);

	return 2;
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

int PeekStreamByte(lua_State* L) {
	
	LuaStream* stream = lua_toluastream(L, 1);
	size_t pos = luaL_optinteger(L, 2, stream->pos);

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

int ReadStreamByte(lua_State * L) {

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

int ReadLuaStream(lua_State * L) {

	LuaStream* stream = lua_toluastream(L, 1);
	long len = luaL_optinteger(L, 2, stream->len - stream->pos);
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

int WriteLuaValue(lua_State * L) {

	LuaStream* stream = lua_toluastream(L, 1);
	long size = luaL_optinteger(L, 3, 0);

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

	if (len <= 0 || !raw || !StreamWrite(stream, raw, len)) {
		lua_pushinteger(L, 0);
	}
	else {
		lua_pushinteger(L, len);
	}

	return 1;
}

int WriteStreamByte(lua_State * L) {

	LuaStream* stream = lua_toluastream(L, 1);
	int byte = lua_tointeger(L, 2);

	if (byte > 255 || byte < 0) {

		lua_pop(L, lua_gettop(L));
		lua_pushboolean(L, false);
		lua_pushstring(L, "Byte out of range, must be between 0 and 255");
		return 2;
	}

	BYTE raw = byte;

	if (!StreamWrite(stream, &raw, 1)) {

		lua_pop(L, lua_gettop(L));
		lua_pushboolean(L, false);
		lua_pushstring(L, "Unable to allocate memory");
		return 2;
	}

	lua_pop(L, lua_gettop(L));
	lua_pushboolean(L, true);

	return 1;
}

int NewStream(lua_State * L) {

	int init = luaL_optinteger(L, 1, 1048576);

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

	return 1;
}

LuaStream* lua_pushluastream(lua_State * L) {

	LuaStream* stream = (LuaStream*)lua_newuserdata(L, sizeof(LuaStream));

	if (stream == NULL)
		luaL_error(L, "Unable to push namedpipe");

	luaL_getmetatable(L, STREAM);
	lua_setmetatable(L, -2);

	memset(stream, 0, sizeof(LuaStream));

	return stream;
}

LuaStream * lua_toluastream(lua_State * L, int index) {

	LuaStream* pipe = (LuaStream*)luaL_checkudata(L, index, STREAM);

	if (pipe == NULL)
		luaL_error(L, "parameter is not a %s", STREAM);

	return pipe;
}

int luastream_gc(lua_State * L) {

	LuaStream* pipe = lua_toluastream(L, 1);

	if (pipe && pipe->alloc) {
		free(pipe->data);
		ZeroMemory(pipe, sizeof(LuaStream));
	}

	return 0;
}

int luastream_tostring(lua_State * L) {
	char tim[100];
	sprintf(tim, "Stream: 0x%08X", lua_toluastream(L, 1));
	lua_pushfstring(L, tim);
	return 1;
}