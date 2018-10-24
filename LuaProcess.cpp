#include "LuaProcess.h"
#include <string.h>
#include <Windows.h>
#include <psapi.h>

LuaProcess * lua_toprocess(lua_State *L, int index) {

	LuaProcess * proc = (LuaProcess*)lua_touserdata(L, index);
	if (proc == NULL)
		luaL_error(L, "paramter is not a %s", LUAPROCESS);
	return proc;
}

LuaProcess * lua_pushprocess(lua_State *L) {

	LuaProcess * proc = (LuaProcess*)lua_newuserdata(L, sizeof(LuaProcess));
	if (proc == NULL)
		luaL_error(L, "Unable to create timer");
	luaL_getmetatable(L, LUAPROCESS);
	lua_setmetatable(L, -2);
	memset(proc, 0, sizeof(LuaProcess));
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
	for (int n = 0; n < needed; n++) {
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

	int processid = luaL_optinteger(L, 1, 0);
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
	bool noconsole = lua_toboolean(L, 4);
	const char * file_stdout = luaL_optstring(L, 5, NULL);
	const char * file_stdin = luaL_optstring(L, 6, NULL);
	const char * file_stderr = luaL_optstring(L, 7, NULL);

	DWORD flag = CREATE_NEW_CONSOLE | NORMAL_PRIORITY_CLASS;
	if (noconsole) {
		flag = NORMAL_PRIORITY_CLASS;
	}

	STARTUPINFO info;
	PROCESS_INFORMATION processInfo;

	ZeroMemory(&info, sizeof(STARTUPINFO));
	info.cb = sizeof(info);
	ZeroMemory(&processInfo, sizeof(PROCESS_INFORMATION));

	bool anyhandle = false;

	SECURITY_ATTRIBUTES sa;
	sa.nLength = sizeof(sa);
	sa.lpSecurityDescriptor = NULL;
	sa.bInheritHandle = TRUE;

	if (file_stderr) {

		if (FileExists(file_stderr))
			info.hStdError = CreateFileA(file_stderr, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, &sa, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
		else
			info.hStdError = CreateFileA(file_stderr, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, &sa, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, NULL);

		anyhandle = true;
	}
	else {
		info.hStdError = INVALID_HANDLE_VALUE;
	}

	if (file_stdout) {

		if (FileExists(file_stdout))
			info.hStdOutput = CreateFileA(file_stdout, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, &sa, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
		else
			info.hStdOutput = CreateFileA(file_stdout, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, &sa, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, NULL);

		anyhandle = true;
	}
	else {
		info.hStdOutput = INVALID_HANDLE_VALUE;
	}

	if (file_stdin) {

		info.hStdInput = CreateFileA(file_stdin, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ, &sa, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

		anyhandle = true;
	}
	else {
		info.hStdInput = INVALID_HANDLE_VALUE;
	}

	if (anyhandle)
		info.dwFlags |= STARTF_USESTDHANDLES;

	char * writablebuffer = (char*)calloc(strlen(cmd) + 1, sizeof(LPSTR));

	if (writablebuffer) {
		strcpy(writablebuffer, cmd);
	}

	if (CreateProcess(appname, (LPSTR)writablebuffer, NULL, NULL, false, flag, NULL, dir, &info, &processInfo)) {

		lua_pop(L, lua_gettop(L));
		LuaProcess * proc = lua_pushprocess(L);
		proc->writablebuffer = writablebuffer;
		proc->info = info;
		proc->processInfo = processInfo;
		proc->hstderr = info.hStdError;
		proc->hstdin = info.hStdInput;
		proc->hstdout = info.hStdOutput;

		SYSTEM_INFO sysInfo;
		FILETIME ftime, fsys, fuser;

		GetSystemInfo(&sysInfo);
		proc->numProcessors = sysInfo.dwNumberOfProcessors;

		GetSystemTimeAsFileTime(&ftime);
		memcpy(&proc->lastCPU, &ftime, sizeof(FILETIME));

		GetProcessTimes(proc->processInfo.hProcess, &ftime, &ftime, &fsys, &fuser);
		memcpy(&proc->lastSysCPU, &fsys, sizeof(FILETIME));
		memcpy(&proc->lastUserCPU, &fuser, sizeof(FILETIME));

		return 1;
	}
	else {

		if (writablebuffer) {
			free(writablebuffer);
		}

		if (info.hStdError != INVALID_HANDLE_VALUE) {
			CloseHandle(info.hStdError);
		}

		if (info.hStdOutput != INVALID_HANDLE_VALUE) {
			CloseHandle(info.hStdOutput);
		}

		if (info.hStdInput != INVALID_HANDLE_VALUE) {
			CloseHandle(info.hStdInput);
		}

		lua_pop(L, lua_gettop(L));
		lua_pushnil(L);
		lua_pushfstring(L, "Unable to open process %d", GetLastError());
		return 2;
	}
}

int GetSetPriority(lua_State *L) {

	LuaProcess * proc = lua_toprocess(L, 1);
	DWORD prio = GetPriorityClass(proc->processInfo.hProcess);

	if (lua_type(L, 2) == LUA_TNUMBER) {
		prio = SetPriorityClass(proc->processInfo.hProcess, lua_tointeger(L, 2));
		lua_pop(L, lua_gettop(L));
		lua_pushboolean(L, prio);
	}
	else {
		lua_pop(L, lua_gettop(L));
		lua_pushinteger(L, prio);
	}

	return 1;
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
	percent = (sys.QuadPart - proc->lastSysCPU.QuadPart) +
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
	if (TerminateProcess(proc->processInfo.hProcess, lua_tointeger(L, 2))) {
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

	if (proc->hstderr != INVALID_HANDLE_VALUE) {
		CloseHandle(proc->hstderr);
	}

	if (proc->hstdin != INVALID_HANDLE_VALUE) {
		CloseHandle(proc->hstdin);
	}

	if (proc->hstdout != INVALID_HANDLE_VALUE) {
		CloseHandle(proc->hstdout);
	}

	if (proc->writablebuffer) {
		free(proc->writablebuffer);
		proc->writablebuffer = NULL;
	}

	lua_pop(L, 1);
	return 0;
}

int process_tostring(lua_State *L) {

	char tim[100];
	sprintf(tim, "Process: 0x%08X", lua_toprocess(L, 1));
	lua_pushfstring(L, tim);
	return 1;
}