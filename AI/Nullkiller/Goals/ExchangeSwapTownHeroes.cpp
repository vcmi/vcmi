/*
* ExchangeSwapTownHeroes.cpp, part of VCMI engine
*
* Authors: listed in file AUTHORS in main folder
*
* License: GNU General Public License v2.0 or later
* Full text of license available in license.txt file, in main folder
*
*/
#include "StdInc.h"
#include "ExchangeSwapTownHeroes.h"
#include "ExecuteHeroChain.h"
#include "../VCAI.h"
#include "../FuzzyHelper.h"
#include "../AIhelper.h"
#include "../../../lib/mapping/CMap.h" //for victory conditions
#include "../../../lib/CPathfinder.h"
#include "../Engine/Nullkiller.h"

extern boost::thread_specific_ptr<CCallback> cb;
extern boost::thread_specific_ptr<VCAI> ai;
extern FuzzyHelper * fh;

using namespace Goals;

ExchangeSwapTownHeroes::ExchangeSwapTownHeroes(const CGTownInstance * town, const CGHeroInstance * garrisonHero)
	:CGoal(Goals::EXCHANGE_SWAP_TOWN_HEROES), town(town), garrisonHero(garrisonHero)
{
}

std::string ExchangeSwapTownHeroes::name() const
{
	return "Exchange and swap heroes of " + town->name;
}

std::string ExchangeSwapTownHeroes::completeMessage() const
{
	return "Exchange and swap heroes of " + town->name + " compelete";
}

bool ExchangeSwapTownHeroes::operator==(const ExchangeSwapTownHeroes & other) const
{
	return town == other.town;
}

TSubgoal ExchangeSwapTownHeroes::whatToDoToAchieve()
{
	return iAmElementar();
}

void ExchangeSwapTownHeroes::accept(VCAI * ai)
{
	if(!garrisonHero)
	{
		if(!town->garrisonHero)
			throw cannotFulfillGoalException("Invalid configuration. There is no hero in town garrison.");
		
		cb->swapGarrisonHero(town);
		ai->buildArmyIn(town);
		ai->nullkiller->unlockHero(town->visitingHero.get());
		logAi->debug("Extracted hero %s from garrison of %s", town->visitingHero->name, town->name);

		return;
	}

	if(town->visitingHero && town->visitingHero.get() != garrisonHero)
		cb->swapGarrisonHero(town);

	ai->moveHeroToTile(town->visitablePos(), garrisonHero);

	cb->swapGarrisonHero(town); // selected hero left in garrison with strongest army
	ai->nullkiller->lockHero(town->garrisonHero.get());

	if(town->visitingHero)
	{
		ai->nullkiller->unlockHero(town->visitingHero.get());
		makePossibleUpgrades(town->visitingHero);
	}

	logAi->debug("Put hero %s to garrison of %s", town->garrisonHero->name, town->name);
}