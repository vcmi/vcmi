/*
* BuildThis.h, part of VCMI engine
*
* Authors: listed in file AUTHORS in main folder
*
* License: GNU General Public License v2.0 or later
* Full text of license available in license.txt file, in main folder
*
*/
#pragma once

#include "CGoal.h"

struct HeroPtr;
class VCAI;
class FuzzyHelper;

namespace Goals
{
	class DLL_EXPORT BuildThis : public CGoal<BuildThis>
	{
	public:
		BuildThis() //should be private, but unit test uses it
			: CGoal(Goals::BUILD_STRUCTURE)
		{
		}
		BuildThis(BuildingID Bid, const CGTownInstance * tid)
			: CGoal(Goals::BUILD_STRUCTURE)
		{
			bid = Bid.getNum();
			town = tid;
			priority = 1;
		}
		BuildThis(BuildingID Bid)
			: CGoal(Goals::BUILD_STRUCTURE)
		{
			bid = Bid.getNum();
			priority = 1;
		}
		TGoalVec getAllPossibleSubgoals() override
		{
			return TGoalVec();
		}
		TSubgoal whatToDoToAchieve() override;
		//bool fulfillsMe(TSubgoal goal) override;
		bool operator==(const BuildThis & other) const override;
	};
}
