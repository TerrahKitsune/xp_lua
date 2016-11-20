#include "GFFMain.h"
#include "LuaGff.h"

static const struct luaL_Reg cfunctions[] = {
	{ "OpenFile", OpenGffFile },
	{ "OpenString", OpenGffString },
	{ "SaveToFile", SaveGffToFile },
	{ "SaveToString", SaveGffToString },	
	{ NULL, NULL }
}; 

int luaopen_gff(lua_State *L) {
	luaL_newlibtable(L, cfunctions);
	luaL_setfuncs(L, cfunctions, 0);
	return 1;
}