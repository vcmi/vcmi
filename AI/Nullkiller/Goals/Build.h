namespace Nullkiller
{

/*
* Build.h, part of VCMI engine
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
	class DLL_EXPORT Build : public CGoal<Build>
	{
	public:
		Build()
			: CGoal(Goals::BUILD)
		{
			priority = 1;
		}
		TGoalVec getAllPossibleSubgoals() override;
		TSubgoal whatToDoToAchieve() override;
		bool fulfillsMe(TSubgoal goal) override;

		virtual bool operator==(const Build & other) const override
		{
			return true;
		}
	};
}

}
