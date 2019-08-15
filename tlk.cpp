#include "tlk.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>

int tlk_open(lua_State *L) {

	size_t len;
	const char * file = luaL_checklstring(L, 1, &len);
	FILE * f = fopen(file, "rb");
	if (!f) {
		lua_pop(L, lua_gettop(L));
		lua_pushnil(L);
		return 1;
	}
	else
		lua_pop(L, lua_gettop(L));

	TlkHeader header;
	if (fread(&header, 1, sizeof(TlkHeader), f) != sizeof(TlkHeader)) {
		fclose(f);
		lua_pushnil(L);
		return 1;
	}

	if (tolower(header.FileType[0]) != 't' ||
		tolower(header.FileType[1]) != 'l' ||
		tolower(header.FileType[2]) != 'k' ||
		tolower(header.FileType[3]) != ' ')
	{
		fclose(f);
		lua_pushnil(L);
		return 1;
	}

	LuaTLK * tlk = lua_pushtlk(L);

	tlk->file = f;
	memcpy(&tlk->Header, &header, sizeof(TlkHeader));
	tlk->filename = (char*)calloc(len + 1, sizeof(char));
	memcpy(tlk->filename, file, sizeof(char)*len);

	return 1;
}

int tlk_create(lua_State *L) {

	const char * file = luaL_checkstring(L, 1);
	if (!lua_istable(L, 2)) {
		lua_pop(L, lua_gettop(L));
		luaL_error(L, "Parameter 2 is not a table");
		return 0;
	}

	int languageid = (int)luaL_optinteger(L, 3, 0);
	const char * version = luaL_optstring(L, 4, "V3.0");

	if (strlen(version) != 4) {
		lua_pop(L, lua_gettop(L));
		luaL_error(L, "Version parameter must be 4 characters");
		return 0;
	}

	FILE * f = fopen(file, "wb");

	if (!f) {
		lua_pop(L, lua_gettop(L));
		luaL_error(L, "Unable to open file: %s", file);
		return 0;
	}

	TlkHeader header;
	TlkStringData data;
	memset(&data, 0, sizeof(TlkStringData));
	memset(&header, 0, sizeof(TlkHeader));
	memcpy(&header.FileType, "TLK ", 4);
	memcpy(&header.FileVersion, version, 4);
	memcpy(&header.LanguageID, &languageid, sizeof(unsigned int));

	lua_pop(L, lua_gettop(L) - 2);

	fwrite(&header, sizeof(TlkHeader), 1, f);
	fflush(f);

	size_t len;
	const char * str;
	int n = -1;
	do {

		lua_pushnumber(L, ++n);
		lua_gettable(L, -2);

		str = lua_tolstring(L, -1, &len);

		fwrite(&data, sizeof(TlkStringData), 1, f);
		fflush(f);

		lua_pop(L, 1);

	} while (str);

	unsigned int currentoffset = sizeof(TlkHeader) + (n * sizeof(TlkStringData));

	memcpy(&header.StringCount, &n, sizeof(unsigned int));
	header.StringEntriesOffset = currentoffset;

	fseek(f, 0, SEEK_SET);
	fwrite(&header, sizeof(TlkHeader), 1, f);
	fflush(f);
	rewind(f);

	n = -1;
	do {

		lua_pushnumber(L, ++n);
		lua_gettable(L, -2);

		str = lua_tolstring(L, -1, &len);

		if (!str)
			break;

		fseek(f, sizeof(TlkHeader) + (n * sizeof(TlkStringData)), SEEK_SET);

		if (len > 0) {
			data.OffsetToString = currentoffset - header.StringEntriesOffset;
			data.StringSize = len;
			data.Flags = TEXT_PRESENT;
		}
		else {
			data.OffsetToString = 0;
			data.StringSize = 0;
			data.Flags = 0;
		}

		fwrite(&data, sizeof(TlkStringData), 1, f);
		fflush(f);

		if (len > 0) {
			fseek(f, currentoffset, SEEK_SET);
			currentoffset += fwrite(str, sizeof(char), len, f);

			fflush(f);
		}

		lua_pop(L, 1);

	} while (true);

	fclose(f);

	lua_pop(L, 1);

	return tlk_open(L);
}

int tlk_defragment(lua_State *L) {

	LuaTLK * tlk = (LuaTLK*)lua_totlk(L, 1);
	int extra = (int)luaL_optinteger(L, 2, 0);

	rewind(tlk->file);
	if (fseek(tlk->file, sizeof(TlkHeader), SEEK_SET) != 0) {
		lua_pop(L, lua_gettop(L));
		lua_pushboolean(L, false);
		return 1;
	}

	FILE * tmp = tmpfile();

	if (!tmp) {
		lua_pop(L, lua_gettop(L));
		lua_pushboolean(L, false);
		return 1;
	}

	if (fwrite(&tlk->Header, sizeof(TlkHeader), 1, tmp) != 1) {
		fclose(tmp);
		lua_pop(L, lua_gettop(L));
		lua_pushboolean(L, false);
		return 1;
	}

	TlkHeader header;
	TlkStringData data;
	memset(&data, 0, sizeof(TlkStringData));
	memcpy(&header, &tlk->Header, sizeof(TlkHeader));

	for (size_t i = 0; i < tlk->Header.StringCount + extra; i++)
	{
		if (fwrite(&data, sizeof(TlkStringData), 1, tmp) != 1) {
			fclose(tmp);
			lua_pop(L, lua_gettop(L));
			lua_pushboolean(L, false);
			return 1;
		}
	}

	header.StringEntriesOffset = ftell(tmp);
	header.StringCount = tlk->Header.StringCount + extra;

	rewind(tmp);
	if (fwrite(&header, sizeof(TlkHeader), 1, tmp) != 1) {
		fclose(tmp);
		lua_pop(L, lua_gettop(L));
		lua_pushboolean(L, false);
		return 1;
	}

	rewind(tlk->file);
	if (fseek(tlk->file, sizeof(TlkHeader), SEEK_SET) != 0) {
		fclose(tmp);
		lua_pop(L, lua_gettop(L));
		lua_pushboolean(L, false);
		return 1;
	}

	size_t buffersize = 1024;
	char * buffer = (char*)calloc(buffersize, sizeof(char));

	if (!buffer) {
		fclose(tmp);
		lua_pop(L, lua_gettop(L));
		lua_pushboolean(L, false);
		return 1;
	}

	size_t pos;
	size_t currentdata = header.StringEntriesOffset;

	for (size_t i = 0; i < tlk->Header.StringCount; i++)
	{
		//Read header
		if (fread(&data, sizeof(TlkStringData), 1, tlk->file) != 1) {
			free(buffer);
			fclose(tmp);
			lua_pop(L, lua_gettop(L));
			lua_pushboolean(L, false);
			return 1;
		}

		if ((data.Flags & 0x00000001) > 0 && data.StringSize > 0) {

			//Check buffer size
			if (data.StringSize > buffersize) {
				free(buffer);
				buffersize = data.StringSize + 1;
				buffer = (char*)calloc(buffersize, sizeof(char));
				if (!buffer) {
					fclose(tmp);
					lua_pop(L, lua_gettop(L));
					lua_pushboolean(L, false);
					return 1;
				}
			}

			//Store position
			pos = ftell(tlk->file);

			//Seek to data
			if (fseek(tlk->file, tlk->Header.StringEntriesOffset + data.OffsetToString, SEEK_SET) != 0) {
				free(buffer);
				fclose(tmp);
				lua_pop(L, lua_gettop(L));
				lua_pushboolean(L, false);
				return 1;
			}

			//Read data
			if (fread(buffer, sizeof(char), data.StringSize, tlk->file) != data.StringSize) {
				free(buffer);
				fclose(tmp);
				lua_pop(L, lua_gettop(L));
				lua_pushboolean(L, false);
				return 1;
			}

			//Restore position
			if (fseek(tlk->file, pos, SEEK_SET) != 0) {
				free(buffer);
				fclose(tmp);
				lua_pop(L, lua_gettop(L));
				lua_pushboolean(L, false);
				return 1;
			}

			//Store pos
			pos = ftell(tmp);

			//Seek to data
			if (fseek(tmp, currentdata, SEEK_SET) != 0) {
				free(buffer);
				fclose(tmp);
				lua_pop(L, lua_gettop(L));
				lua_pushboolean(L, false);
				return 1;
			}

			//Write data
			if (fwrite(buffer, sizeof(char), data.StringSize, tmp) != data.StringSize) {
				free(buffer);
				fclose(tmp);
				lua_pop(L, lua_gettop(L));
				lua_pushboolean(L, false);
				return 1;
			}

			//Restore position
			if (fseek(tmp, pos, SEEK_SET) != 0) {
				free(buffer);
				fclose(tmp);
				lua_pop(L, lua_gettop(L));
				lua_pushboolean(L, false);
				return 1;
			}

			//Corret offsets
			data.OffsetToString = currentdata - header.StringEntriesOffset;
			currentdata += data.StringSize;
		}

		//Write dataheader
		if (fwrite(&data, sizeof(TlkStringData), 1, tmp) != 1) {
			free(buffer);
			fclose(tmp);
			lua_pop(L, lua_gettop(L));
			lua_pushboolean(L, false);
			return 1;
		}
	}

	fclose(tlk->file);
	tlk->file = fopen(tlk->filename, "wb");
	if (!tlk->file) {
		tlk->file = fopen(tlk->filename, "rb");
		free(buffer);
		fclose(tmp);
		lua_pop(L, lua_gettop(L));
		lua_pushboolean(L, false);
		return 1;
	}

	rewind(tmp);

	pos = 0;

	do {

		pos = fread(buffer, sizeof(char), buffersize, tmp);
		fwrite(buffer, sizeof(char), pos, tlk->file);
		fflush(tlk->file);

	} while (pos > 0);

	fclose(tlk->file);
	tlk->file = fopen(tlk->filename, "rb");

	memcpy(&tlk->Header, &header, sizeof(TlkHeader));

	free(buffer);
	fclose(tmp);

	lua_pop(L, lua_gettop(L));
	lua_pushboolean(L, true);

	return 1;
}

int tlk_setsound(lua_State *L) {

	LuaTLK * tlk = (LuaTLK*)lua_totlk(L, 1);
	unsigned int index = (unsigned int)luaL_checkinteger(L, 2);

	if (index > tlk->Header.StringCount || index < 0) {
		lua_pop(L, lua_gettop(L));
		lua_pushboolean(L, false);
		return 1;
	}

	unsigned int filepos = sizeof(TlkHeader) + (sizeof(TlkStringData) * index);
	TlkStringData entry;
	rewind(tlk->file);
	if (fseek(tlk->file, filepos, SEEK_SET) != 0) {
		lua_pop(L, lua_gettop(L));
		lua_pushboolean(L, false);
		return 1;
	}

	if (fread(&entry, 1, sizeof(TlkStringData), tlk->file) != sizeof(TlkStringData)) {
		lua_pop(L, lua_gettop(L));
		lua_pushboolean(L, false);
		return 1;
	}

	fclose(tlk->file);
	tlk->file = fopen(tlk->filename, "r+b");
	if (!tlk->file) {
		tlk->file = fopen(tlk->filename, "rb");
		lua_pop(L, lua_gettop(L));
		lua_pushboolean(L, false);
		return 1;
	}

	size_t len;
	const char * resref = luaL_optlstring(L, 3, "", &len);

	if (len > 16) {
		lua_pop(L, lua_gettop(L));
		lua_pushboolean(L, false);
		return 1;
	}

	if (len <= 0) {
		memset(entry.SoundResRef, 0, 16);
		entry.Flags &= 0xFFFFFFF9;
	}
	else {
		memset(entry.SoundResRef, 0, 16);
		memcpy(entry.SoundResRef, resref, len);
		entry.Flags |= 0x00000002;

		if (lua_gettop(L) >= 4 && lua_type(L, 4) == LUA_TNUMBER) {
			entry.Flags |= 0x00000004;
			entry.SoundLength = (float)luaL_optnumber(L, 4, 0);
		}
		else {
			entry.Flags &= 0xFFFFFFFB;
			entry.SoundLength = 0;
		}
	}

	if (lua_gettop(L) >= 5) {
		entry.VolumeVariance = (unsigned int)luaL_checkinteger(L, 5);
	}

	if (lua_gettop(L) >= 6) {
		entry.PitchVariance = (unsigned int)luaL_checkinteger(L, 6);
	}

	fseek(tlk->file, filepos, SEEK_SET);
	fwrite(&entry, sizeof(TlkStringData), 1, tlk->file);
	fflush(tlk->file);

	fclose(tlk->file);
	tlk->file = fopen(tlk->filename, "rb");

	lua_pop(L, lua_gettop(L));
	lua_pushboolean(L, true);
	return 1;
}

int tlk_info(lua_State *L) {
	LuaTLK * tlk = (LuaTLK*)lua_totlk(L, 1);
	lua_pop(L, lua_gettop(L));
	lua_pushinteger(L, tlk->Header.StringCount);
	lua_pushinteger(L, tlk->Header.LanguageID);
	lua_pushlstring(L, tlk->Header.FileVersion, 4);
	return 3;
}

int tlk_setstrref(lua_State *L) {

	LuaTLK * tlk = (LuaTLK*)lua_totlk(L, 1);
	unsigned int index = (unsigned int)luaL_checkinteger(L, 2);
	size_t len;
	const char * data = luaL_checklstring(L, 3, &len);

	if (index > tlk->Header.StringCount || index < 0) {
		lua_pop(L, lua_gettop(L));
		lua_pushboolean(L, false);
		return 1;
	}

	unsigned int filepos = sizeof(TlkHeader) + (sizeof(TlkStringData) * index);
	TlkStringData entry;
	rewind(tlk->file);
	if (fseek(tlk->file, filepos, SEEK_SET) != 0) {
		lua_pop(L, lua_gettop(L));
		lua_pushboolean(L, false);
		return 1;
	}

	if (fread(&entry, 1, sizeof(TlkStringData), tlk->file) != sizeof(TlkStringData)) {
		lua_pop(L, lua_gettop(L));
		lua_pushboolean(L, false);
		return 1;
	}

	fclose(tlk->file);
	tlk->file = fopen(tlk->filename, "r+b");
	if (!tlk->file) {
		tlk->file = fopen(tlk->filename, "rb");
		lua_pop(L, lua_gettop(L));
		lua_pushboolean(L, false);
		return 1;
	}

	if (len <= 0) {
		entry.OffsetToString = 0;
		entry.StringSize = 0;
		entry.Flags &= 0xFFFFFFFFE;
	}
	else {

		bool had = (entry.Flags & 0x00000001) > 0;

		entry.Flags |= 0x00000001;

		if (len > entry.StringSize || !had) {
			fseek(tlk->file, 0, SEEK_END);
			entry.OffsetToString = ftell(tlk->file) - tlk->Header.StringEntriesOffset;
			entry.StringSize = len;
			fwrite(data, sizeof(char), len, tlk->file);
			fflush(tlk->file);
		}
		else {
			fseek(tlk->file, tlk->Header.StringEntriesOffset + entry.OffsetToString, SEEK_SET);
			entry.StringSize = len;
			fwrite(data, sizeof(char), len, tlk->file);
			fflush(tlk->file);
		}
	}

	fseek(tlk->file, filepos, SEEK_SET);
	fwrite(&entry, sizeof(TlkStringData), 1, tlk->file);
	fflush(tlk->file);

	fclose(tlk->file);
	tlk->file = fopen(tlk->filename, "rb");
	lua_pop(L, lua_gettop(L));
	lua_pushboolean(L, true);

	return 1;
}


int tlk_get(lua_State *L)
{
	LuaTLK * tlk = (LuaTLK*)lua_totlk(L, 1);
	lua_Integer index = luaL_checkinteger(L, 2);

	if (index > tlk->Header.StringCount || index < 0) {
		lua_pop(L, lua_gettop(L));
		lua_pushnil(L);
		return 1;
	}

	TlkStringData entry;
	rewind(tlk->file);
	if (fseek(tlk->file, sizeof(TlkHeader) + (sizeof(TlkStringData) * (long)index), SEEK_SET) != 0) {
		lua_pop(L, lua_gettop(L));
		lua_pushnil(L);
		return 1;
	}

	if (fread(&entry, 1, sizeof(TlkStringData), tlk->file) != sizeof(TlkStringData)) {
		lua_pop(L, lua_gettop(L));
		lua_pushnil(L);
		return 1;
	}

	char resref[17];
	memset(resref, 0, 17);
	lua_pop(L, lua_gettop(L));
	lua_createtable(L, 0, 6);

	lua_pushstring(L, "Flags");
	lua_pushinteger(L, entry.Flags);
	lua_settable(L, -3);

	memcpy(resref, entry.SoundResRef, 16);

	lua_pushstring(L, "SoundResRef");
	lua_pushstring(L, resref);
	lua_settable(L, -3);

	lua_pushstring(L, "VolumeVariance");
	lua_pushinteger(L, entry.VolumeVariance);
	lua_settable(L, -3);

	lua_pushstring(L, "PitchVariance");
	lua_pushinteger(L, entry.PitchVariance);
	lua_settable(L, -3);

	lua_pushstring(L, "SoundLength");
	lua_pushnumber(L, entry.SoundLength);
	lua_settable(L, -3);

	char * str;

	if (entry.Flags & TEXT_PRESENT) {
		if (fseek(tlk->file, tlk->Header.StringEntriesOffset + entry.OffsetToString, SEEK_SET) != 0) {
			lua_pop(L, lua_gettop(L));
			lua_pushnil(L);
			return 1;
		}

		str = (char*)malloc(entry.StringSize);
		if (!str) {
			lua_pop(L, lua_gettop(L));
			lua_pushnil(L);
			return 1;
		}

		if (fread(str, 1, entry.StringSize, tlk->file) != entry.StringSize)
		{
			free(str);
			lua_pop(L, lua_gettop(L));
			lua_pushnil(L);
			return 1;
		}

		lua_pushstring(L, "String");
		lua_pushlstring(L, str, entry.StringSize);
		lua_settable(L, -3);

		free(str);
	}
	else {
		lua_pushstring(L, "String");
		lua_pushstring(L, "");
		lua_settable(L, -3);
	}

	return 1;
}

int tlk_getall(lua_State *L) {

	LuaTLK * tlk = (LuaTLK*)lua_totlk(L, 1);

	rewind(tlk->file);
	if (fseek(tlk->file, sizeof(TlkHeader), SEEK_SET) != 0) {
		lua_pop(L, lua_gettop(L));
		lua_pushnil(L);
		return 1;
	}

	long int prev;
	TlkStringData entry;
	char * str;

	lua_pop(L, lua_gettop(L));
	lua_createtable(L, tlk->Header.StringCount, 0);

	for (unsigned int n = 0; n < tlk->Header.StringCount; n++) {

		if (fread(&entry, 1, sizeof(TlkStringData), tlk->file) != sizeof(TlkStringData)) {
			lua_pop(L, lua_gettop(L));
			lua_pushnil(L);
			return 1;
		}

		if (entry.Flags & TEXT_PRESENT)
		{
			prev = ftell(tlk->file);
			if (fseek(tlk->file, tlk->Header.StringEntriesOffset + entry.OffsetToString, SEEK_SET) != 0) {
				lua_pop(L, lua_gettop(L));
				lua_pushnil(L);
				return 1;
			}

			str = (char*)malloc(entry.StringSize);
			if (!str) {
				lua_pop(L, lua_gettop(L));
				lua_pushnil(L);
				return 1;
			}

			size_t test = fread(str, 1, entry.StringSize, tlk->file);

			if (test != entry.StringSize)
			{
				free(str);
				lua_pop(L, lua_gettop(L));
				lua_pushnil(L);
				return 1;
			}

			lua_pushlstring(L, str, entry.StringSize);
			free(str);
			fseek(tlk->file, prev, SEEK_SET);
		}
		else {
			lua_pushstring(L, "");
		}

		lua_rawseti(L, -2, n);
	}

	return 1;
}

LuaTLK * lua_pushtlk(lua_State *L) {
	LuaTLK * tlk = (LuaTLK*)lua_newuserdata(L, sizeof(LuaTLK));
	if (tlk == NULL)
		luaL_error(L, "Unable to push tlk");
	luaL_getmetatable(L, TLK);
	lua_setmetatable(L, -2);
	memset(tlk, 0, sizeof(LuaTLK));
	return tlk;
}

LuaTLK * lua_totlk(lua_State *L, int index) {
	LuaTLK * tlk = (LuaTLK*)luaL_checkudata(L, index, TLK);
	if (tlk == NULL)
		luaL_error(L, "parameter is not a %s", TLK);
	return tlk;
}

int tlk_gc(lua_State *L) {

	LuaTLK * tlk = lua_totlk(L, 1);

	if (tlk->filename) {
		free(tlk->filename);
		tlk->filename = NULL;
	}

	if (tlk->file) {
		fclose(tlk->file);
		tlk = NULL;
	}
	return 0;
}

int tlk_tostring(lua_State *L) {
	char tim[100];
	sprintf(tim, "Tlk: 0x%08X", lua_totlk(L, 1));
	lua_pushfstring(L, tim);
	return 1;
}