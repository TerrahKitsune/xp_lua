#pragma once
#include "lua_main_incl.h"

static const char * RESOURCELISTKEY = "ResList";

typedef struct res_list_entry {
	int key;
	const char * value;
} res_list_entry;

static const res_list_entry resourcetypes[]={
	{ 0, "res" },
	{ 1, "bmp" },
	{ 3, "tga" },
	{ 4, "wav" },
	{ 6, "plt" },
	{ 7, "ini" },
	{ 8, "bmu" },
	{ 10, "txt" },
	{ 2002, "mdl" },
	{ 2009, "nss" },
	{ 2010, "ncs" },
	{ 2012, "are" },
	{ 2013, "set" },
	{ 2014, "ifo" },
	{ 2015, "bic" },
	{ 2016, "wok" },
	{ 2017, "2da" },
	{ 2022, "txi" },
	{ 2023, "git" },
	{ 2025, "uti" },
	{ 2027, "utc" },
	{ 2029, "dlg" },
	{ 2030, "itp" },
	{ 2032, "utt" },
	{ 2033, "dds" },
	{ 2035, "uts" },
	{ 2036, "ltr" },
	{ 2037, "gff" },
	{ 2038, "fac" },
	{ 2040, "ute" },
	{ 2042, "utd" },
	{ 2044, "utp" },
	{ 2045, "dft" },
	{ 2046, "gic" },
	{ 2047, "gui" },
	{ 2051, "utm" },
	{ 2052, "dwk" },
	{ 2053, "pwk" },
	{ 2056, "jrl" },
	{ 2058, "utw" },
	{ 2060, "ssf" },
	{ 2064, "ndb" },
	{ 2065, "ptm" },
	{ 2066, "ptt" },
	{ 3001, "usc" },
	{ 3002, "trn" },
	{ 3003, "utr" },
	{ 3004, "uen" },
	{ 3005, "ult" },
	{ 3006, "sef" },
	{ 3007, "pfx" },
	{ 3008, "cam" },
	{ 3011, "upe" },
	{ 3015, "pfb" },
	{ 3018, "bbx" },
	{ 3020, "wlk" },
	{ 3021, "xml" },
	{ 3035, "trx" },
	{ 3036, "trn" },
	{ 3037, "trx" },
	{ 4000, "mdb" },
	{ 4002, "spt" },
	{ 4003, "gr2" },
	{ 4004, "fxa" },
	{ 4005, "fxe" },
	{ 4007, "jpg" },
	//THIS MUST BE THE LAST ENTRY, don't change!
	{ 0xFFFF, NULL }
};

const char * lua_resource_getextension(lua_State * L, int key, const char * extdefault=NULL);
int lua_resource_getresourceid(lua_State * L, const char * file, size_t len=0);
int lua_pushresourcelist(lua_State * L);