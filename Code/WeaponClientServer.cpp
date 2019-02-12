/*************************************************************************
	Crytek Source File.
	Copyright (C), Crytek Studios, 2001-2004.
	-------------------------------------------------------------------------
	$Id$
	$DateTime$

	-------------------------------------------------------------------------
	History:
	- 15:2:2006   12:50 : Created by Márcio Martins

*************************************************************************/
#include "StdAfx.h"
#include "Weapon.h"
#include "Actor.h"
#include "Game.h"
#include "GameRules.h"
#include "Player.h"
#include <IVehicleSystem.h>
#include "nCX/nCX_Anticheat.h"

#define CHECK_OWNER_REQUEST()	\
	{ \
		uint16 channelId=m_pGameFramework->GetGameChannelId(pNetChannel);	\
		IActor *pOwnerActor=GetOwnerActor(); \
		if (pOwnerActor && pOwnerActor->GetChannelId()!=channelId && !IsDemoPlayback()) \
			return true; \
	}

//------------------------------------------------------------------------
int CWeapon::NetGetCurrentAmmoCount() const
{
	if (!m_fm)
		return 0;

	return GetAmmoCount(m_fm->GetAmmoType());
}

//------------------------------------------------------------------------
void CWeapon::NetSetCurrentAmmoCount(int count)
{
	if (!m_fm)
		return;

	SetAmmoCount(m_fm->GetAmmoType(), count);
}

//------------------------------------------------------------------------
void CWeapon::NetShoot(const Vec3 &hit, int predictionHandle)
{
	if (m_fm)
		m_fm->NetShoot(hit, predictionHandle);
}

//------------------------------------------------------------------------
void CWeapon::NetShootEx(const Vec3 &pos, const Vec3 &dir, const Vec3 &vel, const Vec3 &hit, float extra, int predictionHandle)
{
	if (m_fm)
		m_fm->NetShootEx(pos, dir, vel, hit, extra, predictionHandle);
}

//------------------------------------------------------------------------
void CWeapon::NetStartFire()
{
	if (m_fm)
		m_fm->NetStartFire();
}

//------------------------------------------------------------------------
void CWeapon::NetStopFire()
{
	if (m_fm)
		m_fm->NetStopFire();
}

//------------------------------------------------------------------------
void CWeapon::NetStartMeleeAttack(bool weaponMelee)
{
	if (weaponMelee && m_melee)
		m_melee->NetStartFire();
	else if (m_fm)
		m_fm->NetStartFire();
}

//------------------------------------------------------------------------
void CWeapon::NetMeleeAttack(bool weaponMelee, const Vec3 &pos, const Vec3 &dir)
{
	if (weaponMelee && m_melee)
	{
		m_melee->NetShootEx(pos, dir, ZERO, ZERO, 1.0f, 0);
		if (IsServer())
			m_pGameplayRecorder->Event(GetOwner(), GameplayEvent(eGE_WeaponMelee, 0, 0, (void *)GetEntityId()));
	}
	else if (m_fm)
	{
		m_fm->NetShootEx(pos, dir, ZERO, ZERO, 1.0f, 0);
		if (IsServer())
			m_pGameplayRecorder->Event(GetOwner(), GameplayEvent(eGE_WeaponMelee, 0, 0, (void *)GetEntityId()));
	}
}

//------------------------------------------------------------------------
void CWeapon::NetZoom(float fov)
{
	if (CActor *pOwner=GetOwnerActor())
	{
		if (pOwner->IsClient())
			return;

		SActorParams *pActorParams = pOwner->GetActorParams();
		if (!pActorParams)
			return;

		pActorParams->viewFoVScale = fov;
	}
}

//------------------------------------------------------------------------
void CWeapon::RequestShoot(IEntityClass* pAmmoType, const Vec3 &pos, const Vec3 &dir, const Vec3 &vel, const Vec3 &hit, float extra, int predictionHandle, uint16 seq, uint8 seqr, bool forceExtended)
{
	IActor *pActor=m_pGameFramework->GetClientActor();

	if ((!pActor || pActor->IsClient()) && IsClient())
	{
		if (pActor)
			pActor->GetGameObject()->Pulse('bang');

		GetGameObject()->Pulse('bang');

		if (IsServerSpawn(pAmmoType) || forceExtended)
			GetGameObject()->InvokeRMI(CWeapon::SvRequestShootEx(), SvRequestShootExParams(pos, dir, vel, hit, extra, predictionHandle, seq, seqr), eRMI_ToServer);
		else
			GetGameObject()->InvokeRMI(CWeapon::SvRequestShoot(), SvRequestShootParams(pos, dir, hit, predictionHandle, seq, seqr), eRMI_ToServer);
	}
	else if (!IsClient() && IsServer())
	{
		if (IsServerSpawn(pAmmoType) || forceExtended)
		{
			GetGameObject()->InvokeRMI(CWeapon::ClShoot(), ClShootParams(pos+dir*5.0f, predictionHandle), eRMI_ToAllClients);
			NetShootEx(pos, dir, vel, hit, extra, predictionHandle);
		}
		else
		{
			GetGameObject()->InvokeRMI(CWeapon::ClShoot(), ClShootParams(hit, predictionHandle), eRMI_ToAllClients);
			NetShoot(hit, predictionHandle);
		}
	}
}

//------------------------------------------------------------------------
void CWeapon::RequestMeleeAttack(bool weaponMelee, const Vec3 &pos, const Vec3 &dir, uint16 seq)
{
	IActor *pActor=m_pGameFramework->GetClientActor();
	if ((!pActor || pActor->IsClient()) && IsClient())
		GetGameObject()->InvokeRMI(CWeapon::SvRequestMeleeAttack(), RequestMeleeAttackParams(weaponMelee, pos, dir, seq), eRMI_ToServer);
	else if (!IsClient() && IsServer())
	{
		GetGameObject()->InvokeRMI(CWeapon::ClMeleeAttack(), ClMeleeAttackParams(weaponMelee, pos, dir), eRMI_ToAllClients);
		NetMeleeAttack(weaponMelee, pos, dir);
	}
}

//------------------------------------------------------------------------
void CWeapon::RequestStartFire()
{
	IActor *pActor=m_pGameFramework->GetClientActor();
	if ((!pActor || pActor->IsClient()) && IsClient())
		GetGameObject()->InvokeRMI(CWeapon::SvRequestStartFire(), EmptyParams(), eRMI_ToServer);
	else if (!IsClient() && IsServer())
		GetGameObject()->InvokeRMI(CWeapon::ClStartFire(), EmptyParams(), eRMI_ToAllClients);
}

//------------------------------------------------------------------------
void CWeapon::RequestStartMeleeAttack(bool weaponMelee)
{
	IActor *pActor=m_pGameFramework->GetClientActor();
	if ((!pActor || pActor->IsClient()) && IsClient())
		GetGameObject()->InvokeRMI(CWeapon::SvRequestStartMeleeAttack(), RequestStartMeleeAttackParams(weaponMelee), eRMI_ToServer);
	else if (!IsClient() && IsServer())
	{
		GetGameObject()->InvokeRMI(CWeapon::ClStartMeleeAttack(), RequestStartMeleeAttackParams(weaponMelee), eRMI_ToAllClients);
		NetStartMeleeAttack(weaponMelee);
	}
}

//------------------------------------------------------------------------
void CWeapon::RequestZoom(float fov)
{
	IActor *pActor=m_pGameFramework->GetClientActor();
	if ((!pActor || pActor->IsClient()) && IsClient())
		GetGameObject()->InvokeRMI(CWeapon::SvRequestZoom(), ZoomParams(fov), eRMI_ToServer);
	else if (!IsClient() && IsServer())
	{
		GetGameObject()->InvokeRMI(CWeapon::ClZoom(), ZoomParams(fov), eRMI_ToAllClients);
		NetZoom(fov);
	}
}


//------------------------------------------------------------------------
void CWeapon::RequestStopFire()
{
	IActor *pActor=m_pGameFramework->GetClientActor();
	if ((!pActor || pActor->IsClient()) && IsClient())
		GetGameObject()->InvokeRMI(CWeapon::SvRequestStopFire(), EmptyParams(), eRMI_ToServer);
	else if (!IsClient() && IsServer())
		GetGameObject()->InvokeRMI(CWeapon::ClStopFire(), EmptyParams(), eRMI_ToAllClients);
}

//------------------------------------------------------------------------
void CWeapon::RequestReload()
{
	IActor *pActor=m_pGameFramework->GetClientActor();
	if ((!pActor || pActor->IsClient()) && IsClient())
		GetGameObject()->InvokeRMI(SvRequestReload(), EmptyParams(), eRMI_ToServer);
	else if (!IsClient() && IsServer())
		GetGameObject()->InvokeRMI(CWeapon::ClReload(), EmptyParams(), eRMI_ToAllClients);
}

//-----------------------------------------------------------------------
void CWeapon::RequestCancelReload()
{
	IActor *pActor=m_pGameFramework->GetClientActor();
	if ((!pActor || pActor->IsClient()) && IsClient())
		GetGameObject()->InvokeRMI(SvRequestCancelReload(), EmptyParams(), eRMI_ToServer);
	else if (!IsClient() && IsServer())
		GetGameObject()->InvokeRMI(CWeapon::ClCancelReload(), EmptyParams(), eRMI_ToAllClients);
}

//------------------------------------------------------------------------
void CWeapon::RequestFireMode(int fmId)
{
	IActor *pActor=m_pGameFramework->GetClientActor();
	if (!pActor || pActor->IsClient())
	{
		if (gEnv->bServer)
			SetCurrentFireMode(fmId);	// serialization will fix the rest.
		else
			GetGameObject()->InvokeRMI(SvRequestFireMode(), SvRequestFireModeParams(fmId), eRMI_ToServer);
	}
}

//------------------------------------------------------------------------
void CWeapon::RequestLock(EntityId id, int partId)
{
	IActor *pActor=m_pGameFramework->GetClientActor();
	if (!pActor || pActor->IsClient())
	{
		if (gEnv->bServer)
		{
			if (m_fm)
				m_fm->Lock(id, partId);

			GetGameObject()->InvokeRMI(CWeapon::ClLock(), LockParams(id, partId), eRMI_ToRemoteClients);
		}
		else
			GetGameObject()->InvokeRMI(SvRequestLock(), LockParams(id, partId), eRMI_ToServer);
	}
}

//------------------------------------------------------------------------
void CWeapon::RequestUnlock()
{
	IActor *pActor=m_pGameFramework->GetClientActor();
	if (!pActor || pActor->IsClient())
		GetGameObject()->InvokeRMI(SvRequestUnlock(), EmptyParams(), eRMI_ToServer);
}

//------------------------------------------------------------------------
void CWeapon::RequestWeaponRaised(bool raise)
{
	if(gEnv->bMultiplayer)
	{
		CActor* pActor = GetOwnerActor();
		if(pActor && pActor->IsClient())
		{
			if (gEnv->bServer)
				GetGameObject()->InvokeRMI(ClWeaponRaised(), WeaponRaiseParams(raise), eRMI_ToRemoteClients|eRMI_NoLocalCalls);
			else
				GetGameObject()->InvokeRMI(SvRequestWeaponRaised(), WeaponRaiseParams(raise), eRMI_ToServer);
		}
	}
}

//------------------------------------------------------------------------
void CWeapon::SendEndReload()
{
	int channelId=0;
	if (CActor* pActor = GetOwnerActor())
		channelId=pActor->GetChannelId();

	GetGameObject()->InvokeRMI(ClEndReload(), EmptyParams(), eRMI_ToClientChannel|eRMI_NoLocalCalls, channelId);
}

//------------------------------------------------------------------------
IMPLEMENT_RMI(CWeapon, SvRequestStartFire)
{
	int channelId = m_pGameFramework->GetGameChannelId(pNetChannel);
	if (channelId)
	{
		if (CActor *pActor = static_cast<CActor *>(m_pGameFramework->GetIActorSystem()->GetActorByChannelId(channelId)))
		{
			if (pActor->GetEntityId() == GetOwnerId())
			{
				GetGameObject()->InvokeRMI(ClStartFire(), params, eRMI_ToOtherClients | eRMI_NoLocalCalls, channelId); //0x04|0x10000
				//nCX
				m_BulletsOut = 0.0f;
				m_BulletsPassed = 0.0f;
				m_SpinupTime = gEnv->pTimer->GetCurrTime();
				NetStartFire();
			}
		}
	}
	return true;
}

//------------------------------------------------------------------------
IMPLEMENT_RMI(CWeapon, SvRequestStopFire)
{
	int channelId = m_pGameFramework->GetGameChannelId(pNetChannel);
	if (channelId)
	{
		if (CActor *pActor = static_cast<CActor *>(m_pGameFramework->GetIActorSystem()->GetActorByChannelId(channelId)))
		{
			if (pActor->GetEntityId() == GetOwnerId())
			{
				GetGameObject()->InvokeRMI(ClStopFire(), params, 0x04 | 0x10000, channelId);
				NetStopFire();
				//nCX reset anticheat 
				if (m_FireControl)
					m_FireControl = false;

				m_CoolDownCheck = 0;
				m_FireCheck = 0;
				m_SpinupTime = 0.0f;
			}
		}
	}
	return true;
}

//------------------------------------------------------------------------
IMPLEMENT_RMI(CWeapon, ClStartFire)
{
	NetStartFire();
	return true;
}

//------------------------------------------------------------------------
IMPLEMENT_RMI(CWeapon, ClStopFire)
{
	NetStopFire();
	return true;
}

//------------------------------------------------------------------------
IMPLEMENT_RMI(CWeapon, SvRequestShoot)
{
	int channelId = m_pGameFramework->GetGameChannelId(pNetChannel);
	if (channelId)
	{
		if (CActor *pActor = static_cast<CActor *>(m_pGameFramework->GetIActorSystem()->GetActorByChannelId(channelId)))
		{
			if (NULL == &params)
				return false;
			
			if (OutOfAmmo(false))
			{
				CryLogAlways("OutOfAmmo requests shots 1");
				return false;
			}

			//Idk what is this
			pActor->GetGameObject()->Pulse('bang');
			GetGameObject()->Pulse('bang');
            
            //RMI Spoof
			if (pActor->GetEntityId() != GetOwnerId())
            {
				nCX_Anticheat::CheatDetected(pActor->GetEntityId(), "RMI Spoof", "pActor->GetEntityId() != RMI:GetOwnerId()", true);
                return false;
            }
			//Longpoke
            if (nCX_Anticheat::CheckLongpoke(pActor, params.seq))
                return false;

			if (!pActor->m_IsLagging)//Process anticheat only if player is not lagging !
			{
				float currTime = gEnv->pTimer->GetCurrTime();
				//Shoot Pos Spoof
				IEntityClass *pClass = GetEntity()->GetClass();
				if (pClass != sDetonatorClass && pClass != sRadarKitClass && pClass != sAlienMountClass && pClass != sVehicleMOARMounted)
					if (nCX_Anticheat::CheckShootPos(pActor, params.pos, currTime))
						return false;

			    //RapidFire & NoRecoil
				if (nCX_Anticheat::CheckRecoil(this, params))
					return false;
				
                //Weapon SpinUp time Check
				float spinUpTime = GetFireMode(GetCurrentFireMode())->GetSpinUpTime();
				if (spinUpTime > 0.01f)
				{
					if (currTime - m_SpinupTime < spinUpTime)
					{
						nCX_Anticheat::CheatDetected(pActor->GetEntityId(), "Weapon Spinup", pClass->GetName(), true);
						m_SpinupTime = 0.0f;
					}
				}
                
				//Weapon Cooldown Check
				if (pClass == sMOARClass || pClass == sVehicleMOARMounted || pClass == sAlienMountClass || pClass == sShiTenClass || pClass == sAsian50CalClass || pClass == sAsianCoaxialGun || pClass == sUSCoaxialGun || pClass == sUSCoaxialGun_VTOL || pClass == sVehicleUSMachinegun || pClass == sVehicleShiTenV2 || pClass == sAvengerCannon)
				{
					++m_CoolDownCheck;
					if (((pClass == sMOARClass || pClass == sVehicleMOARMounted) && m_CoolDownCheck == 13) || ((pClass == sShiTenClass || pClass == sAsianCoaxialGun || pClass == sVehicleShiTenV2) && m_CoolDownCheck == 90) || (pClass == sUSCoaxialGun_VTOL && m_CoolDownCheck == 62))
					{
						string info;
						nCX_Anticheat::CheatDetected(pActor->GetEntityId(), "Weapon Cooldown", pClass->GetName(), false);
						m_FireControl = true;
					}
				}
			}
			else
				m_FireCheck = 0;
		}
		//gEnv->pPhysicalWorld->AddEventClient(1, 1);
		++m_BulletsOut;
		static ray_hit rh;
		IEntity* pEntity = NULL;
		if (gEnv->pPhysicalWorld->RayWorldIntersection(params.pos, params.dir*4096.0f, ent_all & ~ent_terrain, rwi_stop_at_pierceable | rwi_ignore_back_faces, &rh, 1))
			pEntity = gEnv->pEntitySystem->GetEntityFromPhysics(rh.pCollider);
        
        if (pEntity)
		{
			if (INetContext* pNC = m_pGameFramework->GetNetContext())
			{
				if (pNC->IsBound(pEntity->GetId()))
				{
					AABB bbox; pEntity->GetWorldBounds(bbox);
					bool hit0 = bbox.GetRadius() < 1.0f;
					Vec3 hitLocal = pEntity->GetWorldTM().GetInvertedFast() * rh.pt;
					GetGameObject()->InvokeRMIWithDependentObject(ClShootX(), ClShootXParams(pEntity->GetId(), hit0, hitLocal, params.predictionHandle), 0x04 | 0x10000, pEntity->GetId(), channelId);
				}
			}
		}
		else
			GetGameObject()->InvokeRMI(ClShoot(), ClShootParams(params.hit, params.predictionHandle), 0x04 | 0x10000, channelId);

		m_fm->NetShoot(params.hit, params.predictionHandle);
	}
	return true;
}

//------------------------------------------------------------------------
IMPLEMENT_RMI(CWeapon, SvRequestShootEx)
{
	int channelId = m_pGameFramework->GetGameChannelId(pNetChannel);
	if (channelId)
	{
		if (CActor *pActor = static_cast<CActor *>(m_pGameFramework->GetIActorSystem()->GetActorByChannelId(channelId)))
		{
			if (NULL == &params)
				return false;

			if (OutOfAmmo(false))
			{
				CryLogAlways("OutOfAmmo requests shots 2");
				return false;
			}
			//idk whats this
			pActor->GetGameObject()->Pulse('bang');
			GetGameObject()->Pulse('bang');

			if (pActor->GetEntityId() == GetOwnerId())
			{
				IEntityClass *pClass = GetEntity()->GetClass();

				if (!pActor->m_IsLagging)
				{
					//Shoot Pos Spoof
					float Distance = (pActor->GetEntity()->GetWorldPos() - params.pos).len2();
					if (Distance > 0.0f)
						Distance = cry_sqrtf_fast(Distance);

					float Treshold = 50.0f;
					if (Distance > Treshold)
					{
						if (IVehicle *pVehicle = pActor->GetLinkedVehicle())
						{
							const SVehicleStatus& status = pVehicle->GetStatus();
							if (status.speed > Treshold)
								Treshold = status.speed;
						}
					}
					if (Distance > Treshold)
					{
						if (gEnv->pTimer->GetCurrTime() - pActor->m_WeaponCheatDelay > 3.0f)
						{
							string info;
							nCX_Anticheat::CheatDetected(pActor->GetEntityId(), "ShootEx Pos", info.Format("%.2fm", Distance).c_str(), false);
							pActor->m_WeaponCheatDelay = gEnv->pTimer->GetCurrTime() + 10.0f;
						}
						return true;
					}
					if (IFireMode *pFireMode = GetFireMode(GetCurrentFireMode()))
					{
						//RapidFire check
						++m_FireCheck;
						float rate = pFireMode->GetFireRate();
						if (rate > 0.0f && ((rate < 50.f && m_FireCheck > 1) || (rate > 50.f && m_FireCheck > int(ceil(rate / 33.0f)))))
						{
							nCX_Anticheat::CheatDetected(pActor->GetEntityId(), "Rapid Fire", pClass->GetName(), false);
							CryLogAlways("[AntiCheat] Rapid Fire Ex: %s | FireMode: %d | Firerate: (%d) Max %.4f | Heat: %.2f | Recoil Amount: %.4f | Recoil: %.4f | Spread: %.4f | SpinUp: %.2f | SpinDown: %.2f ", pClass->GetName(), GetCurrentFireMode(), m_FireCheck, pFireMode->GetFireRate(), pFireMode->GetHeat(), pFireMode->GetRecoilMultiplier(), pFireMode->GetRecoil(), pFireMode->GetSpread(), pFireMode->GetSpinUpTime(), pFireMode->GetSpinDownTime());
							m_FireCheck = 0;
							return true;
						}
					}
					else
						m_FireCheck = 0;
				}

				//AntiCheat passed, process shoot
				GetGameObject()->InvokeRMI(ClShoot(), ClShootParams(params.pos + params.dir*5.0f, params.predictionHandle), 0x04 | 0x10000, channelId);
				NetShootEx(params.pos, params.dir, params.vel, params.hit, params.extra, params.predictionHandle);
				//Can we remove this??
				if (CGameRules *pGameRules = g_pGame->GetGameRules())
					pGameRules->ValidateShot(pActor->GetEntityId(), GetEntityId(), params.seq, params.seqr);

				//if (pClass == sRocketLauncherClass)
				//	Reload(true);//CHRIS weird workaround but it works!
				//must be done everytime we shoot.. otherwise it will drop before reload
			}
		}
	}
	return true;
}

//------------------------------------------------------------------------
IMPLEMENT_RMI(CWeapon, ClShoot)
{
	NetShoot(params.hit, params.predictionHandle);

	return true;
}

//------------------------------------------------------------------------
IMPLEMENT_RMI(CWeapon, ClShootX)
{
	if (IEntity* pEntity = gEnv->pEntitySystem->GetEntity(params.eid))
	{
		Vec3 hit = pEntity->GetWorldTM() * params.hit;
		NetShoot(hit, params.predictionHandle);
	}
	else
	{
		GameWarning("ClShootX: invalid entity id %.8x", params.eid);
	}

	return true;
}

//------------------------------------------------------------------------
IMPLEMENT_RMI(CWeapon, SvRequestStartMeleeAttack)
{
	GetGameObject()->InvokeRMI(CWeapon::ClStartMeleeAttack(), params, eRMI_ToOtherClients, m_pGameFramework->GetGameChannelId(pNetChannel));

	CActor *pActor=GetActorByNetChannel(pNetChannel);
	IActor *pLocalActor=m_pGameFramework->GetClientActor();
	bool isLocal = pLocalActor && pActor && (pLocalActor->GetChannelId() == pActor->GetChannelId());

	if (!isLocal)
		NetStartMeleeAttack(params.wmelee);

	return true;
}

//------------------------------------------------------------------------
IMPLEMENT_RMI(CWeapon, ClStartMeleeAttack)
{
	NetStartMeleeAttack(params.wmelee);

	return true;
}

//------------------------------------------------------------------------
IMPLEMENT_RMI(CWeapon, SvRequestMeleeAttack)
{
	int channelId = m_pGameFramework->GetGameChannelId(pNetChannel);
	if (channelId)
	{
		if (CActor *pActor = static_cast<CActor *>(m_pGameFramework->GetIActorSystem()->GetActorByChannelId(channelId)))
		{
			if (NULL == &params)
				return false;

			if (pActor->GetEntityId() == GetOwnerId())
			{
				GetGameObject()->InvokeRMI(CWeapon::ClMeleeAttack(), ClMeleeAttackParams(params.wmelee, params.pos, params.dir), 0x04 | 0x10000, channelId);
				CPlayer *pPlayer = static_cast<CPlayer *>(pActor);
				CNanoSuit *pSuit = pPlayer->GetNanoSuit();
				if (pSuit && !pActor->m_GodMode)
				{
					ENanoMode curMode = pSuit->GetMode();
					if (curMode == NANOMODE_STRENGTH)
						pSuit->SetSuitEnergy(pSuit->GetSuitEnergy() - 20.0f);
					//else if (curMode == NANOMODE_CLOAK) //no cloak disable after punch
					//	pSuit->SetSuitEnergy(0.0f);

					if (pSuit->IsInvulnerable())
						pSuit->SetInvulnerability(false);
				}
				//Probable melee double hit fix
				//NetMeleeAttack(params.wmelee, params.pos, params.dir); ?
				if (params.wmelee && m_melee)
					m_melee->NetShootEx(params.pos, params.dir, ZERO, ZERO, 1.0f, 0);
				else if (m_fm)
					m_fm->NetShootEx(params.pos, params.dir, ZERO, ZERO, 1.0f, 0);
			}
			if (CGameRules *pGameRules = g_pGame->GetGameRules())
				pGameRules->ValidateShot(pActor->GetEntityId(), GetEntityId(), params.seq, 0);
		}
	}
	return true;
}

//------------------------------------------------------------------------
IMPLEMENT_RMI(CWeapon, ClMeleeAttack)
{
	NetMeleeAttack(params.wmelee, params.pos, params.dir);

	return true;
}

//------------------------------------------------------------------------
IMPLEMENT_RMI(CWeapon, SvRequestZoom)
{
	CHECK_OWNER_REQUEST();

	bool ok=true;
	CActor *pActor=GetActorByNetChannel(pNetChannel);
	if (!pActor || pActor->GetHealth()<=0)
		ok=false;

	if (ok)
	{
		GetGameObject()->InvokeRMI(CWeapon::ClZoom(), params,
			eRMI_ToOtherClients|eRMI_NoLocalCalls, m_pGameFramework->GetGameChannelId(pNetChannel));

		IActor *pLocalActor=m_pGameFramework->GetClientActor();
		bool isLocal = pLocalActor && (pLocalActor->GetChannelId() == pActor->GetChannelId());

		if (!isLocal)
			NetZoom(params.fov);

		int event=eGE_ZoomedOut;
		if (params.fov<0.99f)
			event=eGE_ZoomedIn;
		m_pGameplayRecorder->Event(GetOwner(), GameplayEvent(event, 0, 0, (void *)GetEntityId()));
	}

	return true;
}

//------------------------------------------------------------------------
IMPLEMENT_RMI(CWeapon, ClZoom)
{
	NetZoom(params.fov);

	return true;
}


//------------------------------------------------------------------------
IMPLEMENT_RMI(CWeapon, SvRequestFireMode)
{
	SetCurrentFireMode(params.id);
	return true;
}

//------------------------------------------------------------------------
IMPLEMENT_RMI(CWeapon, ClSetFireMode)
{
	SetCurrentFireMode(params.id);
	return true;
}

//------------------------------------------------------------------------
IMPLEMENT_RMI(CWeapon, SvRequestReload)
{
	bool ok=true;
	CActor *pActor=GetActorByNetChannel(pNetChannel);
	/*if (!pActor || pActor->GetHealth()<=0)
		ok=false;*/

	// ~Crysis Co-op
	
	if (ok)
	{
		GetGameObject()->InvokeRMI(CWeapon::ClReload(), params, eRMI_ToOtherClients|eRMI_NoLocalCalls, m_pGameFramework->GetGameChannelId(pNetChannel));

		IActor *pLocalActor=m_pGameFramework->GetClientActor();
		bool isLocal = pLocalActor && (pLocalActor->GetChannelId() == pActor->GetChannelId());

		if (!isLocal && m_fm)
			m_fm->Reload(0);

		m_pGameplayRecorder->Event(GetOwner(), GameplayEvent(eGE_WeaponReload, 0, 0, (void *)GetEntityId()));
	}

	return true;
}

//------------------------------------------------------------------------
IMPLEMENT_RMI(CWeapon, ClReload)
{
	if (m_fm)
	{
		if(m_zm)
			m_fm->Reload(m_zm->GetCurrentStep());
		else
			m_fm->Reload(false);
	}

	return true;
}

//------------------------------------------------------------------------
IMPLEMENT_RMI(CWeapon, ClEndReload)
{
	if(m_fm)
		m_fm->NetEndReload();

	return true;
}

//------------------------------------------------------------------------
IMPLEMENT_RMI(CWeapon, SvRequestCancelReload)
{
	if(m_fm)
	{
		m_fm->CancelReload();
		GetGameObject()->InvokeRMI(CWeapon::ClCancelReload(), params, eRMI_ToRemoteClients);
	}

	return true;
}

//------------------------------------------------------------------------
IMPLEMENT_RMI(CWeapon, ClCancelReload)
{
	if(m_fm)
		m_fm->CancelReload();

	return true;
}

//------------------------------------------------------------------------
IMPLEMENT_RMI(CWeapon, ClLock)
{
	if (m_fm)
		m_fm->Lock(params.entityId, params.partId);

	return true;
}

//------------------------------------------------------------------------
IMPLEMENT_RMI(CWeapon, ClUnlock)
{
	if (m_fm)
		m_fm->Unlock();

	return true;
}


//------------------------------------------------------------------------
IMPLEMENT_RMI(CWeapon, SvRequestLock)
{
	if (m_fm)
		m_fm->Lock(params.entityId, params.partId);

	GetGameObject()->InvokeRMI(CWeapon::ClLock(), params, eRMI_ToRemoteClients);

	return true;
}

//------------------------------------------------------------------------
IMPLEMENT_RMI(CWeapon, SvRequestUnlock)
{
	if (m_fm)
		m_fm->Unlock();

	GetGameObject()->InvokeRMI(CWeapon::ClUnlock(), params, eRMI_ToRemoteClients);

	return true;
}

//------------------------------------------------------------------------
IMPLEMENT_RMI(CWeapon, SvRequestWeaponRaised)
{
	CHECK_OWNER_REQUEST();

	GetGameObject()->InvokeRMI(CWeapon::ClWeaponRaised(), params, eRMI_ToAllClients);

	return true;
}

//------------------------------------------------------------------------
IMPLEMENT_RMI(CWeapon, ClWeaponRaised)
{
	CActor* pActor = GetOwnerActor();
	if(pActor && !pActor->IsClient())
		RaiseWeapon(params.raise);

	return true;
}