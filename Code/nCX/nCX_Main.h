#pragma once
#include <iostream>
#include <IGameObject.h>
#include <IGameRulesSystem.h>
#include "Actor.h"

class nCX
{
public:

	//Override calls from Game classes
	static void Init()							{ nCX i; i.Init_nCX(); };
	static void InitChat()						{ nCX e; e.LoadChatEntities(); };
	static bool IsAdmin(int channelId)			{ nCX a;  return stl::find(a.m_Admins, channelId); };
	static void Update(float frameTime)			{ nCX f; f.OnUpdate(frameTime); };
	static int CurrentChannel(int Id, bool set) { nCX c; if (set) { c.m_CurrChannel = Id; } else return c.m_CurrChannel; }
	static const char* GetIP(int channelId)		{ nCX ip; return ip.GetIPAddress(channelId); };
	static const char* GetID(int channelId)		{ nCX id; return id.GetProfileId(channelId); };
	//Event overrides
	static void ClientConnect(CActor *pActor, int channelId, bool reset) { nCX con; con.OnClientConnect(pActor, channelId, reset); };
	static void ClientDisconnect(CActor *pActor, int channelId, const char *desc) { nCX dcon; dcon.OnClientDisconnect(pActor, channelId, desc); };
	static bool ChatMessage(int channelId, int sourceId, int type, string msg) { nCX chat; return chat.OnChatMessage(channelId, sourceId, type, msg); };
	static bool RadioMessage(int channelId, int sourceId, int msg) { nCX chat; return chat.OnRadioMessage(channelId, sourceId, msg); };
	static void TeamChange(int channelId, int entityId, int teamId) { nCX team; return team.OnChangeTeam(channelId, entityId, teamId); };
	static void ChangeSpectatorMode(int channelId, int entityId, int targetId, int mode, bool resetAll) { nCX spec; spec.OnChangeSpectatorMode(channelId, entityId, targetId, mode, resetAll); };
	static void SimpleHit(int channelId, int shooterId, int targetId, int type, float value, int weaponId) { nCX hit;  hit.OnSimpleHit(channelId, shooterId, targetId, type, value, weaponId); }
	static void Hit(HitInfo params, int channelId) { nCX hi; hi.OnHit(params, channelId); };
	static void LogFile(const char* which, const char* msg) { nCX log; log.LogToFile(which, msg); };
	//Functions
	string					CensorCheck(string msg);
	virtual void			MsgFromChatEntity(int ent, int channelId, const char *msg);
	virtual void			PingGuard(int ping, int channelId);
	virtual int				GetAveragePing() { return m_AveragePing; };
	virtual const char*		GetProfileId(int channelId);
	virtual const char*		GetIPAddress(int channelId);
	virtual void			GetCountry(int channelId, string &country, string &shortc);
	virtual const char*		GetPort(int channelId);
	virtual const char*		GetHostName(int channelId);
	virtual bool			BanPlayer(int channelId, int time, const char* Reason, const char* BannedBy);
	virtual bool			UnbanPlayer(int index, const char* UnbannedBy);
	virtual bool			LogToFile(const char* which, const char* msg);
	virtual void			LogToConsole(const char* msg, int channelId = 0, bool toOther = false);
	virtual void			SaveBanList(const char* line = "");
	virtual void			ClientPacket(int channelId, const char* msg);
	virtual void			SendAdminMessage(int type, const char *msg);

	//Values
	string						m_RootDir;
	string						m_LogRoot;
	SmartScriptTable			m_nCXLuaTab;
	std::vector<string>			m_CensorList;
	std::vector<EntityId>		m_ChatEnts;
	bool						m_UseChatCensor;

protected:
	//Functions
	void	Init_nCX();
	void	LoadConfig();
	void	LoadChatCommands();
	void	LoadChatEntities();
	void	OnUpdate(float frameTime);
	void	TickTimer();
	void	MinTimer();
	void	SeqTimer();
	//Event Functions
	virtual void	OnClientConnect(CActor *pActor, int channeldId, bool reset);
	virtual void	OnClientDisconnect(CActor *pActor, int channelId, const char *desc);
	virtual bool	OnChatMessage(int channelId, int sourceId, int type, string msg);
	virtual bool	OnRadioMessage(int channelId, int sourceId, int msg);
	virtual void	OnChangeTeam(int channelId, int entityId, int teamId);
	virtual void	OnChangeSpectatorMode(int channelId, int entityId, int targetId, int mode, bool resetAll);
	virtual void	OnSimpleHit(int channelId, int shooterId, int targetId, int type, float value, int weaponId);
	virtual void	OnHit(HitInfo params, int channelId);
	//Values
	float				m_TickTimer;
	int					m_MinTimer;
	int					m_SeqTimer;
	int					m_AveragePing;
	int					m_PlayerCount;
	int					m_CurrChannel;
	SmartScriptTable	m_onClientScript;
	std::vector<int>	m_Admins;

	//Structs
	inline void ConvertWString(const wstring& s, string& d)
	{
		d.resize(wstring::_strlen(s));
		char* dst = d.begin();
		const wchar_t* src = s;

		while (char c = (char)(*src++ & 0xFF))
		{
			*dst++ = c;
		}
	}
	struct SBanSystem
	{
		SBanSystem() {};
		SBanSystem(int time, string ID, string IP, string Name, string Host, string Date, string Reason, string BannedBy) : time(time), ID(ID), IP(IP), Name(Name), Host(Host), Date(Date), Reason(Reason), BannedBy(BannedBy) {};

		int time;
		string ID;
		string IP;
		string Name;
		string Host;
		string Date;
		string Reason;
		string BannedBy;
	};
	typedef std::map<int, SBanSystem> TBanMap;
	TBanMap	m_BanSystem;
	
};