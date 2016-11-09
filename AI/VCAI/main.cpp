#include "StdInc.h"
#include "VCAI.h"

#ifdef __GNUC__
#define strcpy_s(a, b, c) strncpy(a, c, b)
#endif

static const char *g_cszAiName = "VCAI";

extern "C" DLL_EXPORT int GetGlobalAiVersion()
{
	return AI_INTERFACE_VER;
}

extern "C" DLL_EXPORT void GetAiName(char* name)
{
	strcpy_s(name, strlen(g_cszAiName) + 1, g_cszAiName);
}

extern "C" DLL_EXPORT void GetNewAI(std::shared_ptr<CGlobalAI> &out)
{
	out = std::make_shared<VCAI>();
}

#ifdef VCMI_ANDROID
#	include "../../lib/CGameInterface.h"

	__attribute__((constructor)) void AndroidEntryPoint() 
	{
		std::function<void(char*)> nameFun = GetAiName;
		std::function<void(std::shared_ptr<CGlobalAI>&)> aiFun = GetNewAI;
		CGameInterfaceAndroidRegisterAIHook(nameFun, aiFun);
	}
#endif