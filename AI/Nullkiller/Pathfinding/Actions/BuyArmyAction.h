/*
* BuyArmyAction.h, part of VCMI engine
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
	class BuyArmyAction : public SpecialAction
	{
	private:

	public:
		bool canAct(const AIPathNode * source) const override
		{
			return true;
		}

		void execute(const CGHeroInstance * hero) const override;
		std::string toString() const override;
	};
}

}
