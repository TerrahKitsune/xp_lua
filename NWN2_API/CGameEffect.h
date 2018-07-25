#pragma once
#include "NWN2_API.h"

struct CGameEffect {

	DWORD LinkId; //0
	DWORD dword4; //4
	DWORD Type; //8
	DWORD SubType; //C
	float Duration; //10
	DWORD ExpireDay; //14
	DWORD ExpireTime; //18
	nwn_objid_t Creator; //1C
	int SpellId; //20
	DWORD IsExposed; //24
	DWORD ShowIcon; //28
	int CasterLevel; //2C
	DWORD dword30; //30
	DWORD dword34; //34
	int AllocEffectInts; //38
	int NumbEffectInts; //3C
	int * effectInts; //40
	DWORD dword44; //44
	CExoString EffectString[6]; //48
	nwn_objid_t EffectObjects[4]; //78
	DWORD dword88; //88
	DWORD dword8C; //8C
	DWORD dword90; //90
	DWORD dword94; //94
	DWORD dword98; //98
	DWORD dword9C; //9C

	//158
};