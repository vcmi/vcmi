namespace Nullkiller
{

/*
* GatherArmy.h, part of VCMI engine
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
class AIGateway;
class FuzzyHelper;

namespace Goals
{
	class DLL_EXPORT GatherArmy : public CGoal<GatherArmy>
	{
	public:
		GatherArmy()
			: CGoal(Goals::GATHER_ARMY)
		{
		}
		GatherArmy(int val)
			: CGoal(Goals::GATHER_ARMY)
		{
			value = val;
			priority = 2.5;
		}
		TGoalVec getAllPossibleSubgoals() override;
		TSubgoal whatToDoToAchieve() override;
		std::string completeMessage() const override;
		virtual bool operator==(const GatherArmy & other) const override;
	};
}

}
