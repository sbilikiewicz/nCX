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

bool nCX_Anticheat::LongpokeCheck(CActor *pActor, seq)
{
    float currTime = gEnv->pTimer->GetCurrTime();
	if (seq == 1)
	{
		if (currTime - pActor->m_WeaponCheatDelay > 3.0f)
		{
			OnCheatDetected(pActor->GetEntityId(), "Longpoke", "seq", true);
			pActor->m_WeaponCheatDelay = currTime + 10.0f;
		    return true;
        }
	}
    return false;
}

bool nCX_Anticheat::RecoilCheck(CWeapon *pWeapon, IHitInfo params)
{
    int currFireMode = pWeapon->GetCurrentFireMode();
	if (IFireMode *pFireMode = pWeapon->GetFireMode(currFireMode))
	{
		++m_FireCheck;
		float rate = pFireMode->GetFireRate();
		if (pClass != sDetonatorClass && rate > 0.0f && ((rate < 50.f && m_FireCheck > 1) || (rate > 50.f && m_FireCheck > int(ceil(rate / 33.0f))))) //50.0f
		{
			OnCheatDetected(pWeapon->GetOwner()->GetEntityId(), "Rapid Fire", pClass->GetName(), false);
			m_FireCheck = 0;
			pActor->m_WeaponCheatDelay = currTime + 3.0f;
			return true;
		}
		if (pClass == sSCARClass || pClass == sFY71Class || pClass == sSMGClass || pClass == sHurricaneClass || (sAlienMountClass && currFireMode == 1))
		{
			/* not needed right now
            int clip = pFireMode->GetAmmoCount();
			if (clip == 0)
				CryLogAlways("$4Clip is empty and still shooting!");*/

			float x = abs(pWeapon->m_LastRecoil.x - params.dir.x);
			float y = abs(pWeapon->m_LastRecoil.y - params.dir.y);
			float z = abs(pWeapon->m_LastRecoil.z - params.dir.z);
			float Average = (x + y + z) / 3;
			// 0.0001f check this
			if ((Average < 0.0001f) && (currFireMode < 1))
			{
				++pWeapon->m_RecoilWarning;
				if (pWeapon->m_RecoilWarning == 5)
				{
					string info;
					const char* type = "No Recoil";
					if (Average > 0.00001f)
					{
						type = "Low Recoil";
						info.Format("%.5f, %s", Average, pClass->GetName());
					}
					else
						info.Format("%s", pClass->GetName());

					nCX_Anticheat::CheatDetected(pActor->GetEntityId(), type, info.c_str(), true);
					pWeapon->m_RecoilWarning = 0;
					return true;
				}
			}
			else
				pWeapon->m_RecoilWarning = 0;

			pWeapon->m_LastRecoil = params.dir;
	   }
    }
    return false;
}

bool nCX_Anticheat::RecoilCheck(CWeapon *pWeapon, IHitInfo params)
{

}