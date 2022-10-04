namespace Nullkiller
{

/*
* Build.cpp, part of VCMI engine
*
* Authors: listed in file AUTHORS in main folder
*
* License: GNU General Public License v2.0 or later
* Full text of license available in license.txt file, in main folder
*
*/
#include "StdInc.h"
#include "Build.h"
#include "BuildThis.h"
#include "../AIGateway.h"
#include "../AIUtility.h"
#include "../AIhelper.h"
#include "../FuzzyHelper.h"
#include "../ResourceManager.h"
#include "../BuildingManager.h"
#include "../../../lib/mapping/CMap.h" //for victory conditions
#include "../../../lib/CPathfinder.h"
#include "../../../lib/StringConstants.h"


extern boost::thread_specific_ptr<CCallback> cb;
extern boost::thread_specific_ptr<AIGateway> ai;
extern FuzzyHelper * fh;

using namespace Goals;

TGoalVec Build::getAllPossibleSubgoals()
{
	TGoalVec ret;

	for(const CGTownInstance * t : cb->getTownsInfo())
	{
		//start fresh with every town
		ai->ah->getBuildingOptions(t);
		auto immediateBuilding = ai->ah->immediateBuilding();
		auto expensiveBuilding = ai->ah->expensiveBuilding();

		//handling for early town development to save money and focus on income
		if(!t->hasBuilt(ai->ah->getMaxPossibleGoldBuilding(t)) && expensiveBuilding.is_initialized())
		{
			auto potentialBuilding = expensiveBuilding.get();
			switch(expensiveBuilding.get().bid)
			{
			case BuildingID::TOWN_HALL:
			case BuildingID::CITY_HALL:
			case BuildingID::CAPITOL:
			case BuildingID::FORT:
			case BuildingID::CITADEL:
			case BuildingID::CASTLE:
				//If above buildings are next to be bought, but no money... do not buy anything else, try to gather resources for these. Simple but has to suffice for now.
				auto goal = ai->ah->whatToDo(potentialBuilding.price, sptr(BuildThis(potentialBuilding.bid, t).setpriority(2.25)));
				ret.push_back(goal);
				return ret;
				break;
			}
		}

		if(immediateBuilding.is_initialized())
		{
			ret.push_back(sptr(BuildThis(immediateBuilding.get().bid, t).setpriority(2))); //prioritize buildings we can build quick
		}
		else //try build later
		{
			if(expensiveBuilding.is_initialized())
			{
				auto potentialBuilding = expensiveBuilding.get(); //gather resources for any we can't afford
				auto goal = ai->ah->whatToDo(potentialBuilding.price, sptr(BuildThis(potentialBuilding.bid, t).setpriority(0.5)));
				ret.push_back(goal);
			}
		}
	}

	if(ret.empty())
		throw cannotFulfillGoalException("BUILD has been realized as much as possible.");
	else
		return ret;
}

TSubgoal Build::whatToDoToAchieve()
{
	return fh->chooseSolution(getAllPossibleSubgoals());
}

bool Build::fulfillsMe(TSubgoal goal)
{
	if(goal->goalType == BUILD || goal->goalType == BUILD_STRUCTURE)
		return (!town || town == goal->town); //building anything will do, in this town if set
	else
		return false;
}

}
