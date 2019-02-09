#if _MSC_VER > 1000
# pragma once
#endif

#include "Trooper.h"


class AI_Trooper : public CTrooper
{
public:
	AI_Trooper();
	virtual ~AI_Trooper();

	enum CoopTimers
	{
		eTIMER_WEAPONDELAY	= 0x110
	};

	virtual bool Init( IGameObject * pGameObject );
	virtual void PostInit( IGameObject * pGameObject );
	virtual void Update(SEntityUpdateContext& ctx, int updateSlot);
	virtual bool NetSerialize(TSerialize ser, EEntityAspects aspect, uint8 profile, int flags);
	virtual void ProcessEvent(SEntityEvent& event);

protected:
	static const EEntityAspects ASPECT_ALIVE = eEA_GameServerDynamic;
	static const EEntityAspects ASPECT_HIDE = eEA_GameServerStatic;

	void RegisterMultiplayerAI();
	void UpdateMovementState();

private:
	Vec3 m_vLookTarget;
	Vec3 m_vAimTarget;

	bool m_bHidden;
};