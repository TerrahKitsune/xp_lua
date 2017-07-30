#include "NWN2_API.h"


CExoString::CExoString() {
	text = NULL;
	len = 0;
}

/*CExoString::CExoString(CExoString const &Source) {

	CNWNXMemory mem;

	if (Source.text && strlen(Source.text) > 0) {
		len = strlen(Source.text) + 1;
		text = (char*)mem.nwnx_malloc(len);
		strcpy_s(text, len, Source.text);
	}
	else {
		text = NULL;
		len = 0;
	}
}

CExoString::CExoString(char const *Source) {

	CNWNXMemory mem;

	if (Source && strlen(Source) > 0) {
		len = strlen(Source) + 1;
		text = (char*)mem.nwnx_malloc(len);
		strcpy_s(text, len, Source);
	}
	else {
		text = NULL;
		len = 0;
	}
}

CExoString * CExoString::CExoStringCpy(char const *Source) {

	CNWNXMemory mem;

	if (text){
		mem.nwnx_free(text);
		text = NULL;
	}


	if (Source && strlen(Source) > 0) {
		len = strlen(Source);
		text = (char*)mem.nwnx_malloc(len + 1);
		strcpy(text, Source);
	}
	else {
		text = NULL;
		len = 0;
	}
	return this;
}

CExoString::CExoString(char const *Source, int Length) {

	CNWNXMemory mem;

	if (Length > 0) {
		len = Length + 1;
		text = (char*)mem.nwnx_malloc(len);
		strncpy_s(text, len, Source, Length);
	}
	else {
		text = NULL;
		len = 0;
	}
}

CExoString::CExoString(int Number) {
	CNWNXMemory mem;

	char c[32];
	sprintf_s(c, 31, "%i", Number);
	len = strlen(c) + 1;
	text = (char*)mem.nwnx_malloc(len);
	strcpy_s(text, len, c);
}

CExoString::~CExoString() {

	CNWNXMemory mem;

	if (text) {
		len = 0;
		mem.nwnx_free(text);
		text = NULL;
	}
}*/