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

namespace NK2AI
{

using namespace Goals;

std::string BuyArmyBehavior::toString() const
{
	return "Buy army";
}

Goals::TGoalVec BuyArmyBehavior::decompose(const Nullkiller * aiNk) const
{
	Goals::TGoalVec tasks;

	auto heroes = ccTl->getHeroesInfo();

	if(heroes.empty())
	{
		return tasks;
	}

	// Simplification: Moved this call before getting into the decomposer
	// aiNk->dangerHitMap->updateHitMap();

	for(auto town : ccTl->getTownsInfo())
	{
		uint8_t closestThreat = aiNk->dangerHitMap->getTileThreat(town->visitablePos()).fastestDanger.turn;

		if (closestThreat >=2 && aiNk->buildAnalyzer->isGoldPressureOverMax() && !town->hasBuilt(BuildingID::CITY_HALL) && ccTl->canBuildStructure(town, BuildingID::CITY_HALL) != EBuildingState::FORBIDDEN)
		{
			return tasks;
		}
		
		auto townArmyAvailableToBuy = aiNk->armyManager->getArmyAvailableToBuyAsCCreatureSet(
			town,
			aiNk->getFreeResources());

		for(const CGHeroInstance * targetHero : heroes)
		{
			if(aiNk->heroManager->getHeroRole(HeroPtr(targetHero)) == HeroRole::MAIN)
			{
				auto reinforcement = aiNk->armyManager->howManyReinforcementsCanGet(
					targetHero,
					targetHero,
					&*townArmyAvailableToBuy,
					TerrainId::NONE);

				if(reinforcement)
					vstd::amin(reinforcement, aiNk->armyManager->howManyReinforcementsCanBuy(town->getUpperArmy(), town));

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
