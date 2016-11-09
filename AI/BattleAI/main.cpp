/*
 * main.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "../../lib/AI_Base.h"
#include "BattleAI.h"

#ifdef __GNUC__
#define strcpy_s(a, b, c) strncpy(a, c, b)
#endif

static const char *g_cszAiName = "Battle AI";

extern "C" DLL_EXPORT int GetGlobalAiVersion()
{
	return AI_INTERFACE_VER;
}

extern "C" DLL_EXPORT void GetAiName(char* name)
{
	strcpy_s(name, strlen(g_cszAiName) + 1, g_cszAiName);
}

extern "C" DLL_EXPORT void GetNewBattleAI(std::shared_ptr<CBattleGameInterface> &out)
{
	out = std::make_shared<CBattleAI>();
}

#ifdef VCMI_ANDROID
#	include "../../lib/CGameInterface.h"
	
	__attribute__((constructor)) void AndroidEntryPoint() 
	{
		std::function<void(char*)> nameFun = GetAiName;
		std::function<void(std::shared_ptr<CBattleGameInterface>&)> aiFun = GetNewBattleAI;
		CGameInterfaceAndroidRegisterAIHook(nameFun, aiFun);
	}
#endif
