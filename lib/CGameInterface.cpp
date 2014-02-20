#include "StdInc.h"
#include "CGameInterface.h"

#include "BattleState.h"
#include "VCMIDirs.h"

#ifdef _WIN32
	#define WIN32_LEAN_AND_MEAN //excludes rarely used stuff from windows headers - delete this line if something is missing
	#include <windows.h> //for .dll libs
#else
	#include <dlfcn.h>
#endif
#include "Connection.h"

/*
 * CGameInterface.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#ifdef __ANDROID__
// we can't use shared libraries on Android so here's a hack
extern "C" DLL_EXPORT void VCAI_GetAiName(char* name);
extern "C" DLL_EXPORT void VCAI_GetNewAI(shared_ptr<CGlobalAI> &out);

extern "C" DLL_EXPORT void StupidAI_GetAiName(char* name);
extern "C" DLL_EXPORT void StupidAI_GetNewBattleAI(shared_ptr<CGlobalAI> &out);

extern "C" DLL_EXPORT void BattleAI_GetAiName(char* name);
extern "C" DLL_EXPORT void BattleAI_GetNewBattleAI(shared_ptr<CBattleGameInterface> &out);
#endif

template<typename rett>
shared_ptr<rett> createAny(std::string dllname, std::string methodName)
{
	typedef void(*TGetAIFun)(shared_ptr<rett>&); 
	typedef void(*TGetNameFun)(char*); 

	char temp[150];

	TGetAIFun getAI = nullptr;
	TGetNameFun getName = nullptr;

#ifdef __ANDROID__
	// this is awful but it seems using shared libraries on some devices is even worse
	if (dllname.find("libVCAI.so") != std::string::npos) {
		getName = (TGetNameFun)VCAI_GetAiName;
		getAI = (TGetAIFun)VCAI_GetNewAI;
	} else if (dllname.find("libStupidAI.so") != std::string::npos) {
		getName = (TGetNameFun)StupidAI_GetAiName;
		getAI = (TGetAIFun)StupidAI_GetNewBattleAI;
	} else if (dllname.find("libBattleAI.so") != std::string::npos) {
		getName = (TGetNameFun)BattleAI_GetAiName;
		getAI = (TGetAIFun)BattleAI_GetNewBattleAI;
	} else {
		throw std::runtime_error("Don't know what to do with " + dllname + " and method " + methodName);
	}
#else

#ifdef _WIN32
	HINSTANCE dll = LoadLibraryA(dllname.c_str());
	if (dll)
	{
		getName = (TGetNameFun)GetProcAddress(dll,"GetAiName");
		getAI = (TGetAIFun)GetProcAddress(dll,methodName.c_str());
	}
#else
	void *dll = dlopen(dllname.c_str(), RTLD_LOCAL | RTLD_LAZY);
	if (dll)
	{
		getName = (TGetNameFun)dlsym(dll,"GetAiName");
		getAI = (TGetAIFun)dlsym(dll,methodName.c_str());
	}
	else
        logGlobal->errorStream() << "Error: " << dlerror();
#endif
	if (!dll)
	{
        logGlobal->errorStream() << "Cannot open dynamic library ("<<dllname<<"). Throwing...";
		throw std::runtime_error("Cannot open dynamic library");
	}
	else if(!getName || !getAI)
	{
        logGlobal->errorStream() << dllname << " does not export method " << methodName;
#ifdef _WIN32
		FreeLibrary(dll);
#else
		dlclose(dll);
#endif
		throw std::runtime_error("Cannot find method " + methodName);
	}

#endif // __ANDROID__

	getName(temp);
    logGlobal->infoStream() << "Loaded " << temp;

	shared_ptr<rett> ret;
	getAI(ret);
	if(!ret)
        logGlobal->errorStream() << "Cannot get AI!";

	return ret;
}

template<typename rett>
shared_ptr<rett> createAnyAI(std::string dllname, std::string methodName)
{
    logGlobal->infoStream() << "Opening " << dllname;
	std::string filename = VCMIDirs::get().libraryName(dllname);

	auto ret = createAny<rett>(VCMIDirs::get().libraryPath() + "/AI/" + filename, methodName);
	ret->dllName = dllname;
	return ret;
}

shared_ptr<CGlobalAI> CDynLibHandler::getNewAI(std::string dllname)
{
	return createAnyAI<CGlobalAI>(dllname, "GetNewAI");
}

shared_ptr<CBattleGameInterface> CDynLibHandler::getNewBattleAI(std::string dllname )
{
	return createAnyAI<CBattleGameInterface>(dllname, "GetNewBattleAI");
}

shared_ptr<CScriptingModule> CDynLibHandler::getNewScriptingModule(std::string dllname)
{
	return createAny<CScriptingModule>(dllname, "GetNewModule");
}

BattleAction CGlobalAI::activeStack( const CStack * stack )
{
	BattleAction ba; ba.actionType = Battle::DEFEND;
	ba.stackNumber = stack->ID;
	return ba;
}

CGlobalAI::CGlobalAI()
{
	human = false;
}

void CAdventureAI::battleNewRound(int round)
{
	battleAI->battleNewRound(round);
}

void CAdventureAI::battleCatapultAttacked(const CatapultAttack & ca)
{
	battleAI->battleCatapultAttacked(ca);
}

void CAdventureAI::battleStart(const CCreatureSet *army1, const CCreatureSet *army2, int3 tile, const CGHeroInstance *hero1, const CGHeroInstance *hero2, bool side)
{
	assert(!battleAI);
	assert(cbc);
	battleAI = CDynLibHandler::getNewBattleAI(getBattleAIName());
	battleAI->init(cbc);
	battleAI->battleStart(army1, army2, tile, hero1, hero2, side);
}

void CAdventureAI::battleStacksAttacked(const std::vector<BattleStackAttacked> & bsa)
{
	battleAI->battleStacksAttacked(bsa);
}

void CAdventureAI::actionStarted(const BattleAction &action)
{
	battleAI->actionStarted(action);
}

void CAdventureAI::battleNewRoundFirst(int round)
{
	battleAI->battleNewRoundFirst(round);
}

void CAdventureAI::actionFinished(const BattleAction &action)
{
	battleAI->actionFinished(action);
}

void CAdventureAI::battleStacksEffectsSet(const SetStackEffect & sse)
{
	battleAI->battleStacksEffectsSet(sse);
}

void CAdventureAI::battleStacksRemoved(const BattleStacksRemoved & bsr)
{
	battleAI->battleStacksRemoved(bsr);
}

void CAdventureAI::battleObstaclesRemoved(const std::set<si32> & removedObstacles)
{
	battleAI->battleObstaclesRemoved(removedObstacles);
}

void CAdventureAI::battleNewStackAppeared(const CStack * stack)
{
	battleAI->battleNewStackAppeared(stack);
}

void CAdventureAI::battleStackMoved(const CStack * stack, std::vector<BattleHex> dest, int distance)
{
	battleAI->battleStackMoved(stack, dest, distance);
}

void CAdventureAI::battleAttack(const BattleAttack *ba)
{
	battleAI->battleAttack(ba);
}

void CAdventureAI::battleSpellCast(const BattleSpellCast *sc)
{
	battleAI->battleSpellCast(sc);
}

void CAdventureAI::battleEnd(const BattleResult *br)
{
	battleAI->battleEnd(br);
	battleAI.reset();
}

void CAdventureAI::battleStacksHealedRes(const std::vector<std::pair<ui32, ui32> > & healedStacks, bool lifeDrain, bool tentHeal, si32 lifeDrainFrom)
{
	battleAI->battleStacksHealedRes(healedStacks, lifeDrain, tentHeal, lifeDrainFrom);
}

BattleAction CAdventureAI::activeStack(const CStack * stack)
{
	return battleAI->activeStack(stack);
}

void CAdventureAI::yourTacticPhase(int distance)
{
	battleAI->yourTacticPhase(distance);
}

void CAdventureAI::saveGame(COSer<CSaveFile> &h, const int version) /*saving */
{
	LOG_TRACE_PARAMS(logAi, "version '%i'", version);
	CGlobalAI::saveGame(h, version);
	bool hasBattleAI = static_cast<bool>(battleAI);
	h << hasBattleAI;
	if(hasBattleAI)
	{
		h << std::string(battleAI->dllName);
		battleAI->saveGame(h, version);
	}
}

void CAdventureAI::loadGame(CISer<CLoadFile> &h, const int version) /*loading */
{
	LOG_TRACE_PARAMS(logAi, "version '%i'", version);
	CGlobalAI::loadGame(h, version);
	bool hasBattleAI = false;
	h >> hasBattleAI;
	if(hasBattleAI)
	{
		std::string dllName;
		h >> dllName;
		battleAI = CDynLibHandler::getNewBattleAI(dllName);
		assert(cbc); //it should have been set by the one who new'ed us
		battleAI->init(cbc);
		//battleAI->loadGame(h, version);
	}
}

void CBattleGameInterface::saveGame(COSer<CSaveFile> &h, const int version)
{
}

void CBattleGameInterface::loadGame(CISer<CLoadFile> &h, const int version)
{
}
