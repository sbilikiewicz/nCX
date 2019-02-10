#pragma once
#include <iostream>
#include <IGameObject.h>
#include <IGameRulesSystem.h>
#include "Actor.h"

class nCX_Anticheat
{
public:
	//static reference
	static void		CheatDetected(EntityId entityId, const char* cheat, const char* info, bool sure) { nCX_Anticheat ch; ch.OnCheatDetected(entityId, cheat, info, sure); };
	static void		CheckShootPos(IActor *pActor, Vec3 DefinedPos, float time) { nCX_Anticheat ch1; ch1.ShootPositionCheck(pActor, DefinedPos, time); };
protected:
	//Private functions
	void	OnCheatDetected(EntityId entityId, const char* cheat, const char* info, bool sure);
	void	ShootPositionCheck(IActor *pActor, Vec3 DefinedPos, float time);
};