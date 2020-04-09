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

#include "plua.h"
#include "./NWN2_API/NWN2_API.h"

/***************************************************************************
NWNX and DLL specific functions
***************************************************************************/

PLua* plugin;

DLLEXPORT Plugin* GetPluginPointerV2()
{
	return plugin;
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
	if (ul_reason_for_call == DLL_PROCESS_ATTACH)
	{
		plugin = new PLua();

		TCHAR szPath[MAX_PATH];
		GetModuleFileName(hModule, szPath, MAX_PATH);
		plugin->SetPluginFullPath(szPath);
	}
	else if (ul_reason_for_call == DLL_PROCESS_DETACH)
	{
		delete plugin;
	}
	return TRUE;
}


/***************************************************************************
Implementation of Lua Plugin
***************************************************************************/

PLua::PLua()
{
	header = _T(
		"NWNX Lua Plugin V.0.3.69\n" \
		"(c) 2020 by Robin Karlsson (Terrahkitsune)\n" \
		"Lua (c) PUC-Rio: https://www.lua.org/ \n"\
		"visit us at http://www.nwnx.org\n");

	description = _T(
		"This plugin provides a lua engine and environment.");

	subClass = _T("LUA");
	version = _T("0.3.69");
	buffer = NULL;
	Engine = new LuaEngine();

	NWN2_InitMem();
}

PLua::~PLua()
{
	wxLogMessage(wxT("* Plugin unloaded."));
	delete Engine;
}

bool PLua::Init(TCHAR* nwnxhome)
{
	assert(GetPluginFileName());

	/* Log file */
	wxString logfile(nwnxhome);
	logfile.append(wxT("\\"));
	logfile.append(GetPluginFileName());
	logfile.append(wxT(".txt"));
	logger = new wxLogNWNX(logfile, wxString(header.c_str()));

	wxLogMessage(wxT("* Plugin initialized."));

	return true;
}

void PLua::GetFunctionClass(TCHAR* fClass)
{
	_tcsncpy_s(fClass, 128, wxT("LUA"), 4);
}

void PLua::SetString(char* sFunction, char* sParam1, int nParam2, char* sValue)
{
	//wxLogTrace(TRACE_VERBOSE, wxT("* Plugin SetString(0x%x, %s, %d, %s)"), 0x0, sParam1, nParam2, sValue);

#ifdef UNICODE
	wxString wxRequest(sFunction, wxConvUTF8);
	wxString function(sFunction, wxConvUTF8);
	wxString timerName(sParam1, wxConvUTF8);
#else
	wxString wxRequest(sFunction);
	wxString function(sFunction);
	wxString timerName(sParam1);
#endif


	if (function == wxT(""))
	{
		wxLogMessage(wxT("* Function not specified."));
		return;
	}
	else if (function == wxT("Log")) {
		Engine->Log = nParam2 > 0;
		wxLogMessage(wxT("* Logging Set to = %d"), Engine->Log);
	}
	else{

		if(Engine->Log)
			wxLogMessage(wxT("* %s(%s, %d, %s)"), sFunction, sParam1, nParam2, sValue);

		char * result = Engine->RunFunction(sFunction, sParam1, nParam2, sValue);
		if (result)
			delete[]result;
		else if (Engine->GetLastError()) {
			if (Engine->Log)
				wxLogMessage(wxT("! %s"), Engine->GetLastError());
		}
	}
}

char* PLua::GetString(char* sFunction, char* sParam1, int nParam2)
{
	//wxLogTrace(TRACE_VERBOSE, wxT("* Plugin GetString(0x%x, %s, %d)"), 0x0, sParam1, nParam2);

#ifdef UNICODE
	wxString wxRequest(sFunction, wxConvUTF8);
	wxString function(sFunction, wxConvUTF8);
	wxString timerName(sParam1, wxConvUTF8);
#else
	wxString wxRequest(sFunction);
	wxString function(sFunction);
	wxString timerName(sParam1);
#endif
	if (buffer){
		delete[]buffer;
		buffer = NULL;
	}

	if (function == wxT(""))
	{
		if (Engine->Log)
			wxLogMessage(wxT("* Function not specified."));
		return NULL;
	}
	else if (function == wxT("RunString"))
	{
		if (Engine->Log)
			wxLogMessage(wxT("* RunString( %s )"), sParam1);
		buffer = Engine->RunString(sParam1, "RunString");
		if (buffer) {
			if (Engine->Log)
				wxLogMessage(wxT("= %s"), buffer);
		}
		else if (Engine->GetLastError()) {
			if (Engine->Log)
				wxLogMessage(wxT("! %s"), Engine->GetLastError());
		}
	}
	else if (function == wxT("LastError"))
	{
		const char * err = Engine->GetLastError();
		if (err){
			buffer = new char[strlen(err)+1];
			if (buffer)
				strcpy(buffer, err);
		}

		return buffer;
	}

	return buffer;
}