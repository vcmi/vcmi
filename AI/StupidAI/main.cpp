#include "StdInc.h"

#include "../../lib/AI_Base.h"
#include "StupidAI.h"

#ifdef __GNUC__
#define strcpy_s(a, b, c) strncpy(a, c, b)
#endif

#ifdef VCMI_ANDROID
#define GetGlobalAiVersion StupidAI_GetGlobalAiVersion
#define GetAiName StupidAI_GetAiName
#define GetNewBattleAI StupidAI_GetNewBattleAI
#endif

static const char *g_cszAiName = "Stupid AI 0.1";

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
	out = std::make_shared<CStupidAI>();
}
