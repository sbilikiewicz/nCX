#include "StdAfx.h"
#include "AI_Scout.h"
#include "nCX/nCX_AI.h"

#include "CompatibilityAlienMovementController.h"

AI_Scout::AI_Scout() :
	m_vLookTarget(Vec3(0,0,0)),
	m_vAimTarget(Vec3(0,0,0)),
	m_bHidden(false)
{
}

AI_Scout::~AI_Scout()
{
}

bool AI_Scout::Init(IGameObject * pGameObject)
{
	CScout::Init(pGameObject);

	return true;
}

void AI_Scout::PostInit(IGameObject * pGameObject)
{
	CScout::PostInit(pGameObject);

	pGameObject->SetAIActivation(eGOAIAM_Always);

	if (gEnv->bServer)
		GetEntity()->SetTimer(eTIMER_WEAPONDELAY, 1000);
}

void AI_Scout::RegisterMultiplayerAI()
{
	if (GetHealth() <= 0 && GetEntity()->GetAI())
	{
		gEnv->bMultiplayer = false;

		IScriptTable* pScriptTable = GetEntity()->GetScriptTable();
		gEnv->pScriptSystem->BeginCall(pScriptTable, "UnregisterAI");
		gEnv->pScriptSystem->PushFuncParam(pScriptTable);
		gEnv->pScriptSystem->EndCall(pScriptTable);
		gEnv->bMultiplayer = true;
		CryLogAlways("ReSpawning dead %s", GetEntity()->GetName());
		nCX_AI::ReSpawn(GetEntity());
	}
	else if (!GetEntity()->GetAI() && GetHealth() > 0)
	{
		gEnv->bMultiplayer = false;

		IScriptTable* pScriptTable = GetEntity()->GetScriptTable();
		gEnv->pScriptSystem->BeginCall(pScriptTable, "RegisterAI");
		gEnv->pScriptSystem->PushFuncParam(pScriptTable);
		gEnv->pScriptSystem->EndCall(pScriptTable);
		gEnv->bMultiplayer = true;
	}
}

void AI_Scout::Update(SEntityUpdateContext& ctx, int updateSlot)
{
	CScout::Update(ctx, updateSlot);

	// Register AI System in MP
	if (gEnv->bServer && !gEnv->bEditor)
		RegisterMultiplayerAI();

	// Movement reqeust stuff so proper anims play on client
	if (gEnv->bServer)
	{
		SMovementState currMovement = static_cast<CCompatibilityAlienMovementController*>(GetMovementController())->GetMovementReqState();

		m_vLookTarget = currMovement.eyePosition + currMovement.bodyDirection;
		m_vAimTarget = currMovement.eyePosition + currMovement.aimDirection;

		if (GetHealth() > 0.f)
			GetGameObject()->ChangedNetworkState(ASPECT_ALIVE);
	}
	else
	{
		UpdateMovementState();
	}

}

void AI_Scout::UpdateMovementState()
{
	CMovementRequest request;
	request.SetBodyTarget(m_vLookTarget);
	request.SetLookTarget(m_vLookTarget);
	request.SetAimTarget(m_vAimTarget);


	SetActorMovement(SMovementRequestParams(request));
}

void AI_Scout::ProcessEvent(SEntityEvent& event)
{
	CScout::ProcessEvent(event);

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
					char* equip;
					if (props->GetValue("equip_EquipmentPack", equip))
					{
						if (equip[0] == '\0')
							equip = "Alien_Scout_Gunner";

						gEnv->pGame->GetIGameFramework()->GetIItemSystem()->GetIEquipmentManager()->GiveEquipmentPack(this, equip, true, true);
					}
				}

				break;
			}
		}
		break;
	}
}

bool AI_Scout::NetSerialize(TSerialize ser, EEntityAspects aspect, uint8 profile, int flags)
{
	if (!CScout::NetSerialize(ser, aspect, profile, flags))
		return false;

	bool bReading = ser.IsReading();

	switch (aspect)
	{
		case ASPECT_ALIVE:
		{
			ser.Value("vLookTarget", m_vLookTarget, 'wrld');
			ser.Value("vAimTarget", m_vAimTarget, 'wrld');
			break;
		}
		case ASPECT_HIDE:
		{
			ser.Value("bHide", m_bHidden, 'bool');

			if (bReading)
				GetEntity()->Hide(m_bHidden);

			break;
		}
	}

	return true;
}

