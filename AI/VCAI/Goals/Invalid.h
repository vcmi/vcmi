/*
* Invalid.h, part of VCMI engine
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

namespace Goals
{
	class DLL_EXPORT Invalid : public CGoal<Invalid>
	{
	public:
		Invalid()
			: CGoal(Goals::INVALID)
		{
			priority = -1e10;
		}
		TGoalVec getAllPossibleSubgoals() override
		{
			return TGoalVec();
		}
		TSubgoal whatToDoToAchieve() override
		{
			return iAmElementar();
		}

		bool operator==(const Invalid & other) const override
		{
			return true;
		}
	};
}
