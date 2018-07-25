#pragma once
#include "NWN2_API.h"
#include "nwn2heap.h"


#ifdef _M_IX86
#pragma comment(lib, "NWN2_MemoryMgr_amdxp.lib")
#else
#pragma comment(lib, "NWN2_MemoryMgr.lib")
#endif

HINSTANCE NWN2_MemoryMgr = NULL;
void * NWN2_Heap_Deallocate = NULL;

void NWN2_InitMem(){

	NWN2_MemoryMgr = GetModuleHandle("NWN2_MemoryMgr_amdxp.dll");
	if (NWN2_MemoryMgr == NULL)
		return;

	NWN2_Heap_Deallocate = GetProcAddress(NWN2_MemoryMgr,"?Deallocate@NWN2_Heap@@SAXPAX@Z");
}

void * NWN2_Malloc(size_t size){

	NWN2_HeapMgr *pHeapMgr = NWN2_HeapMgr::Instance();
	NWN2_Heap *pHeap = pHeapMgr->GetDefaultHeap();
	return pHeap->Allocate(size);
}

//Freesnippet created by Skywing
__declspec(naked)
void
__cdecl
NWN2_Free(void * P)
{
#ifdef _M_IX86
	__asm
	{
		jmp     dword ptr[NWN2_Heap_Deallocate]
	}
#else
	if (P != NULL)
		HeapFree(GetProcessHeap(), 0, P);
#endif
}