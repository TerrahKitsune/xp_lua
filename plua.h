/***************************************************************************
This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

***************************************************************************/
#pragma once
#define DLLEXPORT extern "C" __declspec(dllexport)

#include "windows.h"
#include "../plugin.h"
#include "../../misc/log.h"
#include "wx/tokenzr.h"
#include "wx/hashset.h"

#include "LuaEngine.h"

class PLua : public Plugin
{
public:
	PLua();
	~PLua();

	bool Init(TCHAR* nwnxhome);

	int GetInt(char* sFunction, char* sParam1, int nParam2) { return 0; }
	void SetInt(char* sFunction, char* sParam1, int nParam2, int nValue) {};
	float GetFloat(char* sFunction, char* sParam1, int nParam2) { return 0.0; }
	void SetFloat(char* sFunction, char* sParam1, int nParam2, float fValue) {};
	void SetString(char* sFunction, char* sParam1, int nParam2, char* sValue);
	char* GetString(char* sFunction, char* sParam1, int nParam2);
	void GetFunctionClass(TCHAR* fClass);

private:
	LuaEngine * Engine;
	wxLogNWNX * logger;
	char * buffer;
};