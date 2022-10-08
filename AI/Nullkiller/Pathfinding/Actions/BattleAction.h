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

#include "SpecialAction.h"
#include "../../../../lib/CGameState.h"

namespace NKAI
{

namespace AIPathfinding
{
	class BattleAction : public SpecialAction
	{
	private:
		const int3 targetTile;

	public:
		BattleAction(const int3 targetTile)
			:targetTile(targetTile)
		{
		}

		virtual void execute(const CGHeroInstance * hero) const override;

		virtual std::string toString() const override;
	};
}

}
