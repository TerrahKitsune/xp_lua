#pragma once

#include <stdlib.h>
#include <stdio.h>
#include <limits.h>

#ifdef WIN32
#include <windows.h>
#endif

#include "./detours/include/detours.h"
#pragma comment(lib, "./detours/lib/detours.lib")

int Hook(DWORD address, PVOID detour);