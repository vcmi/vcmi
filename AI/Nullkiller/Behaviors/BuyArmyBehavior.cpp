/*
* BuyArmyBehavior.cpp, part of VCMI engine
*
* Authors: listed in file AUTHORS in main folder
*
* License: GNU General Public License v2.0 or later
* Full text of license available in license.txt file, in main folder
*
*/
#include "StdInc.h"
#include "BuyArmyBehavior.h"
#include "../AIGateway.h"
#include "../AIUtility.h"
#include "../Goals/BuyArmy.h"
#include "../Engine/Nullkiller.h"

namespace NKAI
{

using namespace Goals;

std::string BuyArmyBehavior::toString() const
{
	return "Buy army";
}

Goals::TGoalVec BuyArmyBehavior::decompose(const Nullkiller * ai) const
{
	Goals::TGoalVec tasks;

	auto heroes = cb->getHeroesInfo();

	if(heroes.empty())
	{
		return tasks;
	}

	ai->dangerHitMap->updateHitMap();

	for(auto town : cb->getTownsInfo())
	{
		uint8_t closestThreat = ai->dangerHitMap->getTileThreat(town->visitablePos()).fastestDanger.turn;

		if (closestThreat >=2 && ai->buildAnalyzer->isGoldPressureHigh() && !town->hasBuilt(BuildingID::CITY_HALL) && cb->canBuildStructure(town, BuildingID::CITY_HALL) != EBuildingState::FORBIDDEN)
		{
			return tasks;
		}
		
		auto townArmyAvailableToBuy = ai->armyManager->getArmyAvailableToBuyAsCCreatureSet(
			town,
			ai->getFreeResources());

		for(const CGHeroInstance * targetHero : heroes)
		{
			if(ai->heroManager->getHeroRole(targetHero) == HeroRole::MAIN)
			{
				auto reinforcement = ai->armyManager->howManyReinforcementsCanGet(
					targetHero,
					targetHero,
					&*townArmyAvailableToBuy,
					TerrainId::NONE);

				if(reinforcement)
					vstd::amin(reinforcement, ai->armyManager->howManyReinforcementsCanBuy(town->getUpperArmy(), town));

				if(reinforcement)
				{
					tasks.push_back(Goals::sptr(Goals::BuyArmy(town, reinforcement).setpriority(reinforcement)));
				}
			}
		}
	}

	return tasks;
}

}
