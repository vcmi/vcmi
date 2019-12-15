/*
* BattleAction.h, part of VCMI engine
*
* Authors: listed in file AUTHORS in main folder
*
* License: GNU General Public License v2.0 or later
* Full text of license available in license.txt file, in main folder
*
*/

#pragma once

#include "ISpecialAction.h"

namespace AIPathfinding
{
	class BattleAction : public ISpecialAction
	{
	private:
		const int3 targetTile;

	public:
		BattleAction(const int3 targetTile)
			:targetTile(targetTile)
		{
		}

		virtual Goals::TSubgoal whatToDo(const CGHeroInstance * hero) const override;
	};
}