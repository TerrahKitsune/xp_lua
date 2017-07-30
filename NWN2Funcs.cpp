#include "NWN2Funcs.h"
#include "stdlib.h"
#include "windows.h"

void * (__fastcall *pGetCreatureByGameObjectID)(void*CExoApp, void*, nwn_objid_t objid) = (void*(__fastcall *)(void*, void*, nwn_objid_t))0x0054A1B0;

//Retrive CServerExoApp (this->)
void * GetCServerExoApp(){
	DWORD exoappaddr = *(DWORD*)0x86442C;
	return *(void **)(exoappaddr + 4);
}

//CServerExoApp::GetCreatureByGameObjectID(nwn_objid_t)
void * GetCreatureByGameObjectID(nwn_objid_t objectid){

	return pGetCreatureByGameObjectID(GetCServerExoApp(), NULL, objectid);
}