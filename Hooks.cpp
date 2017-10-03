#include "Hooks.h"

int Hook(DWORD address, PVOID detour){

	DetourTransactionBegin();
	DetourUpdateThread(GetCurrentThread());
	int detour_success = DetourAttach(&(PVOID&)address, detour) == 0;
	detour_success |= DetourTransactionCommit() == 0;

	return detour_success;
}