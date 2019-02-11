/**********************************************************
#####  ##   ##
####    ## ##   ##  ## ##  Crysis nCX V3.0
##  ##   ## ##        ###     by MrHorseDick
##   ##  ## ##        ###       and
##    ## ## ##   ##  ## ##        MrCtaoistrach
##     ####  #####  ##   ##
**********************************************************/
#include "StdAfx.h"
#include "nCX_Anticheat.h"
#include "nCX_Main.h"
#include "GameRules.h"
#include "GameCvars.h"
#include "Weapon.h"
#include "IVehicleSystem.h"
#include "FreezingBeam.h"
#include "Player.h"
#include <regex>
#include <StlUtils.h>

void nCX_Anticheat::OnCheatDetected(EntityId entityId, const char* cheat, const char* info, bool sure)
{
	ICVar *pCVar = gEnv->pConsole->GetCVar("time_scale");
	int scale = pCVar->GetFVal();
	if (scale == 1.0f)
	{
		if (CActor *pActor = g_pGame->GetGameRules()->GetActorByEntityId(entityId))
		{
			bool lagging = pActor->m_IsLagging;
			if (lagging && !sure && cheat != "RMI Flood")
				return;

			string msg;
			msg.Format("Detected %s for player %s : %s", cheat, pActor->GetEntity()->GetName(), info);
			if (!sure)
			{
				string status;
				int ping = 0;
				g_pGame->GetSynchedStorage()->GetEntityValue(entityId, 103, ping);
				if (lagging)
					status.Format(" (%d | HighLatency %d)", g_pGame->m_ServerPerformance, ping);
				else
					status.Format(" (%d | Ping %d)", g_pGame->m_ServerPerformance, ping);

				msg.append(status.c_str());
			}
			nCX::LogFile("Action", msg.c_str());
			/*if (IScriptSystem *pScript = gEnv->pScriptSystem) //Lua
			{
			pScript->BeginCall(m_nCXLuaTab, "CheatControl");
			pScript->PushFuncParam(m_nCXLuaTab);
			pScript->PushFuncParam(pActor->GetEntity()->GetScriptTable());
			pScript->PushFuncParam(cheat);
			pScript->PushFuncParam(info);
			pScript->PushFuncParam(lagging);
			pScript->PushFuncParam(sure);
			pScript->EndCall();
			}*/
		}
	}
}

bool nCX_Anticheat::ShootPositionCheck(CActor *pActor, Vec3 DefinedPos, float time)
{
	float Treshold = 50.0f;
	Vec3 ActorPos = pActor->GetEntity()->GetWorldPos();
	float Distance = (ActorPos - DefinedPos).len2();//x*x +y*y + z*z
	if (Distance > 0.0f)
		Distance = sqrtf(Distance);

	if (Distance > Treshold)
	{
		if (IVehicle *pVehicle = pActor->GetLinkedVehicle())
		{
			const SVehicleStatus& status = pVehicle->GetStatus();
			if (status.speed > Treshold)
				Treshold = status.speed;

		}
		if (time - pActor->m_WeaponCheatDelay > 3.0f)
		{
			string info;
			OnCheatDetected(pActor->GetEntityId(), "Shoot Pos", info.Format("%.2fm", Distance).c_str(), false);
			return true;
		}
	}
	return false;
}