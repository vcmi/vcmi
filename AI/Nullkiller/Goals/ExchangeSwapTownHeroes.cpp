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

	if(town->getGarrisonHero())
		affectedObjects.push_back(town->getGarrisonHero()->id);

	if(town->getVisitingHero())
		affectedObjects.push_back(town->getVisitingHero()->id);

	return affectedObjects;
}

bool ExchangeSwapTownHeroes::isObjectAffected(ObjectInstanceID id) const
{
	return town->id == id
		|| (town->getVisitingHero() && town->getVisitingHero()->id == id)
		|| (town->getGarrisonHero() && town->getGarrisonHero()->id == id);
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
	if(!getGarrisonHero())
	{
		auto currentGarrisonHero = town->getGarrisonHero();
		
		if(!currentGarrisonHero)
			throw cannotFulfillGoalException("Invalid configuration. There is no hero in town garrison.");
		
		cb->swapGarrisonHero(town);

		if(currentGarrisonHero != town->getVisitingHero())
		{
			logAi->error("VisitingHero is empty, expected %s", currentGarrisonHero->getNameTranslated());
			return;
		}

		ai->buildArmyIn(town);
		ai->nullkiller->unlockHero(currentGarrisonHero);
		logAi->debug("Extracted hero %s from garrison of %s", currentGarrisonHero->getNameTranslated(), town->getNameTranslated());

		return;
	}

	if(town->getVisitingHero() && town->getVisitingHero() != getGarrisonHero())
		cb->swapGarrisonHero(town);

	ai->makePossibleUpgrades(town);
	ai->moveHeroToTile(town->visitablePos(), getGarrisonHero());

	auto upperArmy = town->getUpperArmy();
	
	if(!town->getGarrisonHero())
	{
		if (!getGarrisonHero()->canBeMergedWith(*town))
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
		ai->nullkiller->lockHero(getGarrisonHero(), lockingReason);
	}

	if(town->getVisitingHero() && town->getVisitingHero() != getGarrisonHero())
	{
		ai->nullkiller->unlockHero(town->getVisitingHero());
		ai->makePossibleUpgrades(town->getVisitingHero());
	}

	logAi->debug("Put hero %s to garrison of %s", getGarrisonHero()->getNameTranslated(), town->getNameTranslated());
}

}
