#include <StdAfx.h>
#include <IAISystem.h>
#include <IGame.h>
#include <IGameFramework.h>
#include <IActorSystem.h>
#include <IAgent.h>
#include <IItemSystem.h>
#include "nCX_AI.h"
#include <IVehicleSystem.h>

// Static nCX_AI class instance forward declaration.
nCX_AI nCX_AI::s_instance = nCX_AI();

nCX_AI::nCX_AI() :
	m_nInitialized(0)
{
}

nCX_AI::~nCX_AI()
{
}

bool nCX_AI::Initialize()
{
	gEnv->pGame->GetIGameFramework()->GetILevelSystem()->AddListener(this);

	ICVar* pAIUpdateAlways = gEnv->pConsole->GetCVar("ai_UpdateAllAlways");
	ICVar* pCheatCvar = gEnv->pConsole->GetCVar("sv_cheatprotection");

	if (pAIUpdateAlways)
		pAIUpdateAlways->ForceSet("1");

	if (pCheatCvar)
		pCheatCvar->ForceSet("0");

	return true;
}

void nCX_AI::CompleteInit()
{
	//gEnv->pSystem->SetIDialogSystem(m_pDialogSystem);
}

void nCX_AI::Shutdown()
{
	gEnv->pGame->GetIGameFramework()->GetILevelSystem()->RemoveListener(this);
}

bool bReinited = false;


// Summary:
//	Updates the nCX_AI instance.
void nCX_AI::Update(float fFrameTime)
{
	//Not using right now
	/*if (m_pDialogSystem)
		m_pDialogSystem->Update(fFrameTime);*/

	//Check if AI System works fine
	/*ICVar* pAIUpdateAlways = gEnv->pConsole->GetCVar("ai_UpdateAllAlways");
	if (pAIUpdateAlways->GetIVal() != 1 || gEnv->pAISystem->GetUpdateAllAlways() != true)
	{
		gEnv->bMultiplayer = false;
		pAIUpdateAlways->ForceSet("1");
		gEnv->pAISystem->Enable(true);
		CryLogAlways("AI not updated, turning ON");
		gEnv->bMultiplayer = true;
	}*/

	// Registers vehicles into the AI system // moved to nCX ticktimer
	
	//Not using right now
	//nCX_AI::GetInstance()->Update(fFrameTime);
}

void nCX_AI::OnLoadingStart(ILevelInfo *pLevel)
{
	m_nInitialized = 0;

	gEnv->bMultiplayer = false;
	if (!gEnv->pAISystem->Init())
		CryLogAlways("[ERROR] AI System Initialization Failed");

	gEnv->pAISystem->FlushSystem();
	gEnv->pAISystem->Enable();
	gEnv->pAISystem->LoadNavigationData(pLevel->GetPath(), "mission0");
	
	ICVar* pSystemUpdate = gEnv->pConsole->GetCVar("ai_systemupdate");
	if (gEnv->bServer)
		pSystemUpdate->Set(1);
	else
		pSystemUpdate->Set(0);

	gEnv->bMultiplayer = true;
}

void nCX_AI::OnLoadingComplete(ILevel *pLevel)
{
	int nInitialized = 0;
	gEnv->bMultiplayer = false;
	m_nInitialized = 1;
	
	gEnv->pAISystem->Reset(IAISystem::RESET_ENTER_GAME);
	gEnv->bMultiplayer = true;
}

void nCX_AI::ReSpawnEntity(IEntity *pEntity)
{
	SEntitySpawnParams spawnParams;
	spawnParams.pArchetype = pEntity->GetArchetype();
	spawnParams.sName = pEntity->GetName();
	spawnParams.vPosition = pEntity->GetPos(); //Temporary - change it to first spawn pos
	spawnParams.vPosition.z = spawnParams.vPosition.z + 100;
	spawnParams.qRotation = pEntity->GetRotation();
	spawnParams.nFlags = pEntity->GetFlags();// ENTITY_FLAG_SPAWNED | ENTITY_FLAG_NET_PRESENT | ENTITY_FLAG_CASTSHADOW;
	//After filling params remove dead entity and respawn it
	gEnv->pEntitySystem->RemoveEntity(pEntity->GetId(), false);
	gEnv->pEntitySystem->SpawnEntity(spawnParams);
}
