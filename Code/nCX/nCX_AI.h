//#ifndef _CoopSystem_H_
//#define _CoopSystem_H_

#include <ILevelSystem.h>

class nCX_AI
	: public ILevelSystemListener
{
private:
	// Static CCoopSystem class instance forward declaration.
	static nCX_AI s_instance;

	nCX_AI();
	~nCX_AI();

public:
	// Summary:
	//	Gets an pointer to the static CCoopSystem instance.
	static inline nCX_AI* GetInstance() {
		return &s_instance;
	}

	// Summary:
	//	Initializes the nCX_AI instance.
	bool Initialize();

	// Summary:
	//	Initialize Complete
	void CompleteInit();

	// Summary:
	//	Shuts down the nCX_AI instance.
	void Shutdown();

	// Summary:
	//	Updates the nCX_AI instance.
	void Update(float fFrameTime);


	// ILevelSystemListener
	virtual void OnLevelNotFound(const char *levelName) { };
	virtual void OnLoadingStart(ILevelInfo *pLevel);
	virtual void OnLoadingComplete(ILevel *pLevel);
	virtual void OnLoadingError(ILevelInfo *pLevel, const char *error) { };
	virtual void OnLoadingProgress(ILevelInfo *pLevel, int progressAmount) { };
	// ~ILevelSystemListener

private:
	int					m_nInitialized;

	IEntityClass* m_pEntityClassPlayer;
	IEntityClass* m_pEntityClassGrunt;
	IEntityClass* m_pEntityClassAlien;
	IEntityClass* m_pEntityClassScout;
	IEntityClass* m_pEntityClassTrooper;
	IEntityClass* m_pEntityClassHunter;

private:

};

//#endif // _CoopSystem_H_