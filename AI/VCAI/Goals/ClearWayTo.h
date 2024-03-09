/*
* ClearWayTo.h, part of VCMI engine
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
	class DLL_EXPORT ClearWayTo : public CGoal<ClearWayTo>
	{
	public:
		ClearWayTo()
			: CGoal(Goals::CLEAR_WAY_TO)
		{
		}
		ClearWayTo(int3 Tile)
			: CGoal(Goals::CLEAR_WAY_TO)
		{
			tile = Tile;
			priority = 5;
		}
		ClearWayTo(int3 Tile, HeroPtr h)
			: CGoal(Goals::CLEAR_WAY_TO)
		{
			tile = Tile;
			hero = h;
			priority = 5;
		}
		TGoalVec getAllPossibleSubgoals() override;
		TSubgoal whatToDoToAchieve() override;
		bool fulfillsMe(TSubgoal goal) override;
		bool operator==(const ClearWayTo & other) const override;
	};
}
