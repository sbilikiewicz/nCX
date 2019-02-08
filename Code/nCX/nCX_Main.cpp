/**********************************************************
             #####  ##   ##
 ####    ## ##   ##  ## ##  Crysis nCX V3.0
##  ##   ## ##        ###     by MrHorseDick
##   ##  ## ##        ###       and
##    ## ## ##   ##  ## ##        MrCtaoistrach
##     ####  #####  ##   ##
**********************************************************/
#include "StdAfx.h"
#include "nCX_Main.h"
#include "nCX_PCInfo.h"
#include "GameRules.h"
#include "IVehicleSystem.h"
#include <regex>

string nCX::CensorCheck(string msg)
{
	std::string str(msg.c_str());
	for (std::vector<string>::const_iterator it = m_CensorList.begin(); it != m_CensorList.end(); ++it)
	{
		std::regex reg(it->c_str());
		str = std::regex_replace(str, reg, "<censored>");
	}
	msg = string(str.c_str());
	return msg;
}

void nCX::MsgFromChatEntity(int ent, int channelId, const char *msg)
{
	if (!m_ChatEnts.at(ent))
		return;

	if (CGameRules *pRules = g_pGame->GetGameRules())
	{
		if (channelId > 0)

			pRules->GetGameObject()->InvokeRMI(pRules->ClChatMessage(), CGameRules::ChatMessageParams(eChatToTarget, m_ChatEnts.at(ent), 1, msg, false), 0x01, channelId);
		else if (channelId == 0)
			pRules->GetGameObject()->InvokeRMI(pRules->ClChatMessage(), CGameRules::ChatMessageParams(eChatToAll, m_ChatEnts.at(ent), 1, msg, false), 0x08);
		else
		{
			CGameRules::ChatMessageParams params(eChatToTarget, m_ChatEnts.at(ent), 1, msg, false);
			for (std::vector<int>::const_iterator con = m_Admins.begin(); con != m_Admins.end(); ++con)
				pRules->GetGameObject()->InvokeRMI(pRules->ClChatMessage(), params, 0x01, *con);
		}
	}
}

void nCX::PingGuard(int ping, int channelId)
{
	if (CActor *pActor = static_cast<CActor *>(g_pGame->GetGameRules()->GetActorByChannelId(channelId)))
	{
		int limit = gEnv->pConsole->GetCVar("nCX_HighPingLimit")->GetIVal();
		bool admin = IsAdmin(channelId) && (ping <= limit * 2);
		string msg;
		++pActor->m_HighPing;
		switch (pActor->m_HighPing)
		{
		case 5:
			MsgFromChatEntity(4, channelId, msg.Format("(1/3) Your Ping is too high (%dms, Limit: %d)", ping, limit).c_str());
			break;
		case 10:
			MsgFromChatEntity(4, channelId, msg.Format("(2/3) Your Ping is still too high (%dms,  Limit: %d)", ping, limit).c_str());
			break;
		case 15:
			msg.Format("(3/3) Your Ping is still too high!");
			msg.append(admin ? " Fix your connection..." : " You will be kicked in 10 seconds...");
			MsgFromChatEntity(4, channelId, msg.c_str());
			break;
		case 25:
			if (!admin)
			{
				string info;
				info.Format("Player %s was kicked from Server (Ping: %dms, Limit: %d)", pActor->GetEntity()->GetName(), ping, limit);
				LogToConsole(info.c_str(), channelId, true);
				info.insert(0, "[ PING:GUARD ] ");
				g_pGame->GetGameRules()->SendTextMessage(eTextMessageError, info.c_str(), 0x04, channelId);
				BanPlayer(channelId, -1, msg.Format("High Ping (ping: %dms / limit: %d)", ping, limit).c_str(), "Ping Guard");
			}
			else
				pActor->m_HighPing = 0;

			break;
		}
	}
}

const char* nCX::GetProfileId(int channelId)
{
	string ProfileId;
	if (INetChannel *pNetChannel = g_pGame->GetIGameFramework()->GetNetChannel(channelId))
		return ProfileId.Format("%d", pNetChannel->GetProfileId()).c_str();

	return "";
}

const char* nCX::GetIPAddress(int channelId)
{
	if (INetChannel *pNetChannel = g_pGame->GetIGameFramework()->GetNetChannel(channelId))
	{
		#if defined(WIN64)
			const int offset = 0xd0;
		#else
			const int offset = 0x78;
		#endif
		unsigned char* p = &(((unsigned char*)pNetChannel)[offset + 3]);
		unsigned int ip = *p--;
		ip = (ip << 8) + *p--;
		ip = (ip << 8) + *p--;
		ip = (ip << 8) + *p;
		string sAddress;
		unsigned char octet[4] = { 0, 0, 0, 0 };
		for (int i = 0; i<4; ++i)
			octet[i] = (ip >> (i * 8)) & 0xFF;

		return sAddress.Format("%d.%d.%d.%d", (int)octet[3], (int)octet[2], (int)octet[1], (int)octet[0]).c_str();
	}
	return "";
}

void nCX::GetCountry(int channelId, string &country, string &shortc)
{
	if (INetChannel *pNetChannel = g_pGame->GetIGameFramework()->GetNetChannel(channelId))
	{
		string IP = GetIPAddress(channelId);
		shortc = nCX_PCInfo::query(IP.c_str());
		if (!shortc.empty())
		{
			shortc = shortc.MakeUpper();
			wstring tmp;
			if (shortc == "GB") // doesn't detect GB 
				shortc = "UK";

			if (gEnv->pSystem->GetLocalizationManager()->LocalizeString((string)"@ui_country_" + shortc, tmp, true))
				ConvertWString(tmp, country);

			if (!shortc.compareNoCase("rs"))
				country = "Serbia";
			else if (country.find("@") != -1)
			{
				CryLogAlways("Localization : Localizing failed (%s, %s)", shortc, GetHostName(channelId));
				country = "Europe";
				shortc = "EU";
			}
		}
		else
		{
			CryLogAlways("Localization : Query failed (%s)", GetHostName(channelId));
			country = "Europe";
			shortc = "EU";
		}
	}
}

const char* nCX::GetPort(int channelId)
{
	if (INetChannel *pNetChannel = g_pGame->GetIGameFramework()->GetNetChannel(channelId))
	{
		#if defined(WIN64)
			const int offset = 0xd4;
		#else
			const int offset = 0x7c;
		#endif
		unsigned char* p = &(((unsigned char*)pNetChannel)[offset + 1]);
		unsigned int port = *p--;
		port = (port << 8) + *p;
		string PT;
		return PT.Format("%d", (int)port).c_str();
	}
	return "";
}

const char* nCX::GetHostName(int channelId)
{
	if (INetChannel *pNetChannel = g_pGame->GetIGameFramework()->GetNetChannel(channelId))
	{
		int pos = 0;
		string full(pNetChannel->GetName());
		string host(full.Tokenize(":", pos));
		if (!host.empty())
			return host.c_str();
		else
			return full.c_str();
	}
	return "";
}

bool nCX::BanPlayer(int channelId, int time, const char* Reason, const char* BannedBy)
{
	if (IsAdmin(channelId))
		return false;

	if (CActor *pActor = static_cast<CActor *>(g_pGame->GetGameRules()->GetActorByChannelId(channelId)))
	{
		if (INetChannel *pNetChannel = gEnv->pGame->GetIGameFramework()->GetNetChannel(channelId))
		{
			string Name = pActor->GetEntity()->GetName();
			if (time == -1)
			{
				pNetChannel->Disconnect(eDC_Kicked, Reason);
				string msg;
				LogToFile("Action", msg.Format("%s kicked %s for %s", BannedBy, Name, Reason).c_str());
			}
			else
			{
				char ndate[10];
				_strdate(ndate);
				char ntime[10];
				_strtime(ntime);

				string date = (string)ndate + " | " + (string)ntime;

				string ID = GetProfileId(channelId);
				string IP = GetIPAddress(channelId);
				string Host = GetHostName(channelId);

				m_BanSystem.insert(TBanMap::value_type(m_BanSystem.size() + 1, SBanSystem(time, ID, IP, Name, Host, date, Reason, BannedBy)));
				pNetChannel->Disconnect(eDC_Banned, Reason);

				string add;
				SaveBanList(add.Format("nCX.LoadBan('%s', '%s', '%s', '%s', '%s', '%s', '%s', %d); \n", Name.c_str(), ID.c_str(), IP.c_str(), Host.c_str(), date.c_str(), Reason, BannedBy, time).c_str());

				string msg;
				msg.Format("%s banned %s for %s", BannedBy, Name.c_str(), Reason);
				if (time > 0)
				{
					string add;
					msg.append(add.Format(" (%d minutes)", time).c_str());
				}
				LogToFile("Action", msg.c_str());
			}
			return true;
		}
	}
	return false;
}

bool nCX::UnbanPlayer(int index, const char* UnbannedBy)
{
	TBanMap::iterator eit = m_BanSystem.find(index);
	if (eit != m_BanSystem.end())
	{
		string msg;
		LogToFile("Action", msg.Format("%s unbanned %s (%s, %s, %s)", UnbannedBy, eit->second.Name.c_str(), eit->second.ID.c_str(), eit->second.IP.c_str(), eit->second.Host.c_str()).c_str());
		m_BanSystem.erase(eit);
		SaveBanList();
		return true;
	}
	else
		return false;

}

bool nCX::LogToFile(const char* which, const char* msg, bool console)
{
	if (console)
		CryLogAlways("[Umbra %s] %s", which, msg);

	string file;
	FILE * pLog = fopen(file.Format("%s%s.log", m_LogRoot.c_str(), which).c_str(), "a");
	if (pLog != NULL)
	{
		char time[10];
		_strtime(time);
		string put;
		fputs(put.Format("[%s] %s\n", time, msg).c_str(), pLog);
		fclose(pLog);
		return true;
	}
	return false;
}

void nCX::LogToConsole(const char* msg, int channelId, bool toOther)
{
	//int space = g_pGameCVars->nCX_ConsoleSpace+9;//why do we have a cvar for that.. it wont change anytime
	//string placeholder("                                                                           ");
	/*if (space>0 && placeholder.size()>space)
	placeholder.resize(space);
	else if (space <= 0)
	placeholder = "";*/

	string info;
	info.Format("%40s %s", "$4nCX $9:", msg).c_str();

	if (channelId == 0)
		g_pGame->GetGameRules()->SendTextMessage(eTextMessageConsole, info, 0x08);
	else if (toOther)
		g_pGame->GetGameRules()->SendTextMessage(eTextMessageConsole, info, 0x04, channelId);
	else
		g_pGame->GetGameRules()->SendTextMessage(eTextMessageConsole, info, 0x01, channelId);
}

void nCX::SaveBanList(const char* line)
{
	string adds = line;
	bool overwrite = adds.length() == 0;
	FILE * pBan = fopen(m_RootDir + "//nCX//" + "BanList.lua", overwrite ? "w" : "a");
	if (pBan != NULL)
	{
		if (overwrite)
		{
			for (TBanMap::const_iterator eit = m_BanSystem.begin(); eit != m_BanSystem.end(); ++eit)
			{
				string add;
				fputs(add.Format("nCX.LoadBan('%s', '%s', '%s', '%s', '%s', '%s', '%s', %d); \n", eit->second.Name, eit->second.ID, eit->second.IP, eit->second.Host, eit->second.Date, eit->second.Reason, eit->second.BannedBy, eit->second.time).c_str(), pBan);
			}
		}
		else
			fputs(line, pBan);
		
		if (IScriptSystem *pScript = gEnv->pScriptSystem) //Lua
		{
			pScript->BeginCall(m_nCXLuaTab, "OnFileChange");
			pScript->PushFuncParam(m_nCXLuaTab);
			pScript->PushFuncParam("BanList");
			pScript->PushFuncParam(fseek(pBan, 0, SEEK_END));
			pScript->EndCall();
			fclose(pBan);
		}
	}
}

void nCX::ClientPacket(int channelId, const char* msg)
{
	if (channelId > 0)
	{
		CActor *pActor = g_pGame->GetGameRules()->GetActorByChannelId(channelId);
		if (IScriptSystem *pScript = gEnv->pScriptSystem) //Lua
		{
			pScript->BeginCall(m_onClientScript, "ClWorkComplete");
			pScript->PushFuncParam(m_onClientScript);
			pScript->PushFuncParam(channelId);
			pScript->PushFuncParam(ScriptHandle(pActor->GetEntityId()));
			pScript->PushFuncParam(msg);
			pScript->EndCall();
		}
	}
}

void nCX::SendAdminMessage(int type, const char *msg)
{
	if (CGameRules *pRules = g_pGame->GetGameRules())
	{
		for (std::vector<int>::const_iterator con = m_Admins.begin(); con != m_Admins.end(); ++con)
		{
			if (type > 3)
			{
				if (type == 4)
					LogToConsole(msg, *con);
				else
				{
					int msgtype = type - 3;
					pRules->SendTextMessage((ETextMessageType)msgtype, msg, 0x01, *con);
				}
			}
			else
				pRules->GetGameObject()->InvokeRMI(pRules->ClSetObjectiveStatus(), CGameRules::SetObjectiveStatusParams(msg, type), 0x01, *con);
		}
	}
}

void nCX::OnUpdate(float frameTime)
{
	if ((frameTime - m_TickTimer) > 1.0f)
	{
		TickTimer();
		++m_MinTimer;
		m_TickTimer = frameTime;
		if (m_MinTimer > 59)
		{
			MinTimer();
			m_MinTimer = 0;
			++m_SeqTimer;
			if (m_SeqTimer > 9)
			{
				m_SeqTimer = 0;
				SeqTimer();
			}
		}
	}
}

void nCX::TickTimer()
{
	int PlayerCount = 0;
	float AveragePing = 0.0f;
	if (CGameRules *pRules = g_pGame->GetGameRules())
	{
		for (std::vector<int>::const_iterator it = pRules->m_channelIds.begin(); it != pRules->m_channelIds.end(); ++it)
		{
			if (INetChannel *pNetChannel = g_pGame->GetIGameFramework()->GetNetChannel(*it))
			{
				if (CActor *pActor = static_cast<CActor *>(g_pGame->GetGameRules()->GetActorByChannelId(*it)))
				{
					//Ping procession
					int ping = pNetChannel->GetPing(true) * 1000;
					int SmoothedPing = floor(ping - (ping*0.2));
					ping = SmoothedPing<1 ? 1 : SmoothedPing;

					AveragePing = AveragePing + ping;
					pRules->SetSynchedEntityValue(pActor->GetEntityId(), 103, ping);

					if (ping > gEnv->pConsole->GetCVar("nCX_HighPingLimit")->GetIVal())
						PingGuard(ping, *it);

					pActor->SequenceChecks();
					++PlayerCount;
				}
			}
		}
		m_PlayerCount = PlayerCount;
		m_AveragePing = PlayerCount>0 ? floor(AveragePing / PlayerCount) : 0;
		/*Update defined lua timers
		for (unsigned int i = 0; i < m_TimerTable.size(); i++)
		{
			if (m_TimerTable[i])
			{
				IEntity* pTimer = gEnv->pEntitySystem->GetEntity(m_TimerTable[i]);
				if (pTimer && pTimer != NULL)
				{
					IScriptTable *pScriptTable = pTimer->GetScriptTable();
					if (pScriptTable && pScriptTable->HaveValue("OnTimer"))
					{
						m_pScriptSystem->BeginCall(pScriptTable, "OnTimer");
						m_pScriptSystem->PushFuncParam(pScriptTable);
						m_pScriptSystem->EndCall();
					}
				}
			}
		}*/
		//Unfreeze frozen vehicles
		CGameRules::TFrozenEntities::iterator fit;
		for (fit = pRules->m_frozen.begin(); fit != pRules->m_frozen.end();)
		{
			if (g_pGame->GetIGameFramework()->GetIVehicleSystem()->GetVehicle(fit->first))
			{
				EntityId id = fit->first;
				pRules->m_frozen.erase(fit);
				pRules->FreezeEntity(id, false, false);
				fit = pRules->m_frozen.begin();
			}
			else
				++fit;
		}
		//Scan vehicles to register them with AI system
		//TODO: move this to all vehicle spawn functions
		IVehicleIteratorPtr iter = gEnv->pGame->GetIGameFramework()->GetIVehicleSystem()->CreateVehicleIterator();
		while (IVehicle* pVehicle = iter->Next())
		{
			if (IEntity *pEntity = pVehicle->GetEntity())
			{
				if (!pEntity->GetAI())
				{
					gEnv->bMultiplayer = false;
					IScriptTable* pScriptTable = pEntity->GetScriptTable();
					if (pScriptTable)
					{
						gEnv->pScriptSystem->BeginCall(pScriptTable, "RegisterVehicleAI");
						gEnv->pScriptSystem->PushFuncParam(pScriptTable);
						gEnv->pScriptSystem->EndCall(pScriptTable);
					}
					gEnv->bMultiplayer = true;
				}
			}
		}

		//UpdateEntitySchedules
		//UpdateEntitySchedules();

		//Check voting
		//if (!m_VoteSystem.empty())
		//	CheckVotingProgress();

		//NetChannelScanner
		int ChanVal = m_CurrChannel + 1;
		if (INetChannel *pNetChannel = g_pGame->GetIGameFramework()->GetNetChannel(ChanVal))
		{
			++m_CurrChannel;
			bool access = false;
			string IP = GetIPAddress(ChanVal);
			string ID = GetProfileId(ChanVal);
			//admins set onclientconnect
			for (std::vector<int>::const_iterator con = m_Admins.begin(); con != m_Admins.end(); ++con)
			{
				if (*con == ChanVal)
					access = true;
				else
				{
					string msg("New connection from [ " + (string)pNetChannel->GetNickname() + " : " + IP + " ]");
					pRules->GetGameObject()->InvokeRMI(pRules->ClSetObjectiveStatus(), CGameRules::SetObjectiveStatusParams(msg, 2), 0x01, *con);
				}
			}
			//
			int maxplayers = gEnv->pConsole->GetCVar("sv_maxplayers")->GetIVal();
			if ((maxplayers != 0) && (m_PlayerCount >= maxplayers))
			{
				//if (!access)
				pNetChannel->Disconnect(eDC_ServerFull, "Server full");

				return;
			}
			for (TBanMap::const_iterator entry = m_BanSystem.begin(); entry != m_BanSystem.end(); ++entry)
			{ //entry->second.ID == ID
				if (entry->second.IP == IP)
				{
					pNetChannel->Disconnect(eDC_Banned, entry->second.Reason.c_str());
					string msg;
					string nickName = pNetChannel->GetNickname();
					if (nickName.empty())
						nickName = "Nomad";

					SendAdminMessage(4, msg.Format("Connection from %s (%s) rejected (#%d, $4%s$9)", nickName, IP.c_str(), entry->first, entry->second.Reason.c_str()).c_str());
					string log;
					string Host = GetHostName(ChanVal);
					string Port = GetPort(ChanVal);
					LogToFile("Connection", log.Format("%-26s | %-2s | %-5d | %-9s | %-15s | %-5s | %s (Connection Rejected : #%d, %s)", nickName, "EU", ChanVal, ID.c_str(), IP.c_str(), Port.c_str(), Host.c_str(), entry->first, entry->second.Reason.c_str()).c_str());
					break;
				}
			}
		}
	}
}

void nCX::MinTimer()
{
	//Time bans
	/*if (!m_BanSystem.empty())
	{
		for (TBanMap::iterator eit = m_BanSystem.begin(); eit != m_BanSystem.end(); ++eit)
		{
			SBanSystem &ban = eit->second;
			if (ban.time != 0)
			{
				--ban.time;
				if (ban.time == 0)
				{
				string info;
				info.Format("Ban for player %s (%s) expired", ban.Name.c_str(), ban.IP.c_str()).c_str();
				SendAdminMessage(4, info);
				LogToFile("Action", info);
				m_BanSystem.erase(eit);
				m_ResortBanSystem = true;
				SaveBanList();
			}
		}
	}*/

	FILE * pBan = fopen(m_RootDir + "\\nCX\\" + "BanList.lua", "w");
	if (pBan != NULL)
	{
		for (TBanMap::const_iterator eit = m_BanSystem.begin(); eit != m_BanSystem.end(); ++eit)
		{
			string add;
			fputs(add.Format("nCX.LoadBan('%s', '%s', '%s', '%s', '%s', '%s', '%s', %d); \n", eit->second.Name, eit->second.ID, eit->second.IP, eit->second.Host, eit->second.Date, eit->second.Reason, eit->second.BannedBy, eit->second.time).c_str(), pBan);
		}
		if (IScriptSystem *pScript = gEnv->pScriptSystem) //Lua
		{
			pScript->BeginCall(m_nCXLuaTab, "OnFileChange");
			pScript->PushFuncParam(m_nCXLuaTab);
			pScript->PushFuncParam("BanList");
			pScript->PushFuncParam(fseek(pBan, 0, SEEK_END));
			pScript->EndCall();
			fclose(pBan);
		}
	}

	/*if (m_ResortBanSystem)
	{
	m_ResortBanSystem = false;
	m_BanSystem.clear();
	string file;
	gEnv->pScriptSystem->ExecuteFile(m_GlobalDataPath + "BanList.lua", false, true);
	}
	}*/

	/*Check logDir
	char date[10];
	_strdate(date);
	string Date = date;
	Date.replace("/", "-");
	Date = m_RootPath + "Logs\\" + Date + "\\";
	gEnv->pCryPak->MakeDir(Date.c_str());
	m_LogRoot = Date;*/
}

void nCX::SeqTimer()
{
	string msg;
	ICVar *pCVar = gEnv->pConsole->GetCVar("sv_maxplayers");
	int maxPlayers = pCVar->GetFVal();
	LogToFile("Performance", msg.Format("Players: %2d/%d   Memory Usage: %4d   Performance: %3d   Ping Average: %4d   Highest Channel: %4d", m_PlayerCount, maxPlayers, ((gEnv->pSystem->GetUsedMemory() / 999) / 990), g_pGame->m_ServerPerformance, m_AveragePing, m_CurrChannel).c_str());
}