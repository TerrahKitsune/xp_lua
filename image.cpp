#include "image.h"
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <stdlib.h> 
#include <windows.h> 

static int iterator;

BOOL CALLBACK MonitorEnumProcCallback(_In_  HMONITOR hMonitor, _In_  HDC DevC, _In_  LPRECT lprcMonitor, _In_  LPARAM dwData) {

	LuaImage * image = (LuaImage*)dwData;

	if (++iterator != image->screen) {
		return TRUE;
	}

	MONITORINFO  info;
	info.cbSize = sizeof(MONITORINFO);

	BOOL monitorInfo = GetMonitorInfo(hMonitor, &info);

	if (monitorInfo) {

		if (image->Height == 0) {
			image->Height = info.rcMonitor.bottom - info.rcMonitor.top;
		}

		if (image->Width == 0) {
			image->Width = info.rcMonitor.right - info.rcMonitor.left;
		}

		DWORD FileSize = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER) + (sizeof(RGBTRIPLE) + 1 * (image->Width*image->Height * 4));

		if (image->Data) {
			free(image->Data);
		}

		image->Data = (BYTE*)calloc(FileSize, sizeof(1));

		if (image->Data)
		{
			image->DataSize = FileSize;
		}
		else
		{
			return FALSE;
		}

		PBITMAPFILEHEADER BFileHeader = (PBITMAPFILEHEADER)image->Data;
		PBITMAPINFOHEADER  BInfoHeader = (PBITMAPINFOHEADER)&image->Data[sizeof(BITMAPFILEHEADER)];

		BFileHeader->bfType = 0x4D42; // BM
		BFileHeader->bfSize = sizeof(BITMAPFILEHEADER);
		BFileHeader->bfOffBits = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);

		BInfoHeader->biSize = sizeof(BITMAPINFOHEADER);
		BInfoHeader->biPlanes = 1;
		BInfoHeader->biBitCount = 24;
		BInfoHeader->biCompression = BI_RGB;
		BInfoHeader->biHeight = image->Height;
		BInfoHeader->biWidth = image->Width;

		RGBTRIPLE *Image = (RGBTRIPLE*)&image->Data[sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER)];
		RGBTRIPLE color;

		HDC CaptureDC = CreateCompatibleDC(DevC);
		HBITMAP CaptureBitmap = CreateCompatibleBitmap(DevC, image->Width, image->Height);
		SelectObject(CaptureDC, CaptureBitmap);
		BitBlt(CaptureDC, 0, 0, image->Width, image->Height, DevC, info.rcMonitor.left + image->StartX, info.rcMonitor.top + image->StartY, SRCCOPY | CAPTUREBLT);
		GetDIBits(CaptureDC, CaptureBitmap, 0, image->Height, Image, (LPBITMAPINFO)BInfoHeader, DIB_RGB_COLORS);

		/*DWORD Junk;
		HANDLE FH = CreateFileA(BmpName, GENERIC_WRITE, FILE_SHARE_WRITE, 0, CREATE_ALWAYS, 0, 0);
		WriteFile(FH, BmpFileData, FileSize, &Junk, 0);
		CloseHandle(FH);*/

		return FALSE;
	}

	return TRUE;
}

int lua_setpixels(lua_State *L) {

	LuaImage * img = lua_toimage(L, 1);

	size_t arraylen = lua_rawlen(L, 2);
	size_t imglen = img->Height * img->Width;

	if (arraylen != imglen) {
		lua_pop(L, lua_gettop(L));
		lua_pushnil(L);
		lua_pushstring(L, "Size of image does not match pixel array size");
		return 2;
	}

	PBITMAPFILEHEADER BFileHeader = (PBITMAPFILEHEADER)img->Data;

	RGBTRIPLE * Image = (RGBTRIPLE*)&img->Data[BFileHeader->bfOffBits];
	RGBTRIPLE * rgb;
	size_t n = 0;

	lua_pushnil(L);
	while (lua_next(L, -2) != 0) {

		n = lua_tointeger(L, -2) - 1;

		if (n < 0 || n >= imglen) {
			lua_pop(L, lua_gettop(L));
			lua_pushboolean(L, false);
			lua_pushstring(L, "Invalid array key");
			return 2;
		}

		rgb = &Image[n];

		lua_pushstring(L, "r");
		lua_gettable(L, -2);

		if (lua_isnumber(L, -1)) {
			rgb->rgbtRed = max(min(lua_tonumber(L, -1), 255), 0);
		}
		else {
			lua_pop(L, lua_gettop(L));
			lua_pushboolean(L, false);
			lua_pushstring(L, "Missing pixel value");
			return 2;
		}

		lua_pop(L, 1);
		lua_pushstring(L, "g");
		lua_gettable(L, -2);

		if (lua_isnumber(L, -1)) {
			rgb->rgbtGreen = max(min(lua_tonumber(L, -1), 255), 0);
		}
		else {
			lua_pop(L, lua_gettop(L));
			lua_pushboolean(L, false);
			lua_pushstring(L, "Missing pixel value");
			return 2;
		}

		lua_pop(L, 1);
		lua_pushstring(L, "b");
		lua_gettable(L, -2);

		if (lua_isnumber(L, -1)) {
			rgb->rgbtBlue = max(min(lua_tonumber(L, -1), 255), 0);
		}
		else {
			lua_pop(L, lua_gettop(L));
			lua_pushboolean(L, false);
			lua_pushstring(L, "Missing pixel value");
			return 2;
		}

		lua_pop(L, 2);
	}

	lua_pop(L, lua_gettop(L));
	lua_pushboolean(L, true);

	return 1;
}

int lua_getpixels(lua_State *L) {

	LuaImage * img = lua_toimage(L, 1);

	PBITMAPFILEHEADER BFileHeader = (PBITMAPFILEHEADER)img->Data;

	RGBTRIPLE * Image = (RGBTRIPLE*)&img->Data[BFileHeader->bfOffBits];
	RGBTRIPLE * rgb;
	size_t len = img->Height * img->Width;

	lua_pop(L, lua_gettop(L));

	lua_createtable(L, len, 0);

	for (size_t n = 0; n < len; n++) {
		rgb = &Image[n];

		lua_createtable(L, 0, 3);

		lua_pushstring(L, "r");
		lua_pushinteger(L, rgb->rgbtRed);
		lua_settable(L, -3);

		lua_pushstring(L, "g");
		lua_pushinteger(L, rgb->rgbtGreen);
		lua_settable(L, -3);

		lua_pushstring(L, "b");
		lua_pushinteger(L, rgb->rgbtBlue);
		lua_settable(L, -3);

		lua_rawseti(L, -2, n + 1);
	}

	return 1;
}

int lua_screenshot(lua_State *L) {

	int x, y, startx, starty;
	x = luaL_optinteger(L, 1, 0);
	y = luaL_optinteger(L, 2, 0);
	startx = luaL_optinteger(L, 3, 0);
	starty = luaL_optinteger(L, 4, 0);
	int screen = luaL_optinteger(L, 5, 1);

	lua_pop(L, lua_gettop(L));

	LuaImage * img = lua_pushimage(L);
	HDC DevC = GetDC(NULL);
	img->screen = screen;
	img->Width = x;
	img->Height = y;
	img->StartX = startx;
	img->StartY = starty;
	iterator = 0;
	BOOL b = EnumDisplayMonitors(DevC, NULL, MonitorEnumProcCallback, (LPARAM)img);

	if (!img->Data) {
		lua_pop(L, 1);
		lua_pushnil(L);
		return 1;
	}

	return 1;
}

int lua_getsize(lua_State *L) {

	LuaImage * img = lua_toimage(L, 1);

	lua_pop(L, lua_gettop(L));

	lua_pushinteger(L, img->Width);
	lua_pushinteger(L, img->Height);

	return 2;
}

int lua_crop(lua_State *L) {

	LuaImage * original = lua_toimage(L, 1);
	int w = luaL_checkinteger(L, 2);
	int h = luaL_checkinteger(L, 3);
	int x = luaL_checkinteger(L, 4);
	int y = luaL_checkinteger(L, 5);

	if (w + x > original->Width || h + y > original->Height || x < 0 || y < 0 || w <= 0 || h <= 0) {

		lua_pop(L, lua_gettop(L));
		lua_pushnil(L);
		lua_pushstring(L, "Crop out of bounds");
		return 2;
	}

	LuaImage * image = lua_pushimage(L);

	DWORD FileSize = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER) + (sizeof(RGBTRIPLE) + 1 * (w*h * 4));
	BYTE * buffer = (BYTE*)calloc(FileSize, sizeof(1));

	if (!buffer) {
		lua_pop(L, lua_gettop(L));
		lua_pushnil(L);
		lua_pushstring(L, "Unable to allocate memory");
		return 2;
	}

	image->Data = buffer;
	image->DataSize = FileSize;
	image->Height = h;
	image->Width = w;

	PBITMAPFILEHEADER BFileHeader = (PBITMAPFILEHEADER)image->Data;
	PBITMAPINFOHEADER  BInfoHeader = (PBITMAPINFOHEADER)&image->Data[sizeof(BITMAPFILEHEADER)];

	BFileHeader->bfType = 0x4D42; // BM
	BFileHeader->bfSize = sizeof(BITMAPFILEHEADER);
	BFileHeader->bfOffBits = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);

	BInfoHeader->biSize = sizeof(BITMAPINFOHEADER);
	BInfoHeader->biPlanes = 1;
	BInfoHeader->biBitCount = 24;
	BInfoHeader->biCompression = BI_RGB;
	BInfoHeader->biHeight = image->Height;
	BInfoHeader->biWidth = image->Width;

	RGBTRIPLE *Original = (RGBTRIPLE*)&original->Data[sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER)];
	RGBTRIPLE *Image = (RGBTRIPLE*)&image->Data[sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER)];

	size_t coord_x, coord_y;
	size_t i = 0;
	size_t n;
	size_t hoffset = (original->Height - h) - y;
	
	for (coord_y = hoffset; coord_y < h + hoffset; coord_y++) {
		for (coord_x = x; coord_x < w + x; coord_x++) {
			n = coord_x + original->Width * coord_y;
			memcpy(&Image[i++], &Original[n], sizeof(RGBTRIPLE));
		}
	}

	//bool ok;
	//for (size_t n = 0; n < original->DataSize; n++) {

	//	coord_y = (n / original->Width);
	//	coord_x = (n % original->Width);

	//	ok = coord_y >= y && coord_y - y < h && coord_x >= x && coord_x - x < w;

	//	//printf("%u | %u = %d\n", coord_x, coord_y, ok);

	//	if (ok) {
	//		memcpy(&Image[i++], &Original[coord_x + original->Width * coord_y], sizeof(RGBTRIPLE));
	//	}
	//}

	lua_copy(L, lua_gettop(L), 1);
	lua_pop(L, lua_gettop(L) - 1);

	return 1;
}

int lua_createimage(lua_State *L) {

	int w = luaL_checkinteger(L, 1);
	int h = luaL_checkinteger(L, 2);

	lua_pop(L, lua_gettop(L));
	LuaImage * image = lua_pushimage(L);

	DWORD FileSize = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER) + (sizeof(RGBTRIPLE) + 1 * (w*h * 4));
	BYTE * buffer = (BYTE*)calloc(FileSize, sizeof(1));

	if (!buffer) {
		lua_pop(L, lua_gettop(L));
		lua_pushnil(L);
		lua_pushstring(L, "Unable to allocate memory");
		return 2;
	}

	image->Data = buffer;
	image->DataSize = FileSize;
	image->Height = h;
	image->Width = w;

	PBITMAPFILEHEADER BFileHeader = (PBITMAPFILEHEADER)image->Data;
	PBITMAPINFOHEADER  BInfoHeader = (PBITMAPINFOHEADER)&image->Data[sizeof(BITMAPFILEHEADER)];

	BFileHeader->bfType = 0x4D42; // BM
	BFileHeader->bfSize = sizeof(BITMAPFILEHEADER);
	BFileHeader->bfOffBits = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);

	BInfoHeader->biSize = sizeof(BITMAPINFOHEADER);
	BInfoHeader->biPlanes = 1;
	BInfoHeader->biBitCount = 24;
	BInfoHeader->biCompression = BI_RGB;
	BInfoHeader->biHeight = image->Height;
	BInfoHeader->biWidth = image->Width;

	return 1;
}

int lua_setpixelmatrix(lua_State *L) {

	LuaImage * img = lua_toimage(L, 1);

	PBITMAPFILEHEADER BFileHeader = (PBITMAPFILEHEADER)img->Data;

	RGBTRIPLE * Image = (RGBTRIPLE*)&img->Data[BFileHeader->bfOffBits];
	RGBTRIPLE * rgb;
	int x, y, i;

	lua_pushnil(L);
	while (lua_next(L, -2) != 0) {

		y = lua_tointeger(L, -2) - 1;

		if (y < 0 || y >= img->Height) {
			lua_pop(L, lua_gettop(L));
			lua_pushboolean(L, false);
			lua_pushstring(L, "Invalid array key");
			return 2;
		}

		lua_pushnil(L);
		while (lua_next(L, -2) != 0) {

			x = lua_tointeger(L, -2) - 1;

			if (x < 0 || x >= img->Width) {
				lua_pop(L, lua_gettop(L));
				lua_pushboolean(L, false);
				lua_pushstring(L, "Invalid array key");
				return 2;
			}

			i = x + img->Width * (img->Height - y - 1);

			rgb = &Image[i];

			lua_pushstring(L, "r");
			lua_gettable(L, -2);

			if (lua_isnumber(L, -1)) {
				rgb->rgbtRed = max(min(lua_tonumber(L, -1), 255), 0);
			}
			else {
				lua_pop(L, lua_gettop(L));
				lua_pushboolean(L, false);
				lua_pushstring(L, "Missing pixel value");
				return 2;
			}

			lua_pop(L, 1);
			lua_pushstring(L, "g");
			lua_gettable(L, -2);

			if (lua_isnumber(L, -1)) {
				rgb->rgbtGreen = max(min(lua_tonumber(L, -1), 255), 0);
			}
			else {
				lua_pop(L, lua_gettop(L));
				lua_pushboolean(L, false);
				lua_pushstring(L, "Missing pixel value");
				return 2;
			}

			lua_pop(L, 1);
			lua_pushstring(L, "b");
			lua_gettable(L, -2);

			if (lua_isnumber(L, -1)) {
				rgb->rgbtBlue = max(min(lua_tonumber(L, -1), 255), 0);
			}
			else {
				lua_pop(L, lua_gettop(L));
				lua_pushboolean(L, false);
				lua_pushstring(L, "Missing pixel value");
				return 2;
			}

			lua_pop(L, 2);
		}

		lua_pop(L, 1);
	}

	lua_pop(L, lua_gettop(L));
	lua_pushboolean(L, true);

	return 1;
}

int lua_getpixelmatrix(lua_State *L) {

	LuaImage * img = lua_toimage(L, 1);

	size_t i = 0;
	PBITMAPFILEHEADER BFileHeader = (PBITMAPFILEHEADER)img->Data;

	RGBTRIPLE * Image = (RGBTRIPLE*)&img->Data[BFileHeader->bfOffBits];
	RGBTRIPLE * rgb;

	lua_createtable(L, img->Height, 0);

	for (int coord_y = img->Height-1; coord_y >= 0; coord_y--) {

		lua_createtable(L, img->Width, 0);

		for (int coord_x = 0; coord_x < img->Width; coord_x++) {
			
			i = coord_x + img->Width * coord_y;

			rgb = &Image[i];

			lua_createtable(L, 0, 3);

			lua_pushstring(L, "r");
			lua_pushinteger(L, rgb->rgbtRed);
			lua_settable(L, -3);

			lua_pushstring(L, "g");
			lua_pushinteger(L, rgb->rgbtGreen);
			lua_settable(L, -3);

			lua_pushstring(L, "b");
			lua_pushinteger(L, rgb->rgbtBlue);
			lua_settable(L, -3);

			lua_rawseti(L, -2, coord_x + 1);
		}

		lua_rawseti(L, -2, (img->Height-coord_y));
	}

	lua_copy(L, lua_gettop(L), 1);
	lua_pop(L, lua_gettop(L) - 1);

	return 1;
}

int lua_loadfromfile(lua_State *L) {

	const char * file = luaL_checkstring(L, 1);
	HANDLE FH = CreateFileA(file, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, 0, 0);

	if (FH == INVALID_HANDLE_VALUE) {

		lua_pop(L, lua_gettop(L));
		lua_pushboolean(L, false);
		lua_pushstring(L, "Unable to open file");
		return 2;
	}

	DWORD size = GetFileSize(FH, NULL);

	if (size == INVALID_FILE_SIZE) {
		lua_pop(L, lua_gettop(L));
		lua_pushboolean(L, false);
		lua_pushstring(L, "Unable to retrive filesize");
		CloseHandle(FH);
		return 2;
	}

	BYTE * buffer = (BYTE*)calloc(size, sizeof(BYTE));

	if (!buffer) {
		lua_pop(L, lua_gettop(L));
		lua_pushboolean(L, false);
		lua_pushstring(L, "Unable to allocate memory");
		CloseHandle(FH);
		return 2;
	}

	DWORD read;
	if (!ReadFile(FH, buffer, size, &read, NULL) || read != size) {
		lua_pop(L, lua_gettop(L));
		lua_pushboolean(L, false);
		lua_pushstring(L, "Unable to read file");
		CloseHandle(FH);
		free(buffer);
		return 2;
	}

	CloseHandle(FH);

	PBITMAPFILEHEADER BFileHeader = (PBITMAPFILEHEADER)buffer;
	PBITMAPINFOHEADER  BInfoHeader = (PBITMAPINFOHEADER)&buffer[sizeof(BITMAPFILEHEADER)];

	if (BFileHeader->bfType != 0x4D42) {
		lua_pop(L, lua_gettop(L));
		lua_pushboolean(L, false);
		lua_pushstring(L, "Invalid filetype (bmp only)");
		free(buffer);
		return 2;
	}
	else if (BFileHeader->bfSize < sizeof(BITMAPFILEHEADER) ||
		BFileHeader->bfOffBits < sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER)) {
		lua_pop(L, lua_gettop(L));
		lua_pushboolean(L, false);
		lua_pushstring(L, "Invalid sizes, can only load simple bmp files");
		free(buffer);
		return 2;
	}
	else if (BInfoHeader->biSize != sizeof(BITMAPINFOHEADER) ||
		BInfoHeader->biPlanes != 1 ||
		BInfoHeader->biBitCount != 24 ||
		BInfoHeader->biCompression != BI_RGB) {
		lua_pop(L, lua_gettop(L));
		lua_pushboolean(L, false);
		lua_pushstring(L, "Invalid bits or multiplane, can only load simple bmp files");
		free(buffer);
		return 2;
	}

	lua_pop(L, lua_gettop(L));
	LuaImage * img = lua_pushimage(L);
	img->Data = buffer;
	img->DataSize = size;
	img->Height = BInfoHeader->biHeight;
	img->Width = BInfoHeader->biWidth;

	return 1;
}

int lua_savetofile(lua_State *L) {

	LuaImage * img = lua_toimage(L, 1);
	const char * file = luaL_checkstring(L, 2);

	DWORD Junk;
	HANDLE FH = CreateFileA(file, GENERIC_WRITE, FILE_SHARE_WRITE, 0, CREATE_ALWAYS, 0, 0);
	if (FH == INVALID_HANDLE_VALUE) {
		lua_pop(L, lua_gettop(L));
		lua_pushboolean(L, false);
		return 1;
	}

	if (WriteFile(FH, img->Data, img->DataSize, &Junk, 0) && Junk == img->DataSize) {
		
		lua_pop(L, lua_gettop(L));
		lua_pushboolean(L, true);
	}
	else {
		lua_pop(L, lua_gettop(L));
		lua_pushboolean(L, true);
	}

	CloseHandle(FH);

	return 1;
}

LuaImage * lua_pushimage(lua_State *L) {
	LuaImage * image = (LuaImage*)lua_newuserdata(L, sizeof(LuaImage));
	if (image == NULL)
		luaL_error(L, "Unable to push image");
	luaL_getmetatable(L, IMAGE);
	lua_setmetatable(L, -2);
	memset(image, 0, sizeof(LuaImage));

	return image;
}

LuaImage * lua_toimage(lua_State *L, int index) {
	LuaImage * image = (LuaImage*)luaL_checkudata(L, index, IMAGE);
	if (image == NULL)
		luaL_error(L, "parameter is not a %s", IMAGE);
	return image;
}

int image_gc(lua_State *L) {

	LuaImage * img = lua_toimage(L, 1);

	if (img->Data) {
		free(img->Data);
		img->Data = NULL;
	}

	return 0;
}

int image_tostring(lua_State *L) {
	char tim[100];
	sprintf(tim, "Image: 0x%08X", lua_toimage(L, 1));
	lua_pushfstring(L, tim);
	return 1;
}