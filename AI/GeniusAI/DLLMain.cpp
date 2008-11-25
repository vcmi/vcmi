#define VCMI_DLL
#pragma warning (disable: 4100 4251 4245 4018 4081)
#include "../../AI_Base.h"
#pragma warning (default: 4100 4251 4245 4018 4081)

#include "CGeniusAI.h"
#include <cstring>
#include <set>

using namespace GeniusAI;

const char *g_cszAiName = "Genius 1.0";

extern "C" DLL_EXPORT int GetGlobalAiVersion()
{
	return AI_INTERFACE_VER;
}

extern "C" DLL_EXPORT void GetAiName(char* name)
{
	strcpy_s(name, strlen(g_cszAiName) + 1, g_cszAiName);
}

extern "C" DLL_EXPORT char * GetAiNameS()
{
	char *ret = new char[strlen(g_cszAiName) + 1];
	strcpy_s(ret, strlen(g_cszAiName), g_cszAiName);
	return ret;
}

extern "C" DLL_EXPORT CGlobalAI * GetNewAI()
{
	return new CGeniusAI();
}

extern "C" DLL_EXPORT void ReleaseAI(CGlobalAI * i)
{
	delete (CGeniusAI*)i;
}