#include "NWN2Funcs.h"
#include "stdlib.h"
#include "windows.h"
#include "./NWN2_API/NWN2_API.h"

struct CExoArrayList
{
	void *buf;
	int length;
	int arraylength;
};

CExoArrayList g_scriptArray = { NULL, 0, 0 };

CNWSGenericObject * (__fastcall *pGetObjectByGameObjectID)(void*CExoApp, void*, nwn_objid_t objid) = (CNWSGenericObject*(__fastcall *)(void*, void*, nwn_objid_t))0x0054d220;
CNWSCreature * (__fastcall *pGetCreatureByGameObjectID)(void*CExoApp, void*, nwn_objid_t objid) = (CNWSCreature*(__fastcall *)(void*, void*, nwn_objid_t))0x0054A1B0;
int(__fastcall *pRunScript)(void*CVirtualMachine, void*, CExoString *, unsigned long oid, CExoArrayList const &varArray, int unk, unsigned int Enum) = (int(__fastcall *)(void*CVirtualMachine, void*, CExoString *, unsigned long oid, CExoArrayList const &varArray, int unk, unsigned int Enum))0x0072B050;

//Retrive CServerExoApp (this->)
void * GetCServerExoApp(){
	DWORD exoappaddr = *(DWORD*)0x86442C;
	return *(void **)(exoappaddr + 4);
}

void * GetCVirtualMachine(){
	void **NWN_VirtualMachine = (void**)0x864424;
	return *NWN_VirtualMachine;
}

CNWSCreatureStats* GetCreatureStats(CNWSObject * obj) {

	if (!obj || obj->ObjectType != CGameObject__OBJECT_TYPE_CREATURE) {
		return NULL;
	}

	CNWSCreatureStats * stats = (CNWSCreatureStats *)*(DWORD *)(obj + 0x1FC4);

	return stats;
}

CNWSGenericObject * GetObjectByGameObjectID(nwn_objid_t objectid) {

	return pGetObjectByGameObjectID(GetCServerExoApp(), NULL, objectid);
}

//CServerExoApp::GetCreatureByGameObjectID(nwn_objid_t)
CNWSCreature * GetCreatureByGameObjectID(nwn_objid_t objectid){

	return pGetCreatureByGameObjectID(GetCServerExoApp(), NULL, objectid);
}

void RunScript(const char * script, nwn_objid_t obj){

	CExoString cescript;

	cescript.len = strlen(script);
	cescript.text = (char*)NWN2_Malloc(cescript.len + 1);

	strcpy(cescript.text, script);

	pRunScript(GetCVirtualMachine(), NULL, &cescript, obj, g_scriptArray, 1, 0);

	NWN2_Free(cescript.text);
}