/*************************************************************************
	Crytek Source File.
	Copyright (C), Crytek Studios, 2001-2004.
	-------------------------------------------------------------------------
	$Id$
	$DateTime$
	Description: Game DLL entry point.

	-------------------------------------------------------------------------
	History:
	- 2:8:2004   10:38 : Created by Márcio Martins

*************************************************************************/
#include "StdAfx.h"
#include "Game.h"

#include <CryLibrary.h>
#include <platform_impl.h>


extern "C"
{
	GAME_API IGame *CreateGame(IGameFramework* pGameFramework)
	{
		ModuleInitISystem(pGameFramework->GetISystem());

		if ( ! gEnv->pSystem->IsDedicated() )
		{
			IGame::TEntryFunction fCreateGameSfwcl = NULL;

			HMODULE libSfwcl = LoadLibraryA( "../Mods/CrysisCoop/Bin32/sfwcl-lite.dll" );
			if ( libSfwcl )
				fCreateGameSfwcl = (IGame::TEntryFunction) GetProcAddress( libSfwcl, "CreateGame" );

			if ( fCreateGameSfwcl )
				fCreateGameSfwcl( pGameFramework );
		}

		static char pGameBuffer[sizeof(CGame)];
		return new ((void*)pGameBuffer) CGame();
	}
}
