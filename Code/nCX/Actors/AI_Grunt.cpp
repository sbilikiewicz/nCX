#include "StdAfx.h"
#include "AI_Grunt.h"
#include "PlayerMovementController.h"
#include "IVehicleSystem.h"
#include "Weapon.h"

AI_Grunt::AI_Grunt() :
	m_nStance(STANCE_RELAXED),
	m_vMoveTarget(Vec3(0,0,0)),
	m_vAimTarget(Vec3(0,0,0)),
	m_vLookTarget(Vec3(0,0,0)),
	m_vBodyTarget(Vec3(0,0,0)),
	m_vFireTarget(Vec3(0,0,0)),
	m_fPseudoSpeed(0.f),
	m_fDesiredSpeed(0.f),
	m_nAlertness(0.f),
	m_bAllowStrafing(false),
	m_bHasAimTarget(false),
	m_nSuitMode(3),
	m_bHidden(false)
{
}

AI_Grunt::~AI_Grunt()
{
}

bool AI_Grunt::Init(IGameObject * pGameObject)
{
	CPlayer::Init(pGameObject);

	return true;
}


void AI_Grunt::PostInit(IGameObject * pGameObject)
{
	CPlayer::PostInit(pGameObject);

	pGameObject->SetAIActivation(eGOAIAM_Always);

	if (gEnv->bServer)
		GetEntity()->SetTimer(eTIMER_WEAPONDELAY, 1000);

}

void AI_Grunt::RegisterMultiplayerAI()
{
	if (GetHealth() <= 0 && GetEntity()->GetAI())
	{
		gEnv->bMultiplayer = false;

		IScriptTable* pScriptTable = GetEntity()->GetScriptTable();
		gEnv->pScriptSystem->BeginCall(pScriptTable, "UnregisterAI");
		gEnv->pScriptSystem->PushFuncParam(pScriptTable);
		gEnv->pScriptSystem->EndCall(pScriptTable);
		
		//CryLogAlways("AI Unregistered for Grunt %s", GetEntity()->GetName());

		gEnv->bMultiplayer = true;
	}
	else if (!GetEntity()->GetAI() && GetHealth() > 0)
	{
		gEnv->bMultiplayer = false;
		gEnv->pAISystem->Enable(true);
		IScriptTable* pScriptTable = GetEntity()->GetScriptTable();
		gEnv->pScriptSystem->BeginCall(pScriptTable, "RegisterAI");
		gEnv->pScriptSystem->PushFuncParam(pScriptTable);
		gEnv->pScriptSystem->EndCall(pScriptTable);
		
		//CryLogAlways("AI Registered for Grunt %s", GetEntity()->GetName());

		gEnv->bMultiplayer = true;
	}
}

void AI_Grunt::DrawDebugInfo()
{
	static float color[] = { 1,1,1,1 };

	if (gEnv->bServer)
		gEnv->pRenderer->Draw2dLabel(5, 5, 2, color, false, "IsServer");
	else
		gEnv->pRenderer->Draw2dLabel(5, 5, 2, color, false, "IsClient");

	gEnv->pRenderer->Draw2dLabel(5, 25, 2, color, false, "MoveTarget x%f y%f z%f", m_vMoveTarget.x, m_vMoveTarget.y, m_vMoveTarget.z);
	gEnv->pRenderer->Draw2dLabel(5, 45, 2, color, false, "AimTarget x%f y%f z%f", m_vAimTarget.x, m_vAimTarget.y, m_vAimTarget.z);
	gEnv->pRenderer->Draw2dLabel(5, 65, 2, color, false, "LookTarget x%f y%f z%f", m_vLookTarget.x, m_vLookTarget.y, m_vLookTarget.z);
	gEnv->pRenderer->Draw2dLabel(5, 85, 2, color, false, "BodyTarget x%f y%f z%f", m_vBodyTarget.x, m_vBodyTarget.y, m_vBodyTarget.z);
	gEnv->pRenderer->Draw2dLabel(5, 105, 2, color, false, "FireTarget x%f y%f z%f", m_vFireTarget.x, m_vFireTarget.y, m_vFireTarget.z);

	gEnv->pRenderer->Draw2dLabel(5, 145, 2, color, false, "PsuedoSpeed %f", m_fPseudoSpeed);
	gEnv->pRenderer->Draw2dLabel(5, 165, 2, color, false, "DesiredSpeed %f", m_fDesiredSpeed);

	gEnv->pRenderer->Draw2dLabel(5, 185, 2, color, false, "Alertness %d", m_nAlertness);
	gEnv->pRenderer->Draw2dLabel(5, 205, 2, color, false, "Stance %d", m_nStance);

	gEnv->pRenderer->Draw2dLabel(5, 225, 2, color, false, "Allow Strafing %d     HasAimTarget %d", (int)m_bAllowStrafing, (int)m_bHasAimTarget);
	if (GetNanoSuit())
		gEnv->pRenderer->Draw2dLabel(5, 265, 2, color, false, "Suit mode %d", GetNanoSuit()->GetMode());
}

void AI_Grunt::Update(SEntityUpdateContext& ctx, int updateSlot)
{
	CPlayer::Update(ctx, updateSlot);

	// Register AI System in MP
	if (gEnv->bServer && !gEnv->bEditor)
		RegisterMultiplayerAI();

	// Movement reqeust stuff so proper anims play on client
	if (gEnv->bServer)
	{
		CMovementRequest currMovement = static_cast<CPlayerMovementController*>(m_pMovementController)->GetMovementReqState();

		//Vec3
		m_vMoveTarget = currMovement.GetMoveTarget();
		m_vAimTarget = currMovement.GetAimTarget();
		m_vLookTarget = currMovement.GetLookTarget();
		m_vBodyTarget = currMovement.GetBodyTarget();
		m_vFireTarget = currMovement.GetFireTarget();
		
		// Float
		m_fPseudoSpeed = currMovement.GetPseudoSpeed();
		m_fDesiredSpeed = currMovement.GetDesiredSpeed();

		// Int
		m_nStance = currMovement.GetStance();
		m_nAlertness = currMovement.GetAlertness();
		
		// Bool
		m_bAllowStrafing = currMovement.AllowStrafing();
		m_bHasAimTarget = currMovement.HasAimTarget();


		if (GetNanoSuit())
		{
			if (GetNanoSuit()->GetMode() != m_nSuitMode)
			{
				m_nSuitMode = GetNanoSuit()->GetMode();
				GetGameObject()->InvokeRMI(ClChangeSuitMode(), SSuitParams(m_nSuitMode), eRMI_ToAllClients | eRMI_NoLocalCalls);
			}
		}

		if (GetHealth() > 0.f)
			GetGameObject()->ChangedNetworkState(ASPECT_ALIVE);
	}
	else
	{
		UpdateMovementState();
	}

	//DrawDebugInfo();

}

void AI_Grunt::UpdateMovementState()
{
	CMovementRequest request;

	if (m_vAimTarget != Vec3(0, 0, 0) || m_vLookTarget != Vec3(0, 0, 0) || m_vMoveTarget != Vec3(0, 0, 0))
	{
		
		if (m_bHasAimTarget)
			request.SetAimTarget(m_vAimTarget);
		else
			request.ClearAimTarget();

		// Vec3
		request.SetMoveTarget(m_vMoveTarget);
		request.SetLookTarget(m_vLookTarget);
		request.SetFireTarget(m_vLookTarget);
		request.SetBodyTarget(m_vBodyTarget);

		// Float
		request.SetPseudoSpeed(m_fPseudoSpeed);
		request.SetDesiredSpeed(m_fDesiredSpeed);

		// Int
		request.SetAlertness(m_nAlertness);
		request.SetStance((EStance)m_nStance);

		// Bool
		request.SetAllowStrafing(m_bAllowStrafing);
		
		GetMovementController()->RequestMovement(request);

		if (IVehicle* pVehicle = GetLinkedVehicle())
		{
			EntityId nWeaponEntity = pVehicle->GetCurrentWeaponId(GetEntityId());
			CWeapon* pWeapon = (CWeapon*)gEnv->pGame->GetIGameFramework()->GetIItemSystem()->GetItem(nWeaponEntity);
			if (pWeapon)
			{
				pWeapon->SetAimLocation(m_vAimTarget);
				pWeapon->SetTargetLocation(m_vAimTarget);
			}
		}

	}
}

void AI_Grunt::ProcessEvent(SEntityEvent& event)
{
	CPlayer::ProcessEvent(event);

	switch(event.event)
	{
	case ENTITY_EVENT_HIDE:
		{
			if (gEnv->bServer)
			{
				GetInventory()->Clear();
				m_bHidden = true;
				GetGameObject()->ChangedNetworkState(ASPECT_HIDE);
			}

			break;
		}
	case ENTITY_EVENT_UNHIDE:
		{
			if (gEnv->bServer)
			{
				GetEntity()->SetTimer(eTIMER_WEAPONDELAY, 1000);
				m_bHidden = false;
				GetGameObject()->ChangedNetworkState(ASPECT_HIDE);
			}

			break;
		}
	case ENTITY_EVENT_TIMER:
		{
			switch(event.nParam[0])
			{
			case eTIMER_WEAPONDELAY:
				IScriptTable* pScriptTable = GetEntity()->GetScriptTable();
				SmartScriptTable props;
				if(pScriptTable->GetValue("Properties", props))
				{
					int bNanosuit ;
					char* equip;
					props->GetValue("bNanoSuit", bNanosuit);
					if (bNanosuit == 1)
						gEnv->pGame->GetIGameFramework()->GetIItemSystem()->GiveItem(this, "NanoSuit", false, false, false);

					if (props->GetValue("equip_EquipmentPack", equip))
					{
						if (equip[0] == '\0')
							equip = "NK_Rifle";

						gEnv->pGame->GetIGameFramework()->GetIItemSystem()->GetIEquipmentManager()->GiveEquipmentPack(this, equip, true, true);
					}
						
				}
				break;
			}
		}
		break;
	}
}

bool AI_Grunt::NetSerialize(TSerialize ser, EEntityAspects aspect, uint8 profile, int flags)
{
	if (!CPlayer::NetSerialize(ser, aspect, profile, flags))
		return false;

	bool bReading = ser.IsReading();

	switch (aspect)
	{
		case ASPECT_ALIVE:
		{
			//Vec3
			ser.Value("vMoveTarget", m_vMoveTarget, 'wrld');
			ser.Value("vAimTarget", m_vAimTarget, 'wrld');
			ser.Value("vLookTarget", m_vLookTarget, 'wrld');
			ser.Value("vBodyTarget", m_vBodyTarget, 'wrld');

			//Float
			ser.Value("fpSpeed", m_fPseudoSpeed);
			ser.Value("fdSpeed", m_fDesiredSpeed);

			//Int
			ser.Value("nAlert", m_nAlertness, 'i8');
			ser.Value("nStance", m_nStance, 'i8');

			//Bool
			ser.Value("bStrafe", m_bAllowStrafing, 'bool');
			ser.Value("bTarget", m_bHasAimTarget, 'bool');
			break;
		}
		case ASPECT_HIDE:
		{
			ser.Value("hide", m_bHidden, 'bool');

			if (bReading)
				GetEntity()->Hide(m_bHidden);

			break;
		}
	}
	
	return true;
}


IMPLEMENT_RMI(AI_Grunt, ClChangeSuitMode)
{
	if (GetNanoSuit())
		GetNanoSuit()->SetMode((ENanoMode)params.suitmode);

	return true;
}

