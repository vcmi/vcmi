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

	auto upgradeSuccessful = ai->makePossibleUpgrades(town);

	auto armyToBuy = ai->nullkiller->armyManager->getArmyAvailableToBuy(town->getUpperArmy(), town);

	if(armyToBuy.empty())
	{
		if(upgradeSuccessful)
			return;

		throw cannotFulfillGoalException("No creatures to buy.");
	}

	for(int i = 0; valueBought < value && i < armyToBuy.size(); i++)
	{
		auto res = cb->getResourceAmount();
		auto & ci = armyToBuy[i];

		if(objid != CreatureID::NONE && ci.creID.getNum() != objid)
			continue;

		vstd::amin(ci.count, res / ci.creID.toCreature()->getFullRecruitCost());

		if(ci.count)
		{
			if (town->getUpperArmy()->stacksCount() == GameConstants::ARMY_SIZE)
			{
				SlotID lowestValueSlot;
				int lowestValue = std::numeric_limits<int>::max();
				for (const auto & slot : town->getUpperArmy()->Slots())
				{
					if (slot.second->getCreatureID() != CreatureID::NONE)
					{
						int currentStackMarketValue =
							slot.second->getCreatureID().toCreature()->getFullRecruitCost().marketValue() * slot.second->getCount();

						if (slot.second->getCreatureID().toCreature()->getFactionID() == town->getFactionID())
							continue;

						if (currentStackMarketValue < lowestValue)
						{
							lowestValue = currentStackMarketValue;
							lowestValueSlot = slot.first;
						}
					}
				}
				if (lowestValueSlot.validSlot())
				{
					cb->dismissCreature(town->getUpperArmy(), lowestValueSlot);
				}
			}
			if (town->getUpperArmy()->stacksCount() < GameConstants::ARMY_SIZE || town->getUpperArmy()->getSlotFor(ci.creID).validSlot()) //It is possible we don't scrap despite we wanted to due to not scrapping stacks that fit our faction
			{
				cb->recruitCreatures(town, town->getUpperArmy(), ci.creID, ci.count, ci.level);
			}
			valueBought += ci.count * ci.creID.toCreature()->getAIValue();
		}
	}

	if(!valueBought)
	{
		throw cannotFulfillGoalException("No creatures to buy.");
	}

	if(town->getVisitingHero() && !town->getGarrisonHero())
	{
		ai->moveHeroToTile(town->visitablePos(), town->getVisitingHero());
	}
}

}
