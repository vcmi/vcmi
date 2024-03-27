/*
* Conquer.h, part of VCMI engine
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
	class DLL_EXPORT Conquer : public CGoal<Conquer>
	{
	public:
		Conquer()
			: CGoal(Goals::CONQUER)
		{
			priority = 10;
		}
		TGoalVec getAllPossibleSubgoals() override;
		TSubgoal whatToDoToAchieve() override;
		bool operator==(const Conquer & other) const override;
	};
}
