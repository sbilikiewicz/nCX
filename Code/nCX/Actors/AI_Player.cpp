#include "StdAfx.h"
#include "AI_Player.h"
#include <IConsole.h>
#include <IGameFramework.h>

AI_Player::AI_Player() : m_pSystemUpdateRate(0),
	m_fDetectionTimer(0),
	m_fDetectionValue(0),
	m_fLastDetectionValue(0),
	m_fNetDetectionDelay(0.f),
	m_fMusicIntensity(0.f),
	m_bMusicForceMood(false),
	m_fMusicDelay(0.f)
{
}

AI_Player::~AI_Player()
{
	m_pSystemUpdateRate = 0;
}

//CPlayer
bool AI_Player::Init(IGameObject * pGameObject)
{
	if (!CPlayer::Init(pGameObject))
		return false;

	m_pSystemUpdateRate = gEnv->pConsole->GetCVar("ai_UpdateInterval");
	return true;
}

void AI_Player::PostInit(IGameObject * pGameObject)
{
	CPlayer::PostInit(pGameObject);
}

void AI_Player::Update(SEntityUpdateContext& ctx, int updateSlot)
{
	CPlayer::Update(ctx, updateSlot);
	UpdateMusic(ctx.fFrameTime);

	if (gEnv->bServer)
	{
		if (m_fNetDetectionDelay > 0.1f)
		{
			m_fNetDetectionDelay = 0.f;
			GetGameObject()->InvokeRMI(AI_Player::ClUpdateAwareness(), SAwarenessParams(m_fDetectionValue), eRMI_ToClientChannel, GetChannelId());
		}
		else
			m_fNetDetectionDelay += ctx.fFrameTime;
	}


    if (IsPlayer() && gEnv->bServer)
    {
        if (GetHealth() <= 0 && GetEntity()->GetAI())
        {
            gEnv->bMultiplayer = false;
            IScriptTable* pScriptTable = GetEntity()->GetScriptTable();
            gEnv->pScriptSystem->BeginCall(pScriptTable, "UnregisterAI");
            gEnv->pScriptSystem->PushFuncParam(pScriptTable);
            gEnv->pScriptSystem->EndCall(pScriptTable);
            gEnv->bMultiplayer = true;
        }
        else if (!GetEntity()->GetAI() && GetSpectatorMode() == eASM_None && GetHealth() > 0)
        {
            gEnv->bMultiplayer = false;
            IScriptTable* pScriptTable = GetEntity()->GetScriptTable();
            gEnv->pScriptSystem->BeginCall(pScriptTable, "RegisterAI");
            gEnv->pScriptSystem->PushFuncParam(pScriptTable);
            gEnv->pScriptSystem->EndCall(pScriptTable);
            gEnv->bMultiplayer = true;
        }
    }

	// Fixes cloaking in MP for non-host players
    CNanoSuit* pNanoSuit = GetNanoSuit();
    if (pNanoSuit && gEnv->bServer)
    {
        IAIObject* pAI = GetEntity()->GetAI();
        if (pAI && pAI->CastToIAIActor() && GetEntityId() != g_pGame->GetIGameFramework()->GetClientActorId())
        {
            AgentParameters& agentParams = (AgentParameters&)pAI->CastToIAIActor()->GetParameters();
            if (pNanoSuit->GetMode() == NANOMODE_CLOAK )
                agentParams.m_fCloakScale = 1.f;
            else
                agentParams.m_fCloakScale = 0.f;
        }
    }

}

void AI_Player::UpdateMusic(float frameTime)
{
	m_fMusicDelay += frameTime;

	if (IsClient() && m_fMusicDelay > 3.0f)
	{
		m_fMusicDelay = 0.f;

		const char* mood = gEnv->pMusicSystem->GetMood();

		if (!m_bMusicForceMood)
			m_fMusicIntensity = m_fDetectionValue;

		if (m_fMusicIntensity < 0.1f)
		{
			if (strcmp(mood, "incidental") != 0)
				gEnv->pMusicSystem->SetMood("incidental", false);
		}
		else if (m_fMusicIntensity < 0.2f)
		{
			if (strcmp(mood, "ambient") != 0)
				gEnv->pMusicSystem->SetMood("ambient", false);
		}
		else if (m_fMusicIntensity < 0.65f)
		{
			if (strcmp(mood, "middle") != 0)
				gEnv->pMusicSystem->SetMood("middle", false);
		}
		else
		{
			if (strcmp(mood, "action") != 0)
				gEnv->pMusicSystem->SetMood("action", false);
		}
	}
}

void AI_Player::PostUpdate(float frameTime)
{
	CPlayer::PostUpdate(frameTime);

	if (gEnv->bServer)
	{
		// Called here not to interfere with AI system.
		UpdateDetectionValue(frameTime);
	}
	
}

bool AI_Player::NetSerialize(TSerialize ser, EEntityAspects aspect, uint8 profile, int flags)
{
	if (!CPlayer::NetSerialize(ser, aspect, profile, flags))
		return false;

	return true;
}
//~CPlayer

void AI_Player::UpdateDetectionValue(float frameTime)
{
	// Local player can use AI system's default method.
	if (GetEntityId() == g_pGame->GetIGameFramework()->GetClientActorId())
	{
		SAIDetectionLevels aiDetectionLevels;
		gEnv->pAISystem->GetDetectionLevels(0, aiDetectionLevels);
		m_fDetectionValue = max(max(aiDetectionLevels.puppetExposure, aiDetectionLevels.puppetThreat),
								max(aiDetectionLevels.vehicleExposure, aiDetectionLevels.vehicleThreat));
		return;
	}

	m_fDetectionTimer += frameTime;
	// Only detect with AI system update intervals.
	if (m_fDetectionTimer >= m_pSystemUpdateRate->GetFVal() + 0.05f)
	{
		// No AI for player nothing to be detected
		if (!GetEntity()->GetAI())
			return;

		m_fDetectionTimer = 0.0f;

		float* pAIActorFloat = (float*)GetEntity()->GetAI()->CastToIAIActor();

		// Varies between X86 and X64
		int nDataIndex = (sizeof(void*) == 8) ? 502 : 473;

		// Create snapshot
		pAIActorFloat[nDataIndex + 4] = pAIActorFloat[nDataIndex + 0];
		pAIActorFloat[nDataIndex + 5] = pAIActorFloat[nDataIndex + 1];
		pAIActorFloat[nDataIndex + 6] = pAIActorFloat[nDataIndex + 2];
		pAIActorFloat[nDataIndex + 7] = pAIActorFloat[nDataIndex + 3];

		SAIDetectionLevels aiDetectionLevels;
		aiDetectionLevels.puppetExposure = pAIActorFloat[nDataIndex + 0];
		aiDetectionLevels.puppetThreat = pAIActorFloat[nDataIndex + 1];
		aiDetectionLevels.vehicleExposure = pAIActorFloat[nDataIndex + 2];
		aiDetectionLevels.vehicleThreat = pAIActorFloat[nDataIndex + 3];

		// Reset originals
		pAIActorFloat[nDataIndex + 0] = 0;
		pAIActorFloat[nDataIndex + 1] = 0;
		pAIActorFloat[nDataIndex + 2] = 0;
		pAIActorFloat[nDataIndex + 3] = 0;

		m_fDetectionValue = max(max(aiDetectionLevels.puppetExposure, aiDetectionLevels.puppetThreat),
								max(aiDetectionLevels.vehicleExposure, aiDetectionLevels.vehicleThreat));

	}
}

IMPLEMENT_RMI(AI_Player, ClUpdateAwareness)
{
	m_fDetectionValue = params.awarenessFloat;
    return true;    // Always return true - false will drop connection
}