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
#include "nCX_AntiCheat.h"
#include "GameRules.h"
#include "GameCvars.h"
#include "Weapon.h"
#include "IVehicleSystem.h"
#include "FreezingBeam.h"
#include "Player.h"
#include <regex>
#include <StlUtils.h>

void nCX::OnClientConnect(CActor *pActor, int channelId, bool reset)
{
	
}

void nCX::OnClientDisconnect(CActor *pActor, int channelId, const char *desc)
{
	m_CurrChannel = m_CurrChannel<channelId ? channelId : m_CurrChannel;
	stl::find_and_erase(m_Admins, channelId);
}

bool nCX::OnChatMessage(int channelId, int sourceId, int type, string msg)
{
	if (CGameRules *pRules = g_pGame->GetGameRules())
	{
		if (CActor *pActor = static_cast<CActor *>(pRules->GetActorByChannelId(channelId)))
		{
			EntityId entId = pActor->GetEntityId();
			if (sourceId == 0)
			{
				nCX_Anticheat::CheatDetected(entId, "Chat Manipulation", "SourceId == 0", true);
				return true;
			}

			CActor *pSource = pRules->GetActorByEntityId(sourceId);
			if (!pSource)
				return true;

			if (channelId != pSource->GetChannelId())
			{
				nCX_Anticheat::CheatDetected(entId, "RMI Manipulation", "RMI:CGameRules:SvRequestChatMessage", true);
				return true;
			}

			//Needed to vote yes/no
			/*if (!m_VoteSystem.empty() && (params.msg == "1" || params.msg == "2"))
			{
			TVoteMap::iterator voted = m_Votes.find(channelId);
			if (voted == m_Votes.end())
			{
			Vote(channelId, params.msg == "1" ? true : false);
			return true;
			}
			}*/
			//Detect commands
			string original_message = msg;
			string command = msg;
			string output;
			int found = command.find("!");
			if (found == -1)
				found = command.find("/");

			if (found == 0 || found == 1)
			{
				command.erase(0, found + 1);
				msg.erase(0, found + 1);

				found = command.find(" ");
				if (found != -1)
				{
					command.erase(found, (int)command.end());
					msg.erase(0, found + 1);
				}
				else
					msg.erase(0, (int)command.end());

				output = command.MakeLower();
			}
			//Call lua
			bool CanSendMsg = true;
			/*SmartScriptTable nCX_Lua;
			gEnv->pScriptSystem->GetGlobalValue("nCX", nCX_Lua);
			if (IScriptSystem *pScript = gEnv->pScriptSystem) //Lua
			{
				pScript->BeginCall(nCX_Lua, "OnChatMessage");
				pScript->PushFuncParam(nCX_Lua);
				pScript->PushFuncParam(type);
				pScript->PushFuncParam(pActor->GetEntity()->GetScriptTable());
				pScript->PushFuncParam(output.c_str());
				pScript->PushFuncParam(msg.c_str());
				pScript->EndCall(CanSendMsg);
			}*/
			if (!CanSendMsg)
				return true;
			else
				msg = original_message; //Restore original message for logging

			bool access = IsAdmin(channelId);

			string chattag = ""; //Guten Tag :D
			string contag = "      ";
			if (pActor->GetSpectatorMode() != 0)
			{
				chattag = "(Spec)  ";
				contag = "$9($1Spec$9)";
			}
			else if (pActor->GetHealth() < 1)
			{
				chattag = "(Dead)  ";
				contag = "$9($1Dead$9)";
			}
			else if (access && pActor->m_GodMode)
			{
				chattag = "(Admin) ";
			}
			++pActor->m_ChatCounter;
			CGameRules::ChatMessageParams newParams((EChatMessageType)type, entId, 0, chattag + msg, (type == 1) ? true : false);
			// left space
			string name = pActor->GetEntity()->GetName();
			string placeholder("                                  ");
			int space = 34; //check this
			if (space - name.size() - 7 > 0)
				placeholder.resize(space - name.size() - 7);
			else
				placeholder = "";

			string log;
			log.Format("%s %s : %s", placeholder.c_str(), name.c_str(), msg.c_str()).c_str();
			LogToFile("Chat", log);

			if (m_UseChatCensor && !access)
			{
				msg = nCX::CensorCheck(msg);
				newParams.msg = msg;
			}

			switch (type)
			{
			case 2:
			{
					  pRules->GetGameObject()->InvokeRMI(pRules->ClChatMessage(), newParams, 0x08);
					  string log;
					  log.Format("%s%s $5%s$9 : %s", placeholder.c_str(), contag.c_str(), name.c_str(), msg.c_str()).c_str();
					  pRules->SendTextMessage(eTextMessageConsole, log, 0x08, 0);
			}
				break;
			case 1:
			{
					  int teamId = pRules->GetTeam(entId);
					  if (teamId)
					  {
						  string teamname = (teamId == 1) ? "NK" : "US";
						  string log;
						  log.Format("%s%s $5%s$9 : ($1Team %s$9) %s", placeholder.c_str(), contag.c_str(), name.c_str(), teamname.c_str(), msg.c_str()).c_str();
						  CGameRules::TPlayerTeamIdMap::const_iterator tit = pRules->m_playerteams.find(teamId);
						  if (tit != pRules->m_playerteams.end())
						  {
							  CGameRules::TPlayers::const_iterator begin = tit->second.begin();
							  CGameRules::TPlayers::const_iterator end = tit->second.end();
							  for (CGameRules::TPlayers::const_iterator it = begin; it != end; ++it)
							  {
								  int chanId = pRules->GetChannelId(*it);
								  pRules->GetGameObject()->InvokeRMI(pRules->ClChatMessage(), newParams, 0x01, chanId);
								  pRules->SendTextMessage(eTextMessageConsole, log, 0x01, chanId);
							  }
						  }
						  for (std::vector<int>::const_iterator con = m_Admins.begin(); con != m_Admins.end(); ++con)
						  {
							  if (CActor *pActor = pRules->GetActorByChannelId(*con))
							  {
								  int teamId1 = pRules->GetTeam(pActor->GetEntityId());
								  if (teamId != teamId1)
									  pRules->SendTextMessage(eTextMessageConsole, log, 0x01, *con);
							  }
						  }
					  }
			}
				break;

			default:
			{
					   //Client can't send msg directly to other client so it's some kind of manipulation, ban him hard:)
					   string info;
					   nCX_Anticheat::CheatDetected(entId, "Chat Manipulation", info.Format("%d", type).c_str(), true);
			}
				break;
			}
		}
	}
	return false;
}

bool nCX::OnRadioMessage(int channelId, int sourceId, int msg)
{
	if (CGameRules *pRules = g_pGame->GetGameRules())
	{
		if (CActor *pActor = static_cast<CActor *>(pRules->GetActorByChannelId(channelId)))
		{
			if (sourceId == 0)
			{
				nCX_Anticheat::CheatDetected(pActor->GetEntityId(), "RMI Manipulation", "0", true);
				return true;
			}

			CActor *pSource = pRules->GetActorByEntityId(sourceId);
			if (!pSource)
				return true;

			if (channelId != pSource->GetChannelId())
			{
				nCX_Anticheat::CheatDetected(pActor->GetEntityId(), "RMI Manipulation", "RMI:CGameRules:SvRequestRadioMessage", true);
				return true;
			}

			//Needed to vote yes/no
			/*if (!m_VoteSystem.empty() && params.msg < 2)
			{
				TVoteMap::iterator voted = m_Votes.find(channelId);
				if (voted == m_Votes.end())
				{
					Vote(channelId, params.msg == 0 ? true : false);
					return true;
				}
			}*/
			EntityId source_Id = pActor->GetEntityId();
			CGameRules::RadioMessageParams newParams(source_Id, msg);
			if (pRules->GetTeamCount() > 1)
			{
				int teamId = pRules->GetTeam(sourceId);
				if (teamId)
				{
					CGameRules::TPlayerTeamIdMap::const_iterator tit = pRules->m_playerteams.find(teamId);
					if (tit != pRules->m_playerteams.end())
					{
						for (CGameRules::TPlayers::const_iterator it = tit->second.begin(); it != tit->second.end(); ++it)
							pRules->GetGameObject()->InvokeRMIWithDependentObject(pRules->ClRadioMessage(), newParams, 0x01, *it, pRules->GetChannelId(*it));
					}
				}
			}
			else
				pRules->GetGameObject()->InvokeRMI(pRules->ClRadioMessage(), newParams, 0x08);
		}
	}
	return true;
}
void nCX::OnChangeTeam(int channelId, int entityId, int teamId)
{
	if (CGameRules *pRules = g_pGame->GetGameRules())
	{
		CActor *pActor = static_cast<CActor *>(pRules->GetActorByChannelId(channelId));
		CActor *pTarget = pRules->GetActorByEntityId(entityId);
		if (!pActor || !pTarget)
			return;

		if (channelId != pTarget->GetChannelId() || !pRules->GetTeamName(teamId))
		{
			nCX_Anticheat::CheatDetected(pActor->GetEntityId(), "RMI Manipulation", "RMI:CGameRules:SvRequestChangeTeam", true);
			return;
		}
		pRules->ChangeTeam(pActor, teamId);
	}
}

void nCX::OnChangeSpectatorMode(int channelId, int entityId, int targetId, int mode, bool resetAll)
{
	if (CGameRules *pRules = g_pGame->GetGameRules())
	{
		CActor *pActor = static_cast<CActor *>(pRules->GetActorByChannelId(channelId));
		CActor *pTarget = pRules->GetActorByEntityId(targetId);
		if (!pActor || !pTarget)
			return;

		if (channelId != pActor->GetChannelId() || mode > 3)
		{
			nCX_Anticheat::CheatDetected(pActor->GetEntityId(), "RMI Manipulation", "RMI:CGameRules:SvRequestSpectatorMode", true);
			return;
		}
		pRules->ChangeSpectatorMode(pActor, mode, targetId, resetAll);
	}
}

void nCX::OnSimpleHit(int channelId, int shooterId, int targetId, int type, float value, int weaponId)
{
	if (CGameRules *pRules = g_pGame->GetGameRules())
	{
		CActor *pActor = static_cast<CActor *>(pRules->GetActorByChannelId(channelId));
		CActor *pShooter = pRules->GetActorByEntityId(shooterId);
		IEntity *pTargetEntity = gEnv->pEntitySystem->GetEntity(targetId);
		if (!pActor || !pShooter || !pTargetEntity)
			return;

		if (channelId != pShooter->GetChannelId())
		{
			nCX_Anticheat::CheatDetected(pActor->GetEntityId(), "RMI Manipulation", "RMI:CGameRules:SvRequestSimpleHit", true);
			return;
		}
		switch (type)
		{
		case 0:
			pRules->AddTaggedEntity(shooterId, targetId, true);
			break;
		case 0xe:
		{
				if (!NumberValid(value))
					return;

				if (IItem *pItem = g_pGame->GetIGameFramework()->GetIItemSystem()->GetItem(weaponId))
				{
						if (CWeapon *pWeapon = static_cast<CWeapon*>(pItem->GetIWeapon()))
						{
							if (pWeapon->GetOwnerId() != pShooter->GetEntityId())
								return;

							if (IFireMode *pFireMode = pWeapon->GetFireMode(pWeapon->GetCurrentFireMode()))
							{
								if (CFreezingBeam *pBeam = static_cast<CFreezingBeam *>(pFireMode))
								{
									if (!pBeam->IsFiring())
										return;

									float beamRange = pBeam->GetBeamRange() * 2.1f;
									float distance = (pShooter->GetEntity()->GetWorldPos() - pTargetEntity->GetWorldPos()).len2();
									if (distance > 0.0f)
										distance = cry_sqrtf_fast(distance);

									if (distance < beamRange)
									{
										IEntityClass *pClass = pItem->GetEntity()->GetClass();
										if (pClass != CItem::sAlienMountClass && pClass != CItem::sMOARClass && pClass != CItem::sVehicleMOARMounted && pClass != CItem::sVehicleMOAR)
										{
											nCX_Anticheat::CheatDetected(pActor->GetEntityId(), "Freeze", pClass->GetName(), true);
											return;
										}
										//Teamfire check
										if (pRules->GetTeam(shooterId) != pRules->GetTeam(targetId))
										{
											CActor *pTargetActor = pRules->GetActorByEntityId(targetId);
											if (pTargetActor)
											{
												pTargetActor->m_frostShooterId = shooterId;
												if (CNanoSuit *pSuit = static_cast<CPlayer *>(pTargetActor)->GetNanoSuit())
												{
													if (pTargetActor->GetHealth() < 1 || pSuit->IsInvulnerable() || pTargetActor->m_GodMode)
														return;
												}
											}
											pRules->FreezeEntity(targetId, true, true, (value>0.999f));
										}
									}
									else
									{
										string info;
										nCX_Anticheat::CheatDetected(pActor->GetEntityId(), "Freeze", info.Format("BeamRange = %.1f, Distance = %.1f", beamRange, distance).c_str(), true);
									}
								}
							}
						}
					}
		}
			break;
		default:
		{
				   string info;
				   nCX_Anticheat::CheatDetected(pActor->GetEntityId(), "SimpleHit Manipulation", info.Format("%d", type).c_str(), true);
		}
			break;
		}
	}
}

void nCX::OnHit(HitInfo params, int channelId)
{
	if (CGameRules *pRules = g_pGame->GetGameRules())
	{
		pRules->ServerHit(params);
	}
}