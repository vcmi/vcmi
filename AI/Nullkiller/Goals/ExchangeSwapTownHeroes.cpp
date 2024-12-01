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
#include "../AIGateway.h"
#include "../Engine/Nullkiller.h"

namespace NKAI
{

using namespace Goals;

ExchangeSwapTownHeroes::ExchangeSwapTownHeroes(
	const CGTownInstance * town, 
	const CGHeroInstance * garrisonHero,
	HeroLockedReason lockingReason)
	:ElementarGoal(Goals::EXCHANGE_SWAP_TOWN_HEROES), town(town), garrisonHero(garrisonHero), lockingReason(lockingReason)
{
}

std::vector<ObjectInstanceID> ExchangeSwapTownHeroes::getAffectedObjects() const
{
	std::vector<ObjectInstanceID> affectedObjects = { town->id };

	if(town->garrisonHero)
		affectedObjects.push_back(town->garrisonHero->id);

	if(town->visitingHero)
		affectedObjects.push_back(town->visitingHero->id);

	return affectedObjects;
}

bool ExchangeSwapTownHeroes::isObjectAffected(ObjectInstanceID id) const
{
	return town->id == id
		|| (town->visitingHero && town->visitingHero->id == id)
		|| (town->garrisonHero && town->garrisonHero->id == id);
}

std::string ExchangeSwapTownHeroes::toString() const
{
	return "Exchange and swap heroes of " + town->getNameTranslated();
}

bool ExchangeSwapTownHeroes::operator==(const ExchangeSwapTownHeroes & other) const
{
	return town == other.town;
}

void ExchangeSwapTownHeroes::accept(AIGateway * ai)
{
	if(!garrisonHero)
	{
		auto currentGarrisonHero = town->garrisonHero;
		
		if(!currentGarrisonHero)
			throw cannotFulfillGoalException("Invalid configuration. There is no hero in town garrison.");
		
		cb->swapGarrisonHero(town);

		if(currentGarrisonHero.get() != town->visitingHero.get())
		{
			logAi->error("VisitingHero is empty, expected %s", currentGarrisonHero->getNameTranslated());
			return;
		}

		ai->buildArmyIn(town);
		ai->nullkiller->unlockHero(currentGarrisonHero.get());
		logAi->debug("Extracted hero %s from garrison of %s", currentGarrisonHero->getNameTranslated(), town->getNameTranslated());

		return;
	}

	if(town->visitingHero && town->visitingHero.get() != garrisonHero)
		cb->swapGarrisonHero(town);

	ai->makePossibleUpgrades(town);
	ai->moveHeroToTile(town->visitablePos(), garrisonHero);

	auto upperArmy = town->getUpperArmy();
	
	if(!town->garrisonHero)
	{
		if (!garrisonHero->canBeMergedWith(*town))
		{
			while (upperArmy->stacksCount() != 0)
			{
				cb->dismissCreature(upperArmy, upperArmy->Slots().begin()->first);
			}
		}
	}
	
	cb->swapGarrisonHero(town);

	if(lockingReason != HeroLockedReason::NOT_LOCKED)
	{
		ai->nullkiller->lockHero(garrisonHero, lockingReason);
	}

	if(town->visitingHero && town->visitingHero != garrisonHero)
	{
		ai->nullkiller->unlockHero(town->visitingHero.get());
		ai->makePossibleUpgrades(town->visitingHero);
	}

	logAi->debug("Put hero %s to garrison of %s", garrisonHero->getNameTranslated(), town->getNameTranslated());
}

}
