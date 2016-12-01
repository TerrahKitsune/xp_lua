#define _CRT_SECURE_NO_WARNINGS
#include "LuaGff.h"
#include "GffHeader.h"
#include <stdlib.h> 
#include <string.h>
#include "GffStruct.h"
#include "GffField.h"
#include "GffLabel.h"

int OpenGffFile(lua_State *L){

	const char * file = luaL_checkstring(L, 1);
	FILE * pFile = fopen(file, "rb");

	if (pFile == NULL){
		luaL_error(L, "%s could not be opened", file);
		return 0;
	}

	fseek(pFile, 0L, SEEK_END);
	long size = ftell(pFile);
	rewind(pFile);

	//Ensure size larger the minimum gffsize
	if (size <= sizeof(GffHeader) + (sizeof(unsigned int) * 2) + 1){
		luaL_error(L, "%s is not large enough to be a valid gff", file);
		return 0;
	}

	Gff* gff = (Gff*)malloc(sizeof(Gff));

	if (gff == NULL){
		luaL_error(L, "unable to allocate %ld bytes for %s", size, file);
		return 0;
	}

	memset(gff, 0, sizeof(Gff));
	gff->raw = (unsigned char*)malloc(size);

	if (gff == NULL){
		luaL_error(L, "unable to allocate %ld bytes for %s", size, file);
		return 0;
	}

	gff->size = size;

	if (fread(gff->raw, 1, size, pFile) != size){
		free(gff->raw);
		free(gff);
		fclose(pFile);
		luaL_error(L, "unable to read %s", file);
		return 0;
	}
	else{
		memcpy(&gff->Header, gff->raw, sizeof(GffHeader));
		fclose(pFile);
	}

	lua_pop(L, 1);

	PushTopLevelStruct(gff, L);

	if (gff->gfftracker != NULL){
		Bail(gff, L, "Gff Malformed, structs still in tracker");
	}
	else{
		free(gff->raw);
		free(gff);
	}

	return 1;
}

int OpenGffString(lua_State *L){

	size_t size;
	const char * binary = luaL_tolstring(L, 1, &size);

	//Ensure size larger the minimum gffsize
	if (binary == NULL || size <= sizeof(GffHeader) + (sizeof(unsigned int) * 2) + 1){
		luaL_error(L, "String is not large enough to be a valid gff");
		return 0;
	}

	Gff* gff = (Gff*)malloc(sizeof(Gff));

	if (gff == NULL){
		luaL_error(L, "unable to allocate %ld bytes for gff", size);
		return 0;
	}

	memset(gff, 0, sizeof(Gff));
	gff->raw = (unsigned char*)malloc(size);

	if (gff == NULL){
		luaL_error(L, "unable to allocate %ld bytes", size);
		return 0;
	}

	gff->size = size;
	memcpy(gff->raw, binary, size);
	memcpy(&gff->Header, gff->raw, sizeof(GffHeader));
	lua_pop(L, 2);
	PushTopLevelStruct(gff, L);
	if (gff->gfftracker != NULL){
		Bail(gff, L, "Gff Malformed, structs still in tracker");
	}
	else{
		/*FILE * f = fopen("header.txt", "w");
		fprintf(f, "%u\n%u\n%u\n%u\n%u\n%u\n%u\n", gff->size, gff->Header.StructCount, gff->Header.FieldCount, gff->Header.LabelCount, gff->Header.FieldDataCount, gff->Header.FieldIndicesCount, gff->Header.ListIndicesCount);
		fprintf(f, "\n%u\n%u\n%u\n%u\n%u\n%u\n%u\n", gff->size, gff->Header.StructOffset, gff->Header.FieldOffset, gff->Header.LabelOffset, gff->Header.FieldDataOffset, gff->Header.FieldIndicesOffset, gff->Header.ListIndicesOffset);
		fclose(f);*/
		UntrackAll(gff);
		StringClear(gff);
		free(gff->raw);
		free(gff);
	}

	return 1;
}

static char PATH[_MAX_PATH + 1];
int SaveGffToFile(lua_State *L){

	const char * file = luaL_checkstring(L, -1);

	memset(PATH, 0, _MAX_PATH + 1);
	strncpy(PATH, file, _MAX_PATH);

	Gff * gff = (Gff*)malloc(sizeof(Gff));
	if (gff == NULL){
		luaL_error(L, "Unable to allocate memory for gff");
		return 0;
	}
	lua_pop(L, 1);

	memset(gff, 0, sizeof(Gff));
	CalculateTopLevelStructSize(L, gff);
	
	gff->Header.StructOffset = sizeof(GffHeader);
	gff->Header.FieldOffset = (sizeof(GffStruct) * gff->Header.StructCount) + gff->Header.StructOffset;
	gff->Header.LabelOffset = (sizeof(GffField) * gff->Header.FieldCount) + gff->Header.FieldOffset;
	gff->Header.FieldDataOffset = (sizeof(GffLabel)*gff->Header.LabelCount) + gff->Header.LabelOffset;
	gff->Header.FieldIndicesOffset = gff->Header.FieldDataCount + gff->Header.FieldDataOffset;
	gff->Header.ListIndicesOffset = gff->Header.FieldIndicesOffset + gff->Header.FieldIndicesCount;

	gff->size = gff->Header.ListIndicesOffset + gff->Header.ListIndicesCount;
	gff->raw = (unsigned char*)malloc(gff->size);
	if (gff->raw == NULL){
		Bail(gff, L, "Unable to allocate memory for gff");
	}

	//GffHeader old;
	//memcpy(&old, &gff->Header, sizeof(GffHeader));

	gff->Header.StructCount = 0;
	gff->Header.FieldCount = 0;
	gff->Header.LabelCount = 0;
	gff->Header.FieldDataCount = 0;
	gff->Header.FieldIndicesCount = 0;
	gff->Header.ListIndicesCount = 0;

	WriteStruct(L, gff);
	memcpy(gff->raw, &gff->Header, sizeof(GffHeader));

	//if (gff->Header.StructCount != old.StructCount)
	//	printf("StructCount missmatch %d - %d\n", gff->Header.StructCount, old.StructCount);

	//if (gff->Header.FieldCount != old.FieldCount)
	//	printf("FieldCount missmatch %d - %d\n", gff->Header.FieldCount, old.FieldCount);

	//if (gff->Header.LabelCount != old.LabelCount)
	//	printf("LabelCount missmatch %d - %d\n", gff->Header.LabelCount, old.LabelCount);

	//if (gff->Header.FieldDataCount != old.FieldDataCount)
	//	printf("FieldDataCount missmatch %d - %d\n", gff->Header.FieldDataCount, old.FieldDataCount);

	//if (gff->Header.FieldIndicesCount != old.FieldIndicesCount)
	//	printf("FieldIndicesCount missmatch %d - %d\n", gff->Header.FieldIndicesCount, old.FieldIndicesCount);

	//if (gff->Header.ListIndicesCount != old.ListIndicesCount)
	//	printf("ListIndicesCount missmatch %d - %d\n", gff->Header.ListIndicesCount, old.ListIndicesCount);

	FILE * target = fopen(PATH, "wb");
	if (target == NULL){
		Bail(gff, L, "Unable to open target file to writing");
	}

	fwrite(gff->raw, 1, gff->size, target);
	fclose(target);
	UntrackAll(gff);
	StringClear(gff);
	free(gff->raw);
	free(gff);

	lua_pop(L, 1);
	return 0;
}

int SaveGffToString(lua_State *L){

	Gff * gff = (Gff*)malloc(sizeof(Gff));
	if (gff == NULL){
		luaL_error(L, "Unable to allocate memory for gff");
		return 0;
	}

	memset(gff, 0, sizeof(Gff));
	CalculateTopLevelStructSize(L, gff);

	gff->Header.StructOffset = sizeof(GffHeader);
	gff->Header.FieldOffset = (sizeof(GffStruct) * gff->Header.StructCount) + gff->Header.StructOffset;
	gff->Header.LabelOffset = (sizeof(GffField) * gff->Header.FieldCount) + gff->Header.FieldOffset;
	gff->Header.FieldDataOffset = (sizeof(GffLabel)*gff->Header.LabelCount) + gff->Header.LabelOffset;
	gff->Header.FieldIndicesOffset = gff->Header.FieldDataCount + gff->Header.FieldDataOffset;
	gff->Header.ListIndicesOffset = gff->Header.FieldIndicesOffset + gff->Header.FieldIndicesCount;

	gff->size = gff->Header.ListIndicesOffset + gff->Header.ListIndicesCount;
	gff->raw = (unsigned char*)malloc(gff->size);
	if (gff->raw == NULL){
		Bail(gff, L, "Unable to allocate memory for gff");
	}

	gff->Header.StructCount = 0;
	gff->Header.FieldCount = 0;
	gff->Header.LabelCount = 0;
	gff->Header.FieldDataCount = 0;
	gff->Header.FieldIndicesCount = 0;
	gff->Header.ListIndicesCount = 0;

	WriteStruct(L, gff);
	memcpy(gff->raw, &gff->Header, sizeof(GffHeader));

	lua_pop(L, 1);
	lua_pushlstring(L, (const char*)gff->raw, gff->size);

	UntrackAll(gff);
	StringClear(gff);
	free(gff->raw);
	free(gff);

 	return 1;
}