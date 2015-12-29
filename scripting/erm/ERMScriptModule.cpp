#include "StdInc.h"
#include "ERMScriptModule.h"

#include "ERMInterpreter.h"

/*
 * ERMScriptingModule.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

IGameEventRealizer *acb;
CPrivilagedInfoCallback *icb;


#ifdef __GNUC__
#define strcpy_s(a, b, c) strncpy(a, c, b)
#endif

const char *g_cszAiName = "(V)ERM interpreter";

extern "C" DLL_EXPORT void GetAiName(char* name)
{
	strcpy_s(name, strlen(g_cszAiName) + 1, g_cszAiName);
}

extern "C" DLL_EXPORT void GetNewModule(std::shared_ptr<CScriptingModule> &out)
{
	out = std::make_shared<ERMInterpreter>();
}