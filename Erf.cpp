#include "Erf.h"
#include <string.h>
#include <stdlib.h>
#include <Windows.h>

static char nullref[17];
static const char * NullTerminatedResRef(const char * resref){
	nullref[16] = '\0';
	strncpy(nullref, resref, 16);
	return nullref;
}

static size_t _Write(const void * data, size_t length, FILE * file, lua_State *L, const char * filename, FILE * readfile, void * dealloc){

	if (fwrite(data, 1, length, file) != sizeof(ERFHeader)){
		fclose(file);
		remove(filename);
		if (readfile)
			fclose(readfile);
		if (dealloc)
			free(dealloc);
		luaL_error(L, "Failed to write header to temp file");
	}
	fflush(file);
	return length;
}

static void _SeekAndRead(FILE * read, long offset, size_t size, void * into, lua_State *L, const char * tmpname, FILE * tmpfile, void * dealloc){

	if (fseek(read, offset, SEEK_SET) || fread(into, 1, size, read) != size){
		fclose(read);
		fclose(tmpfile);
		remove(tmpname);
		if (dealloc)
			free(dealloc);
		luaL_error(L, "Unable to seek in file");
	}
}

int AddFile(lua_State *L){

	char tempfile[_MAX_PATH];
	ERF * luaerf = (ERF*)lua_toerf(L, 1);
	const char * file = luaL_checkstring(L, 2);

	if (strlen(luaerf->File) + 5 >= _MAX_PATH)
		luaL_error(L, "Unable to create temp file, path too long");

	sprintf(tempfile, "%s.tmp", luaerf->File);
	FILE * tmp = fopen(tempfile, "wb");
	FILE * erf = fopen(luaerf->File, "rb");
	if (!erf)
		luaL_error(L, "Unable to open erf file");
	else if (!tmp)
		luaL_error(L, "Unable to create temp file");

	size_t byteswritten = 0;
	luaerf->Header->EntryCount++;
	byteswritten += _Write(luaerf->Header, sizeof(ERFHeader), tmp, L, tempfile, erf, NULL);
	luaerf->Header->EntryCount--;

	ErfLocString locstring;
	char * tempstring;
	long offset = luaerf->Header->OffsetToLocalizedString;
	for (int n = 0; n < luaerf->Header->LanguageCount; n++){
		_SeekAndRead(erf, offset, sizeof(ErfLocString), &locstring, L, tempfile, tmp, NULL);
		offset += sizeof(ErfLocString);
		tempstring = (char*)malloc(locstring.StringSize);
		if (!tempstring){
			fclose(tmp);
			fclose(erf);
			remove(tempfile);
			luaL_error(L, "unable to allocate memory for localized string");
		}
		_SeekAndRead(erf, offset, locstring.StringSize, tempstring, L, tempfile, tmp, tempstring);
		offset += locstring.StringSize;
		byteswritten += _Write(&locstring, sizeof(ErfLocString), tmp, L, tempfile, erf, tempstring);
		free(tempstring);
		tempstring = NULL;
	}

	//...todo

	return 0;
}

int GetResource(lua_State *L){

	ERF * luaerf = (ERF*)lua_toerf(L, 1);
	int key = luaL_checkinteger(L, 2);

	//1 index -> 0 index
	key--;

	if (luaerf->Header->OffsetToResourceList == 0 || luaerf->Header->EntryCount == 0 || key >= luaerf->Header->EntryCount){
		lua_pop(L, 2);
		lua_pushnil(L);
		return 1;
	}

	FILE * file = fopen(luaerf->File, "rb");
	if (!file){
		luaL_error(L, "Unable to open %s to read gff contents", luaerf->File);
	}

	int fileindex = luaerf->Header->OffsetToResourceList + (sizeof(ErfResList) * key);
	if (fseek(file, fileindex, SEEK_SET)){
		fclose(file);
		luaL_error(L, "Unable to seek in file");
	}

	ErfResList node;

	size_t read = fread(&node, 1, sizeof(ErfResList), file);
	if (read != sizeof(ErfResList)){
		fclose(file);
		luaL_error(L, "Unable to read localized strings from file");
	}

	if (fseek(file, node.OffsetToResource, SEEK_SET)){
		fclose(file);
		luaL_error(L, "Unable to seek in file");
	}

	void * buffer = malloc(node.ResourceSize);
	if (!buffer){
		fclose(file);
		luaL_error(L, "Unable to allocate memory for resource");
	}

	read = fread(buffer, 1, node.ResourceSize, file);
	if (read != node.ResourceSize){
		fclose(file);
		free(buffer);
		luaL_error(L, "Unable to read resource from file");
	}

	lua_pop(L, 2);
	lua_pushlstring(L, (const char*)buffer, node.ResourceSize);

	free(buffer);
	fclose(file);

	return 1;
}

int GetKeys(lua_State *L){
	ERF * luaerf = (ERF*)lua_toerf(L, 1);

	if (luaerf->Header->OffsetToKeyList == 0 || luaerf->Header->EntryCount == 0){
		lua_pop(L, 1);
		lua_newtable(L);
		return 1;
	}

	FILE * file = fopen(luaerf->File, "rb");
	if (!file){
		luaL_error(L, "Unable to open %s to read gff contents", luaerf->File);
	}

	if (fseek(file, luaerf->Header->OffsetToKeyList, SEEK_SET)){
		fclose(file);
		luaL_error(L, "Unable to seek in file");
	}

	unsigned int buffersize = sizeof(ErfKey)*luaerf->Header->EntryCount;
	ErfKey * keys = (ErfKey*)calloc(luaerf->Header->EntryCount, sizeof(ErfKey));
	if (!keys){
		fclose(file);
		luaL_error(L, "Unable to allocate buffer for key list");
	}

	size_t read = fread(keys, 1, buffersize, file);
	if (read != buffersize){
		fclose(file);
		free(keys);
		luaL_error(L, "Unable to read from file");
	}
	else
		fclose(file);

	lua_pop(L, 1);
	lua_createtable(L, 0, luaerf->Header->EntryCount);

	for (int n = 0; n < luaerf->Header->EntryCount; n++){

		lua_createtable(L, 4, 0);

		lua_pushstring(L, "ResRef");
		lua_pushstring(L, NullTerminatedResRef(keys[n].ResRef));
		lua_settable(L, -3);

		lua_pushstring(L, "ResID");
		//1 for 1 index
		lua_pushinteger(L, keys[n].ResID + 1);
		lua_settable(L, -3);

		lua_pushstring(L, "ResType");
		lua_pushinteger(L, keys[n].ResType);
		lua_settable(L, -3);

		lua_pushstring(L, "Unused");
		lua_pushinteger(L, keys[n].Unused);
		lua_settable(L, -3);

		lua_rawseti(L, -2, n + 1);
	}

	free(keys);

	return 1;
}

int GetLocalizedStrings(lua_State *L){

	ERF * luaerf = (ERF*)lua_toerf(L, 1);
	if (luaerf->Header->LocalizedStringSize == 0 || luaerf->Header->LanguageCount == 0){
		lua_pop(L, 1);
		lua_newtable(L);
		return 1;
	}

	FILE * file = fopen(luaerf->File, "rb");
	if (!file){
		luaL_error(L, "Unable to open %s to read gff contents", luaerf->File);
	}

	if (fseek(file, luaerf->Header->OffsetToLocalizedString, SEEK_SET)){
		fclose(file);
		luaL_error(L, "Unable to seek in file");
	}

	unsigned int buffersize = luaerf->Header->LocalizedStringSize;
	unsigned char * buffer = (unsigned char *)malloc(buffersize);
	if (!buffer){
		fclose(file);
		luaL_error(L, "Unable to allocate buffer for localized strings");
	}

	size_t read = fread(buffer, 1, buffersize, file);
	if (read != buffersize){
		fclose(file);
		free(buffer);
		luaL_error(L, "Unable to read localized strings from file");
	}
	else
		fclose(file);

	lua_pop(L, 1);
	lua_createtable(L, 0, luaerf->Header->LanguageCount);

	int next = 0;
	ErfLocString * locstr;
	for (int n = 0; n < luaerf->Header->LanguageCount; n++){

		if (next >= buffersize){
			free(buffer);
			luaL_error(L, "Localized string size outside buffer range!");
		}

		locstr = (ErfLocString*)&buffer[next];

		lua_createtable(L, 2, 0);

		lua_pushstring(L, "LanguageID");
		lua_pushinteger(L, locstr->LanguageID);
		lua_settable(L, -3);

		if (locstr->StringSize >= buffersize){
			free(buffer);
			luaL_error(L, "Localized string size outside buffer range!");
		}

		lua_pushstring(L, "String");
		lua_pushlstring(L, locstr->String, locstr->StringSize);
		lua_settable(L, -3);

		lua_rawseti(L, -2, n + 1);

		next = next + sizeof(ErfLocString) + locstr->StringSize;
	}

	free(buffer);
	return 1;
}

int OpenErf(lua_State *L){
	size_t len;
	const char * filename = luaL_checklstring(L, 1, &len);
	FILE * file = fopen(filename, "rb");
	if (!file){
		lua_pop(L, 1);
		lua_pushnil(L);
		return 1;
	}

	ERFHeader * header = (ERFHeader*)malloc(sizeof(ERFHeader));
	if (!header){
		lua_pop(L, 1);
		lua_pushnil(L);
		return 1;
	}

	size_t read = fread(header, 1, sizeof(ERFHeader), file);
	if (read < sizeof(ERFHeader)){
		fclose(file);
		free(header);
		lua_pop(L, 1);
		lua_pushnil(L);
		return 1;
	}
	fclose(file);

	char * filenamebuffer = (char*)malloc(len + 1);
	if (!filenamebuffer){
		free(header);
		lua_pop(L, 1);
		lua_pushnil(L);
		return 1;
	}

	filenamebuffer[len] = '\0';
	memcpy(filenamebuffer, filename, len);
	lua_pop(L, 1);

	ERF * luaerf = lua_pusherf(L);

	if (!luaerf){
		free(header);
		free(filenamebuffer);
		lua_pop(L, 1);
		lua_pushnil(L);
		return 1;
	}

	luaerf->File = filenamebuffer;
	luaerf->Header = header;

	lua_pushinteger(L, header->EntryCount);
	lua_pushlstring(L, header->FileType, 4);
	lua_pushlstring(L, header->Version, 4);

	return 4;
}

ERF * lua_pusherf(lua_State *L){

	ERF * luaerf = (ERF*)lua_newuserdata(L, sizeof(ERF));
	if (luaerf == NULL)
		luaL_error(L, "Unable to allocate memory for erf userdata");
	luaL_getmetatable(L, LERF);
	lua_setmetatable(L, -2);
	memset(luaerf, 0, sizeof(ERF));
	return luaerf;
}

ERF * lua_toerf(lua_State *L, int index){

	ERF * luaerf = (ERF*)lua_touserdata(L, index);
	if (luaerf == NULL)
		luaL_error(L, "paramter is not a %s", LERF);
	return luaerf;
}

int erf_gc(lua_State *L){

	ERF * luaerf = lua_toerf(L, 1);

	if (luaerf->File){
		free(luaerf->File);
		luaerf->File = NULL;
	}

	if (luaerf->Header){
		free(luaerf->Header);
		luaerf->Header = NULL;
	}

	return 0;
}

int erf_tostring(lua_State *L){

	ERF * luaerf = lua_toerf(L, 1);
	char tim[100];
	sprintf(tim, "Erf: 0x%08X File: %s", luaerf, luaerf->File);
	lua_pushfstring(L, tim);
	return 1;
}