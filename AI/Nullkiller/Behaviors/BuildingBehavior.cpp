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
#include "../VCAI.h"
#include "../AIhelper.h"
#include "../AIUtility.h"
#include "../Goals/BuyArmy.h"
#include "../Goals/VisitTile.h"
#include "lib/mapping/CMap.h" //for victory conditions
#include "lib/CPathfinder.h"
#include "../Engine/Nullkiller.h"

extern boost::thread_specific_ptr<CCallback> cb;
extern boost::thread_specific_ptr<VCAI> ai;
extern FuzzyHelper * fh;

using namespace Goals;

std::string BuildingBehavior::toString() const
{
	return "Build";
}

Goals::TGoalVec BuildingBehavior::decompose() const
{
	Goals::TGoalVec tasks;

	TResources resourcesRequired = ai->nullkiller->buildAnalyzer->getResourcesRequiredNow();
	TResources totalDevelopmentCost = ai->nullkiller->buildAnalyzer->getTotalResourcesRequired();
	TResources availableResources = cb->getResourceAmount();
	TResources dailyIncome = ai->nullkiller->buildAnalyzer->getDailyIncome();

	logAi->trace("Resources amount: %s", availableResources.toString());

	resourcesRequired -= availableResources;
	resourcesRequired.positive();

	logAi->trace("daily income: %s", dailyIncome.toString());
	logAi->trace("resources required to develop towns now: %s, total: %s",
		resourcesRequired.toString(),
		totalDevelopmentCost.toString());

	auto & developmentInfos = ai->nullkiller->buildAnalyzer->getDevelopmentInfo();
	auto goldPreasure = ai->nullkiller->buildAnalyzer->getGoldPreasure();

	for(auto & developmentInfo : developmentInfos)
	{
		auto town = developmentInfo.town;

		for(auto & buildingInfo : developmentInfo.toBuild)
		{
			if(goldPreasure < MAX_GOLD_PEASURE || buildingInfo.dailyIncome[Res::GOLD] > 0)
				tasks.push_back(sptr(BuildThis(buildingInfo, developmentInfo)));
		}
	}

	return tasks;
}
