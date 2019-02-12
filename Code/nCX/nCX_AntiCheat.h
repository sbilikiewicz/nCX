#pragma once
#include <iostream>
#include <IGameObject.h>
#include <IGameRulesSystem.h>
#include "Actor.h"
#include "Weapon.h"

class nCX_Anticheat
{
public:
	//static reference
	static void		CheatDetected(EntityId entityId, const char* cheat, const char* info, bool sure) { nCX_Anticheat ch; ch.OnCheatDetected(entityId, cheat, info, sure); };
	static bool		CheckShootPos(CActor *pActor, Vec3 DefinedPos, float time) { nCX_Anticheat ch1; return ch1.ShootPositionCheck(pActor, DefinedPos, time); };
	static bool		CheckLongpoke(CActor *pActor, int seq) { nCX_Anticheat ch2; return ch2.LongpokeCheck(pActor, seq); };
	static bool		CheckRecoil(CWeapon *pWeapon, CWeapon::SvRequestShootParams params) { nCX_Anticheat ch3; return ch3.RecoilCheck(pWeapon, params); };
protected:
	//Private functions
	void	OnCheatDetected(EntityId entityId, const char* cheat, const char* info, bool sure);
	bool	ShootPositionCheck(CActor *pActor, Vec3 DefinedPos, float time);
	bool	LongpokeCheck(CActor *pActor, int seq);
	bool	RecoilCheck(CWeapon *pWeapon, CWeapon::SvRequestShootParams params);
};