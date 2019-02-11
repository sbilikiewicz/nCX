/*************************************************************************
  Crytek Source File.
  Copyright (C), Crytek Studios.
 -------------------------------------------------------------------------
  DllMain.cpp
  Dll Entry point to capture dll instance handle (needed for loading of embedded resources)
 -------------------------------------------------------------------------
  History:
  - 11/2007   :   Created by Marco Koegler
  - 02/2019   :   Edited and optimized by sbilikiewicz
                  https://github.com/sbilikiewicz
                  
*************************************************************************/
#include "StdAfx.h"
#if defined(WIN32) && !defined(XENON)
#include <windows.h>

void* g_hInst = 0;

BOOL APIENTRY DllMain ( HINSTANCE hInst, DWORD reason, LPVOID reserved )
{
	if ( reason == DLL_PROCESS_ATTACH )
		g_hInst = hInst;
	return TRUE;
}
#endif