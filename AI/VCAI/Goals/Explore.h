/*
* Explore.h, part of VCMI engine
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
	class DLL_EXPORT Explore : public CGoal<Explore>
	{
	public:
		Explore()
			: CGoal(Goals::EXPLORE)
		{
			priority = 1;
		}
		Explore(HeroPtr h)
			: CGoal(Goals::EXPLORE)
		{
			hero = h;
			priority = 1;
		}
		TGoalVec getAllPossibleSubgoals() override;
		TSubgoal whatToDoToAchieve() override;
		std::string completeMessage() const override;
		bool fulfillsMe(TSubgoal goal) override;
		virtual bool operator==(const Explore & other) const override;
	};
}
