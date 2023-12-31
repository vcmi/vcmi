/*
 * CGameInterface.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "CGameInterface.h"

#include "CStack.h"
#include "VCMIDirs.h"

#include "serializer/BinaryDeserializer.h"
#include "serializer/BinarySerializer.h"

#ifdef STATIC_AI
# include "AI/VCAI/VCAI.h"
# include "AI/Nullkiller/AIGateway.h"
# include "AI/BattleAI/BattleAI.h"
# include "AI/StupidAI/StupidAI.h"
# include "AI/EmptyAI/CEmptyAI.h"
#else
# ifdef VCMI_WINDOWS
#  include <windows.h> //for .dll libs
# else
#  include <dlfcn.h>
# endif // VCMI_WINDOWS
#endif // STATIC_AI

VCMI_LIB_NAMESPACE_BEGIN

template<typename rett>
std::shared_ptr<rett> createAny(const boost::filesystem::path & libpath, const std::string & methodName)
{
#ifdef STATIC_AI
	// android currently doesn't support loading libs dynamically, so the access to the known libraries
	// is possible only via specializations of this template
	throw std::runtime_error("Could not resolve ai library " + libpath.generic_string());
#else
	using TGetAIFun = void (*)(std::shared_ptr<rett> &);
	using TGetNameFun = void (*)(char *);

	char temp[150];

	TGetAIFun getAI = nullptr;
	TGetNameFun getName = nullptr;

#ifdef VCMI_WINDOWS
#ifdef __MINGW32__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wcast-function-type"
#endif
	HMODULE dll = LoadLibraryW(libpath.c_str());
	if (dll)
	{
		getName = reinterpret_cast<TGetNameFun>(GetProcAddress(dll, "GetAiName"));
		getAI = reinterpret_cast<TGetAIFun>(GetProcAddress(dll, methodName.c_str()));
	}
#ifdef __MINGW32__
#pragma GCC diagnostic pop
#endif
#else // !VCMI_WINDOWS
	void *dll = dlopen(libpath.string().c_str(), RTLD_LOCAL | RTLD_LAZY);
	if (dll)
	{
		getName = reinterpret_cast<TGetNameFun>(dlsym(dll, "GetAiName"));
		getAI = reinterpret_cast<TGetAIFun>(dlsym(dll, methodName.c_str()));
	}
#endif // VCMI_WINDOWS

	if (!dll)
	{
		logGlobal->error("Cannot open dynamic library (%s). Throwing...", libpath.string());
		throw std::runtime_error("Cannot open dynamic library");
	}
	else if(!getName || !getAI)
	{
		logGlobal->error("%s does not export method %s", libpath.string(), methodName);
#ifdef VCMI_WINDOWS
		FreeLibrary(dll);
#else
		dlclose(dll);
#endif
		throw std::runtime_error("Cannot find method " + methodName);
	}

	getName(temp);
	logGlobal->info("Loaded %s", temp);

	std::shared_ptr<rett> ret;
	getAI(ret);
	if(!ret)
		logGlobal->error("Cannot get AI!");

	return ret;
#endif // STATIC_AI
}

#ifdef STATIC_AI

template<>
std::shared_ptr<CGlobalAI> createAny(const boost::filesystem::path & libpath, const std::string & methodName)
{
	if(libpath.stem() == "libNullkiller") {
		return std::make_shared<NKAI::AIGateway>();
	}
	else{
		return std::make_shared<VCAI>();
	}
}

template<>
std::shared_ptr<CBattleGameInterface> createAny(const boost::filesystem::path & libpath, const std::string & methodName)
{
	if(libpath.stem() == "libBattleAI")
		return std::make_shared<CBattleAI>();
	else if(libpath.stem() == "libStupidAI")
		return std::make_shared<CStupidAI>();
	return std::make_shared<CEmptyAI>();
}

#endif // STATIC_AI

template<typename rett>
std::shared_ptr<rett> createAnyAI(const std::string & dllname, const std::string & methodName)
{
	logGlobal->info("Opening %s", dllname);

	const boost::filesystem::path filePath = VCMIDirs::get().fullLibraryPath("AI", dllname);
	auto ret = createAny<rett>(filePath, methodName);
	ret->dllName = dllname;
	return ret;
}

std::shared_ptr<CGlobalAI> CDynLibHandler::getNewAI(const std::string & dllname)
{
	return createAnyAI<CGlobalAI>(dllname, "GetNewAI");
}

std::shared_ptr<CBattleGameInterface> CDynLibHandler::getNewBattleAI(const std::string & dllname)
{
	return createAnyAI<CBattleGameInterface>(dllname, "GetNewBattleAI");
}

#if SCRIPTING_ENABLED
std::shared_ptr<scripting::Module> CDynLibHandler::getNewScriptingModule(const boost::filesystem::path & dllname)
{
	return createAny<scripting::Module>(dllname, "GetNewModule");
}
#endif

CGlobalAI::CGlobalAI()
{
	human = false;
}

void CAdventureAI::battleNewRound(const BattleID & battleID)
{
	battleAI->battleNewRound(battleID);
}

void CAdventureAI::battleCatapultAttacked(const BattleID & battleID, const CatapultAttack & ca)
{
	battleAI->battleCatapultAttacked(battleID, ca);
}

void CAdventureAI::battleStart(const BattleID & battleID, const CCreatureSet * army1, const CCreatureSet * army2, int3 tile,
							   const CGHeroInstance * hero1, const CGHeroInstance * hero2, bool side, bool replayAllowed)
{
	assert(!battleAI);
	assert(cbc);
	battleAI = CDynLibHandler::getNewBattleAI(getBattleAIName());
	battleAI->initBattleInterface(env, cbc);
	battleAI->battleStart(battleID, army1, army2, tile, hero1, hero2, side, replayAllowed);
}

void CAdventureAI::battleStacksAttacked(const BattleID & battleID, const std::vector<BattleStackAttacked> & bsa, bool ranged)
{
	battleAI->battleStacksAttacked(battleID, bsa, ranged);
}

void CAdventureAI::actionStarted(const BattleID & battleID, const BattleAction & action)
{
	battleAI->actionStarted(battleID, action);
}

void CAdventureAI::battleNewRoundFirst(const BattleID & battleID)
{
	battleAI->battleNewRoundFirst(battleID);
}

void CAdventureAI::actionFinished(const BattleID & battleID, const BattleAction & action)
{
	battleAI->actionFinished(battleID, action);
}

void CAdventureAI::battleStacksEffectsSet(const BattleID & battleID, const SetStackEffect & sse)
{
	battleAI->battleStacksEffectsSet(battleID, sse);
}

void CAdventureAI::battleObstaclesChanged(const BattleID & battleID, const std::vector<ObstacleChanges> & obstacles)
{
	battleAI->battleObstaclesChanged(battleID, obstacles);
}

void CAdventureAI::battleStackMoved(const BattleID & battleID, const CStack * stack, std::vector<BattleHex> dest, int distance, bool teleport)
{
	battleAI->battleStackMoved(battleID, stack, dest, distance, teleport);
}

void CAdventureAI::battleAttack(const BattleID & battleID, const BattleAttack * ba)
{
	battleAI->battleAttack(battleID, ba);
}

void CAdventureAI::battleSpellCast(const BattleID & battleID, const BattleSpellCast * sc)
{
	battleAI->battleSpellCast(battleID, sc);
}

void CAdventureAI::battleEnd(const BattleID & battleID, const BattleResult * br, QueryID queryID)
{
	battleAI->battleEnd(battleID, br, queryID);
	battleAI.reset();
}

void CAdventureAI::battleUnitsChanged(const BattleID & battleID, const std::vector<UnitChanges> & units)
{
	battleAI->battleUnitsChanged(battleID, units);
}

void CAdventureAI::activeStack(const BattleID & battleID, const CStack * stack)
{
	battleAI->activeStack(battleID, stack);
}

void CAdventureAI::yourTacticPhase(const BattleID & battleID, int distance)
{
	battleAI->yourTacticPhase(battleID, distance);
}

void CAdventureAI::saveGame(BinarySerializer & h, const int version) /*saving */
{
	LOG_TRACE_PARAMS(logAi, "version '%i'", version);
	bool hasBattleAI = static_cast<bool>(battleAI);
	h & hasBattleAI;
	if(hasBattleAI)
	{
		h & battleAI->dllName;
	}
}

void CAdventureAI::loadGame(BinaryDeserializer & h, const int version) /*loading */
{
	LOG_TRACE_PARAMS(logAi, "version '%i'", version);
	bool hasBattleAI = false;
	h & hasBattleAI;
	if(hasBattleAI)
	{
		std::string dllName;
		h & dllName;
		battleAI = CDynLibHandler::getNewBattleAI(dllName);
		assert(cbc); //it should have been set by the one who new'ed us
		battleAI->initBattleInterface(env, cbc);
	}
}

VCMI_LIB_NAMESPACE_END
