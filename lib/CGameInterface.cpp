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

#ifdef VCMI_WINDOWS
#include <windows.h> //for .dll libs
#elif !defined VCMI_ANDROID
#include <dlfcn.h>
#endif

#include "serializer/BinaryDeserializer.h"
#include "serializer/BinarySerializer.h"

#ifdef VCMI_ANDROID

#include "AI/VCAI/VCAI.h"
#include "AI/Nullkiller/AIGateway.h"
#include "AI/BattleAI/BattleAI.h"

#endif

VCMI_LIB_NAMESPACE_BEGIN

template<typename rett>
std::shared_ptr<rett> createAny(const boost::filesystem::path & libpath, const std::string & methodName)
{
#ifdef VCMI_ANDROID
	// android currently doesn't support loading libs dynamically, so the access to the known libraries
	// is possible only via specializations of this template
	throw std::runtime_error("Could not resolve ai library " + libpath.generic_string());
#else
	typedef void(* TGetAIFun)(std::shared_ptr<rett> &);
	typedef void(* TGetNameFun)(char *);

	char temp[150];

	TGetAIFun getAI = nullptr;
	TGetNameFun getName = nullptr;

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
#endif //!VCMI_ANDROID
}

#ifdef VCMI_ANDROID

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
	return std::make_shared<CBattleAI>();
}

#endif

template<typename rett>
std::shared_ptr<rett> createAnyAI(std::string dllname, const std::string & methodName)
{
	logGlobal->info("Opening %s", dllname);

	const boost::filesystem::path filePath = VCMIDirs::get().fullLibraryPath("AI", dllname);
	auto ret = createAny<rett>(filePath, methodName);
	ret->dllName = std::move(dllname);
	return ret;
}

std::shared_ptr<CGlobalAI> CDynLibHandler::getNewAI(std::string dllname)
{
	return createAnyAI<CGlobalAI>(dllname, "GetNewAI");
}

std::shared_ptr<CBattleGameInterface> CDynLibHandler::getNewBattleAI(std::string dllname)
{
	return createAnyAI<CBattleGameInterface>(dllname, "GetNewBattleAI");
}

#if SCRIPTING_ENABLED
std::shared_ptr<scripting::Module> CDynLibHandler::getNewScriptingModule(const boost::filesystem::path & dllname)
{
	return createAny<scripting::Module>(dllname, "GetNewModule");
}
#endif

BattleAction CGlobalAI::activeStack(const CStack * stack)
{
	BattleAction ba;
	ba.actionType = EActionType::DEFEND;
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

void CAdventureAI::battleStart(const CCreatureSet * army1, const CCreatureSet * army2, int3 tile,
							   const CGHeroInstance * hero1, const CGHeroInstance * hero2, bool side)
{
	assert(!battleAI);
	assert(cbc);
	battleAI = CDynLibHandler::getNewBattleAI(getBattleAIName());
	battleAI->initBattleInterface(env, cbc);
	battleAI->battleStart(army1, army2, tile, hero1, hero2, side);
}

void CAdventureAI::battleStacksAttacked(const std::vector<BattleStackAttacked> & bsa)
{
	battleAI->battleStacksAttacked(bsa);
}

void CAdventureAI::actionStarted(const BattleAction & action)
{
	battleAI->actionStarted(action);
}

void CAdventureAI::battleNewRoundFirst(int round)
{
	battleAI->battleNewRoundFirst(round);
}

void CAdventureAI::actionFinished(const BattleAction & action)
{
	battleAI->actionFinished(action);
}

void CAdventureAI::battleStacksEffectsSet(const SetStackEffect & sse)
{
	battleAI->battleStacksEffectsSet(sse);
}

void CAdventureAI::battleObstaclesChanged(const std::vector<ObstacleChanges> & obstacles)
{
	battleAI->battleObstaclesChanged(obstacles);
}

void CAdventureAI::battleStackMoved(const CStack * stack, std::vector<BattleHex> dest, int distance)
{
	battleAI->battleStackMoved(stack, dest, distance);
}

void CAdventureAI::battleAttack(const BattleAttack * ba)
{
	battleAI->battleAttack(ba);
}

void CAdventureAI::battleSpellCast(const BattleSpellCast * sc)
{
	battleAI->battleSpellCast(sc);
}

void CAdventureAI::battleEnd(const BattleResult * br)
{
	battleAI->battleEnd(br);
	battleAI.reset();
}

void CAdventureAI::battleUnitsChanged(const std::vector<UnitChanges> & units, const std::vector<CustomEffectInfo> & customEffects)
{
	battleAI->battleUnitsChanged(units, customEffects);
}

BattleAction CAdventureAI::activeStack(const CStack * stack)
{
	return battleAI->activeStack(stack);
}

void CAdventureAI::yourTacticPhase(int distance)
{
	battleAI->yourTacticPhase(distance);
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
