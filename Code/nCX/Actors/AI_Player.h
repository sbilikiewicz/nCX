#include "Player.h"

class AI_Player : public CPlayer
{
public:
	AI_Player();
	virtual ~AI_Player();

	//CPlayer
	virtual bool Init(IGameObject * pGameObject);
	virtual void PostInit(IGameObject * pGameObject);
	virtual void PostUpdate(float frameTime);
	virtual void Update(SEntityUpdateContext& ctx, int updateSlot);
	virtual bool NetSerialize(TSerialize ser, EEntityAspects aspect, uint8 profile, int flags);
	//~CPlayer

	struct SAwarenessParams
	{
		SAwarenessParams() {};
		SAwarenessParams(float awarenessFloat): 
			awarenessFloat(awarenessFloat) 
		{};

		float awarenessFloat;
		void SerializeWith(TSerialize ser)
		{
			ser.Value("awarenessFloat", awarenessFloat);
		}
	};

	void ForceMusicMood(float intensity, bool force){m_bMusicForceMood = force; m_fMusicIntensity = intensity; m_fMusicDelay = 5.f;};

	DECLARE_CLIENT_RMI_PREATTACH(ClUpdateAwareness, SAwarenessParams, eNRT_ReliableUnordered);

private:
	void UpdateDetectionValue(float frameTime);
	void UpdateMusic(float frameTime);

public:
	
	// Summary:
	//	Gets the player's current detection value.
	float GetDetectionValue() {
		return m_fDetectionValue;
	}

private:
	struct ICVar* m_pSystemUpdateRate;
	float m_fDetectionTimer;
	float m_fDetectionValue;

	float m_fMusicDelay;

	float m_fLastDetectionValue;

	float m_fNetDetectionDelay;

	float m_fMusicIntensity;
	bool m_bMusicForceMood;
};