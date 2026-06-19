/*
 * AIFactory.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "AIFactory.h"

#include "CGlobalAI.h"

#ifdef ENABLE_NULLKILLER2_AI
#  include "../../AI/Nullkiller2/AIGateway.h"
#endif
#ifdef ENABLE_BATTLE_AI
#  include "../../AI/BattleAI/BattleAI.h"
#endif
#ifdef ENABLE_STUPID_AI
#  include "../../AI/StupidAI/StupidAI.h"
#endif
#ifdef ENABLE_MMAI
#  include "../../AI/MMAI/MMAI.h"
#endif
#include "../../AI/EmptyAI/CEmptyAI.h"

VCMI_LIB_NAMESPACE_BEGIN

std::shared_ptr<CGlobalAI> AIFactory::createAdventureAI(const std::string & name)
{
	logGlobal->info("Creating adventure AI %s", name);

	if(name == "Nullkiller2")
	{
#ifdef ENABLE_NULLKILLER2_AI
		auto ret = std::make_shared<NK2AI::AIGateway>();
		ret->dllName = name;
		return ret;
#else
		throw std::runtime_error("Nullkiller2 is not available in this build!");
#endif
	}

	auto ret = std::make_shared<CEmptyAI>();
	ret->dllName = name;
	return ret;
}

std::shared_ptr<CBattleGameInterface> AIFactory::createBattleAI(const std::string & name)
{
	logGlobal->info("Creating battle AI %s", name);

	if(name == "BattleAI")
#ifdef ENABLE_BATTLE_AI
		return std::make_shared<CBattleAI>();
#else
		throw std::runtime_error("BattleAI is not available in this build!");
#endif

	if(name == "StupidAI")
#ifdef ENABLE_STUPID_AI
		return std::make_shared<CStupidAI>();
#else
		throw std::runtime_error("StupidAI is not available in this build!");
#endif

	if(name == "MMAI")
#ifdef ENABLE_MMAI
		return std::make_shared<MMAI::BAI::Router>();
#else
		throw std::runtime_error("MMAI is not available in this build!");
#endif

	return std::make_shared<CEmptyAI>();
}

bool AIFactory::isAvailableAdventureAI(const std::string & name)
{
	if(name == "EmptyAI")
		return true;
#ifdef ENABLE_NULLKILLER2_AI
	if(name == "Nullkiller2")
		return true;
#endif
	return false;
}

VCMI_LIB_NAMESPACE_END
