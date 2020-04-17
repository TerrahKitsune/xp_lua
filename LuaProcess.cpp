#include "LuaProcess.h"
#include <string.h>
#include <Windows.h>
#include <psapi.h>
#include <tlhelp32.h>

LuaProcess * lua_toprocess(lua_State *L, int index) {

	LuaProcess * proc = (LuaProcess*)lua_touserdata(L, index);
	if (proc == NULL)
		luaL_error(L, "parameter is not a %s", LUAPROCESS);
	return proc;
}

LuaProcess * lua_pushprocess(lua_State *L) {

	LuaProcess * proc = (LuaProcess*)lua_newuserdata(L, sizeof(LuaProcess));
	if (proc == NULL)
		luaL_error(L, "Unable to create processmeta");
	luaL_getmetatable(L, LUAPROCESS);
	lua_setmetatable(L, -2);
	memset(proc, 0, sizeof(LuaProcess));
	proc->hChildStd_IN_Rd = INVALID_HANDLE_VALUE;
	proc->hChildStd_IN_Wr = INVALID_HANDLE_VALUE;
	proc->hChildStd_OUT_Rd = INVALID_HANDLE_VALUE;
	proc->hChildStd_OUT_Wr = INVALID_HANDLE_VALUE;
	proc->hChildStd_ERR_Rd = INVALID_HANDLE_VALUE;
	proc->hChildStd_ERR_Wr = INVALID_HANDLE_VALUE;
	return proc;
}

static char procname[MAX_PATH];
const char * GetProcessName(int id) {

	HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION |
		PROCESS_VM_READ,
		FALSE, id);

	memset(procname, 0, sizeof(MAX_PATH));

	if (hProcess != NULL) {
		HMODULE hMod;
		DWORD cbNeeded;

		if (EnumProcessModules(hProcess, &hMod, sizeof(hMod),
			&cbNeeded))
		{
			GetModuleBaseName(hProcess, hMod, procname, MAX_PATH);
		}
	}

	CloseHandle(hProcess);
	procname[MAX_PATH - 1] = '\0';
	return procname;
}

int GetAllProcesses(lua_State *L) {

	DWORD processes[1024];
	DWORD needed;
	const char *name;
	if (!EnumProcesses(processes, sizeof(processes), &needed))
	{
		lua_pushnil(L);
		return 1;
	}

	needed = needed / sizeof(DWORD);
	lua_createtable(L, 0, needed);
	for (unsigned int n = 0; n < needed; n++) {
		name = GetProcessName(processes[n]);
		if (name[0] != '\0') {
			lua_pushinteger(L, processes[n]);
			lua_pushstring(L, name);
			lua_settable(L, -3);
		}
	}

	return 1;
}

int LuaOpenProcess(lua_State *L) {

	int processid = (int)luaL_optinteger(L, 1, 0);
	lua_pop(L, lua_gettop(L));
	HANDLE proc;
	if (processid == 0) {
		proc = GetCurrentProcess();
		processid = GetProcessId(proc);
	}
	else {
		proc = OpenProcess(PROCESS_ALL_ACCESS, false, processid);
	}

	if (proc) {
		LuaProcess * lproc = lua_pushprocess(L);
		lproc->processInfo.dwProcessId = processid;
		lproc->processInfo.hProcess = proc;

		SYSTEM_INFO sysInfo;
		FILETIME ftime, fsys, fuser;

		GetSystemInfo(&sysInfo);
		lproc->numProcessors = sysInfo.dwNumberOfProcessors;

		GetSystemTimeAsFileTime(&ftime);
		memcpy(&lproc->lastCPU, &ftime, sizeof(FILETIME));

		GetProcessTimes(lproc->processInfo.hProcess, &ftime, &ftime, &fsys, &fuser);
		memcpy(&lproc->lastSysCPU, &fsys, sizeof(FILETIME));
		memcpy(&lproc->lastUserCPU, &fuser, sizeof(FILETIME));
	}
	else {
		lua_pushnil(L);
	}

	return 1;
}

bool FileExists(const char * szPath)
{
	DWORD dwAttrib = GetFileAttributes(szPath);

	return (dwAttrib != INVALID_FILE_ATTRIBUTES &&
		!(dwAttrib & FILE_ATTRIBUTE_DIRECTORY));
}

int StartNewProcess(lua_State *L) {

	const char * appname = lua_tostring(L, 1);
	const char * cmd = lua_tostring(L, 2);
	const char * dir = lua_tostring(L, 3);
	bool noconsole = lua_toboolean(L, 4) > 0;
	bool redirect = false;
	int mask = 0;

	if (!dir) {
		char defaultdir[MAX_PATH];
		defaultdir[GetCurrentDirectory(MAX_PATH, defaultdir)] = '\0';
		dir = defaultdir;
	}

	if (lua_gettop(L) >= 5) {

		if (lua_isboolean(L, 5)) {
			redirect = lua_toboolean(L, 5) > 0;
			mask = LUA_PROC_IN | LUA_PROC_OUT | LUA_PROC_ERR;
		}
		else {
			mask = lua_tointeger(L, 5) & (LUA_PROC_IN | LUA_PROC_OUT | LUA_PROC_ERR);
			redirect = mask > 0;
		}
	}

	DWORD flag = CREATE_NEW_CONSOLE | NORMAL_PRIORITY_CLASS;
	if (noconsole) {
		flag = NORMAL_PRIORITY_CLASS;
	}

	STARTUPINFO info;
	PROCESS_INFORMATION processInfo;
	HANDLE hChildStd_OUT_Rd = INVALID_HANDLE_VALUE;
	HANDLE hChildStd_OUT_Wr = INVALID_HANDLE_VALUE;
	HANDLE hChildStd_IN_Rd = INVALID_HANDLE_VALUE;
	HANDLE hChildStd_IN_Wr = INVALID_HANDLE_VALUE;
	HANDLE hChildStd_ERR_Rd = INVALID_HANDLE_VALUE;
	HANDLE hChildStd_ERR_Wr = INVALID_HANDLE_VALUE;

	ZeroMemory(&info, sizeof(STARTUPINFO));
	info.cb = sizeof(info);
	ZeroMemory(&processInfo, sizeof(PROCESS_INFORMATION));

	if (redirect) {

		SECURITY_ATTRIBUTES saAttr;
		saAttr.nLength = sizeof(SECURITY_ATTRIBUTES);
		saAttr.bInheritHandle = TRUE;
		saAttr.lpSecurityDescriptor = NULL;

		if (mask & LUA_PROC_OUT) {

			if (!CreatePipe(&hChildStd_OUT_Rd, &hChildStd_OUT_Wr, &saAttr, 0)) {
				lua_pop(L, lua_gettop(L));
				lua_pushnil(L);
				lua_pushfstring(L, "Unable to open process %d", GetLastError());
				return 2;
			}

			if (!SetHandleInformation(hChildStd_OUT_Rd, HANDLE_FLAG_INHERIT, 0)) {

				lua_pop(L, lua_gettop(L));
				lua_pushnil(L);
				lua_pushfstring(L, "Unable to open process %d", GetLastError());
				return 2;
			}

			info.hStdOutput = hChildStd_OUT_Wr;
		}

		if (mask & LUA_PROC_IN) {

			if (!CreatePipe(&hChildStd_IN_Rd, &hChildStd_IN_Wr, &saAttr, 0)) {
				lua_pop(L, lua_gettop(L));
				lua_pushnil(L);
				lua_pushfstring(L, "Unable to open process %d", GetLastError());
				return 2;
			}

			if (!SetHandleInformation(hChildStd_IN_Wr, HANDLE_FLAG_INHERIT, 0)) {

				lua_pop(L, lua_gettop(L));
				lua_pushnil(L);
				lua_pushfstring(L, "Unable to open process %d", GetLastError());
				return 2;
			}

			info.hStdInput = hChildStd_IN_Rd;
		}

		if (mask & LUA_PROC_ERR) {

			if (!CreatePipe(&hChildStd_ERR_Rd, &hChildStd_ERR_Wr, &saAttr, 0)) {
				lua_pop(L, lua_gettop(L));
				lua_pushnil(L);
				lua_pushfstring(L, "Unable to open process %d", GetLastError());
				return 2;
			}

			if (!SetHandleInformation(hChildStd_ERR_Rd, HANDLE_FLAG_INHERIT, 0)) {

				lua_pop(L, lua_gettop(L));
				lua_pushnil(L);
				lua_pushfstring(L, "Unable to open process %d", GetLastError());
				return 2;
			}

			info.hStdError = hChildStd_ERR_Wr;
		}


		info.dwFlags |= STARTF_USESTDHANDLES;
	}

	if (CreateProcess(appname, (LPSTR)cmd, NULL, NULL, redirect, flag, NULL, dir, &info, &processInfo)) {

		lua_pop(L, lua_gettop(L));
		LuaProcess * proc = lua_pushprocess(L);
		proc->info = info;
		proc->processInfo = processInfo;

		SYSTEM_INFO sysInfo;
		FILETIME ftime, fsys, fuser;

		GetSystemInfo(&sysInfo);
		proc->numProcessors = sysInfo.dwNumberOfProcessors;

		GetSystemTimeAsFileTime(&ftime);
		memcpy(&proc->lastCPU, &ftime, sizeof(FILETIME));

		GetProcessTimes(proc->processInfo.hProcess, &ftime, &ftime, &fsys, &fuser);
		memcpy(&proc->lastSysCPU, &fsys, sizeof(FILETIME));
		memcpy(&proc->lastUserCPU, &fuser, sizeof(FILETIME));

		proc->hChildStd_IN_Rd = hChildStd_IN_Rd;
		proc->hChildStd_IN_Wr = hChildStd_IN_Wr;
		proc->hChildStd_OUT_Rd = hChildStd_OUT_Rd;
		proc->hChildStd_OUT_Wr = hChildStd_OUT_Wr;
		proc->hChildStd_ERR_Rd = hChildStd_ERR_Rd;
		proc->hChildStd_ERR_Wr = hChildStd_ERR_Wr;

		return 1;
	}
	else {
		lua_pop(L, lua_gettop(L));
		lua_pushnil(L);
		lua_pushfstring(L, "Unable to open process %d", GetLastError());
		return 2;
	}
}

int WriteToPipe(lua_State *L) {

	LuaProcess * proc = lua_toprocess(L, 1);
	size_t len;
	const char * data = luaL_checklstring(L, 2, &len);
	DWORD written;
	if (proc->hChildStd_IN_Wr == INVALID_HANDLE_VALUE) {
		lua_pop(L, lua_gettop(L));
		lua_pushinteger(L, -1);
		return 1;
	}

	BOOL success = WriteFile(proc->hChildStd_IN_Wr, data, len, &written, NULL);

	if (!success) {
		lua_pop(L, lua_gettop(L));
		lua_pushinteger(L, -1);
		return 1;
	}

	FlushFileBuffers(proc->hChildStd_IN_Wr);

	lua_pop(L, lua_gettop(L));
	lua_pushinteger(L, written);
	return 1;
}

int ReadFromPipe(lua_State *L) {

	LuaProcess * proc = lua_toprocess(L, 1);
	unsigned int buffersize = (unsigned int)luaL_optinteger(L, 2, 1048576);

	if (proc->hChildStd_OUT_Rd == INVALID_HANDLE_VALUE) {
		lua_pop(L, lua_gettop(L));
		lua_pushnil(L);
		return 1;
	}

	if (buffersize <= 0) {
		buffersize = 1;
	}

	char * data = (char*)gff_malloc(buffersize);
	if (!data) {
		lua_pop(L, lua_gettop(L));
		lua_pushnil(L);
		return 1;
	}

	DWORD read = 1;
	BOOL success = PeekNamedPipe(proc->hChildStd_OUT_Rd, data, 1, NULL, &read, NULL);

	if (success) {

		if (read <= 0) {
			gff_free(data);
			lua_pop(L, lua_gettop(L));
			lua_pushnil(L);
			return 1;
		}

		success = ReadFile(proc->hChildStd_OUT_Rd, data, buffersize - 1, &read, NULL);
		data[read] = '\0';
	}

	if (!success) {
		gff_free(data);
		lua_pop(L, lua_gettop(L));
		lua_pushnil(L);
		return 1;
	}

	lua_pop(L, lua_gettop(L));
	lua_pushlstring(L, data, read);
	gff_free(data);
	return 1;
}

int ErrorFromPipe(lua_State *L) {

	LuaProcess * proc = lua_toprocess(L, 1);
	unsigned int buffersize = (unsigned int)luaL_optinteger(L, 2, 1048576);

	if (proc->hChildStd_ERR_Rd == INVALID_HANDLE_VALUE) {
		lua_pop(L, lua_gettop(L));
		lua_pushnil(L);
		return 1;
	}

	if (buffersize <= 0) {
		buffersize = 1;
	}

	char * data = (char*)gff_malloc(buffersize);
	if (!data) {
		lua_pop(L, lua_gettop(L));
		lua_pushnil(L);
		return 1;
	}

	DWORD read = 1;
	BOOL success = PeekNamedPipe(proc->hChildStd_ERR_Rd, data, 1, NULL, &read, NULL);

	if (success) {

		if (read <= 0) {
			gff_free(data);
			lua_pop(L, lua_gettop(L));
			lua_pushnil(L);
			return 1;
		}

		success = ReadFile(proc->hChildStd_ERR_Rd, data, buffersize - 1, &read, NULL);
		data[read] = '\0';
	}

	if (!success) {
		gff_free(data);
		lua_pop(L, lua_gettop(L));
		lua_pushnil(L);
		return 1;
	}

	lua_pop(L, lua_gettop(L));
	lua_pushlstring(L, data, read);
	gff_free(data);
	return 1;
}

int GetSetPriority(lua_State *L) {

	LuaProcess * proc = lua_toprocess(L, 1);
	DWORD prio = GetPriorityClass(proc->processInfo.hProcess);

	if (lua_type(L, 2) == LUA_TNUMBER) {
		prio = (DWORD)SetPriorityClass(proc->processInfo.hProcess, (DWORD)lua_tointeger(L, 2));
		lua_pop(L, lua_gettop(L));
		lua_pushboolean(L, prio > 0);
	}
	else {
		lua_pop(L, lua_gettop(L));
		lua_pushinteger(L, prio > 0);
	}

	return 1;
}

int GetThreads(lua_State *L) {

	LuaProcess * proc = lua_toprocess(L, 1);

	HANDLE hThreadSnap = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
	THREADENTRY32 te32;

	if (hThreadSnap == INVALID_HANDLE_VALUE) {
		lua_pop(L, lua_gettop(L));
		lua_pushnil(L);
		lua_pushstring(L, "Unable to retrive snapshot");
		return 2;
	}

	te32.dwSize = sizeof(THREADENTRY32);

	if (!Thread32First(hThreadSnap, &te32))
	{
		CloseHandle(hThreadSnap);
		lua_pop(L, lua_gettop(L));
		lua_pushnil(L);
		lua_pushstring(L, "Unable to retrive any threads");
		return 2;
	}

	lua_pop(L, lua_gettop(L));
	lua_newtable(L);
	int n = 0;
	do
	{
		if (te32.th32OwnerProcessID == proc->processInfo.dwProcessId)
		{
			lua_createtable(L, 0, 3);

			lua_pushstring(L, "ID");
			lua_pushinteger(L, te32.th32ThreadID);
			lua_settable(L, -3);

			lua_pushstring(L, "BasePrio");
			lua_pushinteger(L, te32.tpBasePri);
			lua_settable(L, -3);

			lua_pushstring(L, "DeltaPrio");
			lua_pushinteger(L, te32.tpDeltaPri);
			lua_settable(L, -3);

			lua_rawseti(L, -2, ++n);
		}
	} while (Thread32Next(hThreadSnap, &te32));

	CloseHandle(hThreadSnap);

	return 1;
}

int GetSetAffinity(lua_State *L) {

	LuaProcess * proc = lua_toprocess(L, 1);
	DWORD newmask;
	DWORD process, system;

	bool ok = GetProcessAffinityMask(proc->processInfo.hProcess, &process, &system) > 0;

	if (!ok) {
		lua_pop(L, lua_gettop(L));
		lua_pushnil(L);
		lua_pushstring(L, "Unable to retrive process affinity mask");
		return 1;
	}

	if (lua_isnumber(L, 2)) {
		newmask = (DWORD)lua_tointeger(L, 2);
		ok = SetProcessAffinityMask(proc->processInfo.hProcess, newmask) > 0;

		if (!ok) {
			lua_pop(L, lua_gettop(L));
			lua_pushnil(L);
			lua_pushstring(L, "Unable to set process affinity mask");
			return 1;
		}
	}

	lua_pop(L, lua_gettop(L));
	lua_pushinteger(L, process);
	lua_pushinteger(L, system);

	return 2;
}

int GetProcId(lua_State *L) {

	LuaProcess * proc = lua_toprocess(L, 1);
	DWORD id = proc->processInfo.dwProcessId;
	lua_pop(L, 1);
	lua_pushinteger(L, id);
	return 1;
}

int GetProcName(lua_State *L) {

	LuaProcess * proc = lua_toprocess(L, 1);
	DWORD id = proc->processInfo.dwProcessId;
	lua_pop(L, 1);
	lua_pushstring(L, GetProcessName(id));
	return 1;
}

double getCurrentValue(LuaProcess * proc) {
	FILETIME ftime, fsys, fuser;
	ULARGE_INTEGER now, sys, user;
	double percent;

	GetSystemTimeAsFileTime(&ftime);
	memcpy(&now, &ftime, sizeof(FILETIME));

	GetProcessTimes(proc->processInfo.hProcess, &ftime, &ftime, &fsys, &fuser);
	memcpy(&sys, &fsys, sizeof(FILETIME));
	memcpy(&user, &fuser, sizeof(FILETIME));
	percent = (double)(sys.QuadPart - proc->lastSysCPU.QuadPart) +
		(user.QuadPart - proc->lastUserCPU.QuadPart);
	percent /= (now.QuadPart - proc->lastCPU.QuadPart);
	percent /= proc->numProcessors;
	proc->lastCPU = now;
	proc->lastUserCPU = user;
	proc->lastSysCPU = sys;

	return percent * 100;
}

int GetCPU(lua_State *L) {

	LuaProcess * proc = lua_toprocess(L, 1);

	lua_pop(L, lua_gettop(L));
	lua_pushnumber(L, getCurrentValue(proc));

	return 1;
}

int GetMemory(lua_State *L) {

	LuaProcess * proc = lua_toprocess(L, 1);

	PROCESS_MEMORY_COUNTERS_EX pmc;
	GetProcessMemoryInfo(proc->processInfo.hProcess, (PROCESS_MEMORY_COUNTERS*)&pmc, sizeof(pmc));
	SIZE_T virtualMemUsedByMe = pmc.PrivateUsage;

	lua_pop(L, lua_gettop(L));
	lua_pushnumber(L, pmc.WorkingSetSize);

	return 1;
}

int StopProcess(lua_State *L) {

	LuaProcess * proc = lua_toprocess(L, 1);
	if (TerminateProcess(proc->processInfo.hProcess, (UINT)lua_tointeger(L, 2))) {
		lua_pop(L, lua_gettop(L));
		lua_pushboolean(L, true);
		return 1;
	}

	lua_pop(L, lua_gettop(L));
	lua_pushboolean(L, false);
	return 1;
}

int GetExitCode(lua_State *L) {
	LuaProcess * proc = lua_toprocess(L, 1);

	lua_pop(L, 1);
	DWORD lpExitCode;
	if (GetExitCodeProcess(proc->processInfo.hProcess, &lpExitCode)) {
		if (lpExitCode == STILL_ACTIVE)
			lua_pushnil(L);
		else
			lua_pushinteger(L, lpExitCode);
	}
	else
	{
		lua_pushnil(L);
	}
	return 1;
}

int process_gc(lua_State *L) {

	LuaProcess * proc = lua_toprocess(L, 1);
	if (proc->processInfo.hProcess)
		CloseHandle(proc->processInfo.hProcess);
	if (proc->processInfo.hThread)
		CloseHandle(proc->processInfo.hThread);

	if (proc->hChildStd_IN_Rd != INVALID_HANDLE_VALUE)
		CloseHandle(proc->hChildStd_IN_Rd);
	if (proc->hChildStd_IN_Wr != INVALID_HANDLE_VALUE)
		CloseHandle(proc->hChildStd_IN_Wr);
	if (proc->hChildStd_OUT_Rd != INVALID_HANDLE_VALUE)
		CloseHandle(proc->hChildStd_OUT_Rd);
	if (proc->hChildStd_OUT_Wr != INVALID_HANDLE_VALUE)
		CloseHandle(proc->hChildStd_OUT_Wr);
	if (proc->hChildStd_ERR_Rd != INVALID_HANDLE_VALUE)
		CloseHandle(proc->hChildStd_ERR_Rd);
	if (proc->hChildStd_ERR_Wr != INVALID_HANDLE_VALUE)
		CloseHandle(proc->hChildStd_ERR_Wr);

	lua_pop(L, 1);
	return 0;
}

int process_tostring(lua_State *L) {

	char tim[100];
	sprintf(tim, "Process: 0x%08X", lua_toprocess(L, 1));
	lua_pushfstring(L, tim);
	return 1;
}