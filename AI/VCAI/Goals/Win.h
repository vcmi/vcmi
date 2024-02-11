/*
* Win.h, part of VCMI engine
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
	class DLL_EXPORT Win : public CGoal<Win>
	{
	public:
		Win()
			: CGoal(Goals::WIN)
		{
			priority = 100;
		}
		TGoalVec getAllPossibleSubgoals() override
		{
			return TGoalVec();
		}
		TSubgoal whatToDoToAchieve() override;

		bool operator==(const Win & other) const override
		{
			return true;
		}
	};
}
