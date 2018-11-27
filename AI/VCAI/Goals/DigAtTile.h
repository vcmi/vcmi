/*
* DigAtTile.h, part of VCMI engine
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
	class DLL_EXPORT DigAtTile : public CGoal<DigAtTile>
		//elementar with hero on tile
	{
	public:
		DigAtTile()
			: CGoal(Goals::DIG_AT_TILE)
		{
		}
		DigAtTile(int3 Tile)
			: CGoal(Goals::DIG_AT_TILE)
		{
			tile = Tile;
			priority = 20;
		}
		TGoalVec getAllPossibleSubgoals() override
		{
			return TGoalVec();
		}
		TSubgoal whatToDoToAchieve() override;
		virtual bool operator==(const DigAtTile & other) const override;
	};
}
