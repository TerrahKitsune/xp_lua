#define _CRT_SECURE_NO_WARNINGS
#include "GffStruct.h"
#include "GffField.h"
#include <string.h>

unsigned int WriteStructFields(lua_State*L, Gff * gff, GffStruct *gffstruct){

	unsigned int result;
	gffstruct->DataOrDataOffset = 0xFFFFFFFF;
	if (gffstruct->FieldCount <= 0){
		gffstruct->FieldCount = 0;
		result = 0xFFFFFFFF;
	}
	else{

		unsigned int * fields = (unsigned int *)&gff->raw[gff->Header.FieldIndicesOffset + gff->Header.FieldIndicesCount];
		result = gff->Header.FieldIndicesCount;
		unsigned int cnt = 0;

		lua_pushnil(L);
		while (lua_next(L, -2) != 0) {
			cnt++;
			lua_pop(L, 1);
		}

		if (gffstruct->FieldCount > 1){
			gff->Header.FieldIndicesCount += (sizeof(unsigned int)*cnt);
		}
		else if (cnt != gffstruct->FieldCount){
			Bail(gff,L,"FieldCount does not match actual count");
		}

		bool single = true;
		cnt = 0;
		lua_pushnil(L);
		while (lua_next(L, -2) != 0) {
			if (gffstruct->FieldCount == 1 && single){
				result = WriteField(L, gff);
				single = false;
			}
			else{
				fields[cnt++] = WriteField(L, gff);
			}
			lua_pop(L, 1);
		}
	}

	return result;
}

unsigned int WriteStruct(lua_State*L, Gff* gff){

	GffStruct * gffstruct = (GffStruct *)&gff->raw[gff->Header.StructOffset];
	gffstruct = &gffstruct[gff->Header.StructCount];
	int result = gff->Header.StructCount;
	gff->Header.StructCount++;

	lua_pushstring(L, "FieldCount");
	lua_gettable(L, -2);
	gffstruct->FieldCount = (unsigned int)lua_tointeger(L, -1);
	lua_pop(L, 1);

	lua_pushstring(L, "Type");
	lua_gettable(L, -2);
	gffstruct->Type = (unsigned int)lua_tointeger(L, -1);
	lua_pop(L, 1);

	lua_pushstring(L, "Fields");
	lua_gettable(L, -2);
	gffstruct->DataOrDataOffset = WriteStructFields(L, gff, gffstruct);
	lua_pop(L, 1);

	return result;
}

size_t CalculateStructFields(lua_State*L, Gff* gff){

	size_t result = 0;
	int FieldIndices = 0;
	lua_pushnil(L);
	while (lua_next(L, -2) != 0) {
		if (lua_istable(L, -1)){
			//printf("%s -> 0x%08X\n", lua_tostring(L, -2), lua_topointer(L, -1));
			result += CalculateFieldSize(L, gff);
			FieldIndices++;
		}
		else
			Bail(gff, L, "Non table detected in field list");

		lua_pop(L, 1);
	}

	if (FieldIndices > 1){
		gff->Header.FieldIndicesCount += (FieldIndices*sizeof(unsigned int));
		result += (FieldIndices*sizeof(unsigned int));
	}

	//POP FIELDS OF AND SET OUR COUNT
	lua_pop(L, 1);

	//Reset the field count to the actual field count
	lua_pushstring(L, "FieldCount");
	lua_pushinteger(L, FieldIndices);
	lua_settable(L, -3);

	return result;
}

size_t CalculateStructSize(lua_State*L, Gff* gff){

	if (!lua_istable(L, -1)){
		Bail(gff, L, "No struct table on stack top");
	}

	int nSize = TrackCount(gff);
	lua_checkstack(L, nSize * 2);

	const void * tbl = lua_topointer(L, -1);
	TrackOrBail(L, gff, tbl);

	//lua_pushstring(L, "FieldCount");
	//lua_gettable(L, -2);
	//if (!lua_isinteger(L, -1)){
	//	Bail(gff, L, "FieldCount field missing from table");
	//}
	//lua_pop(L, 1);

	lua_pushstring(L, "Type");
	lua_gettable(L, -2);
	if (!lua_isinteger(L, -1)){
		Bail(gff, L, "Type field missing from struct");
	}
	lua_pop(L, 1);

	lua_pushstring(L, "Fields");
	lua_gettable(L, -2);
	if (!lua_istable(L, -1)){
		Bail(gff, L, "Fields missing from struct");
	}
	size_t size = CalculateStructFields(L, gff);
	//NO POP

	gff->Header.StructCount++;

	size += sizeof(GffStruct);

	UntrackOrBail(L, gff, tbl);

	return size;
}

size_t CalculateTopLevelStructSize(lua_State*L, Gff* gff){

	if (!lua_istable(L, -1)){
		Bail(gff, L, "Parameter is not a table");
	}

	lua_pushstring(L, "FileType");
	lua_gettable(L, -2);
	if (!lua_isstring(L, -1)){
		Bail(gff, L, "FileType field missing from topstruct");
	}
	strncpy(gff->Header.FileType, lua_tostring(L, -1), 4);
	lua_pop(L, 1);

	lua_pushstring(L, "FileVersion");
	lua_gettable(L, -2);
	if (!lua_isstring(L, -1)){
		Bail(gff, L, "FileVersion field missing from topstruct");
	}
	strncpy(gff->Header.FileVersion, lua_tostring(L, -1), 4);
	lua_pop(L, 1);

	return CalculateStructSize(L, gff);
}

void PushStructFields(Gff * gff, lua_State *L, GffStruct *gffstruct){

	if (gffstruct->FieldCount <= 0){
		lua_newtable(L);
	}
	else if (gffstruct->FieldCount == 1){
		lua_createtable(L, 0, 1);
		PushField(L, gff, gffstruct->DataOrDataOffset);
		lua_rawseti(L, -2, 1);
	}
	else{
		lua_createtable(L, 0, gffstruct->FieldCount);

		if (gff->Header.FieldIndicesOffset + gffstruct->DataOrDataOffset >= gff->size ||
			gffstruct->DataOrDataOffset >= gff->Header.FieldIndicesCount){
			Bail(gff, L, "GFF Malformed, field indices outside data range");
		}

		unsigned int * fields = (unsigned int *)(&gff->raw[gff->Header.FieldIndicesOffset + gffstruct->DataOrDataOffset]);

		for (unsigned int n = 0; n < gffstruct->FieldCount; n++){
			PushField(L, gff, fields[n]);
			lua_rawseti(L, -2, n + 1);
		}
	}
}

void PushTopLevelStruct(Gff * gff, lua_State *L){

	lua_createtable(L, 0, 5);

	lua_pushstring(L, "FileType");
	lua_pushlstring(L, gff->Header.FileType, 4);
	lua_settable(L, -3);

	lua_pushstring(L, "FileVersion");
	lua_pushlstring(L, gff->Header.FileVersion, 4);
	lua_settable(L, -3);

	GffStruct * top = (GffStruct *)SetGffPointer(sizeof(GffStruct), gff, gff->Header.StructOffset);
	if (top == NULL){
		Bail(gff, L, "GFF Malformed, unable to retrive top-struct");
	}

	TrackOrBail(L, gff, top);

	lua_pushstring(L, "FieldCount");
	lua_pushinteger(L, top->FieldCount);
	lua_settable(L, -3);

	lua_pushstring(L, "Type");
	lua_pushinteger(L, top->Type);
	lua_settable(L, -3);

	lua_pushstring(L, "Fields");
	PushStructFields(gff, L, top);
	lua_settable(L, -3);

	lua_pushstring(L, "gff");
	lua_pushstring(L, "topstruct");
	lua_settable(L, -3);

	UntrackOrBail(L, gff, top);
}

void PushStruct(Gff * gff, lua_State *L, unsigned int index){

	int nSize = TrackCount(gff);
	lua_checkstack(L, nSize * 2);
	lua_createtable(L, 0, 3);

	GffStruct * top = GetStruct(gff, L, index);
	TrackOrBail(L, gff, top);
	if (top == NULL){
		Bail(gff, L, "GFF Malformed, unable to retrive top-struct");
	}

	lua_pushstring(L, "FieldCount");
	lua_pushinteger(L, top->FieldCount);
	lua_settable(L, -3);

	lua_pushstring(L, "Type");
	lua_pushinteger(L, top->Type);
	lua_settable(L, -3);

	lua_pushstring(L, "Fields");
	PushStructFields(gff, L, top);
	lua_settable(L, -3);

	lua_pushstring(L, "gff");
	lua_pushstring(L, "struct");
	lua_settable(L, -3);

	UntrackOrBail(L, gff, top);
}

GffStruct * GetStruct(Gff * gff, lua_State *L, unsigned int index){

	if (index >= gff->Header.StructCount || gff->Header.StructOffset + (sizeof(GffStruct)*index) >= gff->size){
		Bail(gff, L, "GFF Malformed, struct index out of bounds");
	}

	return (GffStruct *)&gff->raw[gff->Header.StructOffset + (sizeof(GffStruct)*index)];
}