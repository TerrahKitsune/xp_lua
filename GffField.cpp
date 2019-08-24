#include "GffField.h"
#include "GffLabel.h"
#include <string.h>

unsigned int WriteField(lua_State *L, Gff * gff){

	GffField * field = (GffField *)&gff->raw[gff->Header.FieldOffset];
	field = &field[gff->Header.FieldCount];
	unsigned int offset = gff->Header.FieldCount;
	gff->Header.FieldCount++;

	lua_pushstring(L, "Type");
	lua_gettable(L, -2);
	field->Type = (unsigned int)lua_tointeger(L, -1);
	lua_pop(L, 1);

	lua_pushstring(L, "Label");
	lua_gettable(L, -2);
	field->LabelIndex = WriteLabel(L, gff);
	lua_pop(L, 1);

	lua_pushstring(L, "Data");
	lua_gettable(L, -2);
	field->DataOrDataOffset = WriteFieldData(L, gff, field->Type);
	lua_pop(L, 1);

	return offset;
}

unsigned int WriteFieldData(lua_State *L, Gff * gff, unsigned int type){

	unsigned int result = 0;
	unsigned long data;
	double ddata;
	float fdata;
	void * ptr;
	size_t len;
	size_t size;
	const char * string;
	CExoString * exostring;
	ResRef * resref;
	CExoLocString * exolocstring;
	CExoLocStringSubString * exolocsubstring;
	StructList * list;
	int cnt;

	switch (type)
	{
	case 0:
	case 1:
		data = (unsigned int)lua_tointeger(L, -1);
		memcpy(&result, &data, sizeof(unsigned char));
		break;
	case 2:
	case 3:
		data = (unsigned int)lua_tointeger(L, -1);
		memcpy(&result, &data, sizeof(unsigned short));
		break;
	case 4:
	case 5:
		data = (unsigned int)lua_tointeger(L, -1);
		memcpy(&result, &data, sizeof(unsigned int));
		break;
	case 6:
	case 7:
		ptr = &gff->raw[gff->Header.FieldDataOffset + gff->Header.FieldDataCount];
		result = gff->Header.FieldDataCount;
		gff->Header.FieldDataCount += sizeof(long);
		data = (unsigned int)lua_tointeger(L, -1);
		memcpy(ptr, &data, sizeof(long));
		break;
	case 8:
		fdata = (float)lua_tonumber(L, -1);
		memcpy(&result, &fdata, sizeof(float));
		break;
	case 9:
		ptr = &gff->raw[gff->Header.FieldDataOffset + gff->Header.FieldDataCount];
		result = gff->Header.FieldDataCount;
		gff->Header.FieldDataCount += sizeof(double);
		ddata = (unsigned int)lua_tonumber(L, -1);
		memcpy(ptr, &ddata, sizeof(double));
		break;
	case 13:
	case 10:
		exostring = (CExoString*)&gff->raw[gff->Header.FieldDataOffset + gff->Header.FieldDataCount];
		result = gff->Header.FieldDataCount;
		string = lua_tolstring(L, -1, &len);
		exostring->Length = len;
		memcpy(exostring->data, string, len);
		gff->Header.FieldDataCount += (sizeof(CExoString) + len);
		break;
	case 11:
		resref = (ResRef*)&gff->raw[gff->Header.FieldDataOffset + gff->Header.FieldDataCount];
		result = gff->Header.FieldDataCount;
		string = lua_tolstring(L, -1, &len);
		resref->Length = len > RESREF_LENGTH ? RESREF_LENGTH : len;
		memcpy(resref->data, string, len);
		gff->Header.FieldDataCount += (sizeof(unsigned char) + len);
		break;
	case 12:

		exolocstring = (CExoLocString*)&gff->raw[gff->Header.FieldDataOffset + gff->Header.FieldDataCount];
		result = gff->Header.FieldDataCount;
		gff->Header.FieldDataCount += sizeof(CExoLocString);

		size = sizeof(CExoLocString) - sizeof(unsigned int);

		lua_pushstring(L, "StringRef");
		lua_gettable(L, -2);
		if (lua_isinteger(L, -1))
			exolocstring->StringRef = (unsigned int)lua_tointeger(L, -1);
		else
			exolocstring->StringRef = -1;
		lua_pop(L, 1);

		lua_pushstring(L, "Strings");
		lua_gettable(L, -2);
		lua_pushnil(L);

		cnt = 0;
		while (lua_next(L, -2) != 0) {
			if (lua_istable(L, -1)){

				exolocsubstring = (CExoLocStringSubString*)&gff->raw[gff->Header.FieldDataOffset + gff->Header.FieldDataCount];

				lua_pushstring(L, "String");
				lua_gettable(L, -2);
				string = lua_tolstring(L, -1, &len);
				exolocsubstring->StringLength = len;
				lua_pop(L, 1);

				memcpy(exolocsubstring->Data, string, len);

				lua_pushstring(L, "StringID");
				lua_gettable(L, -2);
				if (lua_isinteger(L, -1))
					exolocsubstring->StringID = (int)lua_tointeger(L, -1);
				else
					exolocsubstring->StringID = 0;
				lua_pop(L, 1);

				size += sizeof(CExoLocStringSubString) + len;
				gff->Header.FieldDataCount += sizeof(CExoLocStringSubString) + len;

				cnt++;
			}
			lua_pop(L, 1);
		}

		exolocstring->StringCount = cnt;
		exolocstring->TotalSize = size;

		lua_pop(L, 1);
		break;
	case 14:	
		result = WriteStruct(L, gff);
		break;
	case 15:

		list = (StructList*)&gff->raw[gff->Header.ListIndicesOffset + gff->Header.ListIndicesCount];
		result = gff->Header.ListIndicesCount;
		gff->Header.ListIndicesCount += sizeof(StructList);

		lua_pushnil(L);
		while (lua_next(L, -2) != 0) {
			gff->Header.ListIndicesCount += sizeof(unsigned int);
			lua_pop(L, 1);
		}

		cnt = 0;
		lua_pushnil(L);
		while (lua_next(L, -2) != 0) {
			list->StructIndecies[cnt] = WriteStruct(L, gff);
			cnt++;
			lua_pop(L, 1);
		}

		list->Length = cnt;

		break;
	default:
		break;
	}

	return result;
}

size_t CalculateFieldDataSize(lua_State *L, Gff * gff, int type, const char * label){

	size_t size = 0;
	size_t subsize = 0;
	int indices = 0;

	switch (type)
	{
	case 0:
	case 1:
		if (!lua_isinteger(L, -1)){
			Bail(gff, L, NULL);
			luaL_error(L, "Field type (%d) invalid on field %s", type, label);
		}
		break;
	case 2:
	case 3:
		if (!lua_isinteger(L, -1)){
			Bail(gff, L, NULL);
			luaL_error(L, "Field type (%d) invalid on field %s", type, label);
		}
		break;
	case 4:
	case 5:
		if (!lua_isinteger(L, -1)){
			Bail(gff, L, NULL);
			luaL_error(L, "Field type (%d) invalid on field %s", type, label);
		}
		break;
	case 6:
	case 7:
		size = sizeof(long);
		if (!lua_isinteger(L, -1)){
			Bail(gff, L, NULL);
			luaL_error(L, "Field type (%d) invalid on field %s", type, label);
		}
		gff->Header.FieldDataCount += size;
		break;
	case 8:
		if (!lua_isnumber(L, -1)){
			Bail(gff, L, NULL);
			luaL_error(L, "Field type (%d) invalid on field %s", type, label);
		}
		break;
	case 9:
		size = sizeof(double);
		if (!lua_isnumber(L, -1)){
			Bail(gff, L, NULL);
			luaL_error(L, "Field type (%d) invalid on field %s", type, label);
		}
		gff->Header.FieldDataCount += size;
		break;
	case 10:
		//void data is the same as exostring
	case 13:
		if (!lua_isstring(L, -1)){
			Bail(gff, L, NULL);
			luaL_error(L, "Field type (%d) invalid on field %s", type, label);
		}
		lua_tolstring(L, -1, &size);
		size += sizeof(CExoString);
		gff->Header.FieldDataCount += size;
		break;
	case 11:
		if (!lua_isstring(L, -1)){
			Bail(gff, L, NULL);
			luaL_error(L, "Field type (%d) invalid on field %s", type, label);
		}

		lua_tolstring(L, -1, &size);
		if (size > RESREF_LENGTH)
			size = RESREF_LENGTH;

		size++;
		gff->Header.FieldDataCount += size;
		break;
	case 12:

		if (!lua_istable(L, -1)){
			Bail(gff, L, NULL);
			luaL_error(L, "Field type (%d) invalid on field %s", type, label);
		}

		lua_pushstring(L, "Strings");
		lua_gettable(L, -2);
		if (!lua_istable(L, -1)){
			Bail(gff, L, NULL);
			luaL_error(L, "ExoLocString on field %s is missing table Strings", type, label);
		}

		size = sizeof(CExoLocString);
		lua_pushnil(L);
		while (lua_next(L, -2) != 0) {
			if (lua_istable(L, -1)){
				lua_pushstring(L, "String");
				lua_gettable(L, -2);
				if (!lua_isstring(L, -1)){
					Bail(gff, L, NULL);
					luaL_error(L, "ExoLocString substring on field %s is not a string", type, label);
				}
				lua_tolstring(L, -1, &subsize);
				subsize += sizeof(CExoLocStringSubString);
				size += subsize;
				lua_pop(L, 1);
			}
			lua_pop(L, 1);
		}

		lua_pop(L, 1);
		gff->Header.FieldDataCount += size;
		break;
	case 14:
		if (!lua_istable(L, -1)){
			Bail(gff, L, NULL);
			luaL_error(L, "Field type (%d) invalid on field %s", type, label);
		}

		size = CalculateStructSize(L, gff);
		break;
	case 15:

		if (!lua_istable(L, -1)){
			Bail(gff, L, NULL);
			luaL_error(L, "Field type (%d) invalid on field %s", type, label);
		}

		lua_pushnil(L);
		while (lua_next(L, -2) != 0) {
			if (lua_istable(L, -1)){
				size += CalculateStructSize(L, gff);
				indices++;
			}
			else {
				Bail(gff, L, NULL);
				luaL_error(L, "Non table detected in structlist on field %s", type, label);
			}

			lua_pop(L, 1);
		}

		gff->Header.ListIndicesCount += (sizeof(StructList) + (indices * sizeof(unsigned int)));

		break;
	default:
		Bail(gff, L, NULL);
		luaL_error(L, "Field unknown type %d on field %s", type, label);
		break;
	}

	return size;
}

size_t CalculateFieldSize(lua_State *L, Gff * gff){

	int type = -1;
	size_t size = 0;
	size_t labelsize = 0;
	const char * label = NULL;
	lua_pushstring(L, "Type");
	lua_gettable(L, -2);
	if (!lua_isinteger(L, -1)){
		Bail(gff, L, "Type field missing from field table");
	}
	else
		type = (int)lua_tointeger(L, -1);
	lua_pop(L, 1);

	lua_pushstring(L, "Label");
	lua_gettable(L, -2);
	if (!lua_isstring(L, -1)){
		Bail(gff, L, "Label field missing from field table");
	}
	else
		label = NullTerminatedLabel((GffLabel*)lua_tostring(L, -1));
	lua_pop(L, 1);

	if (StringExist(gff, label, strlen(label)) == -1){
		StringAdd(gff, label, strlen(label), gff->stringcount);
		gff->Header.LabelCount++;
		labelsize = sizeof(GffLabel);
	}

	lua_pushstring(L, "Data");
	lua_gettable(L, -2);
	if (lua_isnoneornil(L, -1)){
		Bail(gff, L, "Data field missing from field table");
	}
	else
		size = CalculateFieldDataSize(L, gff, type, label);
	lua_pop(L, 1);

	gff->Header.FieldCount++;

	return sizeof(GffField) + size + labelsize;
}

void PushField(lua_State *L, Gff * gff, unsigned int fieldindex){

	if (fieldindex >= gff->Header.FieldCount){
		Bail(gff, L, "Malformed gff, unable to retrive field outside field count");
	}

	GffField * field = &((GffField*)(&gff->raw[gff->Header.FieldOffset]))[fieldindex];
	lua_createtable(L, 0, 3);

	lua_pushstring(L, "Type");
	lua_pushinteger(L, field->Type);
	lua_settable(L, -3);

	lua_pushstring(L, "Data");
	PushFieldData(L, gff, field);
	lua_settable(L, -3);

	lua_pushstring(L, "Label");
	PushLabel(L, gff, field->LabelIndex);
	lua_settable(L, -3);

	lua_pushstring(L, "gff");
	lua_pushstring(L, "field");
	lua_settable(L, -3);
}

void PushFieldData(lua_State *L, Gff * gff, GffField *gfffield){

	char bytedata;
	short shortdata;
	int intdata;
	float fdata;
	unsigned long uldata;
	long ldata;
	double ddata;
	CExoString * cdata;
	ResRef * rdata;
	CExoLocString * lsdata;

	switch (gfffield->Type)
	{
	case 1:
		memcpy(&bytedata, &gfffield->DataOrDataOffset, sizeof(char));
		lua_pushinteger(L, bytedata);
		break;
	case 3:
		memcpy(&shortdata, &gfffield->DataOrDataOffset, sizeof(short));
		lua_pushinteger(L, shortdata);
		break;
	case 5:
		memcpy(&intdata, &gfffield->DataOrDataOffset, sizeof(int));
		lua_pushinteger(L, intdata);
		break;
	case 6:
		CopyFromData(L, gff, gfffield->DataOrDataOffset, &uldata, sizeof(unsigned long));
		lua_pushinteger(L, uldata);
		break;
	case 7:
		CopyFromData(L, gff, gfffield->DataOrDataOffset, &ldata, sizeof(long));
		lua_pushinteger(L, ldata);
		break;
	case 8:
		memcpy(&fdata, &gfffield->DataOrDataOffset, sizeof(float));
		lua_pushnumber(L, fdata);
		break;
	case 9:
		CopyFromData(L, gff, gfffield->DataOrDataOffset, &ddata, sizeof(double));
		lua_pushnumber(L, ddata);
		break;
	case 10:
	case 13:
		cdata = (CExoString*)GetPtrFromData(L, gff, gfffield->DataOrDataOffset, 1);
		if (cdata->Length + gfffield->DataOrDataOffset > gff->Header.FieldDataCount ||
			cdata->Length + gfffield->DataOrDataOffset + gff->Header.FieldDataOffset > gff->size){
			Bail(gff, L, "Length of CExoString is invalid");
		}
		lua_pushlstring(L, cdata->data, cdata->Length);
		break;
	case 11:
		rdata = (ResRef*)GetPtrFromData(L, gff, gfffield->DataOrDataOffset, 1);
		lua_pushlstring(L, rdata->data, rdata->Length > RESREF_LENGTH ? RESREF_LENGTH : rdata->Length);
		break;
	case 12:
		lsdata = (CExoLocString*)GetPtrFromData(L, gff, gfffield->DataOrDataOffset, sizeof(CExoLocString));
		PushCExoLocString(L, lsdata, gff, gfffield->DataOrDataOffset);
		break;
	case 14:
		PushStruct(gff, L, gfffield->DataOrDataOffset);
		break;
	case 15:
		PushStructList(L, gff, gfffield->DataOrDataOffset);
		break;
		//Byte, char, word, short, dword, int or unknown
	default:
		lua_pushinteger(L, gfffield->DataOrDataOffset);
		break;
	}
}

void PushStructList(lua_State *L, Gff * gff, unsigned int offset){

	if (gff->Header.ListIndicesOffset + offset >= gff->size || offset >= gff->Header.ListIndicesCount){
		Bail(gff, L, "GFF Malformed, list indices outside gff range");
	}

	StructList * list = (StructList*)&gff->raw[gff->Header.ListIndicesOffset + offset];

	if (list->Length*sizeof(unsigned int) >= gff->Header.ListIndicesCount){
		Bail(gff, L, "GFF Malformed, indices list length longer then gff indices list");
	}

	lua_createtable(L, list->Length, 0);

	for (unsigned int n = 0; n<list->Length; n++){

		PushStruct(gff, L, list->StructIndecies[n]);
		lua_rawseti(L, -2, n + 1);
	}
}

void PushCExoLocString(lua_State *L, CExoLocString * locstr, Gff * gff, unsigned int originaloffset){

	if (locstr->TotalSize + sizeof(unsigned int) + originaloffset > gff->Header.FieldDataCount ||
		locstr->TotalSize + sizeof(unsigned int) + originaloffset + gff->Header.FieldDataOffset > gff->size){
		Bail(gff, L, "Malformed gff, unable to CExoLocString->TotalSize reaches outside the datafield span");
	}

	CExoLocStringSubString * cursor = (CExoLocStringSubString *)&locstr->Strings;
	int offset;

	lua_createtable(L, 0, 4);

	lua_pushstring(L, "TotalSize");
	lua_pushinteger(L, locstr->TotalSize);
	lua_settable(L, -3);

	lua_pushstring(L, "StringRef");
	lua_pushinteger(L, locstr->StringRef);
	lua_settable(L, -3);

	lua_pushstring(L, "StringCount");
	lua_pushinteger(L, locstr->StringCount);
	lua_settable(L, -3);

	lua_pushstring(L, "Strings");
	lua_createtable(L, locstr->StringCount, 0);

	for (unsigned int n = 0; n < locstr->StringCount; n++){

		if (sizeof(CExoLocStringSubString) + originaloffset > gff->Header.FieldDataCount ||
			sizeof(CExoLocStringSubString) + originaloffset + gff->Header.FieldDataOffset > gff->size){
			Bail(gff, L, "Malformed gff, unable to CExoLocStringSubString");
		}

		lua_createtable(L, 0, locstr->StringCount);

		lua_pushstring(L, "StringID");
		lua_pushinteger(L, cursor->StringID);
		lua_settable(L, -3);

		lua_pushstring(L, "StringLength");
		lua_pushinteger(L, cursor->StringLength);
		lua_settable(L, -3);

		if (cursor->StringLength + sizeof(CExoLocStringSubString) + originaloffset > gff->Header.FieldDataCount ||
			cursor->StringLength + sizeof(CExoLocStringSubString) + originaloffset + gff->Header.FieldDataOffset > gff->size){
			Bail(gff, L, "Malformed gff, unable to CExoLocStringSubString");
		}

		lua_pushstring(L, "String");
		lua_pushlstring(L, cursor->Data, cursor->StringLength);
		lua_settable(L, -3);

		lua_rawseti(L, -2, n + 1);
		offset = (sizeof(unsigned long) * 2) + cursor->StringLength;
		cursor = (CExoLocStringSubString *)&locstr->Strings[offset];
		//originaloffset += offset;
	}

	lua_settable(L, -3);
}

void CopyFromData(lua_State *L, Gff * gff, unsigned offset, void * dst, size_t size){
	memcpy(dst, GetPtrFromData(L, gff, offset, size), size);
}

void * GetPtrFromData(lua_State *L, Gff * gff, unsigned offset, size_t size){

	if (size + offset > gff->Header.FieldDataCount || size + offset + gff->Header.FieldDataOffset > gff->size){
		Bail(gff, L, "Malformed gff, unable to retrive complex field data");
	}

	return &gff->raw[gff->Header.FieldDataOffset + offset];
}