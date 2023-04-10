/*
* BuyArmy.cpp, part of VCMI engine
*
* Authors: listed in file AUTHORS in main folder
*
* License: GNU General Public License v2.0 or later
* Full text of license available in license.txt file, in main folder
*
*/
#include "../StdInc.h"
#include "BuyArmy.h"
#include "../../../lib/mapObjects/CGTownInstance.h"
#include "../AIGateway.h"
#include "../Engine/Nullkiller.h"


namespace NKAI
{

extern boost::thread_specific_ptr<CCallback> cb;
extern boost::thread_specific_ptr<AIGateway> ai;

using namespace Goals;

bool BuyArmy::operator==(const BuyArmy & other) const
{
	return town == other.town && objid == other.objid;
}

std::string BuyArmy::toString() const
{
	return "Buy army at " + town->getNameTranslated();
}

void BuyArmy::accept(AIGateway * ai)
{
	ui64 valueBought = 0;
	//buy the stacks with largest AI value

	auto upgradeSuccessfull = ai->makePossibleUpgrades(town);

	auto armyToBuy = ai->nullkiller->armyManager->getArmyAvailableToBuy(town->getUpperArmy(), town);

	if(armyToBuy.empty())
	{
		if(upgradeSuccessfull)
			return;

		throw cannotFulfillGoalException("No creatures to buy.");
	}

	for(int i = 0; valueBought < value && i < armyToBuy.size(); i++)
	{
		auto res = cb->getResourceAmount();
		auto & ci = armyToBuy[i];

		if(objid != -1 && ci.creID != objid)
			continue;

		vstd::amin(ci.count, res / ci.cre->getFullRecruitCost());

		if(ci.count)
		{
			cb->recruitCreatures(town, town->getUpperArmy(), ci.creID, ci.count, ci.level);
			valueBought += ci.count * ci.cre->getAIValue();
		}
	}

	if(!valueBought)
	{
		throw cannotFulfillGoalException("No creatures to buy.");
	}

	if(town->visitingHero)
	{
		ai->moveHeroToTile(town->visitablePos(), town->visitingHero.get());
	}
}

}
