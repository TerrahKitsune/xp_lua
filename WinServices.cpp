#include "WinServices.h"

void pushservicestatus(lua_State* L, SERVICE_STATUS* status, const char* displayname, const char* servicename) {

	int total = 7;

	if (displayname) {
		total++;
	}

	if (servicename) {
		total++;
	}

	lua_createtable(L, 0, total);

	if (displayname) {
		lua_pushstring(L, "DisplayName");
		lua_pushstring(L, displayname);
		lua_settable(L, -3);
	}

	if (servicename) {
		lua_pushstring(L, "ServiceName");
		lua_pushstring(L, servicename);
		lua_settable(L, -3);
	}

	lua_pushstring(L, "CheckPoint");
	lua_pushinteger(L, status->dwCheckPoint);
	lua_settable(L, -3);

	lua_pushstring(L, "ControlsAccepted");
	lua_pushinteger(L, status->dwControlsAccepted);
	lua_settable(L, -3);

	lua_pushstring(L, "CurrentState");
	lua_pushinteger(L, status->dwCurrentState);
	lua_settable(L, -3);

	lua_pushstring(L, "ServiceSpecificExitCode");
	lua_pushinteger(L, status->dwServiceSpecificExitCode);
	lua_settable(L, -3);

	lua_pushstring(L, "ServiceType");
	lua_pushinteger(L, status->dwServiceType);
	lua_settable(L, -3);

	lua_pushstring(L, "WaitHint");
	lua_pushinteger(L, status->dwWaitHint);
	lua_settable(L, -3);

	lua_pushstring(L, "Win32ExitCode");
	lua_pushinteger(L, status->dwWin32ExitCode);
	lua_settable(L, -3);
}

int getallservices(lua_State * L) {

	SC_HANDLE hHandle = OpenSCManager(NULL, NULL, SC_MANAGER_ENUMERATE_SERVICE);

	if (hHandle == NULL) {
		lua_pop(L, lua_gettop(L));
		lua_pushnil(L);
		return 1;
	}

	ENUM_SERVICE_STATUS service;
	DWORD dwBytesNeeded = 0;
	DWORD dwServicesReturned = 0;
	DWORD dwResumedHandle = 0;
	DWORD dwServiceType = SERVICE_TYPE_ALL;

	BOOL retVal = EnumServicesStatus(hHandle, dwServiceType, SERVICE_STATE_ALL,
		&service, sizeof(ENUM_SERVICE_STATUS), &dwBytesNeeded, &dwServicesReturned,
		&dwResumedHandle);

	if (retVal) {
		CloseServiceHandle(hHandle);
		lua_pop(L, lua_gettop(L));
		lua_pushnil(L);
		return 1;
	}

	lua_pop(L, lua_gettop(L));
	lua_createtable(L, (int)dwServicesReturned, 0);

	if (GetLastError() == ERROR_MORE_DATA) {

		DWORD dwBytes = sizeof(ENUM_SERVICE_STATUS) + dwBytesNeeded;
		ENUM_SERVICE_STATUS* pServices = NULL;
		pServices = new ENUM_SERVICE_STATUS[dwBytes];

		EnumServicesStatus(hHandle, dwServiceType, SERVICE_STATE_ALL,
			pServices, dwBytes, &dwBytesNeeded, &dwServicesReturned, &dwResumedHandle);

		ENUM_SERVICE_STATUS* status;

		for (unsigned iIndex = 0; iIndex < dwServicesReturned; iIndex++) {

			status = (pServices + iIndex);

			pushservicestatus(L, &status->ServiceStatus, status->lpDisplayName, status->lpServiceName);

			lua_rawseti(L, -2, iIndex + 1);
		}
		delete[] pServices;
		pServices = NULL;
	}

	CloseServiceHandle(hHandle);

	return 1;
}

int getstatus(lua_State * L) {

	LuaWinService* luaservice = lua_toluawinservice(L, 1);
	SERVICE_STATUS status;

	BOOL ret = QueryServiceStatus(luaservice->hService, &status);

	lua_pop(L, lua_gettop(L));

	if (!ret) {
		lua_pushnil(L);
		return 1;
	}

	pushservicestatus(L, &status, NULL, NULL);

	return 1;
}

int getconfig(lua_State * L) {

	LuaWinService* luaservice = lua_toluawinservice(L, 1);
	DWORD needed;
	QueryServiceConfig(luaservice->hService, NULL, 0, &needed);

	lua_pop(L, lua_gettop(L));

	if (needed <= 0) {
		lua_pushnil(L);
		return 1;
	}

	QUERY_SERVICE_CONFIG* config = (QUERY_SERVICE_CONFIG*)calloc(needed, sizeof(BYTE));

	if (!config) {
		luaL_error(L, "Unable to allocate memory");
	}

	if (!QueryServiceConfig(luaservice->hService, config, needed, &needed)) {
		free(config);
		lua_pushnil(L);
		return 1;
	}

	lua_createtable(L, 0, 9);

	lua_pushstring(L, "ErrorControl");
	lua_pushinteger(L, config->dwErrorControl);
	lua_settable(L, -3);

	lua_pushstring(L, "ServiceType");
	lua_pushinteger(L, config->dwServiceType);
	lua_settable(L, -3);

	lua_pushstring(L, "StartType");
	lua_pushinteger(L, config->dwStartType);
	lua_settable(L, -3);

	lua_pushstring(L, "TagId");
	lua_pushinteger(L, config->dwTagId);
	lua_settable(L, -3);

	lua_pushstring(L, "BinaryPathName");
	lua_pushstring(L, config->lpBinaryPathName);
	lua_settable(L, -3);

	lua_pushstring(L, "Dependencies");
	lua_pushstring(L, config->lpDependencies);
	lua_settable(L, -3);

	lua_pushstring(L, "DisplayName");
	lua_pushstring(L, config->lpDisplayName);
	lua_settable(L, -3);

	lua_pushstring(L, "LoadOrderGroup");
	lua_pushstring(L, config->lpLoadOrderGroup);
	lua_settable(L, -3);

	lua_pushstring(L, "ServiceStartName");
	lua_pushstring(L, config->lpServiceStartName);
	lua_settable(L, -3);

	free(config);

	return 1;
}

int openservice(lua_State * L) {

	const char* name = luaL_checkstring(L, 1);
	
	DWORD access = GENERIC_ALL;

	if (lua_toboolean(L, 2)) {
		access = GENERIC_READ;
	}

	SC_HANDLE hHandle = OpenSCManager(NULL, NULL, GENERIC_READ);

	if (hHandle == NULL) {
		lua_pop(L, lua_gettop(L));
		lua_pushnil(L);
		return 1;
	}

	SC_HANDLE hService = OpenService(hHandle, name, access);

	if (hService == NULL) {
		CloseServiceHandle(hHandle);
		lua_pop(L, lua_gettop(L));
		lua_pushnil(L);
		return 1;
	}

	CloseServiceHandle(hHandle);

	lua_pop(L, lua_gettop(L));

	LuaWinService* luaservice = lua_pushluawinservice(L);

	luaservice->hService = hService;

	return 1;
}

int startservice(lua_State* L) {

	LuaWinService* luaservice = lua_toluawinservice(L, 1);

	lua_pop(L, lua_gettop(L));

	lua_pushboolean(L, StartService(luaservice->hService, 0, NULL));

	return 1;
}

int stopservice(lua_State* L) {

	LuaWinService* luaservice = lua_toluawinservice(L, 1);

	lua_pop(L, lua_gettop(L));

	SERVICE_STATUS status;

	lua_pushboolean(L, ControlService(luaservice->hService, SERVICE_CONTROL_STOP, &status));

	return 1;
}

LuaWinService* lua_pushluawinservice(lua_State * L) {

	LuaWinService* service = (LuaWinService*)lua_newuserdata(L, sizeof(LuaWinService));

	if (service == NULL)
		luaL_error(L, "Unable to push windows service");

	luaL_getmetatable(L, WINSERVICE);
	lua_setmetatable(L, -2);

	memset(service, 0, sizeof(LuaWinService));

	return service;
}

LuaWinService * lua_toluawinservice(lua_State * L, int index) {

	LuaWinService* pipe = (LuaWinService*)luaL_checkudata(L, index, WINSERVICE);

	if (pipe == NULL)
		luaL_error(L, "parameter is not a %s", WINSERVICE);

	return pipe;
}

int luawinservice_gc(lua_State * L) {

	LuaWinService* service = lua_toluawinservice(L, 1);

	if (service->hService) {
		CloseServiceHandle(service->hService);
		service->hService = NULL;
	}

	return 0;
}

int luawinservice_tostring(lua_State * L) {
	char tim[100];
	sprintf(tim, "WinService: 0x%08X", lua_toluawinservice(L, 1));
	lua_pushfstring(L, tim);
	return 1;
}