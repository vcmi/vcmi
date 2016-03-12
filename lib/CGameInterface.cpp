#include "StdInc.h"
#include "CGameInterface.h"

#include "BattleState.h"
#include "VCMIDirs.h"

#ifdef VCMI_WINDOWS
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

#ifdef VCMI_ANDROID
// we can't use shared libraries on Android so here's a hack
extern "C" DLL_EXPORT void VCAI_GetAiName(char* name);
extern "C" DLL_EXPORT void VCAI_GetNewAI(std::shared_ptr<CGlobalAI> &out);

extern "C" DLL_EXPORT void StupidAI_GetAiName(char* name);
extern "C" DLL_EXPORT void StupidAI_GetNewBattleAI(std::shared_ptr<CGlobalAI> &out);

extern "C" DLL_EXPORT void BattleAI_GetAiName(char* name);
extern "C" DLL_EXPORT void BattleAI_GetNewBattleAI(std::shared_ptr<CBattleGameInterface> &out);
#endif

template<typename rett>
std::shared_ptr<rett> createAny(const boost::filesystem::path& libpath, const std::string& methodName)
{
	typedef void(*TGetAIFun)(std::shared_ptr<rett>&); 
	typedef void(*TGetNameFun)(char*); 

	char temp[150];

	TGetAIFun getAI = nullptr;
	TGetNameFun getName = nullptr;

#ifdef VCMI_ANDROID
	// this is awful but it seems using shared libraries on some devices is even worse
	const std::string filename = libpath.filename().string();
	if (filename == "libVCAI.so")
	{
		getName = (TGetNameFun)VCAI_GetAiName;
		getAI = (TGetAIFun)VCAI_GetNewAI;
	}
	else if (filename == "libStupidAI.so")
	{
		getName = (TGetNameFun)StupidAI_GetAiName;
		getAI = (TGetAIFun)StupidAI_GetNewBattleAI;
	}
	else if (filename == "libBattleAI.so")
	{
		getName = (TGetNameFun)BattleAI_GetAiName;
		getAI = (TGetAIFun)BattleAI_GetNewBattleAI;
	}
	else
		throw std::runtime_error("Don't know what to do with " + libpath.string() + " and method " + methodName);
#else // !VCMI_ANDROID
#ifdef VCMI_WINDOWS
	HMODULE dll = LoadLibraryW(libpath.c_str());
	if (dll)
	{
		getName = (TGetNameFun)GetProcAddress(dll, "GetAiName");
		getAI = (TGetAIFun)GetProcAddress(dll, methodName.c_str());
	}
#else // !VCMI_WINDOWS
	void *dll = dlopen(libpath.string().c_str(), RTLD_LOCAL | RTLD_LAZY);
	if (dll)
	{
		getName = (TGetNameFun)dlsym(dll, "GetAiName");
		getAI = (TGetAIFun)dlsym(dll, methodName.c_str());
	}
	else
		logGlobal->errorStream() << "Error: " << dlerror();
#endif // VCMI_WINDOWS
	if (!dll)
	{
		logGlobal->errorStream() << "Cannot open dynamic library ("<<libpath<<"). Throwing...";
		throw std::runtime_error("Cannot open dynamic library");
	}
	else if(!getName || !getAI)
	{
		logGlobal->errorStream() << libpath << " does not export method " << methodName;
#ifdef VCMI_WINDOWS
		FreeLibrary(dll);
#else
		dlclose(dll);
#endif
		throw std::runtime_error("Cannot find method " + methodName);
	}
#endif // VCMI_ANDROID

	getName(temp);
	logGlobal->infoStream() << "Loaded " << temp;

	std::shared_ptr<rett> ret;
	getAI(ret);
	if(!ret)
		logGlobal->errorStream() << "Cannot get AI!";

	return ret;
}

template<typename rett>
std::shared_ptr<rett> createAnyAI(std::string dllname, const std::string& methodName)
{
	logGlobal->infoStream() << "Opening " << dllname;
	const boost::filesystem::path filePath =
		VCMIDirs::get().libraryPath() / "AI" / VCMIDirs::get().libraryName(dllname);
	auto ret = createAny<rett>(filePath, methodName);
	ret->dllName = std::move(dllname);
	return ret;
}

std::shared_ptr<CGlobalAI> CDynLibHandler::getNewAI(std::string dllname)
{
	return createAnyAI<CGlobalAI>(dllname, "GetNewAI");
}

std::shared_ptr<CBattleGameInterface> CDynLibHandler::getNewBattleAI(std::string dllname )
{
	return createAnyAI<CBattleGameInterface>(dllname, "GetNewBattleAI");
}

std::shared_ptr<CScriptingModule> CDynLibHandler::getNewScriptingModule(std::string dllname)
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

void CAdventureAI::saveGame(COSer & h, const int version) /*saving */
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

void CAdventureAI::loadGame(CISer & h, const int version) /*loading */
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

void CBattleGameInterface::saveGame(COSer & h, const int version)
{
}

void CBattleGameInterface::loadGame(CISer  & h, const int version)
{
}
