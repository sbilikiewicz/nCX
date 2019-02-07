/**********************************************************
####     ##  #####  ##   ##
## ##    ## ##   ##  ## ##  Crysis nCX
##  ##   ## ##        ###     by MrHorseDick
##   ##  ## ##        ###       and
##    ## ## ##   ##  ## ##        MrCtaostrach
##     ####  #####  ##   ##
**********************************************************/
#include "StdAfx.h"
#include "Init.h"
#include "Game.h"
#include "ExInfo.h"
#include "Coop\CoopSystem.h"
#include <IAISystem.h>
#include <ILevelSystem.h>

/*Multithreading
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdio.h>

HANDLE *m_threads = NULL;
DWORD_PTR WINAPI threadMain(void* p);

DWORD_PTR GetNumCPUs() {
	SYSTEM_INFO m_si = { 0, };
	GetSystemInfo(&m_si);
	return (DWORD_PTR)m_si.dwNumberOfProcessors;
}

int wmain() {

	DWORD_PTR c = GetNumCPUs();
	m_threads = new HANDLE[c];
	for (DWORD_PTR i = 0; i < c; i++) {
		DWORD_PTR m_id = 0;
		DWORD_PTR m_mask = 1 << i;
		m_threads[i] = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)threadMain, (LPVOID)i, NULL, &m_id);
		SetThreadAffinityMask(m_threads[i], m_mask);
		//wprintf(L"Creating Thread %d (0x%08x) Assigning to CPU 0x%08x\r\n", i, (LONG_PTR)m_threads[i], m_mask);
		CryLogAlways("[$6nCX$5] Creating multithread instance for core %d", i);
	}
	return 0;
}

DWORD_PTR WINAPI threadMain(void* p) {
	return 0;
}*/

void CInit::InitMod(){
	char path[256];
	CryGetCurrentDirectory(256, path);
	strcat(path, "\\");
	//m_Root = path;
	SmartScriptTable Root;
	//if (gEnv->pScriptSystem->GetGlobalValue("nCX", Root))
	//{
		gEnv->pRenderer->EnableTMU(true);
		gEnv->pRenderer->EnableSwapBuffers(true);
		gEnv->pRenderer->EnableFog(true);
		gEnv->pNetwork->EnableMultithreading(true);
		//Root->SetValue("ROOT", (const char*)path);
		string Spacer = "***************************************";
		Spacer = Spacer + Spacer + Spacer;
		CryLogAlways(Spacer);
		
		CryLogAlways("[$6nCX$5] : CPU                     %s | %dMhz", ExInfo::GetCPU(), ExInfo::GetCPUSpeed());
		int flt = (gEnv->pSystem->GetUsedMemory() / 999) / 990;
		CryLogAlways("[$6nCX$5] : RAM                     %s | %dMB App", ExInfo::GetRAM(), flt);
		CryLogAlways("[$6nCX$5] : CPU Usage               %d %s", ExInfo::GetCpuUsage(), "%");
		//m_Root.replace("\\", "/");
		//CryLogAlways("[$6nCX$5] : ROOT Dir                %s", m_Root);
		CryLogAlways("[$6nCX$5] : System running for      %d minutes", ExInfo::GetSystemTime());
		//CryLogAlways("[$6nCX$5] : Server GUID:            %s", m_pFramework);
		/*string FullRoot = m_Root + "\\Game\\Server\\nCX\\";
		gEnv->pScriptSystem->ExecuteFile(FullRoot + "Config.lua", false);
		SmartScriptTable config;
		if (!nCXTab->GetValue("Config", config))
		{
			CryMessageBox("Config not found! Please check your setup.", "nCX - Error", 0);
			Shutdown();
		}
		SmartScriptTable cvars;
		if (config->GetValue("CVars", cvars))
		{
			IScriptTable::Iterator pCVar = cvars->BeginIteration();
			while (cvars->MoveNext(pCVar))
			{
				if (ICVar *CVar = m_pConsole->GetCVar(pCVar.sKey))
					CVar->ForceSet(pCVar.value.str);
			}
		}*/
		
		ICVar *pCVar = gEnv->pConsole->GetCVar("sv_DedicatedMaxRate");
		if (pCVar){
			CryLogAlways("[$6nCX$5] : Server Frame Rate       %d", pCVar->GetIVal());
		}
		
		CryLogAlways("[$6nCX$5] : Levels in system        %d", gEnv->pGame->GetIGameFramework()->GetILevelSystem()->GetLevelCount());

		
		string CurrMOD = gEnv->pCryPak->GetModDir();
		if (CurrMOD.find("nCX") == string::npos)
		{
			CryMessageBox("Change of mod name is not allowed due to assembly changes", "nCX - Error", 0);
			gEnv->pConsole->ExecuteString("quit"); // check this 
		}
		/*
		if (!gEnv->pScriptSystem->ExecuteFile(FullRoot + "Start.lua", false))
		{
			CryMessageBox("Execution of Start.lua failed! Please check nigga!", "nCX - Error", 0);
			Shutdown();
		}*/
		
		//Init AI
		CCoopSystem::GetInstance()->Initialize();
		CryLogAlways("[$6nCX$5] : Initializing AI System");

		CryLogAlways("[$6nCX$5] : Starting nCX 3.0 on     %s", gEnv->pNetwork->GetHostName());
		CryLogAlways(Spacer);
		//Multithreading
		//wmain();
		
		/*int x = 0;
		while (x == x){
			++x;
		}*/
		//
	//}
}