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

#include "CEmptyAI.h"

std::set<CGlobalAI*> ais;
extern "C" DLL_EXPORT int GetGlobalAiVersion()
{
	return AI_INTERFACE_VER;
}

extern "C" DLL_EXPORT void GetAiName(char* name)
{
	strcpy(name,NAME);
}

extern "C" DLL_EXPORT void GetNewAI(std::shared_ptr<CGlobalAI> &out)
{
	out = std::make_shared<CEmptyAI>();
}
