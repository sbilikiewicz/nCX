/*************************************************************************
nCX dedicated server
Copyright (C), Sbilikiewicz.
https://github.com/sbilikiewicz
-------------------------------------------------------------------------
nCX_Init.cpp
-------------------------------------------------------------------------
History:
- 02/2019   :   Created by sbilikiewicz

*************************************************************************/
#include "StdAfx.h"
#include "nCX_Main.h"
#include "Game.h"
#include "GameRules.h"
#include "nCX_PCInfo.h"
#include "nCX_AI.h"
#include <IAISystem.h>
#include <ILevelSystem.h>
#include <Windows.h>
#include <iostream>
#include <fstream>
#include <cstdlib>

#define WIN32_LEAN_AND_MEAN
#include <stdio.h>

HANDLE *m_threads = NULL;

DWORD_PTR GetNumCPUs() {
	SYSTEM_INFO m_si = { 0, };
	GetSystemInfo(&m_si);
	return (DWORD_PTR)m_si.dwNumberOfProcessors;
}

DWORD_PTR WINAPI nCX_Thread() {
	nCX::UpdateThread();
	return 0;
}

int wmain()
{
	DWORD_PTR c = GetNumCPUs();
	m_threads = new HANDLE[c];
	for (DWORD_PTR i = 0; i < c; i++)
	{
		LPDWORD m_id = 0;
		DWORD_PTR m_mask = 1 << i;
		m_threads[i] = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)nCX_Thread, (LPVOID)i, NULL, m_id);
		SetThreadAffinityMask(m_threads[i], m_mask);
		//wprintf(L"Creating Thread %d (0x%08x) Assigning to CPU 0x%08x\r\n", i, (LONG_PTR)m_threads[i], m_mask);
	}
	CryLogAlways("[$6nCX$5] : Created multithread instances for %d CPU cores", c);
	return 0;
}
void nCX::nCX_Multithread(){
	nCX_Thread();
}

void nCX::Init_nCX(){
	char path[256];
	CryGetCurrentDirectory(256, path);
	strcat(path, "\\");
	m_RootDir = path;
	if (gEnv->pScriptSystem->GetGlobalValue("nCX", m_nCXLuaTab))
	{
		gEnv->pRenderer->EnableTMU(true);
		gEnv->pRenderer->EnableSwapBuffers(true);
		gEnv->pRenderer->EnableFog(true);
		gEnv->pNetwork->EnableMultithreading(true);
		m_nCXLuaTab->SetValue("RootDir", (const char*)path);
		string Spacer = "***************************************";
		Spacer = Spacer + Spacer + Spacer;
		CryLogAlways(Spacer);
		
		CryLogAlways("[$6nCX$5] : CPU                     %s | %dMhz", nCX_PCInfo::GetCPU(), nCX_PCInfo::GetCPUSpeed());
		int flt = (gEnv->pSystem->GetUsedMemory() / 999) / 990;
		CryLogAlways("[$6nCX$5] : RAM                     %s | %dMB App", nCX_PCInfo::GetRAM(), flt);
		CryLogAlways("[$6nCX$5] : CPU Usage               %d %s", nCX_PCInfo::GetCpuUsage(), "%");
		m_RootDir.replace("\\", "/");
		CryLogAlways("[$6nCX$5] : ROOT Dir                %s", m_RootDir);
		CryLogAlways("[$6nCX$5] : System running for      %d minutes", nCX_PCInfo::GetSystemTime());
		CryLogAlways("[$6nCX$5] : Server GUID:            %s", gEnv->pGame->GetIGameFramework()->GetGameGUID());

		ICVar *pCVar = gEnv->pConsole->GetCVar("sv_DedicatedMaxRate");
		int MaxRate = pCVar->GetIVal();
		//CGame::m_CurrentServerRate = MaxRate;
		if (pCVar){
			CryLogAlways("[$6nCX$5] : Server Frame Rate       %d", pCVar->GetIVal());
		}
        
        ICVar *pAICVar = gEnv->pConsole->GetCVar("ai_UpdateInterval");
        int AIUpdateInterval = pAICVar->GetIVal();
        if (pAICVar)
            CryLogAlways("[$6nCX$5] : AI Update Interval       %d", pAICVar->GetIVal());
            
		CryLogAlways("[$6nCX$5] : Levels in system        %d", gEnv->pGame->GetIGameFramework()->GetILevelSystem()->GetLevelCount());

		string CurrMOD = gEnv->pCryPak->GetModDir();
		if (CurrMOD.find("nCX") == string::npos)
		{
			CryMessageBox("Change of mod name is not allowed due to assembly changes", "nCX - Error", 0);
			gEnv->pConsole->ExecuteString("quit"); // check this 
		}

		LoadConfig();
		LoadChatCommands();

		//Init AI
		if (nCX_AI::GetInstance()->Initialize())
			CryLogAlways("[$6nCX$5] : AI system initialized");
		else
			CryLogAlways("[$6nCX$5] : Failed to initiate AI system!");

		//Init multithreading
		//wmain();

		CryLogAlways("[$6nCX$5] : Starting nCX 3.0 on     %s", gEnv->pNetwork->GetHostName());
		CryLogAlways(Spacer);
	}
}

void nCX::LoadConfig(){
	//Assign nCX script
	SmartScriptTable nCX_Lua;
	gEnv->pScriptSystem->GetGlobalValue("nCX", nCX_Lua);
	string nCX_Dir = m_RootDir + "nCX/";
	//Create Logs
	m_LogRoot = nCX_Dir + "Logs\\";
	char date[10];
	_strdate(date);
	string Date = date;
	Date.replace("/", "-");
	m_LogRoot = m_LogRoot + Date + "\\";
	gEnv->pCryPak->MakeDir(m_LogRoot.c_str());
	//Read Config
	if (!gEnv->pScriptSystem->ExecuteFile(nCX_Dir + "Config.lua", false))
	{
		CryMessageBox("Config file not found! Please check your setup.", "nCX - Error", 0);
		g_pGame->Shutdown();
	}
	SmartScriptTable config;
	if (!nCX_Lua->GetValue("Config", config))
	{
		CryMessageBox("Config table not found! Please check your setup.", "nCX - Error", 0);
		g_pGame->Shutdown();
	}
	//Read Cvars
	SmartScriptTable cvars;
	if (config->GetValue("CVars", cvars))
	{
		IScriptTable::Iterator pCVar = cvars->BeginIteration();
		while (cvars->MoveNext(pCVar))
		{
			if (ICVar *CVar = gEnv->pConsole->GetCVar(pCVar.sKey))
				CVar->ForceSet(pCVar.value.str);
		}
	}
	//Read censor words
	SmartScriptTable censored;
	if (config->GetValue("Censor", censored))
	{
		IScriptTable::Iterator pCens = censored->BeginIteration();
		while (censored->MoveNext(pCens))
		{
			stl::push_back_unique(m_CensorList, nCX_PCInfo::RegexBuilder(pCens.value.str));
		}
		censored->EndIteration(pCens);
		if (m_CensorList.size() > 0)
			m_UseChatCensor = true;
	}
	CryLogAlways("[$6nCX$5] : Config.lua loaded successfully");
}

void nCX::LoadChatCommands(){
	m_nCXLuaTab->SetValue("ChatCommand", m_nCXLuaTab);
	std::ifstream inputFile;
	WIN32_FIND_DATA FindData;
	HANDLE hFind;
	hFind = FindFirstFile(m_RootDir + "nCX/ChatCommands/*.lua", &FindData);
	int CommandCounter = 1;
	gEnv->pScriptSystem->ExecuteFile(FindData.cFileName, false);

	while (FindNextFile(hFind, &FindData))
	{
		gEnv->pScriptSystem->ExecuteFile(FindData.cFileName, false);
		++CommandCounter;
	}

	inputFile.close();
	CryLogAlways("[$6nCX$5] : Loaded %d chat commands", CommandCounter);
}

void nCX::LoadChatEntities(){
	if (!m_ChatEnts.empty())
		m_ChatEnts.clear();

	SmartScriptTable CEnts;
	SmartScriptTable nCX_Lua;
	gEnv->pScriptSystem->GetGlobalValue("nCX", nCX_Lua);
	if (nCX_Lua->GetValue("Config", CEnts))
	{
		CEnts->GetValue("ChatEntities", CEnts);
		if (CEnts->Count() != 0)
		{
			IScriptTable::Iterator iter = CEnts->BeginIteration();
			while (CEnts->MoveNext(iter))
			{
				IEntitySystem *pEntity = gEnv->pEntitySystem;
				if (pEntity)
				{
					SEntitySpawnParams params;
					params.vPosition(0, 0, iter.nKey);
					params.pClass = pEntity->GetClassRegistry()->FindClass("Reflex");
					params.sName = iter.value.str;
					IEntity *pChat = pEntity->SpawnEntity(params);
					m_ChatEnts.push_back(pChat->GetId());
				}
			}
			CEnts->EndIteration(iter);
		}
	}
}
