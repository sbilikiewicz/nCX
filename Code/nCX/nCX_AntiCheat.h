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
	static bool		CheckShootPos(CActor *pActor, Vec3 DefinedPos, float time) { nCX_Anticheat ch1; return ch1.ShootPositionCheck(pActor, DefinedPos, time); };
protected:
	//Private functions
	void	OnCheatDetected(EntityId entityId, const char* cheat, const char* info, bool sure);
	bool	ShootPositionCheck(CActor *pActor, Vec3 DefinedPos, float time);
};