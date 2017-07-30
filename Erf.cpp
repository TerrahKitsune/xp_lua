#include "Erf.h"
#include <string.h>
#include <stdlib.h>
#include <Windows.h>
#include "ResourcesTypes.h"
#include <time.h>

static char nullref[33];
static const char * NullTerminatedResRef(const char * resref, int version){

	if (version == 2){
		nullref[32] = '\0';
		strncpy(nullref, resref, 32);
		return nullref;
	}

	nullref[16] = '\0';
	strncpy(nullref, resref, 16);
	return nullref;
}

void * CreateKeyList(size_t len, int version, ERFHeader * header){

	if (version == 1){
		header->OffsetToResourceList = header->OffsetToKeyList + (len * sizeof(ErfKey));
		return calloc(len, sizeof(ErfKey));
	}
	else{
		header->OffsetToResourceList = header->OffsetToKeyList + (len * sizeof(ErfKeyV2));
		return calloc(len, sizeof(ErfKeyV2));
	}
}

void GetResRef(char * buffer, const char * file, int lmax){

	const char * start = file;
	size_t initlen = strlen(file);
	size_t len = 0;

	for (size_t n = 0; n < initlen; n++){
		if (file[n] == '/' &&  n + 1 < initlen){
			start = &file[n + 1];
			len = strlen(start);
		}
	}

	const char * dot = strstr(start, ".");
	if (dot){
		len = dot - start;
	}

	memcpy(buffer, start, min(len, lmax));
}

void FillKey(void * data, int index, int version, ERFHeader * header, ERFBuildEntry * file, lua_State *L){

	if (version == 1){
		ErfKey * key = &((ErfKey *)data)[index];

		key->ResID = index;
		GetResRef(key->ResRef, file->file, 16);
		key->ResType = lua_resource_getresourceid(L, file->file);
	}
	else{
		ErfKeyV2 * keyv2 = &((ErfKeyV2 *)data)[index];

		keyv2->ResID = index;
		GetResRef(keyv2->ResRef, file->file, 32);
		keyv2->ResType = lua_resource_getresourceid(L, file->file);
	}
}

unsigned int FillResEntry(ErfResList * entry, ERFBuildEntry * file, unsigned int offset){

	entry->OffsetToResource = offset;
	entry->ResourceSize = file->len;

	return file->len;
}

bool WriteToFile(FILE * target, FILE * source, size_t len){

	char buffer[1000];
	size_t total = 0;
	size_t read;

	while (total < len){

		read = fread(buffer, sizeof(char), min(1000, len - total), source);

		if (read <= 0)
			return false;

		fwrite(buffer, sizeof(char), read, target);
		fflush(target);
		total += read;
	}

	return true;
}

int CreateErf(lua_State *L){

	size_t len;
	size_t typelen;
	size_t desclen = 3;
	const char * file = luaL_checklstring(L, 1, &len);
	const char * filetype = luaL_checklstring(L, 2, &typelen);
	int version = luaL_optinteger(L, 4, 1);
	const char * desc = luaL_optlstring(L, 5, "LUA", &desclen);

	luaL_checktype(L, 3, LUA_TTABLE);
	lua_pushvalue(L, 3);

	FILE * raw = fopen(file, "wb");

	if (!raw){
		luaL_error(L, "Unable to create file %s", file);
	}

	ERF erf;
	memset(&erf, 0, sizeof(ERF));

	erf.File = (char*)calloc(len + 1, sizeof(char));
	memcpy(erf.File, file, len);

	erf.Header = (ERFHeader*)calloc(1, sizeof(ERFHeader));

	if (version == 1){
		memcpy(erf.Header->Version, "V1.0", 4);
		erf.version = 1;
	}
	else{
		memcpy(erf.Header->Version, "V1.1", 4);
		erf.version = 2;
	}

	memcpy(erf.Header->FileType, filetype, min(typelen, 4));

	time_t rawtime;
	struct tm * timeinfo;

	time(&rawtime);
	timeinfo = localtime(&rawtime);

	erf.Header->BuildYear = timeinfo->tm_year;
	erf.Header->BuildDay = timeinfo->tm_yday;

	lua_pushnil(L);
	while (lua_next(L, -2) != 0) {

		if (lua_isstring(L, -1)){
			erf.Header->EntryCount++;
		}

		lua_pop(L, 1);
	}

	ERFBuildEntry * entries = (ERFBuildEntry*)calloc(erf.Header->EntryCount, sizeof(ERFBuildEntry));
	if (!entries){

		free(erf.Header);
		free(erf.File);
		fclose(raw);
		remove(file);
		luaL_error(L, "Unable to allocate memory");
		return 0;
	}

	size_t lflen;
	const char * lf;

	bool fail = false;

	int n = 0;
	lua_pushnil(L);
	while (lua_next(L, -2) != 0) {

		if (lua_isstring(L, -1)){

			lf = lua_tolstring(L, -1, &lflen);

			if (!lf || lflen >= 260){

				fail = true;
				break;
			}
			else{
				for (size_t i = 0; i < lflen; i++){
					if (lf[i] == '\\')
						entries[n].file[i] = '/';
					else
						entries[n].file[i] = lf[i];
				}
			}

			entries[n].f = fopen(entries[n].file, "rb");

			if (!entries[n].f){
				fail = true;
				break;
			}

			fseek(entries[n].f, 0, SEEK_END);
			entries[n].len = ftell(entries[n].f);
			rewind(entries[n].f);

			n++;
		}

		lua_pop(L, 1);
	}

	void * keylist = NULL;
	ErfResList * reslit = (ErfResList *)calloc(erf.Header->EntryCount, sizeof(ErfResList));
	ErfLocString * locstr = (ErfLocString *)calloc(1, sizeof(ErfLocString) + desclen + 1);
	if (locstr && reslit){

		memcpy(locstr->String, desc, desclen);
		locstr->LanguageID = 0;
		locstr->StringSize = desclen + 1;

		erf.Header->LanguageCount = 1;
		erf.Header->LocalizedStringSize = sizeof(ErfLocString) + desclen + 1;
		erf.Header->OffsetToLocalizedString = sizeof(ERFHeader);
		erf.Header->OffsetToKeyList = erf.Header->OffsetToLocalizedString + erf.Header->LocalizedStringSize;

		keylist = CreateKeyList(erf.Header->EntryCount, erf.version, erf.Header);

		unsigned int offset = erf.Header->OffsetToResourceList + (sizeof(ErfResList) * erf.Header->EntryCount);
	
		if (keylist){
			for (unsigned int i = 0; i < erf.Header->EntryCount; i++){
				FillKey(keylist, i, erf.version, erf.Header, &entries[i], L);
				offset += FillResEntry(&reslit[i], &entries[i], offset);
			}

			fwrite(erf.Header, sizeof(ERFHeader), 1, raw);
			fflush(raw);

			fwrite(locstr, erf.Header->LocalizedStringSize, 1, raw);
			fflush(raw);

			fwrite(keylist, erf.Header->OffsetToResourceList - erf.Header->OffsetToKeyList, 1, raw);
			fflush(raw);

			fwrite(reslit, erf.Header->EntryCount * sizeof(ErfResList), 1, raw);
			fflush(raw);

			for (unsigned int i = 0; i < erf.Header->EntryCount; i++){
				if (!WriteToFile(raw, entries[i].f, entries[i].len)){
					fail = true;
					break;
				}
			}

			if (!fail){
				lua_pop(L, lua_gettop(L));
				ERF * lerf = lua_pusherf(L);
				if (!lerf)
					fail = true;
				else
					memcpy(lerf, &erf, sizeof(ERF));				
			}
		}
		else
			fail = true;
	}
	else{
		fail = true;
	}

	if (fail){

		for (int i = 0; i < n; i++){
			fclose(entries[i].f);
		}

		if (locstr)
			free(locstr);

		if (keylist)
			free(keylist);

		free(reslit);
		free(erf.Header);
		free(erf.File);
		free(entries);
		fclose(raw);
		remove(file);
		luaL_error(L, "Unable to open target file for reading");
		return 0;
	}

	for (unsigned int i = 0; i < erf.Header->EntryCount; i++){
		fclose(entries[i].f);
	}
	free(entries);
	free(locstr);
	free(reslit);

	fclose(raw);

	return 1;
}

int ExtractErf(lua_State *L){

	ERF * luaerf = (ERF*)lua_toerf(L, 1);
	int key = luaL_checkinteger(L, 2);
	const char * path = luaL_checkstring(L, 3);

	//1 index -> 0 index
	key--;

	if (luaerf->Header->OffsetToResourceList == 0 || luaerf->Header->EntryCount == 0 || key >= luaerf->Header->EntryCount){
		luaL_error(L, "Container is invalid");
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

	FILE * target = fopen(path, "wb");
	if (!target){
		fclose(file);
		luaL_error(L, "Unable to create file %s", path);
	}

	char buffer[100];
	unsigned int total = 0;
	unsigned int bread;
	while (total < node.ResourceSize){

		bread = fread(buffer, 1, min(node.ResourceSize - total, 100), file);
		if (bread <= 0){
			fclose(file);
			fclose(target);
			remove(path);
			luaL_error(L, "Unable to read source file");
		}
		else
			total += bread;

		fwrite(buffer, 1, bread, target);
	}

	fflush(target);
	fclose(target);
	fclose(file);

	lua_pop(L, 3);

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

	unsigned int buffersize = 0;
	ErfKey * keys = NULL;
	ErfKeyV2 * keysv2 = NULL;
	void * readinto;
	if (luaerf->version == 1){
		buffersize = sizeof(ErfKey)*luaerf->Header->EntryCount;
		keys = (ErfKey*)calloc(luaerf->Header->EntryCount, sizeof(ErfKey));
		if (!keys){
			fclose(file);
			luaL_error(L, "Unable to allocate buffer for key list");
		}
		readinto = keys;
	}
	else {
		buffersize = sizeof(ErfKeyV2)*luaerf->Header->EntryCount;
		keysv2 = (ErfKeyV2*)calloc(luaerf->Header->EntryCount, sizeof(ErfKeyV2));
		if (!keysv2){
			fclose(file);
			luaL_error(L, "Unable to allocate buffer for key list");
		}
		readinto = keysv2;
	}

	size_t read = fread(readinto, 1, buffersize, file);
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

		lua_createtable(L, 0, 4);

		if (luaerf->version == 1){

			lua_pushstring(L, "ResRef");
			lua_pushstring(L, NullTerminatedResRef(keys[n].ResRef, 1));
			lua_settable(L, -3);

			lua_pushstring(L, "ResID");
			//1 for 1 index
			lua_pushinteger(L, keys[n].ResID + 1);
			lua_settable(L, -3);

			lua_pushstring(L, "ResType");
			lua_pushinteger(L, keys[n].ResType);
			lua_settable(L, -3);

			lua_pushstring(L, "File");
			lua_pushfstring(L, "%s.%s", NullTerminatedResRef(keys[n].ResRef, 2), lua_resource_getextension(L, keys[n].ResType, "bin"));
			lua_settable(L, -3);

			lua_pushstring(L, "Unused");
			lua_pushinteger(L, keys[n].Unused);
			lua_settable(L, -3);
		}
		else{
			lua_pushstring(L, "ResRef");
			lua_pushstring(L, NullTerminatedResRef(keysv2[n].ResRef, 2));
			lua_settable(L, -3);

			lua_pushstring(L, "ResID");
			//1 for 1 index
			lua_pushinteger(L, keysv2[n].ResID + 1);
			lua_settable(L, -3);

			lua_pushstring(L, "ResType");
			lua_pushinteger(L, keysv2[n].ResType);
			lua_settable(L, -3);

			lua_pushstring(L, "File");
			lua_pushfstring(L, "%s.%s", NullTerminatedResRef(keysv2[n].ResRef, 2), lua_resource_getextension(L, keysv2[n].ResType, "bin"));
			lua_settable(L, -3);

			lua_pushstring(L, "Unused");
			lua_pushinteger(L, keysv2[n].Unused);
			lua_settable(L, -3);
		}

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
	int version;
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

	if (tolower(header->Version[0]) == 'v' &&
		tolower(header->Version[1]) == '1' &&
		tolower(header->Version[2]) == '.' &&
		tolower(header->Version[3]) == '0')
	{
		version = 1;
	}
	else if (tolower(header->Version[0]) == 'v' &&
		tolower(header->Version[1]) == '1' &&
		tolower(header->Version[2]) == '.' &&
		tolower(header->Version[3]) == '1')
	{
		version = 2;
	}
	else{
		free(header);
		lua_pop(L, 1);
		lua_pushnil(L);
		return 1;
	}


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
	luaerf->version = version;

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