#include "image.h"
#include "LuaImageMain.h"

static const struct luaL_Reg namedpipefunctions[] = {
	{ "Screenshot", lua_screenshot},
	{ "Save", lua_savetofile },
	{ "Load", lua_loadfromfile },
	{ "Create", lua_createimage },
	{ "Crop", lua_crop },
	{ "GetPixel", lua_getpixel },
	{ "SetPixel", lua_setpixel },
	{ "GetPixels", lua_getpixels },
	{ "SetPixels", lua_setpixels },
	{ "GetPixelMatrix", lua_getpixelmatrix },
	{ "SetPixelMatrix", lua_setpixelmatrix },
	{ "GetSize", lua_getsize },
	{ "Close", image_gc },
	{ NULL, NULL }
};

static const luaL_Reg namedpipemeta[] = {
	{ "__gc",  image_gc },
	{ "__tostring",  image_tostring },
	{ NULL, NULL }
};

int luaopen_image(lua_State *L) {

	luaL_newlibtable(L, namedpipefunctions);
	luaL_setfuncs(L, namedpipefunctions, 0);

	luaL_newmetatable(L, IMAGE);
	luaL_setfuncs(L, namedpipemeta, 0);

	lua_pushliteral(L, "__index");
	lua_pushvalue(L, -3);
	lua_rawset(L, -3);
	lua_pushliteral(L, "__metatable");
	lua_pushvalue(L, -3);
	lua_rawset(L, -3);

	lua_pop(L, 1);
	return 1;
}