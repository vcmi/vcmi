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
#include "BuildingBehavior.h"
#include "../AIGateway.h"
#include "../AIUtility.h"
#include "../Goals/BuyArmy.h"
#include "../Goals/Composition.h"
#include "../Goals/BuildThis.h"
#include "../Goals/SaveResources.h"
#include "../Engine/Nullkiller.h"

namespace NKAI
{

using namespace Goals;

std::string BuildingBehavior::toString() const
{
	return "Build";
}

Goals::TGoalVec BuildingBehavior::decompose(const Nullkiller * ai) const
{
	Goals::TGoalVec tasks;

	TResources resourcesRequired = ai->buildAnalyzer->getResourcesRequiredNow();
	TResources totalDevelopmentCost = ai->buildAnalyzer->getTotalResourcesRequired();
	TResources availableResources = ai->getFreeResources();
	TResources dailyIncome = ai->buildAnalyzer->getDailyIncome();

	logAi->trace("Free resources amount: %s", availableResources.toString());

	resourcesRequired -= availableResources;
	resourcesRequired.positive();

	logAi->trace("daily income: %s", dailyIncome.toString());
	logAi->trace("resources required to develop towns now: %s, total: %s",
		resourcesRequired.toString(),
		totalDevelopmentCost.toString());

	auto & developmentInfos = ai->buildAnalyzer->getDevelopmentInfo();
	auto isGoldPressureLow = !ai->buildAnalyzer->isGoldPressureHigh();

	ai->dangerHitMap->updateHitMap();

	for(auto & developmentInfo : developmentInfos)
	{
		bool emergencyDefense = false;
		uint8_t closestThreat = std::numeric_limits<uint8_t>::max();
		for (auto threat : ai->dangerHitMap->getTownThreats(developmentInfo.town))
		{
			closestThreat = std::min(closestThreat, threat.turn);
		}

		if (closestThreat <= 1 && developmentInfo.town->fortLevel() < CGTownInstance::EFortLevel::CASTLE)
		{
			for (auto& buildingInfo : developmentInfo.toBuild)
			{
				if ( !buildingInfo.notEnoughRes && (buildingInfo.id == BuildingID::CITADEL || buildingInfo.id == BuildingID::CASTLE))
				{
					tasks.push_back(sptr(BuildThis(buildingInfo, developmentInfo)));
					emergencyDefense = true;
				}
			}
		}

		if (!emergencyDefense)
		{
			for (auto& buildingInfo : developmentInfo.toBuild)
			{
				if (isGoldPressureLow || buildingInfo.dailyIncome[EGameResID::GOLD] > 0)
				{
					if (buildingInfo.notEnoughRes)
					{
						if (ai->getLockedResources().canAfford(buildingInfo.buildCost))
							continue;

						Composition composition;

						composition.addNext(BuildThis(buildingInfo, developmentInfo));
						composition.addNext(SaveResources(buildingInfo.buildCost));
						tasks.push_back(sptr(composition));
					}
					else
					{
						tasks.push_back(sptr(BuildThis(buildingInfo, developmentInfo)));
					}
				}
			}
		}
	}

	return tasks;
}

}
